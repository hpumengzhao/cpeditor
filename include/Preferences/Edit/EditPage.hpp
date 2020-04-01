/*
 * Copyright (C) 2019-2020 Ashar Khan <ashar786khan@gmail.com>
 *
 * This file is part of CP Editor.
 *
 * CP Editor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * I will not be responsible if CP Editor behaves in unexpected way and
 * causes your ratings to go down and or lose any important contest.
 *
 * Believe Software is "Software" and it isn't immune to bugs.
 *
 */

#ifndef EDITPAGE_HPP
#define EDITPAGE_HPP

#include "Preferences/PreferencesPage.hpp"
#include <QLabel>

class EditPage : public PreferencesPage
{
    Q_OBJECT

  public:
    EditPage(QWidget *parent = nullptr);

  private:
    bool areSettingsChanged() override;

    void makeUITheSameAsDefault() override;

    void makeUITheSameAsSettings() override;

    void makeSettingsTheSameAsUI() override;

  private:
    QSpinBox *tabWidthSpinBox = nullptr;
    QCheckBox *indentCheckBox = nullptr;
    QCheckBox *textWrapCheckBox = nullptr;
    QCheckBox *replaceTabsWithSpacesCheckBox = nullptr;
    QCheckBox *parenthesesCompletionCheckBox = nullptr;
    QCheckBox *parenthesesRemovalCheckBox = nullptr;
};

#endif // EDITPAGE_HPP
