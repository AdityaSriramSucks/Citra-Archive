// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <QtGlobal>
#ifdef HAVE_CUBEB
#include "audio_core/cubeb_input.h"
#endif
#include "audio_core/sink.h"
#include "audio_core/sink_details.h"
#include "citra_qt/configuration/configuration_shared.h"
#include "citra_qt/configuration/configure_audio.h"
#include "common/settings.h"
#include "core/core.h"
#include "core/frontend/mic.h"
#include "ui_configure_audio.h"

#if defined(__APPLE__)
#include "citra_qt/macos_authorization.h"
#endif

constexpr int DEFAULT_INPUT_DEVICE_INDEX = 0;

ConfigureAudio::ConfigureAudio(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureAudio>()) {
    ui->setupUi(this);

    ui->output_sink_combo_box->clear();
    ui->output_sink_combo_box->addItem(QString::fromUtf8(AudioCore::auto_device_name));
    for (const char* id : AudioCore::GetSinkIDs()) {
        ui->output_sink_combo_box->addItem(QString::fromUtf8(id));
    }

    const bool is_running = Core::System::GetInstance().IsPoweredOn();
    ui->emulation_combo_box->setEnabled(!is_running);

    connect(ui->volume_slider, &QSlider::valueChanged, this,
            &ConfigureAudio::SetVolumeIndicatorText);

    ui->input_device_combo_box->clear();
    ui->input_device_combo_box->addItem(tr("Default"));

#ifdef HAVE_CUBEB
    for (const auto& device : AudioCore::ListCubebInputDevices()) {
        ui->input_device_combo_box->addItem(QString::fromStdString(device));
    }
#endif

    connect(ui->input_type_combo_box, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureAudio::UpdateAudioInputDevices);

    ui->volume_label->setVisible(Settings::IsConfiguringGlobal());
    ui->volume_combo_box->setVisible(!Settings::IsConfiguringGlobal());

    SetupPerGameUI();
    SetConfiguration();

    connect(ui->output_sink_combo_box, qOverload<int>(&QComboBox::currentIndexChanged), this,
            &ConfigureAudio::UpdateAudioOutputDevices);
}

ConfigureAudio::~ConfigureAudio() {}

void ConfigureAudio::SetConfiguration() {
    SetOutputSinkFromSinkID();

    // The device list cannot be pre-populated (nor listed) until the output sink is known.
    UpdateAudioOutputDevices(ui->output_sink_combo_box->currentIndex());
    SetAudioDeviceFromDeviceID();

    ui->toggle_audio_stretching->setChecked(Settings::values.enable_audio_stretching.GetValue());

    const s32 volume =
        static_cast<s32>(Settings::values.volume.GetValue() * ui->volume_slider->maximum());
    ui->volume_slider->setValue(volume);
    SetVolumeIndicatorText(ui->volume_slider->sliderPosition());

    if (!Settings::IsConfiguringGlobal()) {
        if (Settings::values.volume.UsingGlobal()) {
            ui->volume_combo_box->setCurrentIndex(0);
            ui->volume_slider->setEnabled(false);
        } else {
            ui->volume_combo_box->setCurrentIndex(1);
            ui->volume_slider->setEnabled(true);
        }
        ConfigurationShared::SetHighlight(ui->volume_layout,
                                          !Settings::values.volume.UsingGlobal());
        ConfigurationShared::SetHighlight(ui->widget_emulation,
                                          !Settings::values.audio_emulation.UsingGlobal());
        ConfigurationShared::SetPerGameSetting(ui->emulation_combo_box,
                                               &Settings::values.audio_emulation);
    } else {
        s32 selection = static_cast<s32>(Settings::values.audio_emulation.GetValue());
        ui->emulation_combo_box->setCurrentIndex(selection);
    }

    s32 index = static_cast<s32>(Settings::values.mic_input_type.GetValue());
    ui->input_type_combo_box->setCurrentIndex(index);

    UpdateAudioInputDevices(index);
}

void ConfigureAudio::SetOutputSinkFromSinkID() {
    int new_sink_index = 0;

    const QString sink_id = QString::fromStdString(Settings::values.sink_id.GetValue());
    for (int index = 0; index < ui->output_sink_combo_box->count(); index++) {
        if (ui->output_sink_combo_box->itemText(index) == sink_id) {
            new_sink_index = index;
            break;
        }
    }

    ui->output_sink_combo_box->setCurrentIndex(new_sink_index);
}

