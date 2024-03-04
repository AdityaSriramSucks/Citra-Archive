// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <clocale>
#include <filesystem>
#include <fstream>
#include <memory>
#include <thread>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QLabel>
#include <QMessageBox>
#include <QOpenGLFunctions_4_3_Core>
#include <QSysInfo>
#include <QtConcurrent/QtConcurrentRun>
#include <QtGui>
#include <QtWidgets>
#include <fmt/format.h>
#ifdef __APPLE__
#include <unistd.h> // for chdir
#endif
#ifdef _WIN32
#include <windows.h>
#endif
#include "citra_qt/aboutdialog.h"
#include "citra_qt/applets/mii_selector.h"
#include "citra_qt/applets/swkbd.h"
#include "citra_qt/bootmanager.h"
#include "citra_qt/camera/qt_multimedia_camera.h"
#include "citra_qt/camera/still_image_camera.h"
#include "citra_qt/cheats.h"
#include "citra_qt/compatdb.h"
#include "citra_qt/compatibility_list.h"
#include "citra_qt/configuration/config.h"
#include "citra_qt/configuration/configure_dialog.h"
#include "citra_qt/configuration/configure_per_game.h"
#include "citra_qt/debugger/console.h"
#include "citra_qt/debugger/graphics/graphics.h"
#include "citra_qt/debugger/graphics/graphics_breakpoints.h"
#include "citra_qt/debugger/graphics/graphics_cmdlists.h"
#include "citra_qt/debugger/graphics/graphics_surface.h"
#include "citra_qt/debugger/graphics/graphics_tracing.h"
#include "citra_qt/debugger/graphics/graphics_vertex_shader.h"
#include "citra_qt/debugger/ipc/recorder.h"
#include "citra_qt/debugger/lle_service_modules.h"
#include "citra_qt/debugger/profiler.h"
#include "citra_qt/debugger/registers.h"
#include "citra_qt/debugger/wait_tree.h"
#include "citra_qt/discord.h"
#include "citra_qt/game_list.h"
#include "citra_qt/hotkeys.h"
#include "citra_qt/loading_screen.h"
#include "citra_qt/main.h"
#include "citra_qt/movie/movie_play_dialog.h"
#include "citra_qt/movie/movie_record_dialog.h"
#include "citra_qt/multiplayer/state.h"
#include "citra_qt/qt_image_interface.h"
#include "citra_qt/uisettings.h"
#include "citra_qt/updater/updater.h"
#include "citra_qt/util/clickable_label.h"
#include "common/arch.h"
#include "common/common_paths.h"
#include "common/detached_tasks.h"
#include "common/file_util.h"
#include "common/literals.h"
#include "common/logging/backend.h"
#include "common/logging/filter.h"
#include "common/logging/log.h"
#include "common/logging/text_formatter.h"
#include "common/memory_detect.h"
#include "common/microprofile.h"
#include "common/scm_rev.h"
#include "common/scope_exit.h"
#include "common/string_util.h"
#if CITRA_ARCH(x86_64)
#include "common/x64/cpu_detect.h"
#endif
#include "common/settings.h"
#include "core/core.h"
#include "core/dumping/backend.h"
#include "core/file_sys/archive_extsavedata.h"
#include "core/file_sys/archive_source_sd_savedata.h"
#include "core/frontend/applets/default_applets.h"
#include "core/frontend/scope_acquire_context.h"
#include "core/gdbstub/gdbstub.h"
#include "core/hle/service/fs/archive.h"
#include "core/hle/service/nfc/nfc.h"
#include "core/loader/loader.h"
#include "core/movie.h"
#include "core/savestate.h"
#include "game_list_p.h"
#include "input_common/main.h"
#include "network/network_settings.h"
#include "ui_main.h"
#include "video_core/renderer_base.h"
#include "video_core/video_core.h"

#ifdef USE_DISCORD_PRESENCE
#include "citra_qt/discord_impl.h"
#endif

#ifdef ENABLE_FFMPEG_VIDEO_DUMPER
#include "citra_qt/dumping/dumping_dialog.h"
#endif

#ifdef QT_STATICPLUGIN
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif

#ifdef _WIN32
extern "C" {
// tells Nvidia drivers to use the dedicated GPU by default on laptops with switchable graphics
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
}
#endif

constexpr int default_mouse_timeout = 2500;
constexpr int num_options_3d = 5;

/**
 * "Callouts" are one-time instructional messages shown to the user. In the config settings, there
 * is a bitfield "callout_flags" options, used to track if a message has already been shown to the
 * user. This is 32-bits - if we have more than 32 callouts, we should retire and recycle old ones.
 */
enum class CalloutFlag : uint32_t {
    Telemetry = 0x1,
};

void GMainWindow::ShowTelemetryCallout() {
    if (UISettings::values.callout_flags.GetValue() &
        static_cast<uint32_t>(CalloutFlag::Telemetry)) {
        return;
    }

    UISettings::values.callout_flags =
        UISettings::values.callout_flags.GetValue() | static_cast<uint32_t>(CalloutFlag::Telemetry);
    const QString telemetry_message =
        tr("<a href='https://citra-emu.org/entry/telemetry-and-why-thats-a-good-thing/'>Anonymous "
           "data is collected</a> to help improve Citra. "
           "<br/><br/>Would you like to share your usage data with us?");
    if (QMessageBox::question(this, tr("Telemetry"), telemetry_message) != QMessageBox::Yes) {
        NetSettings::values.enable_telemetry = false;
        Settings::Apply();
    }
}

const int GMainWindow::max_recent_files_item;

static void InitializeLogging() {
    Log::Filter log_filter;
    log_filter.ParseFilterString(Settings::values.log_filter.GetValue());
    Log::SetGlobalFilter(log_filter);

    const std::string& log_dir = FileUtil::GetUserPath(FileUtil::UserPath::LogDir);
    FileUtil::CreateFullPath(log_dir);
    Log::AddBackend(std::make_unique<Log::FileBackend>(log_dir + LOG_FILE));
#ifdef _WIN32
    Log::AddBackend(std::make_unique<Log::DebuggerBackend>());
#endif
}

static QString PrettyProductName() {
#ifdef _WIN32
    // After Windows 10 Version 2004, Microsoft decided to switch to a different notation: 20H2
    // With that notation change they changed the registry key used to denote the current version
    QSettings windows_registry(
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"),
        QSettings::NativeFormat);
    const QString release_id = windows_registry.value(QStringLiteral("ReleaseId")).toString();
    if (release_id == QStringLiteral("2009")) {
        const u32 current_build = windows_registry.value(QStringLiteral("CurrentBuild")).toUInt();
        const QString display_version =
            windows_registry.value(QStringLiteral("DisplayVersion")).toString();
        const u32 ubr = windows_registry.value(QStringLiteral("UBR")).toUInt();
        const u32 version = current_build >= 22000 ? 11 : 10;
        return QStringLiteral("Windows %1 Version %2 (Build %3.%4)")
            .arg(QString::number(version), display_version, QString::number(current_build),
                 QString::number(ubr));
    }
#endif
    return QSysInfo::prettyProductName();
}

GMainWindow::GMainWindow()
    : ui{std::make_unique<Ui::MainWindow>()}, config{std::make_unique<Config>()}, emu_thread{
                                                                                      nullptr} {
    InitializeLogging();
    Debugger::ToggleConsole();
    Settings::LogSettings();

    // register types to use in slots and signals
    qRegisterMetaType<std::size_t>("std::size_t");
    qRegisterMetaType<Service::AM::InstallStatus>("Service::AM::InstallStatus");

    LoadTranslation();

    Pica::g_debug_context = Pica::DebugContext::Construct();
    setAcceptDrops(true);
    ui->setupUi(this);
    statusBar()->hide();

    default_theme_paths = QIcon::themeSearchPaths();
    UpdateUITheme();

    SetDiscordEnabled(UISettings::values.enable_discord_presence.GetValue());
    discord_rpc->Update();

    Network::Init();

    Core::Movie::GetInstance().SetPlaybackCompletionCallback([this] {
        QMetaObject::invokeMethod(this, "OnMoviePlaybackCompleted", Qt::BlockingQueuedConnection);
    });

    InitializeWidgets();
    InitializeDebugWidgets();
    InitializeRecentFileMenuActions();
    InitializeSaveStateMenuActions();
    InitializeHotkeys();
    ShowUpdaterWidgets();

    SetDefaultUIGeometry();
    RestoreUIState();

    ConnectMenuEvents();
    ConnectWidgetEvents();
    Connect3DStateEvents();

    LOG_INFO(Frontend, "Citra Version: {} | {}-{}", Common::g_build_fullname, Common::g_scm_branch,
             Common::g_scm_desc);
#if CITRA_ARCH(x86_64)
    const auto& caps = Common::GetCPUCaps();
    std::string cpu_string = caps.cpu_string;
    if (caps.avx || caps.avx2 || caps.avx512) {
        cpu_string += " | AVX";
        if (caps.avx512) {
            cpu_string += "512";
        } else if (caps.avx2) {
            cpu_string += '2';
        }
        if (caps.fma || caps.fma4) {
            cpu_string += " | FMA";
        }
    }
    LOG_INFO(Frontend, "Host CPU: {}", cpu_string);
#endif
    LOG_INFO(Frontend, "Host OS: {}", PrettyProductName().toStdString());
    const auto& mem_info = Common::GetMemInfo();
    using namespace Common::Literals;
    LOG_INFO(Frontend, "Host RAM: {:.2f} GiB", mem_info.total_physical_memory / f64{1_GiB});
    LOG_INFO(Frontend, "Host Swap: {:.2f} GiB", mem_info.total_swap_memory / f64{1_GiB});
    UpdateWindowTitle();

    show();

    game_list->LoadCompatibilityList();
    game_list->PopulateAsync(UISettings::values.game_dirs);

    // Show one-time "callout" messages to the user
    ShowTelemetryCallout();

    mouse_hide_timer.setInterval(default_mouse_timeout);
    connect(&mouse_hide_timer, &QTimer::timeout, this, &GMainWindow::HideMouseCursor);
    connect(ui->menubar, &QMenuBar::hovered, this, &GMainWindow::OnMouseActivity);

    if (UISettings::values.check_for_update_on_start) {
        CheckForUpdates();
    }

    QStringList args = QApplication::arguments();
    if (args.length() >= 2) {
        BootGame(args[1]);
    }
}

GMainWindow::~GMainWindow() {
    // will get automatically deleted otherwise
    if (render_window->parent() == nullptr)
        delete render_window;

    Pica::g_debug_context.reset();
    Network::Shutdown();
}

void GMainWindow::InitializeWidgets() {
#ifdef CITRA_ENABLE_COMPATIBILITY_REPORTING
    ui->action_Report_Compatibility->setVisible(true);
#endif
    render_window = new GRenderWindow(this, emu_thread.get(), false);
    secondary_window = new GRenderWindow(this, emu_thread.get(), true);
    render_window->hide();
    secondary_window->hide();
    secondary_window->setParent(nullptr);

    game_list = new GameList(this);
    ui->horizontalLayout->addWidget(game_list);

    game_list_placeholder = new GameListPlaceholder(this);
    ui->horizontalLayout->addWidget(game_list_placeholder);
    game_list_placeholder->setVisible(false);

    loading_screen = new LoadingScreen(this);
    loading_screen->hide();
    ui->horizontalLayout->addWidget(loading_screen);
    connect(loading_screen, &LoadingScreen::Hidden, this, [&] {
        loading_screen->Clear();
        if (emulation_running) {
            render_window->show();
            render_window->setFocus();
        }
    });

    InputCommon::Init();
    multiplayer_state = new MultiplayerState(this, game_list->GetModel(), ui->action_Leave_Room,
                                             ui->action_Show_Room);
    multiplayer_state->setVisible(false);

    // Setup updater
    updater = new Updater(this);
    UISettings::values.updater_found = updater->HasUpdater();

    // Create status bar
    message_label = new QLabel();
    // Configured separately for left alignment
    message_label->setFrameStyle(QFrame::NoFrame);
    message_label->setContentsMargins(4, 0, 4, 0);
    message_label->setAlignment(Qt::AlignLeft);
    statusBar()->addPermanentWidget(message_label, 1);

    progress_bar = new QProgressBar();
    progress_bar->hide();
    statusBar()->addPermanentWidget(progress_bar);

    emu_speed_label = new QLabel();
    emu_speed_label->setToolTip(tr("Current emulation speed. Values higher or lower than 100% "
                                   "indicate emulation is running faster or slower than a 3DS."));
    game_fps_label = new QLabel();
    game_fps_label->setToolTip(tr("How many frames per second the game is currently displaying. "
                                  "This will vary from game to game and scene to scene."));
    emu_frametime_label = new QLabel();
    emu_frametime_label->setToolTip(
        tr("Time taken to emulate a 3DS frame, not counting framelimiting or v-sync. For "
           "full-speed emulation this should be at most 16.67 ms."));

    for (auto& label : {emu_speed_label, game_fps_label, emu_frametime_label}) {
        label->setVisible(false);
        label->setFrameStyle(QFrame::NoFrame);
        label->setContentsMargins(4, 0, 4, 0);
        statusBar()->addPermanentWidget(label);
    }

    option_3d_button = new QPushButton();
    option_3d_button->setObjectName(QStringLiteral("3DOptionStatusBarButton"));
    option_3d_button->setFocusPolicy(Qt::NoFocus);
    option_3d_button->setToolTip(tr("Indicates the current 3D setting. Click to toggle."));

    factor_3d_slider = new QSlider(Qt::Orientation::Horizontal, this);
    factor_3d_slider->setStyleSheet(QStringLiteral("QSlider { padding: 4px; }"));
    factor_3d_slider->setToolTip(tr("Current 3D factor while 3D is enabled."));
    factor_3d_slider->setRange(0, 100);

    Update3DState();
    statusBar()->insertPermanentWidget(0, option_3d_button);
    statusBar()->insertPermanentWidget(1, factor_3d_slider);

    statusBar()->addPermanentWidget(multiplayer_state->GetStatusText());
    statusBar()->addPermanentWidget(multiplayer_state->GetStatusIcon());

    statusBar()->setVisible(true);

    // Removes an ugly inner border from the status bar widgets under Linux
    setStyleSheet(QStringLiteral("QStatusBar::item{border: none;}"));

    QActionGroup* actionGroup_ScreenLayouts = new QActionGroup(this);
    actionGroup_ScreenLayouts->addAction(ui->action_Screen_Layout_Default);
    actionGroup_ScreenLayouts->addAction(ui->action_Screen_Layout_Single_Screen);
    actionGroup_ScreenLayouts->addAction(ui->action_Screen_Layout_Large_Screen);
    actionGroup_ScreenLayouts->addAction(ui->action_Screen_Layout_Side_by_Side);
    actionGroup_ScreenLayouts->addAction(ui->action_Screen_Layout_Separate_Windows);
}

