/*
 * Add-Model bar: the models that are added most, one click away, the technique in a menu.
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

#include <QtGui/QActionGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMenu>
#include <QtWidgets/QToolButton>

#include "src/ui/guitools/guitools.h"
#include "modeldataholder.h"
#include "modelusage.h"
#include "quickbardialog.h"

#include "addmodelbar.h"

AddModelBar::AddModelBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("add_model_bar"));
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_quick_box = new QWidget;
    QHBoxLayout* quick_layout = new QHBoxLayout(m_quick_box);
    quick_layout->setContentsMargins(0, 0, 0, 0);
    quick_layout->setSpacing(2);
    layout->addWidget(m_quick_box);

    m_gear = new QToolButton;
    m_gear->setIcon(Icon("configure"));
    m_gear->setToolTip(tr("Choose which models this bar shows: the most used ones or a list of your own"));
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
    QAction* customise = gear->addAction(tr("Customise\u2026"));
    connect(customise, &QAction::triggered, this, &AddModelBar::Customise);
    QAction* fix = gear->addAction(tr("Fix the bar as it is"));
    connect(fix, &QAction::triggered, this, &AddModelBar::FixAsItIs);
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
    layout->addWidget(m_gear);

    setEnabled(false);
    m_gear->setVisible(false);
}

void AddModelBar::setWorkspace(MDHDockTitleBar* workspace)
{
    if (m_workspace)
        disconnect(m_workspace, &MDHDockTitleBar::UsageChanged, this, &AddModelBar::Refresh);
    m_workspace = workspace;
    if (m_workspace)
        connect(m_workspace, &MDHDockTitleBar::UsageChanged, this, &AddModelBar::Refresh);
    const bool available = workspace && workspace->ToolsAvailable();
    setEnabled(available);
    m_columns = available ? workspace->IndependentCount() : 0;
    Rebuild();
}

void AddModelBar::Refresh()
{
    Rebuild();
}

void AddModelBar::Rebuild()
{
    QLayout* layout = m_quick_box->layout();
    while (QLayoutItem* item = layout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    const bool available = m_workspace && m_workspace->ToolsAvailable();
    m_gear->setVisible(available);
    if (!available)
        return;

    for (const ModelCatalog::Entry& entry : ModelUsage::Quick(m_columns)) {
        const ModelCatalog::Technique technique = ModelUsage::TechniqueFor(entry, m_columns);
        QToolButton* button = new QToolButton;
        button->setAutoRaise(true);
        button->setProperty("entryKey", entry.key);
        button->setProperty("technique", int(technique));

        // an entry that is offered for one technique (or none) is a plain button, otherwise the arrow lists them
        const bool several = entry.techniques.size() > 1;
        const QString techniqueName = entry.techniques.isEmpty() ? QString() : ModelCatalog::Info(technique).shortName;
        // every entry that has a technique names it, so the buttons read alike whatever they were used with
        button->setText(entry.techniques.isEmpty() ? entry.title : tr("%1 \u00b7 %2").arg(entry.title, techniqueName));
        QString tip = entry.techniques.isEmpty() ? entry.tooltip : tr("%1 \u2014 %2\n%3").arg(entry.title, techniqueName, entry.tooltip);
        if (several)
            tip += tr("\nThe arrow next to the button chooses another technique.");
        button->setToolTip(tip);
        connect(button, &QToolButton::clicked, this, [this, entry, technique]() { Add(entry, technique); });
        button->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(button, &QToolButton::customContextMenuRequested, this, [this, button, entry](const QPoint& pos) {
            QMenu* menu = ContextMenu(entry.key);
            menu->exec(button->mapToGlobal(pos));
            menu->deleteLater();
        });

        if (several) {
            QMenu* menu = new QMenu(button);
            for (const ModelCatalog::Technique t : entry.techniques) {
                const ModelCatalog::TechniqueInfo info = ModelCatalog::Info(t);
                QAction* action = menu->addAction(info.name);
                action->setData(int(t));
                QFont font = action->font();
                font.setBold(t == technique); // the one a click uses
                action->setFont(font);
                connect(action, &QAction::triggered, this, [this, entry, t]() { Add(entry, t); });
            }
            button->setMenu(menu);
            button->setPopupMode(QToolButton::MenuButtonPopup);
        }
        layout->addWidget(button);
    }
}

QMenu* AddModelBar::ContextMenu(const QString& entryKey)
{
    QMenu* menu = new QMenu(this);
    if (ModelUsage::Load().mode == ModelUsage::Mode::Fixed) {
        QAction* remove = menu->addAction(tr("Remove from the bar"));
        connect(remove, &QAction::triggered, this, [this, entryKey]() {
            ModelUsage::Preferences p = ModelUsage::Load();
            p.fixed.removeAll(entryKey);
            ModelUsage::Save(p);
            Refresh();
        });
    } else {
        QAction* fix = menu->addAction(tr("Fix the bar as it is"));
        connect(fix, &QAction::triggered, this, &AddModelBar::FixAsItIs);
    }
    menu->addSeparator();
    QAction* customise = menu->addAction(tr("Customise\u2026"));
    connect(customise, &QAction::triggered, this, &AddModelBar::Customise);
    return menu;
}

void AddModelBar::Customise()
{
    QuickBarDialog dialog(ModelUsage::Load(), this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    ModelUsage::Save(dialog.preferences());
    Refresh();
}

void AddModelBar::FixAsItIs()
{
    ModelUsage::Preferences p = ModelUsage::Load();
    p.fixed.clear();
    for (const ModelCatalog::Entry& entry : ModelUsage::Quick(m_columns))
        p.fixed << entry.key;
    p.mode = ModelUsage::Mode::Fixed;
    ModelUsage::Save(p);
    Refresh();
}

void AddModelBar::Add(const ModelCatalog::Entry& entry, ModelCatalog::Technique technique)
{
    const ModelCatalog::Choice choice = ModelCatalog::ChoiceFor(entry, technique);
    if (!m_workspace || !choice.valid())
        return;
    // counted before it is added: the dialog of a model that needs one may take a while, and a cancelled add still
    // is a wish for this model
    ModelUsage::Record(technique, entry);
    m_workspace->RequestModel(choice);
}