void ConfigureAudio::SetAudioDeviceFromDeviceID() {
    int new_device_index = -1;

    const QString device_id = QString::fromStdString(Settings::values.audio_device_id.GetValue());
    for (int index = 0; index < ui->audio_device_combo_box->count(); index++) {
        if (ui->audio_device_combo_box->itemText(index) == device_id) {
            new_device_index = index;
            break;
        }
    }

    ui->audio_device_combo_box->setCurrentIndex(new_device_index);
}

void ConfigureAudio::SetVolumeIndicatorText(int percentage) {
    ui->volume_indicator->setText(tr("%1%", "Volume percentage (e.g. 50%)").arg(percentage));
}

void ConfigureAudio::ApplyConfiguration() {
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.enable_audio_stretching,
                                             ui->toggle_audio_stretching, audio_stretching);
    ConfigurationShared::ApplyPerGameSetting(&Settings::values.audio_emulation,
                                             ui->emulation_combo_box);
    ConfigurationShared::ApplyPerGameSetting(
        &Settings::values.volume, ui->volume_combo_box, [this](s32) {
            return static_cast<float>(ui->volume_slider->value()) / ui->volume_slider->maximum();
        });

    if (Settings::IsConfiguringGlobal()) {
        Settings::values.sink_id =
            ui->output_sink_combo_box->itemText(ui->output_sink_combo_box->currentIndex())
                .toStdString();
        Settings::values.audio_device_id =
            ui->audio_device_combo_box->itemText(ui->audio_device_combo_box->currentIndex())
                .toStdString();
        Settings::values.mic_input_type =
            static_cast<Settings::MicInputType>(ui->input_type_combo_box->currentIndex());

        if (ui->input_device_combo_box->currentIndex() == DEFAULT_INPUT_DEVICE_INDEX) {
            Settings::values.mic_input_device = Frontend::Mic::default_device_name;
        } else {
            Settings::values.mic_input_device =
                ui->input_device_combo_box->currentText().toStdString();
        }
    }
}

void ConfigureAudio::UpdateAudioOutputDevices(int sink_index) {
    ui->audio_device_combo_box->clear();
    ui->audio_device_combo_box->addItem(QString::fromUtf8(AudioCore::auto_device_name));

    const std::string sink_id = ui->output_sink_combo_box->itemText(sink_index).toStdString();
    for (const auto& device : AudioCore::GetDeviceListForSink(sink_id)) {
        ui->audio_device_combo_box->addItem(QString::fromStdString(device));
    }
}

void ConfigureAudio::UpdateAudioInputDevices(int index) {
#if defined(__APPLE__)
    if (index == 1) {
        AppleAuthorization::CheckAuthorizationForMicrophone();
    }
#endif
    if (Settings::values.mic_input_device.GetValue() != Frontend::Mic::default_device_name) {
        ui->input_device_combo_box->setCurrentText(
            QString::fromStdString(Settings::values.mic_input_device.GetValue()));
    }
}

void ConfigureAudio::RetranslateUI() {
    ui->retranslateUi(this);
}

void ConfigureAudio::SetupPerGameUI() {
    if (Settings::IsConfiguringGlobal()) {
        ui->volume_slider->setEnabled(Settings::values.volume.UsingGlobal());
        return;
    }

    ui->output_sink_combo_box->setVisible(false);
    ui->output_sink_label->setVisible(false);
    ui->audio_device_combo_box->setVisible(false);
    ui->audio_device_label->setVisible(false);
    ui->input_type_label->setVisible(false);
    ui->input_type_combo_box->setVisible(false);
    ui->input_device_label->setVisible(false);
    ui->input_device_combo_box->setVisible(false);
    ui->microphone_layout->setVisible(false);

    connect(ui->volume_combo_box, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        ui->volume_slider->setEnabled(index == 1);
        ConfigurationShared::SetHighlight(ui->volume_layout, index == 1);
    });

    ConfigurationShared::SetColoredComboBox(
        ui->emulation_combo_box, ui->widget_emulation,
        static_cast<u32>(Settings::values.audio_emulation.GetValue(true)));

    ConfigurationShared::SetColoredTristate(
        ui->toggle_audio_stretching, Settings::values.enable_audio_stretching, audio_stretching);
}
