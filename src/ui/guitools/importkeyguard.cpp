/*
 * ImportKeyGuard - keyboard rule and discard question of the spectra and NMR import dialogs
 * Copyright (C) 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
 * Claude Generated (UX package 1 of the import dialogs).
 */

#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>

#include "importkeyguard.h"

ImportKeyGuard::ImportKeyGuard(QDialog* dialog)
    : QObject(dialog)
    , m_dialog(dialog)
{
    dialog->installEventFilter(this);
}

void ImportKeyGuard::removeDefaultButtons()
{
    if (!m_dialog)
        return;
    for (QPushButton* button : m_dialog->findChildren<QPushButton*>()) {
        button->setAutoDefault(false);
        button->setDefault(false);
    }
}

void ImportKeyGuard::markDirty()
{
    if (m_shown)
        m_dirty = true;
}

void ImportKeyGuard::setClean()
{
    m_dirty = false;
}

bool ImportKeyGuard::confirm(const QString& question, const QString& acceptText)
{
    if (m_handler)
        return m_handler(question);
    QMessageBox box(QMessageBox::Question, m_dialog ? m_dialog->windowTitle() : QString(), question, QMessageBox::NoButton, m_dialog);
    QPushButton* accept = box.addButton(acceptText, QMessageBox::DestructiveRole);
    QPushButton* keep = box.addButton(tr("Keep editing"), QMessageBox::RejectRole);
    box.setDefaultButton(keep);
    box.setEscapeButton(keep);
    box.exec();
    return box.clickedButton() == accept;
}

bool ImportKeyGuard::confirmDiscard()
{
    if (!m_dirty)
        return true;
    return confirm(tr("Discard the changes?"), tr("Discard"));
}

bool ImportKeyGuard::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_dialog) {
        if (event->type() == QEvent::Show) {
            // the button box made its first accepting button the default while it was shown
            removeDefaultButtons();
            if (!m_shown) {
                // what the dialog was opened with (files, a stored project) is the unchanged state
                m_shown = true;
                m_dirty = false;
            }
        } else if (event->type() == QEvent::KeyPress) {
            QKeyEvent* key = static_cast<QKeyEvent*>(event);
            // a key that reaches the dialog was not taken by the focused widget: Enter would press OK
            if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
                return true;
        }
    }
    return QObject::eventFilter(watched, event);
}