void GMainWindow::InitializeDebugWidgets() {
    connect(ui->action_Create_Pica_Surface_Viewer, &QAction::triggered, this,
            &GMainWindow::OnCreateGraphicsSurfaceViewer);

    QMenu* debug_menu = ui->menu_View_Debugging;

#if MICROPROFILE_ENABLED
    microProfileDialog = new MicroProfileDialog(this);
    microProfileDialog->hide();
    debug_menu->addAction(microProfileDialog->toggleViewAction());
#endif

    registersWidget = new RegistersWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, registersWidget);
    registersWidget->hide();
    debug_menu->addAction(registersWidget->toggleViewAction());
    connect(this, &GMainWindow::EmulationStarting, registersWidget,
            &RegistersWidget::OnEmulationStarting);
    connect(this, &GMainWindow::EmulationStopping, registersWidget,
            &RegistersWidget::OnEmulationStopping);

    graphicsWidget = new GPUCommandStreamWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, graphicsWidget);
    graphicsWidget->hide();
    debug_menu->addAction(graphicsWidget->toggleViewAction());

    graphicsCommandsWidget = new GPUCommandListWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, graphicsCommandsWidget);
    graphicsCommandsWidget->hide();
    debug_menu->addAction(graphicsCommandsWidget->toggleViewAction());

    graphicsBreakpointsWidget = new GraphicsBreakPointsWidget(Pica::g_debug_context, this);
    addDockWidget(Qt::RightDockWidgetArea, graphicsBreakpointsWidget);
    graphicsBreakpointsWidget->hide();
    debug_menu->addAction(graphicsBreakpointsWidget->toggleViewAction());

    graphicsVertexShaderWidget = new GraphicsVertexShaderWidget(Pica::g_debug_context, this);
    addDockWidget(Qt::RightDockWidgetArea, graphicsVertexShaderWidget);
    graphicsVertexShaderWidget->hide();
    debug_menu->addAction(graphicsVertexShaderWidget->toggleViewAction());

    graphicsTracingWidget = new GraphicsTracingWidget(Pica::g_debug_context, this);
    addDockWidget(Qt::RightDockWidgetArea, graphicsTracingWidget);
    graphicsTracingWidget->hide();
    debug_menu->addAction(graphicsTracingWidget->toggleViewAction());
    connect(this, &GMainWindow::EmulationStarting, graphicsTracingWidget,
            &GraphicsTracingWidget::OnEmulationStarting);
    connect(this, &GMainWindow::EmulationStopping, graphicsTracingWidget,
            &GraphicsTracingWidget::OnEmulationStopping);

    waitTreeWidget = new WaitTreeWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, waitTreeWidget);
    waitTreeWidget->hide();
    debug_menu->addAction(waitTreeWidget->toggleViewAction());
    connect(this, &GMainWindow::EmulationStarting, waitTreeWidget,
            &WaitTreeWidget::OnEmulationStarting);
    connect(this, &GMainWindow::EmulationStopping, waitTreeWidget,
            &WaitTreeWidget::OnEmulationStopping);

    lleServiceModulesWidget = new LLEServiceModulesWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, lleServiceModulesWidget);
    lleServiceModulesWidget->hide();
    debug_menu->addAction(lleServiceModulesWidget->toggleViewAction());
    connect(this, &GMainWindow::EmulationStarting,
            [this] { lleServiceModulesWidget->setDisabled(true); });
    connect(this, &GMainWindow::EmulationStopping, waitTreeWidget,
            [this] { lleServiceModulesWidget->setDisabled(false); });

    ipcRecorderWidget = new IPCRecorderWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, ipcRecorderWidget);
    ipcRecorderWidget->hide();
    debug_menu->addAction(ipcRecorderWidget->toggleViewAction());
    connect(this, &GMainWindow::EmulationStarting, ipcRecorderWidget,
            &IPCRecorderWidget::OnEmulationStarting);
}

void GMainWindow::InitializeRecentFileMenuActions() {
    for (int i = 0; i < max_recent_files_item; ++i) {
        actions_recent_files[i] = new QAction(this);
        actions_recent_files[i]->setVisible(false);
        connect(actions_recent_files[i], &QAction::triggered, this, &GMainWindow::OnMenuRecentFile);

        ui->menu_recent_files->addAction(actions_recent_files[i]);
    }
    ui->menu_recent_files->addSeparator();
    QAction* action_clear_recent_files = new QAction(this);
    action_clear_recent_files->setText(tr("Clear Recent Files"));
    connect(action_clear_recent_files, &QAction::triggered, this, [this] {
        UISettings::values.recent_files.clear();
        UpdateRecentFiles();
    });
    ui->menu_recent_files->addAction(action_clear_recent_files);

    UpdateRecentFiles();
}

void GMainWindow::InitializeSaveStateMenuActions() {
    for (u32 i = 0; i < Core::SaveStateSlotCount; ++i) {
        actions_load_state[i] = new QAction(this);
        actions_load_state[i]->setData(i + 1);
        connect(actions_load_state[i], &QAction::triggered, this, &GMainWindow::OnLoadState);
        ui->menu_Load_State->addAction(actions_load_state[i]);

        actions_save_state[i] = new QAction(this);
        actions_save_state[i]->setData(i + 1);
        connect(actions_save_state[i], &QAction::triggered, this, &GMainWindow::OnSaveState);
        ui->menu_Save_State->addAction(actions_save_state[i]);
    }

    connect(ui->action_Load_from_Newest_Slot, &QAction::triggered, this, [this] {
        UpdateSaveStates();
        if (newest_slot != 0) {
            actions_load_state[newest_slot - 1]->trigger();
        }
    });
    connect(ui->action_Save_to_Oldest_Slot, &QAction::triggered, this, [this] {
        UpdateSaveStates();
        actions_save_state[oldest_slot - 1]->trigger();
    });

    connect(ui->menu_Load_State->menuAction(), &QAction::hovered, this,
            &GMainWindow::UpdateSaveStates);
    connect(ui->menu_Save_State->menuAction(), &QAction::hovered, this,
            &GMainWindow::UpdateSaveStates);

    UpdateSaveStates();
}

void GMainWindow::InitializeHotkeys() {
    hotkey_registry.LoadHotkeys();

    const QString main_window = QStringLiteral("Main Window");
    const QString fullscreen = QStringLiteral("Fullscreen");
    const QString toggle_screen_layout = QStringLiteral("Toggle Screen Layout");
    const QString swap_screens = QStringLiteral("Swap Screens");
    const QString rotate_screens = QStringLiteral("Rotate Screens Upright");

    const auto link_action_shortcut = [&](QAction* action, const QString& action_name) {
        static const QString main_window = QStringLiteral("Main Window");
        action->setShortcut(hotkey_registry.GetKeySequence(main_window, action_name));
        action->setShortcutContext(hotkey_registry.GetShortcutContext(main_window, action_name));
        action->setAutoRepeat(false);
        this->addAction(action);
    };

    link_action_shortcut(ui->action_Load_File, QStringLiteral("Load File"));
    link_action_shortcut(ui->action_Load_Amiibo, QStringLiteral("Load Amiibo"));
    link_action_shortcut(ui->action_Remove_Amiibo, QStringLiteral("Remove Amiibo"));
    link_action_shortcut(ui->action_Exit, QStringLiteral("Exit Citra"));
    link_action_shortcut(ui->action_Restart, QStringLiteral("Restart Emulation"));
    link_action_shortcut(ui->action_Pause, QStringLiteral("Continue/Pause Emulation"));
    link_action_shortcut(ui->action_Stop, QStringLiteral("Stop Emulation"));
    link_action_shortcut(ui->action_Show_Filter_Bar, QStringLiteral("Toggle Filter Bar"));
    link_action_shortcut(ui->action_Show_Status_Bar, QStringLiteral("Toggle Status Bar"));
    link_action_shortcut(ui->action_Fullscreen, fullscreen);
    link_action_shortcut(ui->action_Capture_Screenshot, QStringLiteral("Capture Screenshot"));
    link_action_shortcut(ui->action_Screen_Layout_Swap_Screens, swap_screens);
    link_action_shortcut(ui->action_Screen_Layout_Upright_Screens, rotate_screens);
    link_action_shortcut(ui->action_Enable_Frame_Advancing,
                         QStringLiteral("Toggle Frame Advancing"));
    link_action_shortcut(ui->action_Advance_Frame, QStringLiteral("Advance Frame"));
    link_action_shortcut(ui->action_Load_from_Newest_Slot, QStringLiteral("Load from Newest Slot"));
    link_action_shortcut(ui->action_Save_to_Oldest_Slot, QStringLiteral("Save to Oldest Slot"));

    const auto add_secondary_window_hotkey = [this](QKeySequence hotkey, const char* slot) {
        // This action will fire specifically when secondary_window is in focus
        QAction* secondary_window_action = new QAction(secondary_window);
        secondary_window_action->setShortcut(hotkey);

        connect(secondary_window_action, SIGNAL(triggered()), this, slot);
        secondary_window->addAction(secondary_window_action);
    };

    // Use the same fullscreen hotkey as the main window
    const auto fullscreen_hotkey = hotkey_registry.GetKeySequence(main_window, fullscreen);
    add_secondary_window_hotkey(fullscreen_hotkey, SLOT(ToggleSecondaryFullscreen()));

    const auto toggle_screen_hotkey =
        hotkey_registry.GetKeySequence(main_window, toggle_screen_layout);
    add_secondary_window_hotkey(toggle_screen_hotkey, SLOT(ToggleScreenLayout()));

    const auto swap_screen_hotkey = hotkey_registry.GetKeySequence(main_window, swap_screens);
    add_secondary_window_hotkey(swap_screen_hotkey, SLOT(TriggerSwapScreens()));

    const auto rotate_screen_hotkey = hotkey_registry.GetKeySequence(main_window, rotate_screens);
    add_secondary_window_hotkey(rotate_screen_hotkey, SLOT(TriggerRotateScreens()));

    const auto connect_shortcut = [&](const QString& action_name, const auto& function) {
        const auto* hotkey = hotkey_registry.GetHotkey(main_window, action_name, this);
        connect(hotkey, &QShortcut::activated, this, function);
    };

    connect(hotkey_registry.GetHotkey(main_window, toggle_screen_layout, render_window),
            &QShortcut::activated, this, &GMainWindow::ToggleScreenLayout);

    connect_shortcut(QStringLiteral("Exit Fullscreen"), [&] {
        if (emulation_running) {
            ui->action_Fullscreen->setChecked(false);
            ToggleFullscreen();
        }
    });
    connect_shortcut(QStringLiteral("Toggle Per-Game Speed"), [&] {
        Settings::values.frame_limit.SetGlobal(!Settings::values.frame_limit.UsingGlobal());
        UpdateStatusBar();
    });
    connect_shortcut(QStringLiteral("Toggle Texture Dumping"),
                     [&] { Settings::values.dump_textures = !Settings::values.dump_textures; });
    // We use "static" here in order to avoid capturing by lambda due to a MSVC bug, which makes
    // the variable hold a garbage value after this function exits
    static constexpr u16 SPEED_LIMIT_STEP = 5;
    connect_shortcut(QStringLiteral("Increase Speed Limit"), [&] {
        if (Settings::values.frame_limit.GetValue() == 0) {
            return;
        }
        if (Settings::values.frame_limit.GetValue() < 995 - SPEED_LIMIT_STEP) {
            Settings::values.frame_limit.SetValue(Settings::values.frame_limit.GetValue() +
                                                  SPEED_LIMIT_STEP);
        } else {
            Settings::values.frame_limit = 0;
        }
        UpdateStatusBar();
    });
    connect_shortcut(QStringLiteral("Decrease Speed Limit"), [&] {
        if (Settings::values.frame_limit.GetValue() == 0) {
            Settings::values.frame_limit = 995;
        } else if (Settings::values.frame_limit.GetValue() > SPEED_LIMIT_STEP) {
            Settings::values.frame_limit.SetValue(Settings::values.frame_limit.GetValue() -
                                                  SPEED_LIMIT_STEP);
            UpdateStatusBar();
        }
        UpdateStatusBar();
    });
    connect_shortcut(QStringLiteral("Mute Audio"),
                     [] { Settings::values.audio_muted = !Settings::values.audio_muted; });

    connect_shortcut(QStringLiteral("Toggle 3D"), &GMainWindow::Toggle3D);

    // We use "static" here in order to avoid capturing by lambda due to a MSVC bug, which makes the
    // variable hold a garbage value after this function exits
    static constexpr u16 FACTOR_3D_STEP = 5;
    connect_shortcut(QStringLiteral("Decrease 3D Factor"), [this] {
        const auto factor_3d = Settings::values.factor_3d.GetValue();
        if (factor_3d > 0) {
            if (factor_3d % FACTOR_3D_STEP != 0) {
                Settings::values.factor_3d = factor_3d - (factor_3d % FACTOR_3D_STEP);
            } else {
                Settings::values.factor_3d = factor_3d - FACTOR_3D_STEP;
            }
            UpdateStatusBar();
        }
    });
    connect_shortcut(QStringLiteral("Increase 3D Factor"), [this] {
        const auto factor_3d = Settings::values.factor_3d.GetValue();
        if (factor_3d < 100) {
            if (factor_3d % FACTOR_3D_STEP != 0) {
                Settings::values.factor_3d =
                    factor_3d + FACTOR_3D_STEP - (factor_3d % FACTOR_3D_STEP);
            } else {
                Settings::values.factor_3d = factor_3d + FACTOR_3D_STEP;
            }
            UpdateStatusBar();
        }
    });
}

