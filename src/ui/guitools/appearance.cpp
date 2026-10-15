/*
 * Appearance (light / dark / system colours) of the SupraFit GUI.
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
 * Claude Generated (UI redesign WP5).
 */

#include <QtCore/QEvent>
#include <QtCore/QTimer>

#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>

#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QWidget>

#include <QtCharts/QAbstractAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLegend>

#include "appearance.h"

namespace {

QPalette g_system_palette; ///< palette of the platform at start-up (mode System)
bool g_installed = false;

/*! \brief Dark palette for the Fusion style. */
QPalette DarkPalette()
{
    QPalette p;
    const QColor window(53, 53, 53), base(36, 36, 36), text(230, 230, 230), disabled(128, 128, 128);
    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, QColor(46, 46, 46));
    p.setColor(QPalette::ToolTipBase, QColor(30, 30, 30));
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::PlaceholderText, QColor(150, 150, 150));
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, window);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, QColor(255, 110, 110));
    p.setColor(QPalette::Light, QColor(80, 80, 80));
    p.setColor(QPalette::Midlight, QColor(66, 66, 66));
    p.setColor(QPalette::Mid, QColor(44, 44, 44));
    p.setColor(QPalette::Dark, QColor(32, 32, 32));
    p.setColor(QPalette::Shadow, QColor(20, 20, 20));
    p.setColor(QPalette::Highlight, QColor(42, 130, 218));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link, QColor(110, 170, 240));
    p.setColor(QPalette::LinkVisited, QColor(170, 140, 240));
    for (QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        p.setColor(QPalette::Disabled, role, disabled);
    p.setColor(QPalette::Disabled, QPalette::Base, window);
    p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    return p;
}

/*! \brief Light palette for the Fusion style. */
QPalette LightPalette()
{
    QPalette p(QColor(239, 239, 239), QColor(239, 239, 239));
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, QColor(247, 247, 247));
    p.setColor(QPalette::Text, Qt::black);
    p.setColor(QPalette::WindowText, Qt::black);
    p.setColor(QPalette::ButtonText, Qt::black);
    p.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
    p.setColor(QPalette::ToolTipText, Qt::black);
    p.setColor(QPalette::PlaceholderText, QColor(110, 110, 110));
    p.setColor(QPalette::Highlight, QColor(48, 140, 198));
    p.setColor(QPalette::HighlightedText, Qt::white);
    for (QPalette::ColorRole role : { QPalette::WindowText, QPalette::Text, QPalette::ButtonText })
        p.setColor(QPalette::Disabled, role, QColor(150, 150, 150));
    return p;
}

void RestyleAllCharts()
{
    for (QWidget* widget : QApplication::allWidgets()) {
        if (QChartView* view = qobject_cast<QChartView*>(widget))
            Appearance::ApplyToChart(view->chart());
    }
}

/*! \brief Polishes every widget again after a palette change. A widget that carries a style sheet (or sits
 * below one) keeps the palette it resolved at creation time and ignores later application palettes, which
 * left whole workspaces light after switching to the dark scheme. Claude Generated. */
void RepolishAllWidgets()
{
    for (QWidget* widget : QApplication::allWidgets()) {
        QStyle* style = widget->style();
        style->unpolish(widget);
        style->polish(widget);
        widget->update();
    }
}

/*! \brief Restyles chart views when they are shown, so charts created after Apply() (new projects,
 * dialogs) match the palette as well. */
class ChartStyler : public QObject {
public:
    using QObject::QObject;
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event->type() == QEvent::Show) {
            if (QChartView* view = qobject_cast<QChartView*>(watched))
                Appearance::ApplyToChart(view->chart());
        }
        return QObject::eventFilter(watched, event);
    }
};
}

namespace Appearance {

void Install()
{
    if (g_installed)
        return;
    g_installed = true;
    g_system_palette = QApplication::palette();
    qApp->installEventFilter(new ChartStyler(qApp));
}

void Apply(int mode)
{
    Install();
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    QGuiApplication::styleHints()->setColorScheme(mode == Dark ? Qt::ColorScheme::Dark : (mode == Light ? Qt::ColorScheme::Light : Qt::ColorScheme::Unknown));
#endif
    if (mode == Dark)
        QApplication::setPalette(DarkPalette());
    else if (mode == Light)
        QApplication::setPalette(LightPalette());
    else
        QApplication::setPalette(g_system_palette);
    // let the palette change propagate before the charts read it
    QTimer::singleShot(0, qApp, &RepolishAllWidgets);
    QTimer::singleShot(0, qApp, &RestyleAllCharts);
}

bool IsDark()
{
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

void ApplyToChart(QChart* chart)
{
    if (!chart)
        return;
    const QPalette palette = QApplication::palette();
    const QColor base = palette.color(QPalette::Base);
    const QColor text = palette.color(QPalette::Text);
    QColor grid = text;
    grid.setAlpha(45);
    QColor line = text;
    line.setAlpha(160);

    chart->setBackgroundBrush(base);
    chart->setBackgroundPen(QPen(palette.color(QPalette::Mid)));
    chart->setTitleBrush(text);
    if (chart->legend())
        chart->legend()->setLabelColor(text);
    for (QAbstractAxis* axis : chart->axes()) {
        axis->setLabelsColor(text);
        axis->setTitleBrush(text);
        axis->setGridLineColor(grid);
        axis->setMinorGridLineColor(grid);
        axis->setLinePenColor(line);
    }
}

QColor ContrastText(const QColor& background)
{
    // perceived luminance (ITU-R BT.601)
    const double y = 0.299 * background.redF() + 0.587 * background.greenF() + 0.114 * background.blueF();
    return y > 0.55 ? QColor(Qt::black) : QColor(Qt::white);
}
}
