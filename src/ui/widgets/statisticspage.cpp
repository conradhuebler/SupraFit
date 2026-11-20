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

#include "statisticspage.h"

#include "src/ui/dialogs/resultsdialog.h"
#include "src/ui/dialogs/statisticdialog.h"
#include "src/ui/guitools/guitools.h"
#include "src/ui/guitools/sectionstack.h"
#include "src/ui/guitools/statusline.h"

#include <QtWidgets/QMenu>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>

StatisticsPage::StatisticsPage(StatisticDialog* statistic, ResultsDialog* results, SectionStack* pages, QObject* parent)
    : QObject(parent)
    , m_statistic(statistic)
    , m_results(results)
    , m_pages(pages)
{
}

int StatisticsPage::Install(const QIcon& icon)
{
    // One page for everything about statistics: the analyses to start and the finished results. The list
    // replaces the page list while the page is shown (SectionStack group page); the page itself switches
    // between the statistic dialog and the results view.
    m_statistic->setEmbedded(true);
    m_results->setEmbedded(true);
    connect(m_statistic, &StatisticDialog::AttentionRequested, this, [this]() { Open(false); });
    connect(m_results, &ResultsDialog::AttentionRequested, this, [this]() { Open(true); });
    m_stack = new QStackedWidget;
    m_stack->addWidget(m_statistic);
    // the results with a hint above them while they belong to earlier data (Edit Data, Claude Generated)
    QWidget* results = new QWidget;
    QVBoxLayout* results_layout = new QVBoxLayout(results);
    results_layout->setContentsMargins(0, 0, 0, 0);
    m_outdated_hint = new StatusLine;
    m_outdated_hint->setObjectName(QStringLiteral("resultsOutdatedHint"));
    m_outdated_hint->setMessage(tr("These results belong to earlier data of this model."), StatusLine::Warning);
    m_outdated_hint->hide();
    results_layout->addWidget(m_outdated_hint);
    results_layout->addWidget(m_results, 1);
    m_stack->addWidget(results);
    m_index = m_pages->addGroupPage(
        m_stack, tr("Statistics"),
        [this]() {
            QList<SectionStack::SubEntry> rows;
            for (const Entry& e : Entries())
                rows << SectionStack::SubEntry{ e.title, e.kind == 0, e.icon, e.tooltip };
            return rows;
        },
        [this](int row) {
            const QList<Entry> entries = Entries();
            if (row < 0 || row >= entries.size())
                return;
            const Entry& e = entries[row];
            if (e.kind == 1) {
                m_stack->setCurrentIndex(0);
                m_statistic->setCurrentTab(e.index);
            } else if (e.kind == 2) {
                m_stack->setCurrentIndex(1);
                m_results->ShowEntry(e.type, e.result);
            }
        },
        [this](int row, const QPoint& pos) {
            const QList<Entry> entries = Entries();
            if (row < 0 || row >= entries.size() || entries[row].kind != 2)
                return;
            const Entry e = entries[row];
            QMenu menu;
            menu.addAction(tr("Open in window"), this, [this, e]() { m_results->DetachEntry(e.type, e.result); });
            menu.addSeparator();
            menu.addAction(tr("Save data"), this, [this, e]() { m_results->SaveEntry(e.type, e.result); });
            menu.addAction(tr("Drop raw data"), this, [this, e]() { m_results->DropRawDataOf(e.type, e.result); });
            menu.addAction(tr("Remove"), this, [this, e]() { m_results->RemoveEntry(e.type, e.result); });
            menu.exec(pos);
        },
        icon);
    connect(m_results, &ResultsDialog::ListChanged, this, [this]() { m_pages->refreshGroup(m_index); });
    m_statistic->show(); // hidden by its constructor; a layout does not show an explicitly hidden widget
    return m_index;
}

void StatisticsPage::setOutdated(bool outdated)
{
    if (m_outdated_hint)
        m_outdated_hint->setVisible(outdated);
}

