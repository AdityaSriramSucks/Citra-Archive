﻿// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>
#include <QDialog>
#include <QList>
#include "citra_qt/configuration/config.h"

namespace Core {
class System;
}

class ConfigureAudio;
class ConfigureGeneral;
class ConfigureGraphics;
class ConfigureSystem;

class QGraphicsScene;
class QStandardItem;
class QStandardItemModel;
class QTreeView;
class QVBoxLayout;

namespace Ui {
class ConfigurePerGame;
}

class ConfigurePerGame : public QDialog {
    Q_OBJECT

public:
    // Cannot use std::filesystem::path due to https://bugreports.qt.io/browse/QTBUG-73263
    explicit ConfigurePerGame(QWidget* parent, u64 title_id_, const QString& file_name,
                              Core::System& system_);
    ~ConfigurePerGame() override;

    /// Loads all button configurations to settings file
    void LoadConfiguration();

    /// Save all button configurations to settings file
    void ApplyConfiguration();

    /// Reset the settings for this game
    void ResetDefaults();

private:
    void changeEvent(QEvent* event) override;
    void RetranslateUI();

    void HandleApplyButtonClicked();

    std::unique_ptr<Ui::ConfigurePerGame> ui;
    std::string filename;
    u64 title_id;

    QGraphicsScene* scene;

    std::unique_ptr<Config> game_config;

    Core::System& system;

    std::unique_ptr<ConfigureAudio> audio_tab;
    std::unique_ptr<ConfigureGeneral> general_tab;
    std::unique_ptr<ConfigureGraphics> graphics_tab;
    std::unique_ptr<ConfigureSystem> system_tab;
};
