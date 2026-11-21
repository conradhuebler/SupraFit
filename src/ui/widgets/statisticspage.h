/*
 * SupraFit - the Statistics page of a model pane
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
 * Claude Generated (shared by ModelWidget and MetaModelWidget).
 */

#pragma once

#include "src/core/libmath.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtGui/QIcon>

class ResultsDialog;
class SectionStack;
class StatisticDialog;
class QStackedWidget;

/*! \brief The Statistics page of a model pane: a list with the analyses to start ("New analysis") and the finished results,
 * and a stack that shows the statistic dialog or one result. Claude Generated.
 *
 * ModelWidget and MetaModelWidget use the same page, so a model and a meta model list, start, show and remove their analyses
 * in the same way. The page is a group page of a \c SectionStack: while it is shown the list of the pane is replaced by the
 * list of this page. Both dialogs are used as plain widgets (embedded).
 */
class StatisticsPage : public QObject {
    Q_OBJECT
public:
    StatisticsPage(StatisticDialog* statistic, ResultsDialog* results, SectionStack* pages, QObject* parent = nullptr);

    /*! \brief Add the page to the list of the pane; returns its index there. */
    int Install(const QIcon& icon);
    inline int Index() const { return m_index; }

    /*! \brief Show the page with its list; \p results selects the newest finished analysis. */
    void Open(bool results);
    /*! \brief A finished analysis: list it and show it. */
    void ShowResult(SupraFit::Method type, int index);

    /*! \brief The finished analyses belong to earlier data of the model (Edit Data): a hint above the results. Claude Generated. */
    void setOutdated(bool outdated);
    inline class StatusLine* OutdatedHint() const { return m_outdated_hint; }

    /*! \brief Short, numbered name of a finished analysis ("Monte Carlo #1"). */
    static QString ResultListTitle(SupraFit::Method type, int index);
    /*! \brief Icon of an analysis method; the project only ships a few, so some are shared. */
    static QIcon MethodIcon(SupraFit::Method type);

private:
    struct Entry {
        QString title;
        int kind; ///< 0 heading, 1 analysis, 2 result
        int index; ///< tab of the statistic dialog (analysis)
        SupraFit::Method type; ///< method and number of the result
        int result;
        QIcon icon;
        QString tooltip;
    };
    QList<Entry> Entries() const;

    QPointer<StatisticDialog> m_statistic;
    QPointer<ResultsDialog> m_results;
    SectionStack* m_pages;
    QStackedWidget* m_stack = nullptr; ///< [0] statistic dialog (start), [1] results of finished analyses
    int m_index = -1;
    class StatusLine* m_outdated_hint = nullptr; ///< "These results belong to earlier data of this model." (Claude Generated)
};
