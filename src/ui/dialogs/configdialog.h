/*
 * <one line to give the library's name and an idea of what it does.>
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
 *
 */

#pragma once

#include "src/ui/widgets/optimizerwidget.h"

#include <QtCore/QHash>
#include <QtCore/QPointer>

#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>

class QRadioButton;
class QTabWidget;
class QDialogButtonBox;
class QComboBox;
class QPushButton;
class QVBoxLayout;
class ScientificBox;

class OptimizerDialog : public QDialog {
    Q_OBJECT

public:
    OptimizerDialog(QJsonObject config, QWidget* parent = 0);
    ~OptimizerDialog();
    QJsonObject Config() const { return m_opt_widget->Config(); }

private:
    void setUi();
    void createOptimTab();
    QTabWidget* m_mainwidget;
    QDialogButtonBox* m_buttons;
    QJsonObject m_opt_config;
    OptimizerWidget* m_opt_widget;
};

class ConfigDialog : public QDialog {
    Q_OBJECT

public:
    ConfigDialog(QJsonObject config, QWidget* parent = 0);
    ~ConfigDialog();
    inline QJsonObject Config() const { return m_opt_widget->Config(); }
    virtual void accept() override;

private:
    void setUi();
    void createOptimTab();
    /*! \brief Build one settings tab from the registry (all entries whose group == \p group). Claude Generated */
    QWidget* buildTab(const QString& group);
    /*! \brief Hand-built directory-behaviour section (radios + working-dir picker), inserted where the
     *  Custom "dirlevel"/"workingdir" registry entries appear. Claude Generated */
    void buildDirectorySection(QVBoxLayout* layout);

    // Directory behaviour — hand-built Custom widgets
    QRadioButton *m_current_dir, *m_last_dir, *m_working_dir;
    QLineEdit* m_working;
    QPushButton* m_select_working;

    QHash<QString, QWidget*> m_widgets; //!< registry key -> generated widget (for read-back in accept())

    QTabWidget* m_mainwidget;
    QDialogButtonBox* m_buttons;
    QJsonObject m_opt_config;
    OptimizerWidget* m_opt_widget;
    int m_dirlevel;

private slots:
    void SelectWorkingDir();
};