/*! \brief Rows of the Statistics list: the analyses to start, then the finished results (if any). */
QList<StatisticsPage::Entry> StatisticsPage::Entries() const
{
    // The list is narrow: short names, the full ones are the tooltips; one icon per method for the analysis
    // to start and for its results.
    struct Analysis {
        const char* tab; ///< start of the tab title in the statistic dialog
        const char* shortName;
        SupraFit::Method method;
    };
    static const Analysis analyses[] = {
        { "Cross", QT_TR_NOOP("Cross Validation"), SupraFit::Method::CrossValidation },
        { "Monte", QT_TR_NOOP("Monte Carlo"), SupraFit::Method::MonteCarlo },
        { "Weakened", QT_TR_NOOP("Grid Search"), SupraFit::Method::WeakenedGridSearch },
        { "Model", QT_TR_NOOP("Model Comparison"), SupraFit::Method::ModelComparison },
        { "Landscape", QT_TR_NOOP("Landscape Sampling"), SupraFit::Method::LandscapeSampling },
        { "Scan", QT_TR_NOOP("Scan"), SupraFit::Method::GlobalSearch },
    };

    QList<Entry> rows;
    rows << Entry{ tr("New analysis"), 0, -1, SupraFit::Method::MonteCarlo, -1, QIcon(), QString() };
    const QStringList tabs = m_statistic->TabTitles();
    for (int i = 0; i < tabs.size(); ++i) {
        QString title = tabs[i];
        QIcon icon;
        for (const Analysis& a : analyses)
            if (tabs[i].startsWith(QLatin1String(a.tab))) {
                title = tr(a.shortName);
                icon = MethodIcon(a.method);
            }
        rows << Entry{ title, 1, i, SupraFit::Method::MonteCarlo, -1, icon, title == tabs[i] ? QString() : tabs[i] };
    }
    const QList<ResultsDialog::Entry> results = m_results->Entries();
    if (!results.isEmpty())
        rows << Entry{ tr("Results"), 0, -1, SupraFit::Method::MonteCarlo, -1, QIcon(), QString() };
    for (const ResultsDialog::Entry& e : results)
        rows << Entry{ ResultListTitle(e.type, e.index), 2, -1, e.type, e.index, MethodIcon(e.type), e.title };
    return rows;
}

/*! \brief Short, numbered name of a finished analysis as listed on the Statistics page ("Monte Carlo #1"). */
QString StatisticsPage::ResultListTitle(SupraFit::Method type, int index)
{
    QString name;
    switch (type) {
    case SupraFit::Method::MonteCarlo:
        name = tr("Monte Carlo");
        break;
    case SupraFit::Method::CrossValidation:
        name = tr("Cross Validation");
        break;
    case SupraFit::Method::WeakenedGridSearch:
        name = tr("Grid Search");
        break;
    case SupraFit::Method::ModelComparison:
        name = tr("Model Comparison");
        break;
    case SupraFit::Method::Reduction:
        name = tr("Reduction");
        break;
    case SupraFit::Method::GlobalSearch:
        name = tr("Scan");
        break;
    case SupraFit::Method::LandscapeSampling:
        name = tr("Landscape Sampling");
        break;
    default:
        name = SupraFit::Method2Name(type);
    }
    return tr("%1 #%2").arg(name).arg(index + 1);
}

/*! \brief Icon of an analysis method; the project only ships a few, so some are shared. */
QIcon StatisticsPage::MethodIcon(SupraFit::Method type)
{
    switch (type) {
    case SupraFit::Method::MonteCarlo:
        return Icon("applications-education-science");
    case SupraFit::Method::CrossValidation:
    case SupraFit::Method::Reduction:
        return Icon("split");
    case SupraFit::Method::WeakenedGridSearch:
    case SupraFit::Method::GlobalSearch:
        return Icon("kspread");
    case SupraFit::Method::ModelComparison:
        return Icon("fork");
    case SupraFit::Method::LandscapeSampling:
        return Icon("charts");
    default:
        return QIcon();
    }
}

/*! \brief Show the Statistics page with its list; \p results selects the newest finished analysis. */
void StatisticsPage::Open(bool results)
{
    m_pages->setCurrentTitle(tr("Statistics"), true);
    if (!results)
        return;
    const QList<ResultsDialog::Entry> entries = m_results->Entries();
    if (!entries.isEmpty())
        m_pages->selectSub(ResultListTitle(entries.last().type, entries.last().index));
}

/*! \brief A finished analysis: list it and show it on the Statistics page. */
void StatisticsPage::ShowResult(SupraFit::Method type, int index)
{
    m_results->ShowResult(type, index); // opens the result tab
    m_pages->setCurrentTitle(tr("Statistics"), true);
    m_pages->refreshGroup(m_index);
    m_pages->selectSub(ResultListTitle(type, index));
}
