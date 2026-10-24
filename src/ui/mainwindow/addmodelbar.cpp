/*
 * Add-Model bar: the technique and the models that are added most, one click from the main window.
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


#include <QtGui/QAction>

#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>

#include "src/ui/guitools/guitools.h"
#include "modeldataholder.h"
#include "modelusage.h"

#include "addmodelbar.h"

AddModelBar::AddModelBar(QWidget* parent)
    : QToolBar(tr("Add model"), parent)
{
    setObjectName(QStringLiteral("add_model_bar")); // the window state (place, floating) is kept under this name
    setMovable(true);
    setFloatable(true);
    setAllowedAreas(Qt::AllToolBarAreas);

    m_technique_box = new QWidget;
    QHBoxLayout* technique_layout = new QHBoxLayout(m_technique_box);
    technique_layout->setContentsMargins(0, 0, 6, 0);
    technique_layout->setSpacing(2);
    m_technique_group = new QButtonGroup(this);
    m_technique_group->setExclusive(true);
    addWidget(m_technique_box);

    addSeparator();
    m_quick_box = new QWidget;
    QHBoxLayout* quick_layout = new QHBoxLayout(m_quick_box);
    quick_layout->setContentsMargins(6, 0, 0, 0);
    quick_layout->setSpacing(2);
    addWidget(m_quick_box);

    addSeparator();
    m_more = new QToolButton;
    m_more->setText(tr("More\u2026"));
    m_more->setToolTip(tr("All models: the Model menu"));
    m_more->setPopupMode(QToolButton::InstantPopup);
    m_more->setAutoRaise(true);
    addWidget(m_more);

    m_gear = new QToolButton;
    m_gear->setIcon(Icon("configure"));
    m_gear->setToolTip(tr("What this bar shows"));
    m_gear->setPopupMode(QToolButton::InstantPopup);
    m_gear->setAutoRaise(true);
    QMenu* gear = new QMenu(m_gear);
    QAction* frequent = gear->addAction(tr("Show the most used models"));
    frequent->setCheckable(true);
    QAction* fixed = gear->addAction(tr("Show a fixed list"));
    fixed->setCheckable(true);
    QActionGroup* modes = new QActionGroup(gear);
    modes->addAction(frequent);
    modes->addAction(fixed);
    gear->addSeparator();
    QAction* reset = gear->addAction(tr("Forget what was used"));
    connect(gear, &QMenu::aboutToShow, this, [frequent, fixed]() {
        const bool isFixed = ModelUsage::Load().mode == ModelUsage::Mode::Fixed;
        frequent->setChecked(!isFixed);
        fixed->setChecked(isFixed);
    });
    connect(frequent, &QAction::triggered, this, [this]() {
        ModelUsage::Preferences p = ModelUsage::Load();
        p.mode = ModelUsage::Mode::Frequent;
        ModelUsage::Save(p);
        Refresh();
    });
    connect(fixed, &QAction::triggered, this, [this]() {
        ModelUsage::Preferences p = ModelUsage::Load();
        p.mode = ModelUsage::Mode::Fixed;
        ModelUsage::Save(p);
        Refresh();
    });
    connect(reset, &QAction::triggered, this, [this]() {
        ModelUsage::Reset();
        Refresh();
    });
    m_gear->setMenu(gear);
    addWidget(m_gear);

    setEnabled(false);
}

void AddModelBar::setWorkspace(MDHDockTitleBar* workspace)
{
    m_workspace = workspace;
    const bool available = workspace && workspace->ToolsAvailable();
    setEnabled(available);
    m_more->setMenu(available ? workspace->ModelMenu() : nullptr);
    m_columns = available ? workspace->IndependentCount() : 0;
    if (!available) {
        RebuildQuick();
        return;
    }
    // keep the technique of the session if it fits this project, else the one to start with
    const QList<ModelCatalog::Technique> fitting = ModelCatalog::TechniquesFor(m_columns);
    if (!fitting.contains(m_technique))
        m_technique = ModelUsage::DefaultTechnique(m_columns);
    RebuildTechniques();
    RebuildQuick();
}

void AddModelBar::setTechnique(ModelCatalog::Technique technique)
{
    if (!ModelCatalog::TechniquesFor(m_columns).contains(technique))
        return;
    m_technique = technique;
    for (QAbstractButton* button : m_technique_group->buttons())
        button->setChecked(button->property("technique").toInt() == int(technique));
    RebuildQuick();
}

void AddModelBar::Refresh()
{
    RebuildQuick();
}

void AddModelBar::RebuildTechniques()
{
    // buttons side by side for the techniques that fit the data (one column: calorimetry, kinetics; more: titrations)
    for (QAbstractButton* button : m_technique_group->buttons()) {
        m_technique_group->removeButton(button);
        delete button;
    }
    for (const ModelCatalog::Technique t : ModelCatalog::TechniquesFor(m_columns)) {
        const ModelCatalog::TechniqueInfo info = ModelCatalog::Info(t);
        QToolButton* button = new QToolButton;
        button->setText(info.shortName);
        button->setToolTip(info.name);
        button->setCheckable(true);
        button->setAutoRaise(true);
        button->setProperty("technique", int(t));
        button->setChecked(t == m_technique);
        m_technique_group->addButton(button);
        m_technique_box->layout()->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, t]() { setTechnique(t); });
    }
}

void AddModelBar::RebuildQuick()
{
    QLayout* layout = m_quick_box->layout();
    while (QLayoutItem* item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (!m_workspace || !m_workspace->ToolsAvailable())
        return;
    const QString technique = ModelCatalog::Info(m_technique).shortName;
    for (const ModelCatalog::Entry& entry : ModelUsage::Quick(m_technique, m_columns)) {
        QToolButton* button = new QToolButton;
        button->setText(entry.title);
        button->setToolTip(entry.kind == ModelCatalog::Kind::Stoichiometry
                ? tr("%1 \u2014 %2\n%3").arg(entry.title, technique, entry.tooltip)
                : entry.tooltip);
        button->setAutoRaise(true);
        layout->addWidget(button);
        connect(button, &QToolButton::clicked, this, [this, entry]() { Add(entry); });
    }
}

void AddModelBar::Add(const ModelCatalog::Entry& entry)
{
    if (!m_workspace || !entry.choice.valid())
        return;
    // counted before it is added: the dialog of a model that needs one may take a while, and a cancelled add still
    // is a wish for this model
    ModelUsage::Record(m_technique, entry);
    m_workspace->RequestModel(entry.choice);
}