void GMainWindow::ShowUpdaterWidgets() {
    ui->action_Check_For_Updates->setVisible(UISettings::values.updater_found);
    ui->action_Open_Maintenance_Tool->setVisible(UISettings::values.updater_found);

    connect(updater, &Updater::CheckUpdatesDone, this, &GMainWindow::OnUpdateFound);
}

void GMainWindow::SetDefaultUIGeometry() {
    // geometry: 55% of the window contents are in the upper screen half, 45% in the lower half
    const QRect screenRect = QApplication::desktop()->screenGeometry(this);

    const int w = screenRect.width() * 2 / 3;
    const int h = screenRect.height() / 2;
    const int x = (screenRect.x() + screenRect.width()) / 2 - w / 2;
    const int y = (screenRect.y() + screenRect.height()) / 2 - h * 55 / 100;

    setGeometry(x, y, w, h);
}

void GMainWindow::RestoreUIState() {
    restoreGeometry(UISettings::values.geometry);
    restoreState(UISettings::values.state);
    render_window->restoreGeometry(UISettings::values.renderwindow_geometry);
#if MICROPROFILE_ENABLED
    microProfileDialog->restoreGeometry(UISettings::values.microprofile_geometry);
    microProfileDialog->setVisible(UISettings::values.microprofile_visible.GetValue());
#endif
    ui->action_Cheats->setEnabled(false);

    game_list->LoadInterfaceLayout();

    ui->action_Single_Window_Mode->setChecked(UISettings::values.single_window_mode.GetValue());
    ToggleWindowMode();

    ui->action_Fullscreen->setChecked(UISettings::values.fullscreen.GetValue());
    SyncMenuUISettings();

    ui->action_Display_Dock_Widget_Headers->setChecked(
        UISettings::values.display_titlebar.GetValue());
    OnDisplayTitleBars(ui->action_Display_Dock_Widget_Headers->isChecked());

    ui->action_Show_Filter_Bar->setChecked(UISettings::values.show_filter_bar.GetValue());
    game_list->SetFilterVisible(ui->action_Show_Filter_Bar->isChecked());

    ui->action_Show_Status_Bar->setChecked(UISettings::values.show_status_bar.GetValue());
    statusBar()->setVisible(ui->action_Show_Status_Bar->isChecked());
}

void GMainWindow::OnAppFocusStateChanged(Qt::ApplicationState state) {
    if (!UISettings::values.pause_when_in_background) {
        return;
    }
    if (state != Qt::ApplicationHidden && state != Qt::ApplicationInactive &&
        state != Qt::ApplicationActive) {
        LOG_DEBUG(Frontend, "ApplicationState unusual flag: {} ", state);
    }
    if (ui->action_Pause->isEnabled() &&
        (state & (Qt::ApplicationHidden | Qt::ApplicationInactive))) {
        auto_paused = true;
        OnPauseGame();
    } else if (emulation_running && !emu_thread->IsRunning() && auto_paused &&
               state == Qt::ApplicationActive) {
        auto_paused = false;
        OnStartGame();
    }
}

void GMainWindow::ConnectWidgetEvents() {
    connect(game_list, &GameList::GameChosen, this, &GMainWindow::OnGameListLoadFile);
    connect(game_list, &GameList::OpenDirectory, this, &GMainWindow::OnGameListOpenDirectory);
    connect(game_list, &GameList::OpenFolderRequested, this, &GMainWindow::OnGameListOpenFolder);
    connect(game_list, &GameList::NavigateToGamedbEntryRequested, this,
            &GMainWindow::OnGameListNavigateToGamedbEntry);
    connect(game_list, &GameList::DumpRomFSRequested, this, &GMainWindow::OnGameListDumpRomFS);
    connect(game_list, &GameList::AddDirectory, this, &GMainWindow::OnGameListAddDirectory);
    connect(game_list_placeholder, &GameListPlaceholder::AddDirectory, this,
            &GMainWindow::OnGameListAddDirectory);
    connect(game_list, &GameList::ShowList, this, &GMainWindow::OnGameListShowList);
    connect(game_list, &GameList::PopulatingCompleted, this,
            [this] { multiplayer_state->UpdateGameList(game_list->GetModel()); });

    connect(game_list, &GameList::OpenPerGameGeneralRequested, this,
            &GMainWindow::OnGameListOpenPerGameProperties);

    connect(this, &GMainWindow::EmulationStarting, render_window,
            &GRenderWindow::OnEmulationStarting);
    connect(this, &GMainWindow::EmulationStopping, render_window,
            &GRenderWindow::OnEmulationStopping);
    connect(this, &GMainWindow::EmulationStarting, secondary_window,
            &GRenderWindow::OnEmulationStarting);
    connect(this, &GMainWindow::EmulationStopping, secondary_window,
            &GRenderWindow::OnEmulationStopping);

    connect(&status_bar_update_timer, &QTimer::timeout, this, &GMainWindow::UpdateStatusBar);

    connect(this, &GMainWindow::UpdateProgress, this, &GMainWindow::OnUpdateProgress);
    connect(this, &GMainWindow::CIAInstallReport, this, &GMainWindow::OnCIAInstallReport);
    connect(this, &GMainWindow::CIAInstallFinished, this, &GMainWindow::OnCIAInstallFinished);
    connect(this, &GMainWindow::UpdateThemedIcons, multiplayer_state,
            &MultiplayerState::UpdateThemedIcons);
}

void GMainWindow::ConnectMenuEvents() {
    const auto connect_menu = [&](QAction* action, const auto& event_fn) {
        connect(action, &QAction::triggered, this, event_fn);
        // Add actions to this window so that hiding menus in fullscreen won't disable them
        addAction(action);
        // Add actions to the render window so that they work outside of single window mode
        render_window->addAction(action);
    };

    // File
    connect_menu(ui->action_Load_File, &GMainWindow::OnMenuLoadFile);
    connect_menu(ui->action_Install_CIA, &GMainWindow::OnMenuInstallCIA);
    connect_menu(ui->action_Exit, &QMainWindow::close);
    connect_menu(ui->action_Load_Amiibo, &GMainWindow::OnLoadAmiibo);
    connect_menu(ui->action_Remove_Amiibo, &GMainWindow::OnRemoveAmiibo);

    // Emulation
    connect_menu(ui->action_Pause, &GMainWindow::OnPauseContinueGame);
    connect_menu(ui->action_Stop, &GMainWindow::OnStopGame);
    connect_menu(ui->action_Restart, [this] { BootGame(QString(game_path)); });
    connect_menu(ui->action_Report_Compatibility, &GMainWindow::OnMenuReportCompatibility);
    connect_menu(ui->action_Configure, &GMainWindow::OnConfigure);
    connect_menu(ui->action_Configure_Current_Game, &GMainWindow::OnConfigurePerGame);
    connect_menu(ui->action_Cheats, &GMainWindow::OnCheats);

    // View
    connect_menu(ui->action_Single_Window_Mode, &GMainWindow::ToggleWindowMode);
    connect_menu(ui->action_Display_Dock_Widget_Headers, &GMainWindow::OnDisplayTitleBars);
    connect_menu(ui->action_Show_Filter_Bar, &GMainWindow::OnToggleFilterBar);
    connect(ui->action_Show_Status_Bar, &QAction::triggered, statusBar(), &QStatusBar::setVisible);

    // Multiplayer
    connect(ui->action_View_Lobby, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnViewLobby);
    connect(ui->action_Start_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnCreateRoom);
    connect(ui->action_Leave_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnCloseRoom);
    connect(ui->action_Connect_To_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnDirectConnectToRoom);
    connect(ui->action_Show_Room, &QAction::triggered, multiplayer_state,
            &MultiplayerState::OnOpenNetworkRoom);

    connect_menu(ui->action_Fullscreen, &GMainWindow::ToggleFullscreen);
    connect_menu(ui->action_Screen_Layout_Default, &GMainWindow::ChangeScreenLayout);
    connect_menu(ui->action_Screen_Layout_Single_Screen, &GMainWindow::ChangeScreenLayout);
    connect_menu(ui->action_Screen_Layout_Large_Screen, &GMainWindow::ChangeScreenLayout);
    connect_menu(ui->action_Screen_Layout_Side_by_Side, &GMainWindow::ChangeScreenLayout);
    connect_menu(ui->action_Screen_Layout_Separate_Windows, &GMainWindow::ChangeScreenLayout);
    connect_menu(ui->action_Screen_Layout_Swap_Screens, &GMainWindow::OnSwapScreens);
    connect_menu(ui->action_Screen_Layout_Upright_Screens, &GMainWindow::OnRotateScreens);

    // Movie
    connect_menu(ui->action_Record_Movie, &GMainWindow::OnRecordMovie);
    connect_menu(ui->action_Play_Movie, &GMainWindow::OnPlayMovie);
    connect_menu(ui->action_Close_Movie, &GMainWindow::OnCloseMovie);
    connect_menu(ui->action_Save_Movie, &GMainWindow::OnSaveMovie);
    connect_menu(ui->action_Movie_Read_Only_Mode,
                 [](bool checked) { Core::Movie::GetInstance().SetReadOnly(checked); });
    connect_menu(ui->action_Enable_Frame_Advancing, [this] {
        if (emulation_running) {
            Core::System::GetInstance().frame_limiter.SetFrameAdvancing(
                ui->action_Enable_Frame_Advancing->isChecked());
            ui->action_Advance_Frame->setEnabled(ui->action_Enable_Frame_Advancing->isChecked());
        }
    });
    connect_menu(ui->action_Advance_Frame, [this] {
        auto& system = Core::System::GetInstance();
        if (emulation_running && system.frame_limiter.IsFrameAdvancing()) {
            ui->action_Enable_Frame_Advancing->setChecked(true);
            ui->action_Advance_Frame->setEnabled(true);
            system.frame_limiter.AdvanceFrame();
        }
    });
    connect_menu(ui->action_Capture_Screenshot, &GMainWindow::OnCaptureScreenshot);

#ifdef ENABLE_FFMPEG_VIDEO_DUMPER
    connect_menu(ui->action_Dump_Video, [this] {
        if (ui->action_Dump_Video->isChecked()) {
            OnStartVideoDumping();
        } else {
            OnStopVideoDumping();
        }
    });
#else
    ui->action_Dump_Video->setEnabled(false);
#endif

    // Help
    connect_menu(ui->action_Open_Citra_Folder, &GMainWindow::OnOpenCitraFolder);
    connect_menu(ui->action_FAQ, []() {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://citra-emu.org/wiki/faq/")));
    });
    connect_menu(ui->action_About, &GMainWindow::OnMenuAboutCitra);
    connect_menu(ui->action_Check_For_Updates, &GMainWindow::OnCheckForUpdates);
    connect_menu(ui->action_Open_Maintenance_Tool, &GMainWindow::OnOpenUpdater);
}

void GMainWindow::UpdateMenuState() {
    const bool is_paused = emu_thread == nullptr || !emu_thread->IsRunning();

    const std::array running_actions{
        ui->action_Stop,
        ui->action_Restart,
        ui->action_Configure_Current_Game,
        ui->action_Report_Compatibility,
        ui->action_Load_Amiibo,
        ui->action_Remove_Amiibo,
        ui->action_Pause,
        ui->action_Cheats,
        ui->action_Advance_Frame,
    };

    for (QAction* action : running_actions) {
        action->setEnabled(emulation_running);
    }

    ui->action_Capture_Screenshot->setEnabled(emulation_running && !is_paused);

    if (emulation_running && is_paused) {
        ui->action_Pause->setText(tr("&Continue"));
    } else {
        ui->action_Pause->setText(tr("&Pause"));
    }
}

void GMainWindow::Connect3DStateEvents() {
    connect(option_3d_button, &QPushButton::clicked, this, &GMainWindow::Toggle3D);
    connect(factor_3d_slider, qOverload<int>(&QSlider::valueChanged), this,
            [](int value) { Settings::values.factor_3d = value; });
}

