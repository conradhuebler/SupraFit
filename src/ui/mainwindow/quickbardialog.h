/*
 * Quick-bar dialog: what the bar of the models you add most shows.
 * Copyright (C) 2016 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QtWidgets/QDialog>

#include "modelusage.h"

class QComboBox;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;

/*! \brief Dialog to customise the quick bar (see ModelUsage::Preferences): the most used models (and how many) or a
 * fixed list the operator puts together from everything that can be added - stoichiometries, models of one technique,
 * scripted presets, model files - with the order, and the technique a never used entry starts with. Claude Generated. */
class QuickBarDialog : public QDialog {
    Q_OBJECT
public:
    explicit QuickBarDialog(const ModelUsage::Preferences& preferences, QWidget* parent = nullptr);

    /*! \brief What the dialog stands for now. */
    ModelUsage::Preferences preferences() const;

private:
    void UpdateEnabled();
    void Move(int by);

    QRadioButton* m_frequent = nullptr;
    QRadioButton* m_fixed = nullptr;
    QSpinBox* m_count = nullptr;
    QListWidget* m_list = nullptr;
    QPushButton* m_up = nullptr;
    QPushButton* m_down = nullptr;
    QComboBox* m_technique = nullptr;
};
