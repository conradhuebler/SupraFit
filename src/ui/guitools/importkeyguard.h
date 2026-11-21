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

#pragma once

#include <functional>

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

class QDialog;

/*! \brief Common keyboard rule of the import dialogs (Claude Generated).
 *
 * - Enter/Return never accepts the dialog: a spin box, line edit or table hands the key on to the
 *   dialog when it is done with it, and QDialog would then press the default button. The guard eats
 *   the key at the dialog and takes the default role from every push button (setAutoDefault(false)),
 *   so no button looks like the target of Enter. A focused button still reacts to Space.
 * - ESC goes first to what has the focus: the chart ends a pending zoom or range drag (it keeps the
 *   key), an open table editor or a combo popup closes. Only an ESC that reaches the dialog closes it.
 * - Closing without OK (Cancel, ESC, the window's close button) asks "Discard the changes?" if the
 *   host called markDirty() after the dialog was shown; a dialog opened and closed unchanged closes
 *   without the question. The host calls confirmDiscard() from its reject().
 *
 * The questions go through confirm(), which tests replace by setConfirmHandler() (no modal box).
 * The ITC dialog (Thermogram) has its own rule and does not use the guard. */
class ImportKeyGuard : public QObject {
    Q_OBJECT
public:
    /*! \brief Installs the guard on dialog (event filter, parent = dialog). */
    explicit ImportKeyGuard(QDialog* dialog);

    /*! \brief Takes the default role from all push buttons of the dialog; call after the buttons exist. */
    void removeDefaultButtons();

    /*! \brief Something the user would lose has changed; ignored until the dialog is shown. */
    void markDirty();
    /*! \brief The present state is the unchanged state (also done when the dialog is shown). */
    void setClean();
    inline bool isDirty() const { return m_dirty; }

    /*! \brief True if the dialog may close without OK: nothing changed, or the user discards. */
    bool confirmDiscard();

    /*! \brief Asks question with the buttons acceptText / "Keep editing" (or the test handler); true = go on. */
    bool confirm(const QString& question, const QString& acceptText);

    /*! \brief Replaces the message box (tests); the handler gets the question and returns the answer. */
    inline void setConfirmHandler(const std::function<bool(const QString&)>& handler) { m_handler = handler; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QPointer<QDialog> m_dialog;
    bool m_dirty = false;
    bool m_shown = false;
    std::function<bool(const QString&)> m_handler;
};