void GMainWindow::OnDisplayTitleBars(bool show) {
    QList<QDockWidget*> widgets = findChildren<QDockWidget*>();

    if (show) {
        for (QDockWidget* widget : widgets) {
            QWidget* old = widget->titleBarWidget();
            widget->setTitleBarWidget(nullptr);
            if (old != nullptr)
                delete old;
        }
    } else {
        for (QDockWidget* widget : widgets) {
            QWidget* old = widget->titleBarWidget();
            widget->setTitleBarWidget(new QWidget());
            if (old != nullptr)
                delete old;
        }
    }
}

void GMainWindow::OnCheckForUpdates() {
    explicit_update_check = true;
    CheckForUpdates();
}

void GMainWindow::CheckForUpdates() {
    if (updater->CheckForUpdates()) {
        LOG_INFO(Frontend, "Update check started");
    } else {
        LOG_WARNING(Frontend, "Unable to start check for updates");
    }
}

void GMainWindow::OnUpdateFound(bool found, bool error) {
    if (error) {
        LOG_WARNING(Frontend, "Update check failed");
        return;
    }

    if (!found) {
        LOG_INFO(Frontend, "No updates found");

        // If the user explicitly clicked the "Check for Updates" button, we are
        //  going to want to show them a prompt anyway.
        if (explicit_update_check) {
            explicit_update_check = false;
            ShowNoUpdatePrompt();
        }
        return;
    }

    if (emulation_running && !explicit_update_check) {
        LOG_INFO(Frontend, "Update found, deferring as game is running");
        defer_update_prompt = true;
        return;
    }

    LOG_INFO(Frontend, "Update found!");
    explicit_update_check = false;

    ShowUpdatePrompt();
}

void GMainWindow::ShowUpdatePrompt() {
    defer_update_prompt = false;

    auto result =
        QMessageBox::question(this, tr("Update Available"),
                              tr("An update is available. Would you like to install it now?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    if (result == QMessageBox::Yes) {
        updater->LaunchUIOnExit();
        close();
    }
}

void GMainWindow::ShowNoUpdatePrompt() {
    QMessageBox::information(this, tr("No Update Found"), tr("No update is found."),
                             QMessageBox::Ok, QMessageBox::Ok);
}

void GMainWindow::OnOpenUpdater() {
    updater->LaunchUI();
}

void GMainWindow::PreventOSSleep() {
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
#endif
}

void GMainWindow::AllowOSSleep() {
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
}

bool GMainWindow::LoadROM(const QString& filename) {
    // Shutdown previous session if the emu thread is still active...
    if (emu_thread != nullptr)
        ShutdownGame();

    render_window->InitRenderTarget();
    secondary_window->InitRenderTarget();

    Frontend::ScopeAcquireContext scope(*render_window);

    const QString below_gl43_title = tr("OpenGL 4.3 Unsupported");
    const QString below_gl43_message = tr("Your GPU may not support OpenGL 4.3, or you do not "
                                          "have the latest graphics driver.");

    if (!QOpenGLContext::globalShareContext()->versionFunctions<QOpenGLFunctions_4_3_Core>()) {
        QMessageBox::critical(this, below_gl43_title, below_gl43_message);
        return false;
    }

    Core::System& system{Core::System::GetInstance()};

    const Core::System::ResultStatus result{
        system.Load(*render_window, filename.toStdString(), secondary_window)};

    if (result != Core::System::ResultStatus::Success) {
        switch (result) {
        case Core::System::ResultStatus::ErrorGetLoader:
            LOG_CRITICAL(Frontend, "Failed to obtain loader for {}!", filename.toStdString());
            QMessageBox::critical(
                this, tr("Invalid ROM Format"),
                tr("Your ROM format is not supported.<br/>Please follow the guides to redump your "
                   "<a href='https://citra-emu.org/wiki/dumping-game-cartridges/'>game "
                   "cartridges</a> or "
                   "<a href='https://citra-emu.org/wiki/dumping-installed-titles/'>installed "
                   "titles</a>."));
            break;

        case Core::System::ResultStatus::ErrorSystemMode:
            LOG_CRITICAL(Frontend, "Failed to load ROM!");
            QMessageBox::critical(
                this, tr("ROM Corrupted"),
                tr("Your ROM is corrupted. <br/>Please follow the guides to redump your "
                   "<a href='https://citra-emu.org/wiki/dumping-game-cartridges/'>game "
                   "cartridges</a> or "
                   "<a href='https://citra-emu.org/wiki/dumping-installed-titles/'>installed "
                   "titles</a>."));
            break;

        case Core::System::ResultStatus::ErrorLoader_ErrorEncrypted: {
            QMessageBox::critical(
                this, tr("ROM Encrypted"),
                tr("Your ROM is encrypted. <br/>Please follow the guides to redump your "
                   "<a href='https://citra-emu.org/wiki/dumping-game-cartridges/'>game "
                   "cartridges</a> or "
                   "<a href='https://citra-emu.org/wiki/dumping-installed-titles/'>installed "
                   "titles</a>."));
            break;
        }
        case Core::System::ResultStatus::ErrorLoader_ErrorInvalidFormat:
            QMessageBox::critical(
                this, tr("Invalid ROM Format"),
                tr("Your ROM format is not supported.<br/>Please follow the guides to redump your "
                   "<a href='https://citra-emu.org/wiki/dumping-game-cartridges/'>game "
                   "cartridges</a> or "
                   "<a href='https://citra-emu.org/wiki/dumping-installed-titles/'>installed "
                   "titles</a>."));
            break;

        case Core::System::ResultStatus::ErrorLoader_ErrorGbaTitle:
            QMessageBox::critical(this, tr("Unsupported ROM"),
                                  tr("GBA Virtual Console ROMs are not supported by Citra."));
            break;

        case Core::System::ResultStatus::ErrorVideoCore:
            QMessageBox::critical(
                this, tr("Video Core Error"),
                tr("An error has occurred. Please <a "
                   "href='https://community.citra-emu.org/t/how-to-upload-the-log-file/296'>see "
                   "the "
                   "log</a> for more details. "
                   "Ensure that you have the latest graphics drivers for your GPU."));
            break;

        case Core::System::ResultStatus::ErrorVideoCore_ErrorGenericDrivers:
            QMessageBox::critical(
                this, tr("Video Core Error"),
                tr("You are running default Windows drivers "
                   "for your GPU. You need to install the "
                   "proper drivers for your graphics card from the manufacturer's website."));
            break;

        case Core::System::ResultStatus::ErrorVideoCore_ErrorBelowGL43:
            QMessageBox::critical(this, below_gl43_title, below_gl43_message);
            break;

        default:
            QMessageBox::critical(
                this, tr("Error while loading ROM!"),
                tr("An unknown error occurred. Please see the log for more details."));
            break;
        }
        return false;
    }

    std::string title;
    system.GetAppLoader().ReadTitle(title);
    game_title = QString::fromStdString(title);
    UpdateWindowTitle();

    game_path = filename;

    system.TelemetrySession().AddField(Common::Telemetry::FieldType::App, "Frontend", "Qt");
    return true;
}

void GMainWindow::BootGame(const QString& filename) {
    if (filename.endsWith(QStringLiteral(".cia"))) {
        const auto answer = QMessageBox::question(
            this, tr("CIA must be installed before usage"),
            tr("Before using this CIA, you must install it. Do you want to install it now?"),
            QMessageBox::Yes | QMessageBox::No);

        if (answer == QMessageBox::Yes)
            InstallCIA(QStringList(filename));

        return;
    }

    LOG_INFO(Frontend, "Citra starting...");
    StoreRecentFile(filename); // Put the filename on top of the list

    if (movie_record_on_start) {
        Core::Movie::GetInstance().PrepareForRecording();
    }
    if (movie_playback_on_start) {
        Core::Movie::GetInstance().PrepareForPlayback(movie_playback_path.toStdString());
    }

    u64 title_id{0};
    const std::string path = filename.toStdString();
    const auto loader = Loader::GetLoader(path);

    if (loader != nullptr && loader->ReadProgramId(title_id) == Loader::ResultStatus::Success) {
        // Load per game settings
        const std::string name{FileUtil::GetFilename(filename.toStdString())};
        const std::string config_file_name =
            title_id == 0 ? name : fmt::format("{:016X}", title_id);
        Config per_game_config(config_file_name, Config::ConfigType::PerGameConfig);
        Settings::Apply();

        LOG_INFO(Frontend, "Using per game config file for title id {}", config_file_name);
        Settings::LogSettings();
    }

    // Save configurations
    UpdateUISettings();
    game_list->SaveInterfaceLayout();
    config->Save();

    if (!LoadROM(filename))
        return;

    // Set everything up
    if (movie_record_on_start) {
        Core::Movie::GetInstance().StartRecording(movie_record_path.toStdString(),
                                                  movie_record_author.toStdString());
        movie_record_on_start = false;
        movie_record_path.clear();
        movie_record_author.clear();
    }
    if (movie_playback_on_start) {
        Core::Movie::GetInstance().StartPlayback(movie_playback_path.toStdString());
        movie_playback_on_start = false;
        movie_playback_path.clear();
    }

    if (ui->action_Enable_Frame_Advancing->isChecked()) {
        ui->action_Advance_Frame->setEnabled(true);
        Core::System::GetInstance().frame_limiter.SetFrameAdvancing(true);
    } else {
        ui->action_Advance_Frame->setEnabled(false);
    }

    if (video_dumping_on_start) {
        Layout::FramebufferLayout layout{
            Layout::FrameLayoutFromResolutionScale(VideoCore::GetResolutionScaleFactor())};
        if (!Core::System::GetInstance().VideoDumper().StartDumping(
                video_dumping_path.toStdString(), layout)) {

            QMessageBox::critical(
                this, tr("Citra"),
                tr("Could not start video dumping.<br>Refer to the log for details."));
            ui->action_Dump_Video->setChecked(false);
        }
        video_dumping_on_start = false;
        video_dumping_path.clear();
    }

    // Create and start the emulation thread
    emu_thread = std::make_unique<EmuThread>(*render_window);
    emit EmulationStarting(emu_thread.get());
    emu_thread->start();

    connect(render_window, &GRenderWindow::Closed, this, &GMainWindow::OnStopGame);
    connect(render_window, &GRenderWindow::MouseActivity, this, &GMainWindow::OnMouseActivity);
    connect(secondary_window, &GRenderWindow::Closed, this, &GMainWindow::OnStopGame);
    connect(secondary_window, &GRenderWindow::MouseActivity, this, &GMainWindow::OnMouseActivity);

    // BlockingQueuedConnection is important here, it makes sure we've finished refreshing our views
    // before the CPU continues
    connect(emu_thread.get(), &EmuThread::DebugModeEntered, registersWidget,
            &RegistersWidget::OnDebugModeEntered, Qt::BlockingQueuedConnection);
    connect(emu_thread.get(), &EmuThread::DebugModeEntered, waitTreeWidget,
            &WaitTreeWidget::OnDebugModeEntered, Qt::BlockingQueuedConnection);
    connect(emu_thread.get(), &EmuThread::DebugModeLeft, registersWidget,
            &RegistersWidget::OnDebugModeLeft, Qt::BlockingQueuedConnection);
    connect(emu_thread.get(), &EmuThread::DebugModeLeft, waitTreeWidget,
            &WaitTreeWidget::OnDebugModeLeft, Qt::BlockingQueuedConnection);

    connect(emu_thread.get(), &EmuThread::LoadProgress, loading_screen,
            &LoadingScreen::OnLoadProgress, Qt::QueuedConnection);
    connect(emu_thread.get(), &EmuThread::HideLoadingScreen, loading_screen,
            &LoadingScreen::OnLoadComplete);

    // Update the GUI
    registersWidget->OnDebugModeEntered();
    if (ui->action_Single_Window_Mode->isChecked()) {
        game_list->hide();
        game_list_placeholder->hide();
    }
    status_bar_update_timer.start(1000);

    if (UISettings::values.hide_mouse) {
        mouse_hide_timer.start();
        setMouseTracking(true);
    }

    // show and hide the render_window to create the context
    render_window->show();
    render_window->hide();

    loading_screen->Prepare(Core::System::GetInstance().GetAppLoader());
    loading_screen->show();

    emulation_running = true;
    if (ui->action_Fullscreen->isChecked()) {
        ShowFullscreen();
    }

    OnStartGame();
}

void GMainWindow::ShutdownGame() {
    if (!emulation_running) {
        return;
    }

    if (ui->action_Fullscreen->isChecked()) {
        HideFullscreen();
    }

#ifdef ENABLE_FFMPEG_VIDEO_DUMPER
    if (Core::System::GetInstance().VideoDumper().IsDumping()) {
        game_shutdown_delayed = true;
        OnStopVideoDumping();
        return;
    }
#endif

    AllowOSSleep();

    discord_rpc->Pause();
    emu_thread->RequestStop();

    // Release emu threads from any breakpoints
    // This belongs after RequestStop() and before wait() because if emulation stops on a GPU
    // breakpoint after (or before) RequestStop() is called, the emulation would never be able
    // to continue out to the main loop and terminate. Thus wait() would hang forever.
    // TODO(bunnei): This function is not thread safe, but it's being used as if it were
    Pica::g_debug_context->ClearBreakpoints();

    // Frame advancing must be cancelled in order to release the emu thread from waiting
    Core::System::GetInstance().frame_limiter.SetFrameAdvancing(false);

    emit EmulationStopping();

    // Wait for emulation thread to complete and delete it
    emu_thread->wait();
    emu_thread = nullptr;

    OnCloseMovie();

    discord_rpc->Update();

    Camera::QtMultimediaCameraHandler::ReleaseHandlers();

    // The emulation is stopped, so closing the window or not does not matter anymore
    disconnect(render_window, &GRenderWindow::Closed, this, &GMainWindow::OnStopGame);
    disconnect(secondary_window, &GRenderWindow::Closed, this, &GMainWindow::OnStopGame);

    render_window->hide();
    secondary_window->hide();
    loading_screen->hide();
    loading_screen->Clear();

    if (game_list->IsEmpty()) {
        game_list_placeholder->show();
    } else {
        game_list->show();
    }
    game_list->SetFilterFocus();

    setMouseTracking(false);

    // Disable status bar updates
    status_bar_update_timer.stop();
    message_label_used_for_movie = false;
    emu_speed_label->setVisible(false);
    game_fps_label->setVisible(false);
    emu_frametime_label->setVisible(false);

    UpdateSaveStates();

    emulation_running = false;

    if (defer_update_prompt) {
        ShowUpdatePrompt();
    }

    game_title.clear();
    UpdateWindowTitle();

    game_path.clear();

    // Update the GUI
    UpdateMenuState();

    // When closing the game, destroy the GLWindow to clear the context after the game is closed
    render_window->ReleaseRenderTarget();
    secondary_window->ReleaseRenderTarget();
}

void GMainWindow::StoreRecentFile(const QString& filename) {
    UISettings::values.recent_files.prepend(filename);
    UISettings::values.recent_files.removeDuplicates();
    while (UISettings::values.recent_files.size() > max_recent_files_item) {
        UISettings::values.recent_files.removeLast();
    }

    UpdateRecentFiles();
}

void GMainWindow::UpdateRecentFiles() {
    const int num_recent_files =
        std::min(UISettings::values.recent_files.size(), max_recent_files_item);

    for (int i = 0; i < num_recent_files; i++) {
        const QString text = QStringLiteral("&%1. %2").arg(i + 1).arg(
            QFileInfo(UISettings::values.recent_files[i]).fileName());
        actions_recent_files[i]->setText(text);
        actions_recent_files[i]->setData(UISettings::values.recent_files[i]);
        actions_recent_files[i]->setToolTip(UISettings::values.recent_files[i]);
        actions_recent_files[i]->setVisible(true);
    }

    for (int j = num_recent_files; j < max_recent_files_item; ++j) {
        actions_recent_files[j]->setVisible(false);
    }

    // Enable the recent files menu if the list isn't empty
    ui->menu_recent_files->setEnabled(num_recent_files != 0);
}

void GMainWindow::UpdateSaveStates() {
    if (!Core::System::GetInstance().IsPoweredOn()) {
        ui->menu_Load_State->setEnabled(false);
        ui->menu_Save_State->setEnabled(false);
        return;
    }

    ui->menu_Load_State->setEnabled(true);
    ui->menu_Save_State->setEnabled(true);
    ui->action_Load_from_Newest_Slot->setEnabled(false);

    oldest_slot = newest_slot = 0;
    oldest_slot_time = std::numeric_limits<u64>::max();
    newest_slot_time = 0;

    u64 title_id;
    if (Core::System::GetInstance().GetAppLoader().ReadProgramId(title_id) !=
        Loader::ResultStatus::Success) {
        return;
    }
    auto savestates = Core::ListSaveStates(title_id);
    for (u32 i = 0; i < Core::SaveStateSlotCount; ++i) {
        actions_load_state[i]->setEnabled(false);
        actions_load_state[i]->setText(tr("Slot %1").arg(i + 1));
        actions_save_state[i]->setText(tr("Slot %1").arg(i + 1));
    }
    for (const auto& savestate : savestates) {
        const auto text = tr("Slot %1 - %2")
                              .arg(savestate.slot)
                              .arg(QDateTime::fromSecsSinceEpoch(savestate.time)
                                       .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        actions_load_state[savestate.slot - 1]->setEnabled(true);
        actions_load_state[savestate.slot - 1]->setText(text);
        actions_save_state[savestate.slot - 1]->setText(text);

        ui->action_Load_from_Newest_Slot->setEnabled(true);

        if (savestate.time > newest_slot_time) {
            newest_slot = savestate.slot;
            newest_slot_time = savestate.time;
        }
        if (savestate.time < oldest_slot_time) {
            oldest_slot = savestate.slot;
            oldest_slot_time = savestate.time;
        }
    }
    for (u32 i = 0; i < Core::SaveStateSlotCount; ++i) {
        if (!actions_load_state[i]->isEnabled()) {
            // Prefer empty slot
            oldest_slot = i + 1;
            oldest_slot_time = 0;
            break;
        }
    }
}

void GMainWindow::OnGameListLoadFile(QString game_path) {
    if (ConfirmChangeGame()) {
        BootGame(game_path);
    }
}

void GMainWindow::OnGameListOpenFolder(u64 data_id, GameListOpenTarget target) {
    std::string path;
    std::string open_target;

    switch (target) {
    case GameListOpenTarget::SAVE_DATA: {
        open_target = "Save Data";
        std::string sdmc_dir = FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir);
        path = FileSys::ArchiveSource_SDSaveData::GetSaveDataPathFor(sdmc_dir, data_id);
        break;
    }
    case GameListOpenTarget::EXT_DATA: {
        open_target = "Extra Data";
        std::string sdmc_dir = FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir);
        path = FileSys::GetExtDataPathFromId(sdmc_dir, data_id);
        break;
    }
    case GameListOpenTarget::APPLICATION: {
        open_target = "Application";
        auto media_type = Service::AM::GetTitleMediaType(data_id);
        path = Service::AM::GetTitlePath(media_type, data_id) + "content/";
        break;
    }
    case GameListOpenTarget::UPDATE_DATA: {
        open_target = "Update Data";
        path = Service::AM::GetTitlePath(Service::FS::MediaType::SDMC, data_id + 0xe00000000) +
               "content/";
        break;
    }
    case GameListOpenTarget::TEXTURE_DUMP: {
        open_target = "Dumped Textures";
        path = fmt::format("{}textures/{:016X}/",
                           FileUtil::GetUserPath(FileUtil::UserPath::DumpDir), data_id);
        break;
    }
    case GameListOpenTarget::TEXTURE_LOAD: {
        open_target = "Custom Textures";
        path = fmt::format("{}textures/{:016X}/",
                           FileUtil::GetUserPath(FileUtil::UserPath::LoadDir), data_id);
        break;
    }
    case GameListOpenTarget::MODS: {
        open_target = "Mods";
        path = fmt::format("{}mods/{:016X}/", FileUtil::GetUserPath(FileUtil::UserPath::LoadDir),
                           data_id);
        break;
    }
    case GameListOpenTarget::DLC_DATA: {
        open_target = "DLC Data";
        path = fmt::format("{}Nintendo 3DS/00000000000000000000000000000000/"
                           "00000000000000000000000000000000/title/0004008c/{:08x}/content/",
                           FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir), data_id);
        break;
    }
    case GameListOpenTarget::SHADER_CACHE: {
        open_target = "Shader Cache";
        path = FileUtil::GetUserPath(FileUtil::UserPath::ShaderDir);
        break;
    }
    default:
        LOG_ERROR(Frontend, "Unexpected target {}", static_cast<int>(target));
        return;
    }

    QString qpath = QString::fromStdString(path);

    QDir dir(qpath);
    if (!dir.exists()) {
        QMessageBox::critical(
            this, tr("Error Opening %1 Folder").arg(QString::fromStdString(open_target)),
            tr("Folder does not exist!"));
        return;
    }

    LOG_INFO(Frontend, "Opening {} path for data_id={:016x}", open_target, data_id);

    QDesktopServices::openUrl(QUrl::fromLocalFile(qpath));
}

