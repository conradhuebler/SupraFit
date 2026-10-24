/*
 * More/Less switch: shows or hides a further row of buttons or options.
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

#include <QtCore/QCoreApplication>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QSettings>

#include <QtWidgets/QLayout>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QWidget>

/*! \brief The "..more.." / "..less.." switch: shows or hides a target widget (a second row of buttons,
 * advanced options) with a short height animation.
 *
 * The expanded height comes from the target's layout (including a wrapped FlowLayout at the current width),
 * not from a fixed number; once expanded the height limit is lifted, so a later wrap is never clipped.
 * Collapsed is the default; with an \p id the choice is kept in the settings (`ui/more/<id>`).
 * Without a visible widget (start-up, offscreen tests) the state changes without animation. Claude Generated. */
class MoreLessToggle : public QToolButton {
public:
    explicit MoreLessToggle(QWidget* target, const QString& id = QString(), QWidget* parent = nullptr)
        : QToolButton(parent)
        , m_target(target)
        , m_id(id)
    {
        setAutoRaise(true);
        setToolButtonStyle(Qt::ToolButtonTextOnly);
        setToolTip(QCoreApplication::translate("MoreLessToggle", "Show or hide the further buttons"));
        const bool expanded = !m_id.isEmpty() && QSettings().value(Key(), false).toBool();
        Apply(expanded);
        QObject::connect(this, &QToolButton::clicked, this, [this]() { setExpanded(!m_expanded); });
    }

    bool isExpanded() const { return m_expanded; }

    /*! \brief Show (\p expanded) or hide the target; animated when both this button and the target are on screen. */
    void setExpanded(bool expanded, bool animate = true)
    {
        if (expanded == m_expanded)
            return;
        m_expanded = expanded;
        setText(Label());
        if (!m_id.isEmpty())
            QSettings().setValue(Key(), expanded);

        if (!m_target)
            return;
        if (!animate || !isVisible() || !m_target->isVisible()) {
            m_target->setMaximumHeight(expanded ? QWIDGETSIZE_MAX : 0);
            return;
        }
        const int from = m_target->height();
        const int to = expanded ? FullHeight() : 0;
        m_target->setMaximumHeight(from);
        QPropertyAnimation* animation = new QPropertyAnimation(m_target, "maximumHeight", this);
        animation->setEasingCurve(QEasingCurve::InOutCubic);
        animation->setDuration(200);
        animation->setStartValue(from);
        animation->setEndValue(to);
        QObject::connect(animation, &QPropertyAnimation::finished, this, [this, expanded]() {
            if (m_target && m_expanded == expanded)
                m_target->setMaximumHeight(expanded ? QWIDGETSIZE_MAX : 0); // lift the limit: a later wrap must not clip
        });
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    QString Key() const { return QStringLiteral("ui/more/%1").arg(m_id); }
    QString Label() const
    {
        return m_expanded ? QCoreApplication::translate("MoreLessToggle", "..less..")
                          : QCoreApplication::translate("MoreLessToggle", "..more..");
    }

    void Apply(bool expanded)
    {
        m_expanded = expanded;
        setText(Label());
        if (m_target)
            m_target->setMaximumHeight(expanded ? QWIDGETSIZE_MAX : 0);
    }

    // height of the target when shown: its layout at the current width (a FlowLayout wraps), else its size hint
    int FullHeight() const
    {
        QLayout* layout = m_target->layout();
        if (layout && layout->hasHeightForWidth() && m_target->width() > 0)
            return layout->totalHeightForWidth(m_target->width());
        return m_target->sizeHint().height();
    }

    QWidget* m_target;
    QString m_id;
    bool m_expanded = false;
};