void GMainWindow::OnGameListNavigateToGamedbEntry(u64 program_id,
                                                  const CompatibilityList& compatibility_list) {
    auto it = FindMatchingCompatibilityEntry(compatibility_list, program_id);

    QString directory;
    if (it != compatibility_list.end())
        directory = it->second.second;

    QDesktopServices::openUrl(QUrl(QStringLiteral("https://citra-emu.org/game/") + directory));
}

void GMainWindow::OnGameListDumpRomFS(QString game_path, u64 program_id) {
    auto* dialog = new QProgressDialog(tr("Dumping..."), tr("Cancel"), 0, 0, this);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setWindowFlags(dialog->windowFlags() &
                           ~(Qt::WindowCloseButtonHint | Qt::WindowContextHelpButtonHint));
    dialog->setCancelButton(nullptr);
    dialog->setMinimumDuration(0);
    dialog->setValue(0);

    const auto base_path = fmt::format(
        "{}romfs/{:016X}", FileUtil::GetUserPath(FileUtil::UserPath::DumpDir), program_id);
    const auto update_path =
        fmt::format("{}romfs/{:016X}", FileUtil::GetUserPath(FileUtil::UserPath::DumpDir),
                    program_id | 0x0004000e00000000);
    using FutureWatcher = QFutureWatcher<std::pair<Loader::ResultStatus, Loader::ResultStatus>>;
    auto* future_watcher = new FutureWatcher(this);
    connect(future_watcher, &FutureWatcher::finished, this,
            [this, dialog, base_path, update_path, future_watcher] {
                dialog->hide();
                const auto& [base, update] = future_watcher->result();
                if (base != Loader::ResultStatus::Success) {
                    QMessageBox::critical(
                        this, tr("Citra"),
                        tr("Could not dump base RomFS.\nRefer to the log for details."));
                    return;
                }
                QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(base_path)));
                if (update == Loader::ResultStatus::Success) {
                    QDesktopServices::openUrl(
                        QUrl::fromLocalFile(QString::fromStdString(update_path)));
                }
            });

    auto future = QtConcurrent::run([game_path, base_path, update_path] {
        std::unique_ptr<Loader::AppLoader> loader = Loader::GetLoader(game_path.toStdString());
        return std::make_pair(loader->DumpRomFS(base_path), loader->DumpUpdateRomFS(update_path));
    });
    future_watcher->setFuture(future);
}

void GMainWindow::OnGameListOpenDirectory(const QString& directory) {
    QString path;
    if (directory == QStringLiteral("INSTALLED")) {
        path = QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::SDMCDir) +
                                      "Nintendo "
                                      "3DS/00000000000000000000000000000000/"
                                      "00000000000000000000000000000000/title/00040000");
    } else if (directory == QStringLiteral("SYSTEM")) {
        path = QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::NANDDir) +
                                      "00000000000000000000000000000000/title/00040010");
    } else {
        path = directory;
    }
    if (!QFileInfo::exists(path)) {
        QMessageBox::critical(this, tr("Error Opening %1").arg(path), tr("Folder does not exist!"));
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void GMainWindow::OnGameListAddDirectory() {
    const QString dir_path = QFileDialog::getExistingDirectory(this, tr("Select Directory"));
    if (dir_path.isEmpty())
        return;
    UISettings::GameDir game_dir{dir_path, false, true};
    if (!UISettings::values.game_dirs.contains(game_dir)) {
        UISettings::values.game_dirs.append(game_dir);
        game_list->PopulateAsync(UISettings::values.game_dirs);
    } else {
        LOG_WARNING(Frontend, "Selected directory is already in the game list");
    }
}

void GMainWindow::OnGameListShowList(bool show) {
    if (emulation_running && ui->action_Single_Window_Mode->isChecked())
        return;
    game_list->setVisible(show);
    game_list_placeholder->setVisible(!show);
};

void GMainWindow::OnGameListOpenPerGameProperties(const QString& file) {
    const auto loader = Loader::GetLoader(file.toStdString());

    u64 title_id{};
    if (!loader || loader->ReadProgramId(title_id) != Loader::ResultStatus::Success) {
        QMessageBox::information(this, tr("Properties"),
                                 tr("The game properties could not be loaded."));
        return;
    }

    OpenPerGameConfiguration(title_id, file);
}

void GMainWindow::OnMenuLoadFile() {
    const QString extensions = QStringLiteral("*.").append(
        GameList::supported_file_extensions.join(QStringLiteral(" *.")));
    const QString file_filter = tr("3DS Executable (%1);;All Files (*.*)",
                                   "%1 is an identifier for the 3DS executable file extensions.")
                                    .arg(extensions);
    const QString filename = QFileDialog::getOpenFileName(
        this, tr("Load File"), UISettings::values.roms_path, file_filter);

    if (filename.isEmpty()) {
        return;
    }

    UISettings::values.roms_path = QFileInfo(filename).path();
    BootGame(filename);
}

void GMainWindow::OnMenuInstallCIA() {
    QStringList filepaths = QFileDialog::getOpenFileNames(
        this, tr("Load Files"), UISettings::values.roms_path,
        tr("3DS Installation File (*.CIA*)") + QStringLiteral(";;") + tr("All Files (*.*)"));

    if (filepaths.isEmpty()) {
        return;
    }

    UISettings::values.roms_path = QFileInfo(filepaths[0]).path();
    InstallCIA(filepaths);
}

void GMainWindow::InstallCIA(QStringList filepaths) {
    ui->action_Install_CIA->setEnabled(false);
    game_list->SetDirectoryWatcherEnabled(false);
    progress_bar->show();
    progress_bar->setMaximum(INT_MAX);

    QtConcurrent::run([&, filepaths] {
        Service::AM::InstallStatus status;
        const auto cia_progress = [&](std::size_t written, std::size_t total) {
            emit UpdateProgress(written, total);
        };
        for (const auto& current_path : filepaths) {
            status = Service::AM::InstallCIA(current_path.toStdString(), cia_progress);
            emit CIAInstallReport(status, current_path);
        }
        emit CIAInstallFinished();
    });
}

void GMainWindow::OnUpdateProgress(std::size_t written, std::size_t total) {
    progress_bar->setValue(
        static_cast<int>(INT_MAX * (static_cast<double>(written) / static_cast<double>(total))));
}

void GMainWindow::OnCIAInstallReport(Service::AM::InstallStatus status, QString filepath) {
    QString filename = QFileInfo(filepath).fileName();
    switch (status) {
    case Service::AM::InstallStatus::Success:
        this->statusBar()->showMessage(tr("%1 has been installed successfully.").arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorFailedToOpenFile:
        QMessageBox::critical(this, tr("Unable to open File"),
                              tr("Could not open %1").arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorAborted:
        QMessageBox::critical(
            this, tr("Installation aborted"),
            tr("The installation of %1 was aborted. Please see the log for more details")
                .arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorInvalid:
        QMessageBox::critical(this, tr("Invalid File"), tr("%1 is not a valid CIA").arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorEncrypted:
        QMessageBox::critical(this, tr("Encrypted File"),
                              tr("%1 must be decrypted "
                                 "before being used with Citra. A real 3DS is required.")
                                  .arg(filename));
        break;
    case Service::AM::InstallStatus::ErrorFileNotFound:
        QMessageBox::critical(this, tr("Unable to find File"),
                              tr("Could not find %1").arg(filename));
        break;
    }
}

void GMainWindow::OnCIAInstallFinished() {
    progress_bar->hide();
    progress_bar->setValue(0);
    game_list->SetDirectoryWatcherEnabled(true);
    ui->action_Install_CIA->setEnabled(true);
    game_list->PopulateAsync(UISettings::values.game_dirs);
}

void GMainWindow::OnMenuRecentFile() {
    QAction* action = qobject_cast<QAction*>(sender());
    ASSERT(action);

    const QString filename = action->data().toString();
    if (QFileInfo::exists(filename)) {
        BootGame(filename);
    } else {
        // Display an error message and remove the file from the list.
        QMessageBox::information(this, tr("File not found"),
                                 tr("File \"%1\" not found").arg(filename));

        UISettings::values.recent_files.removeOne(filename);
        UpdateRecentFiles();
    }
}

void GMainWindow::OnStartGame() {
    Camera::QtMultimediaCameraHandler::ResumeCameras();

    PreventOSSleep();

    emu_thread->SetRunning(true);
    qRegisterMetaType<Core::System::ResultStatus>("Core::System::ResultStatus");
    qRegisterMetaType<std::string>("std::string");
    connect(emu_thread.get(), &EmuThread::ErrorThrown, this, &GMainWindow::OnCoreError);

    UpdateMenuState();

    discord_rpc->Update();

    UpdateSaveStates();
}

void GMainWindow::OnRestartGame() {
    Core::System& system = Core::System::GetInstance();
    if (!system.IsPoweredOn()) {
        return;
    }
    // Make a copy since BootGame edits game_path
    BootGame(QString(game_path));
}

void GMainWindow::OnPauseGame() {
    emu_thread->SetRunning(false);
    Camera::QtMultimediaCameraHandler::StopCameras();

    UpdateMenuState();
    AllowOSSleep();
}

void GMainWindow::OnPauseContinueGame() {
    if (emulation_running) {
        if (emu_thread->IsRunning()) {
            OnPauseGame();
        } else {
            OnStartGame();
        }
    }
}

void GMainWindow::OnStopGame() {
    ShutdownGame();
    Settings::RestoreGlobalState(false);
}

void GMainWindow::OnLoadComplete() {
    loading_screen->OnLoadComplete();
    UpdateSecondaryWindowVisibility();
}

void GMainWindow::OnMenuReportCompatibility() {
    if (!NetSettings::values.citra_token.empty() && !NetSettings::values.citra_username.empty()) {
        CompatDB compatdb{this};
        compatdb.exec();
    } else {
        QMessageBox::critical(this, tr("Missing Citra Account"),
                              tr("You must link your Citra account to submit test cases."
                                 "<br/>Go to Emulation &gt; Configure... &gt; Web to do so."));
    }
}

void GMainWindow::ToggleFullscreen() {
    if (!emulation_running) {
        return;
    }
    if (ui->action_Fullscreen->isChecked()) {
        ShowFullscreen();
    } else {
        HideFullscreen();
    }
}

void GMainWindow::ToggleSecondaryFullscreen() {
    if (!emulation_running) {
        return;
    }
    if (secondary_window->isFullScreen()) {
        secondary_window->showNormal();
    } else {
        secondary_window->showFullScreen();
    }
}

void GMainWindow::ShowFullscreen() {
    if (ui->action_Single_Window_Mode->isChecked()) {
        UISettings::values.geometry = saveGeometry();
        ui->menubar->hide();
        statusBar()->hide();
        showFullScreen();
    } else {
        UISettings::values.renderwindow_geometry = render_window->saveGeometry();
        render_window->showFullScreen();
    }
}

void GMainWindow::HideFullscreen() {
    if (ui->action_Single_Window_Mode->isChecked()) {
        statusBar()->setVisible(ui->action_Show_Status_Bar->isChecked());
        ui->menubar->show();
        showNormal();
        restoreGeometry(UISettings::values.geometry);
    } else {
        render_window->showNormal();
        render_window->restoreGeometry(UISettings::values.renderwindow_geometry);
    }
}

void GMainWindow::ToggleWindowMode() {
    if (ui->action_Single_Window_Mode->isChecked()) {
        // Render in the main window...
        render_window->BackupGeometry();
        ui->horizontalLayout->addWidget(render_window);
        render_window->setFocusPolicy(Qt::StrongFocus);
        if (emulation_running) {
            render_window->setVisible(true);
            render_window->setFocus();
            game_list->hide();
        }

    } else {
        // Render in a separate window...
        ui->horizontalLayout->removeWidget(render_window);
        render_window->setParent(nullptr);
        render_window->setFocusPolicy(Qt::NoFocus);
        if (emulation_running) {
            render_window->setVisible(true);
            render_window->RestoreGeometry();
            game_list->show();
        }
    }
}

void GMainWindow::UpdateSecondaryWindowVisibility() {
    if (!emulation_running) {
        return;
    }
    if (Settings::values.layout_option.GetValue() == Settings::LayoutOption::SeparateWindows) {
        secondary_window->RestoreGeometry();
        secondary_window->show();
    } else {
        secondary_window->BackupGeometry();
        secondary_window->hide();
    }
}

void GMainWindow::ChangeScreenLayout() {
    Settings::LayoutOption new_layout = Settings::LayoutOption::Default;

    if (ui->action_Screen_Layout_Default->isChecked()) {
        new_layout = Settings::LayoutOption::Default;
    } else if (ui->action_Screen_Layout_Single_Screen->isChecked()) {
        new_layout = Settings::LayoutOption::SingleScreen;
    } else if (ui->action_Screen_Layout_Large_Screen->isChecked()) {
        new_layout = Settings::LayoutOption::LargeScreen;
    } else if (ui->action_Screen_Layout_Side_by_Side->isChecked()) {
        new_layout = Settings::LayoutOption::SideScreen;
    } else if (ui->action_Screen_Layout_Separate_Windows->isChecked()) {
        new_layout = Settings::LayoutOption::SeparateWindows;
    }

    Settings::values.layout_option = new_layout;
    Settings::Apply();
    UpdateSecondaryWindowVisibility();
}

void GMainWindow::ToggleScreenLayout() {
    const Settings::LayoutOption new_layout = []() {
        switch (Settings::values.layout_option.GetValue()) {
        case Settings::LayoutOption::Default:
            return Settings::LayoutOption::SingleScreen;
        case Settings::LayoutOption::SingleScreen:
            return Settings::LayoutOption::LargeScreen;
        case Settings::LayoutOption::LargeScreen:
            return Settings::LayoutOption::SideScreen;
        case Settings::LayoutOption::SideScreen:
            return Settings::LayoutOption::SeparateWindows;
        case Settings::LayoutOption::SeparateWindows:
            return Settings::LayoutOption::Default;
        default:
            LOG_ERROR(Frontend, "Unknown layout option {}",
                      Settings::values.layout_option.GetValue());
            return Settings::LayoutOption::Default;
        }
    }();

    Settings::values.layout_option = new_layout;
    SyncMenuUISettings();
    Settings::Apply();
    UpdateSecondaryWindowVisibility();
}

void GMainWindow::OnSwapScreens() {
    Settings::values.swap_screen = ui->action_Screen_Layout_Swap_Screens->isChecked();
    Settings::Apply();
}

void GMainWindow::OnRotateScreens() {
    Settings::values.upright_screen = ui->action_Screen_Layout_Upright_Screens->isChecked();
    Settings::Apply();
}

void GMainWindow::TriggerSwapScreens() {
    ui->action_Screen_Layout_Swap_Screens->trigger();
}

void GMainWindow::TriggerRotateScreens() {
    ui->action_Screen_Layout_Upright_Screens->trigger();
}

void GMainWindow::OnCheats() {
    CheatDialog cheat_dialog(this);
    cheat_dialog.exec();
}

void GMainWindow::OnSaveState() {
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);

    Core::System::GetInstance().SendSignal(Core::System::Signal::Save, action->data().toUInt());
    Core::System::GetInstance().frame_limiter.AdvanceFrame();
    newest_slot = action->data().toUInt();
}

void GMainWindow::OnLoadState() {
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);

    Core::System::GetInstance().SendSignal(Core::System::Signal::Load, action->data().toUInt());
    Core::System::GetInstance().frame_limiter.AdvanceFrame();
}

void GMainWindow::OnConfigure() {
    game_list->SetDirectoryWatcherEnabled(false);
    Settings::SetConfiguringGlobal(true);
    ConfigureDialog configureDialog(this, hotkey_registry,
                                    !multiplayer_state->IsHostingPublicRoom());
    connect(&configureDialog, &ConfigureDialog::LanguageChanged, this,
            &GMainWindow::OnLanguageChanged);
    auto old_theme = UISettings::values.theme;
    const int old_input_profile_index = Settings::values.current_input_profile_index;
    const auto old_input_profiles = Settings::values.input_profiles;
    const auto old_touch_from_button_maps = Settings::values.touch_from_button_maps;
    const bool old_discord_presence = UISettings::values.enable_discord_presence.GetValue();
    auto result = configureDialog.exec();
    game_list->SetDirectoryWatcherEnabled(true);
    if (result == QDialog::Accepted) {
        configureDialog.ApplyConfiguration();
        InitializeHotkeys();
        if (UISettings::values.theme != old_theme)
            UpdateUITheme();
        if (UISettings::values.enable_discord_presence.GetValue() != old_discord_presence)
            SetDiscordEnabled(UISettings::values.enable_discord_presence.GetValue());
        if (!multiplayer_state->IsHostingPublicRoom())
            multiplayer_state->UpdateCredentials();
        emit UpdateThemedIcons();
        SyncMenuUISettings();
        game_list->RefreshGameDirectory();
        config->Save();
        if (UISettings::values.hide_mouse && emulation_running) {
            setMouseTracking(true);
            mouse_hide_timer.start();
        } else {
            setMouseTracking(false);
        }
        UpdateSecondaryWindowVisibility();
        Update3DState();
    } else {
        Settings::values.input_profiles = old_input_profiles;
        Settings::values.touch_from_button_maps = old_touch_from_button_maps;
        Settings::LoadProfile(old_input_profile_index);
    }
}

void GMainWindow::OnLoadAmiibo() {
    if (emu_thread == nullptr || !emu_thread->IsRunning()) {
        return;
    }

    const QString extensions{QStringLiteral("*.bin")};
    const QString file_filter = tr("Amiibo File (%1);; All Files (*.*)").arg(extensions);
    const QString filename = QFileDialog::getOpenFileName(this, tr("Load Amiibo"), {}, file_filter);

    if (filename.isEmpty()) {
        return;
    }

    LoadAmiibo(filename);
}

void GMainWindow::LoadAmiibo(const QString& filename) {
    Core::System& system{Core::System::GetInstance()};
    Service::SM::ServiceManager& sm = system.ServiceManager();
    auto nfc = sm.GetService<Service::NFC::Module::Interface>("nfc:u");
    if (nfc == nullptr) {
        return;
    }

    QFile nfc_file{filename};
    if (!nfc_file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Error opening Amiibo data file"),
                             tr("Unable to open Amiibo file \"%1\" for reading.").arg(filename));
        return;
    }

    Service::NFC::AmiiboData amiibo_data{};
    const u64 read_size =
        nfc_file.read(reinterpret_cast<char*>(&amiibo_data), sizeof(Service::NFC::AmiiboData));
    if (read_size != sizeof(Service::NFC::AmiiboData)) {
        QMessageBox::warning(this, tr("Error reading Amiibo data file"),
                             tr("Unable to fully read Amiibo data. Expected to read %1 bytes, but "
                                "was only able to read %2 bytes.")
                                 .arg(sizeof(Service::NFC::AmiiboData))
                                 .arg(read_size));
        return;
    }

    nfc->LoadAmiibo(amiibo_data);
    ui->action_Remove_Amiibo->setEnabled(true);
}

void GMainWindow::OnRemoveAmiibo() {
    Core::System& system{Core::System::GetInstance()};
    Service::SM::ServiceManager& sm = system.ServiceManager();
    auto nfc = sm.GetService<Service::NFC::Module::Interface>("nfc:u");
    if (nfc == nullptr) {
        return;
    }

    nfc->RemoveAmiibo();
    ui->action_Remove_Amiibo->setEnabled(false);
}

void GMainWindow::OnOpenCitraFolder() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(
        QString::fromStdString(FileUtil::GetUserPath(FileUtil::UserPath::UserDir))));
}

void GMainWindow::OnToggleFilterBar() {
    game_list->SetFilterVisible(ui->action_Show_Filter_Bar->isChecked());
    if (ui->action_Show_Filter_Bar->isChecked()) {
        game_list->SetFilterFocus();
    } else {
        game_list->ClearFilter();
    }
}

void GMainWindow::OnCreateGraphicsSurfaceViewer() {
    auto graphicsSurfaceViewerWidget = new GraphicsSurfaceWidget(Pica::g_debug_context, this);
    addDockWidget(Qt::RightDockWidgetArea, graphicsSurfaceViewerWidget);
    // TODO: Maybe graphicsSurfaceViewerWidget->setFloating(true);
    graphicsSurfaceViewerWidget->show();
}

void GMainWindow::OnRecordMovie() {
    MovieRecordDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    movie_record_on_start = true;
    movie_record_path = dialog.GetPath();
    movie_record_author = dialog.GetAuthor();

    if (emulation_running) { // Restart game
        BootGame(QString(game_path));
    }
    ui->action_Close_Movie->setEnabled(true);
    ui->action_Save_Movie->setEnabled(true);
}

void GMainWindow::OnPlayMovie() {
    MoviePlayDialog dialog(this, game_list);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    movie_playback_on_start = true;
    movie_playback_path = dialog.GetMoviePath();
    BootGame(dialog.GetGamePath());

    ui->action_Close_Movie->setEnabled(true);
    ui->action_Save_Movie->setEnabled(false);
}

void GMainWindow::OnCloseMovie() {
    if (movie_record_on_start) {
        QMessageBox::information(this, tr("Record Movie"), tr("Movie recording cancelled."));
        movie_record_on_start = false;
        movie_record_path.clear();
        movie_record_author.clear();
    } else {
        const bool was_running = emu_thread && emu_thread->IsRunning();
        if (was_running) {
            OnPauseGame();
        }

        const bool was_recording =
            Core::Movie::GetInstance().GetPlayMode() == Core::Movie::PlayMode::Recording;
        Core::Movie::GetInstance().Shutdown();
        if (was_recording) {
            QMessageBox::information(this, tr("Movie Saved"),
                                     tr("The movie is successfully saved."));
        }

        if (was_running) {
            OnStartGame();
        }
    }

    ui->action_Close_Movie->setEnabled(false);
    ui->action_Save_Movie->setEnabled(false);
}

void GMainWindow::OnSaveMovie() {
    const bool was_running = emu_thread && emu_thread->IsRunning();
    if (was_running) {
        OnPauseGame();
    }

    if (Core::Movie::GetInstance().GetPlayMode() == Core::Movie::PlayMode::Recording) {
        Core::Movie::GetInstance().SaveMovie();
        QMessageBox::information(this, tr("Movie Saved"), tr("The movie is successfully saved."));
    } else {
        LOG_ERROR(Frontend, "Tried to save movie while movie is not being recorded");
    }

    if (was_running) {
        OnStartGame();
    }
}

void GMainWindow::OnCaptureScreenshot() {
    if (emu_thread == nullptr || !emu_thread->IsRunning()) {
        return;
    }

    OnPauseGame();
    std::string path = UISettings::values.screenshot_path.GetValue();
    if (!FileUtil::IsDirectory(path)) {
        if (!FileUtil::CreateFullPath(path)) {
            QMessageBox::information(this, tr("Invalid Screenshot Directory"),
                                     tr("Cannot create specified screenshot directory. Screenshot "
                                        "path is set back to its default value."));
            path = FileUtil::GetUserPath(FileUtil::UserPath::UserDir);
            path.append("screenshots/");
            UISettings::values.screenshot_path = path;
        };
    }
    const std::string filename =
        game_title.remove(QRegularExpression(QStringLiteral("[\\/:?\"<>|]"))).toStdString();
    const std::string timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("dd.MM.yy_hh.mm.ss.z")).toStdString();
    path.append(fmt::format("/{}_{}.png", filename, timestamp));

    auto* const screenshot_window = secondary_window->HasFocus() ? secondary_window : render_window;
    screenshot_window->CaptureScreenshot(UISettings::values.screenshot_resolution_factor.GetValue(),
                                         QString::fromStdString(path));
    OnStartGame();
}

#ifdef ENABLE_FFMPEG_VIDEO_DUMPER
void GMainWindow::OnStartVideoDumping() {
    DumpingDialog dialog(this);
    if (dialog.exec() != QDialog::DialogCode::Accepted) {
        ui->action_Dump_Video->setChecked(false);
        return;
    }
    const auto path = dialog.GetFilePath();
    if (emulation_running) {
        Layout::FramebufferLayout layout{
            Layout::FrameLayoutFromResolutionScale(VideoCore::GetResolutionScaleFactor())};
        if (!Core::System::GetInstance().VideoDumper().StartDumping(path.toStdString(), layout)) {
            QMessageBox::critical(
                this, tr("Citra"),
                tr("Could not start video dumping.<br>Refer to the log for details."));
            ui->action_Dump_Video->setChecked(false);
        }
    } else {
        video_dumping_on_start = true;
        video_dumping_path = path;
    }
}

void GMainWindow::OnStopVideoDumping() {
    ui->action_Dump_Video->setChecked(false);

    if (video_dumping_on_start) {
        video_dumping_on_start = false;
        video_dumping_path.clear();
    } else {
        const bool was_dumping = Core::System::GetInstance().VideoDumper().IsDumping();
        if (!was_dumping)
            return;

        game_paused_for_dumping = emu_thread->IsRunning();
        OnPauseGame();

        auto future =
            QtConcurrent::run([] { Core::System::GetInstance().VideoDumper().StopDumping(); });
        auto* future_watcher = new QFutureWatcher<void>(this);
        connect(future_watcher, &QFutureWatcher<void>::finished, this, [this] {
            if (game_shutdown_delayed) {
                game_shutdown_delayed = false;
                ShutdownGame();
            } else if (game_paused_for_dumping) {
                game_paused_for_dumping = false;
                OnStartGame();
            }
        });
        future_watcher->setFuture(future);
    }
}
#endif

void GMainWindow::UpdateStatusBar() {
    if (emu_thread == nullptr) {
        status_bar_update_timer.stop();
        return;
    }

    // Update movie status
    const u64 current = Core::Movie::GetInstance().GetCurrentInputIndex();
    const u64 total = Core::Movie::GetInstance().GetTotalInputCount();
    const auto play_mode = Core::Movie::GetInstance().GetPlayMode();
    if (play_mode == Core::Movie::PlayMode::Recording) {
        message_label->setText(tr("Recording %1").arg(current));
        message_label_used_for_movie = true;
        ui->action_Save_Movie->setEnabled(true);
    } else if (play_mode == Core::Movie::PlayMode::Playing) {
        message_label->setText(tr("Playing %1 / %2").arg(current, total));
        message_label_used_for_movie = true;
        ui->action_Save_Movie->setEnabled(false);
    } else if (play_mode == Core::Movie::PlayMode::MovieFinished) {
        message_label->setText(tr("Movie Finished"));
        message_label_used_for_movie = true;
        ui->action_Save_Movie->setEnabled(false);
    } else if (message_label_used_for_movie) { // Clear the label if movie was just closed
        message_label->setText(QString{});
        message_label_used_for_movie = false;
        ui->action_Save_Movie->setEnabled(false);
    }

    auto results = Core::System::GetInstance().GetAndResetPerfStats();

    if (Settings::values.frame_limit.GetValue() == 0) {
        emu_speed_label->setText(tr("Speed: %1%").arg(results.emulation_speed * 100.0, 0, 'f', 0));
    } else {
        emu_speed_label->setText(tr("Speed: %1% / %2%")
                                     .arg(results.emulation_speed * 100.0, 0, 'f', 0)
                                     .arg(Settings::values.frame_limit.GetValue()));
    }
    game_fps_label->setText(tr("Game: %1 FPS").arg(results.game_fps, 0, 'f', 0));
    emu_frametime_label->setText(tr("Frame: %1 ms").arg(results.frametime * 1000.0, 0, 'f', 2));

    emu_speed_label->setVisible(true);
    game_fps_label->setVisible(true);
    emu_frametime_label->setVisible(true);
}

void GMainWindow::Update3DState() {
    static const std::array options_3d = {tr("Off"), tr("Side by Side"), tr("Anaglyph"),
                                          tr("Interlaced"), tr("Reverse Interlaced")};

    option_3d_button->setText(
        tr("3D: %1").arg(options_3d[static_cast<int>(Settings::values.render_3d.GetValue())]));

    factor_3d_slider->setValue(Settings::values.factor_3d.GetValue());
    factor_3d_slider->setVisible(Settings::values.render_3d.GetValue() !=
                                 Settings::StereoRenderOption::Off);
}

void GMainWindow::HideMouseCursor() {
    if (emu_thread == nullptr || !UISettings::values.hide_mouse.GetValue()) {
        mouse_hide_timer.stop();
        ShowMouseCursor();
        return;
    }
    render_window->setCursor(QCursor(Qt::BlankCursor));
    secondary_window->setCursor(QCursor(Qt::BlankCursor));
    if (UISettings::values.single_window_mode.GetValue()) {
        setCursor(QCursor(Qt::BlankCursor));
    }
}

void GMainWindow::ShowMouseCursor() {
    unsetCursor();
    render_window->unsetCursor();
    secondary_window->unsetCursor();
    if (emu_thread != nullptr && UISettings::values.hide_mouse) {
        mouse_hide_timer.start();
    }
}

void GMainWindow::OnMouseActivity() {
    ShowMouseCursor();
}

void GMainWindow::mouseMoveEvent([[maybe_unused]] QMouseEvent* event) {
    OnMouseActivity();
}

void GMainWindow::mousePressEvent([[maybe_unused]] QMouseEvent* event) {
    OnMouseActivity();
}

void GMainWindow::mouseReleaseEvent([[maybe_unused]] QMouseEvent* event) {
    OnMouseActivity();
}

void GMainWindow::OnCoreError(Core::System::ResultStatus result, std::string details) {
    QString status_message;

    QString title, message;
    QMessageBox::Icon error_severity_icon;
    if (result == Core::System::ResultStatus::ErrorSystemFiles) {
        const QString common_message =
            tr("%1 is missing. Please <a "
               "href='https://citra-emu.org/wiki/"
               "dumping-system-archives-and-the-shared-fonts-from-a-3ds-console/'>dump your "
               "system archives</a>.<br/>Continuing emulation may result in crashes and bugs.");

        if (!details.empty()) {
            message = common_message.arg(QString::fromStdString(details));
        } else {
            message = common_message.arg(tr("A system archive"));
        }

        title = tr("System Archive Not Found");
        status_message = tr("System Archive Missing");
        error_severity_icon = QMessageBox::Icon::Critical;
    } else if (result == Core::System::ResultStatus::ErrorSavestate) {
        title = tr("Save/load Error");
        message = QString::fromStdString(details);
        error_severity_icon = QMessageBox::Icon::Warning;
    } else {
        title = tr("Fatal Error");
        message =
            tr("A fatal error occurred. "
               "<a href='https://community.citra-emu.org/t/how-to-upload-the-log-file/296'>Check "
               "the log</a> for details."
               "<br/>Continuing emulation may result in crashes and bugs.");
        status_message = tr("Fatal Error encountered");
        error_severity_icon = QMessageBox::Icon::Critical;
    }

    QMessageBox message_box;
    message_box.setWindowTitle(title);
    message_box.setText(message);
    message_box.setIcon(error_severity_icon);
    if (error_severity_icon == QMessageBox::Icon::Critical) {
        message_box.addButton(tr("Continue"), QMessageBox::RejectRole);
        QPushButton* abort_button = message_box.addButton(tr("Quit Game"), QMessageBox::AcceptRole);
        if (result != Core::System::ResultStatus::ShutdownRequested)
            message_box.exec();

        if (result == Core::System::ResultStatus::ShutdownRequested ||
            message_box.clickedButton() == abort_button) {
            if (emu_thread) {
                ShutdownGame();
                return;
            }
        }
    } else {
        // This block should run when the error isn't too big of a deal
        // e.g. when a save state can't be saved or loaded
        message_box.addButton(tr("OK"), QMessageBox::RejectRole);
        message_box.exec();
    }

    // Only show the message if the game is still running.
    if (emu_thread) {
        emu_thread->SetRunning(true);
        message_label->setText(status_message);
        message_label_used_for_movie = false;
    }
}

void GMainWindow::OnMenuAboutCitra() {
    AboutDialog about{this};
    about.exec();
}

void GMainWindow::Toggle3D() {
    Settings::values.render_3d = static_cast<Settings::StereoRenderOption>(
        (static_cast<int>(Settings::values.render_3d.GetValue()) + 1) % num_options_3d);
    Update3DState();
}

bool GMainWindow::ConfirmClose() {
    if (emu_thread == nullptr || !UISettings::values.confirm_before_closing)
        return true;

    QMessageBox::StandardButton answer =
        QMessageBox::question(this, tr("Citra"), tr("Would you like to exit now?"),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}

void GMainWindow::closeEvent(QCloseEvent* event) {
    if (!ConfirmClose()) {
        event->ignore();
        return;
    }

    UpdateUISettings();
    game_list->SaveInterfaceLayout();
    hotkey_registry.SaveHotkeys();

    // Shutdown session if the emu thread is active...
    if (emu_thread != nullptr)
        ShutdownGame();

    render_window->close();
    secondary_window->close();
    multiplayer_state->Close();
    InputCommon::Shutdown();
    QWidget::closeEvent(event);
}

static bool IsSingleFileDropEvent(const QMimeData* mime) {
    return mime->hasUrls() && mime->urls().length() == 1;
}

static const std::array<std::string, 8> AcceptedExtensions = {"cci",  "3ds", "cxi", "bin",
                                                              "3dsx", "app", "elf", "axf"};

static bool IsCorrectFileExtension(const QMimeData* mime) {
    const QString& filename = mime->urls().at(0).toLocalFile();
    return std::find(AcceptedExtensions.begin(), AcceptedExtensions.end(),
                     QFileInfo(filename).suffix().toStdString()) != AcceptedExtensions.end();
}

static bool IsAcceptableDropEvent(QDropEvent* event) {
    return IsSingleFileDropEvent(event->mimeData()) && IsCorrectFileExtension(event->mimeData());
}

void GMainWindow::AcceptDropEvent(QDropEvent* event) {
    if (IsAcceptableDropEvent(event)) {
        event->setDropAction(Qt::DropAction::LinkAction);
        event->accept();
    }
}

bool GMainWindow::DropAction(QDropEvent* event) {
    if (!IsAcceptableDropEvent(event)) {
        return false;
    }

    const QMimeData* mime_data = event->mimeData();
    const QString& filename = mime_data->urls().at(0).toLocalFile();

    if (emulation_running && QFileInfo(filename).suffix() == QStringLiteral("bin")) {
        // Amiibo
        LoadAmiibo(filename);
    } else {
        // Game
        if (ConfirmChangeGame()) {
            BootGame(filename);
        }
    }
    return true;
}

void GMainWindow::dropEvent(QDropEvent* event) {
    DropAction(event);
}

void GMainWindow::dragEnterEvent(QDragEnterEvent* event) {
    AcceptDropEvent(event);
}

void GMainWindow::dragMoveEvent(QDragMoveEvent* event) {
    AcceptDropEvent(event);
}

bool GMainWindow::ConfirmChangeGame() {
    if (emu_thread == nullptr)
        return true;

    auto answer = QMessageBox::question(
        this, tr("Citra"), tr("The game is still running. Would you like to stop emulation?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}

void GMainWindow::filterBarSetChecked(bool state) {
    ui->action_Show_Filter_Bar->setChecked(state);
    emit(OnToggleFilterBar());
}

void GMainWindow::UpdateUITheme() {
    const QString default_icons = QStringLiteral(":/icons/default");
    const QString& current_theme = UISettings::values.theme;
    const bool is_default_theme = current_theme == QString::fromUtf8(UISettings::themes[0].second);
    QStringList theme_paths(default_theme_paths);

    if (is_default_theme || current_theme.isEmpty()) {
        const QString theme_uri(QStringLiteral(":default/style.qss"));
        QFile f(theme_uri);
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream ts(&f);
            qApp->setStyleSheet(ts.readAll());
            setStyleSheet(ts.readAll());
        } else {
            LOG_ERROR(Frontend,
                      "Unable to open default stylesheet, falling back to empty stylesheet");
            qApp->setStyleSheet({});
            setStyleSheet({});
        }
        theme_paths.append(default_icons);
        QIcon::setThemeName(default_icons);
    } else {
        const QString theme_uri(QLatin1Char{':'} + current_theme + QStringLiteral("/style.qss"));
        QFile f(theme_uri);
        if (f.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream ts(&f);
            qApp->setStyleSheet(ts.readAll());
            setStyleSheet(ts.readAll());
        } else {
            LOG_ERROR(Frontend, "Unable to set style, stylesheet file not found");
        }

        const QString theme_name = QStringLiteral(":/icons/") + current_theme;
        theme_paths.append({default_icons, theme_name});
        QIcon::setThemeName(theme_name);
    }

    QIcon::setThemeSearchPaths(theme_paths);
}

void GMainWindow::LoadTranslation() {
    // If the selected language is English, no need to install any translation
    if (UISettings::values.language == QStringLiteral("en")) {
        return;
    }

    bool loaded;

    if (UISettings::values.language.isEmpty()) {
        // If the selected language is empty, use system locale
        loaded = translator.load(QLocale(), {}, {}, QStringLiteral(":/languages/"));
    } else {
        // Otherwise load from the specified file
        loaded = translator.load(UISettings::values.language, QStringLiteral(":/languages/"));
    }

    if (loaded) {
        qApp->installTranslator(&translator);
    } else {
        UISettings::values.language = QStringLiteral("en");
    }
}

void GMainWindow::OnLanguageChanged(const QString& locale) {
    if (UISettings::values.language != QStringLiteral("en")) {
        qApp->removeTranslator(&translator);
    }

    UISettings::values.language = locale;
    LoadTranslation();
    ui->retranslateUi(this);
    RetranslateStatusBar();
    UpdateWindowTitle();
}

void GMainWindow::OnConfigurePerGame() {
    u64 title_id{};
    Core::System::GetInstance().GetAppLoader().ReadProgramId(title_id);
    OpenPerGameConfiguration(title_id, game_path);
}

void GMainWindow::OpenPerGameConfiguration(u64 title_id, const QString& file_name) {
    Core::System& system = Core::System::GetInstance();

    Settings::SetConfiguringGlobal(false);
    ConfigurePerGame dialog(this, title_id, file_name, system);
    const auto result = dialog.exec();

    if (result != QDialog::Accepted) {
        Settings::RestoreGlobalState(system.IsPoweredOn());
        return;
    } else if (result == QDialog::Accepted) {
        dialog.ApplyConfiguration();
    }

    // Do not cause the global config to write local settings into the config file
    const bool is_powered_on = system.IsPoweredOn();
    Settings::RestoreGlobalState(system.IsPoweredOn());

    if (!is_powered_on) {
        config->Save();
    }
}

void GMainWindow::OnMoviePlaybackCompleted() {
    OnPauseGame();
    QMessageBox::information(this, tr("Playback Completed"), tr("Movie playback completed."));
}

void GMainWindow::UpdateWindowTitle() {
    const QString full_name = QString::fromUtf8(Common::g_build_fullname);

    if (game_title.isEmpty()) {
        setWindowTitle(tr("Citra %1").arg(full_name));
    } else {
        setWindowTitle(tr("Citra %1| %2").arg(full_name, game_title));
    }
}

void GMainWindow::UpdateUISettings() {
    if (!ui->action_Fullscreen->isChecked()) {
        UISettings::values.geometry = saveGeometry();
        UISettings::values.renderwindow_geometry = render_window->saveGeometry();
    }
    UISettings::values.state = saveState();
#if MICROPROFILE_ENABLED
    UISettings::values.microprofile_geometry = microProfileDialog->saveGeometry();
    UISettings::values.microprofile_visible = microProfileDialog->isVisible();
#endif
    UISettings::values.single_window_mode = ui->action_Single_Window_Mode->isChecked();
    UISettings::values.fullscreen = ui->action_Fullscreen->isChecked();
    UISettings::values.display_titlebar = ui->action_Display_Dock_Widget_Headers->isChecked();
    UISettings::values.show_filter_bar = ui->action_Show_Filter_Bar->isChecked();
    UISettings::values.show_status_bar = ui->action_Show_Status_Bar->isChecked();
    UISettings::values.first_start = false;
}

void GMainWindow::SyncMenuUISettings() {
    ui->action_Screen_Layout_Default->setChecked(Settings::values.layout_option.GetValue() ==
                                                 Settings::LayoutOption::Default);
    ui->action_Screen_Layout_Single_Screen->setChecked(Settings::values.layout_option.GetValue() ==
                                                       Settings::LayoutOption::SingleScreen);
    ui->action_Screen_Layout_Large_Screen->setChecked(Settings::values.layout_option.GetValue() ==
                                                      Settings::LayoutOption::LargeScreen);
    ui->action_Screen_Layout_Side_by_Side->setChecked(Settings::values.layout_option.GetValue() ==
                                                      Settings::LayoutOption::SideScreen);
    ui->action_Screen_Layout_Separate_Windows->setChecked(
        Settings::values.layout_option.GetValue() == Settings::LayoutOption::SeparateWindows);
    ui->action_Screen_Layout_Swap_Screens->setChecked(Settings::values.swap_screen.GetValue());
    ui->action_Screen_Layout_Upright_Screens->setChecked(
        Settings::values.upright_screen.GetValue());
}

void GMainWindow::RetranslateStatusBar() {
    if (emu_thread)
        UpdateStatusBar();

    emu_speed_label->setToolTip(tr("Current emulation speed. Values higher or lower than 100% "
                                   "indicate emulation is running faster or slower than a 3DS."));
    game_fps_label->setToolTip(tr("How many frames per second the game is currently displaying. "
                                  "This will vary from game to game and scene to scene."));
    emu_frametime_label->setToolTip(
        tr("Time taken to emulate a 3DS frame, not counting framelimiting or v-sync. For "
           "full-speed emulation this should be at most 16.67 ms."));

    multiplayer_state->retranslateUi();
}

void GMainWindow::SetDiscordEnabled([[maybe_unused]] bool state) {
#ifdef USE_DISCORD_PRESENCE
    if (state) {
        discord_rpc = std::make_unique<DiscordRPC::DiscordImpl>();
    } else {
        discord_rpc = std::make_unique<DiscordRPC::NullImpl>();
    }
#else
    discord_rpc = std::make_unique<DiscordRPC::NullImpl>();
#endif
    discord_rpc->Update();
}

#ifdef main
#undef main
#endif

static void SetHighDPIAttributes() {
#ifdef _WIN32
    // For Windows, we want to avoid scaling artifacts on fractional scaling ratios.
    // This is done by setting the optimal scaling policy for the primary screen.

    // Create a temporary QApplication.
    int temp_argc = 0;
    char** temp_argv = nullptr;
    QApplication temp{temp_argc, temp_argv};

    // Get the current screen geometry.
    const QScreen* primary_screen = QGuiApplication::primaryScreen();
    if (primary_screen == nullptr) {
        return;
    }

    const QRect screen_rect = primary_screen->geometry();
    const int real_width = screen_rect.width();
    const int real_height = screen_rect.height();
    const float real_ratio = primary_screen->logicalDotsPerInch() / 96.0f;

    // Recommended minimum width and height for proper window fit.
    // Any screen with a lower resolution than this will still have a scale of 1.
    constexpr float minimum_width = 1350.0f;
    constexpr float minimum_height = 900.0f;

    const float width_ratio = std::max(1.0f, real_width / minimum_width);
    const float height_ratio = std::max(1.0f, real_height / minimum_height);

    // Get the lower of the 2 ratios and truncate, this is the maximum integer scale.
    const float max_ratio = std::trunc(std::min(width_ratio, height_ratio));

    if (max_ratio > real_ratio) {
        QApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::Round);
    } else {
        QApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::Floor);
    }
#else
    // Other OSes should be better than Windows at fractional scaling.
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
}

int main(int argc, char* argv[]) {
    Common::DetachedTasks detached_tasks;
    MicroProfileOnThreadCreate("Frontend");
    SCOPE_EXIT({ MicroProfileShutdown(); });

    // Init settings params
    QCoreApplication::setOrganizationName(QStringLiteral("Citra team"));
    QCoreApplication::setApplicationName(QStringLiteral("Citra"));

    QSurfaceFormat format;
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapInterval(0);
    // TODO: expose a setting for buffer value (ie default/single/double/triple)
    format.setSwapBehavior(QSurfaceFormat::DefaultSwapBehavior);
    QSurfaceFormat::setDefaultFormat(format);

    SetHighDPIAttributes();

#ifdef __APPLE__
    std::string bin_path = FileUtil::GetBundleDirectory() + DIR_SEP + "..";
    chdir(bin_path.c_str());
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Disables the "?" button on all dialogs. Disabled by default on Qt6.
    QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
    QCoreApplication::setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);

    // Qt changes the locale and causes issues in float conversion using std::to_string() when
    // generating shaders
    setlocale(LC_ALL, "C");

    GMainWindow main_window;

    // Register CameraFactory
    Camera::RegisterFactory("image", std::make_unique<Camera::StillImageCameraFactory>());
    Camera::RegisterFactory("qt", std::make_unique<Camera::QtMultimediaCameraFactory>());
    Camera::QtMultimediaCameraHandler::Init();

    // Register frontend applets
    Frontend::RegisterDefaultApplets();

    Core::System& system = Core::System::GetInstance();
    system.RegisterMiiSelector(std::make_shared<QtMiiSelector>(main_window));
    system.RegisterSoftwareKeyboard(std::make_shared<QtKeyboard>(main_window));

    // Register Qt image interface
    system.RegisterImageInterface(std::make_shared<QtImageInterface>());

    main_window.show();

    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &main_window,
                     &GMainWindow::OnAppFocusStateChanged);

    int result = app.exec();
    detached_tasks.WaitForAllTasks();
    return result;
}
