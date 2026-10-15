/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2017 - 2026 Conrad Hübler <Conrad.Huebler@gmx.net>
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
#include "src/core/toolset.h"

#include "src/core/models/models.h"

#include "src/ui/widgets/textwidget.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "statisticwidget.h"

StatisticWidget::StatisticWidget(const QSharedPointer<AbstractModel> model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{

    /* Overview as a label/value form that uses the available width (1-3 column pairs), followed by the
     * model-specific text and the fast-confidence results. One scroll area around everything, so a
     * scroll bar only appears when the content is taller than the pane. Claude Generated (UI WP8). */
    QWidget* content = new QWidget;
    QVBoxLayout* content_layout = new QVBoxLayout;
    content_layout->setContentsMargins(6, 6, 6, 6);
    m_grid = new QGridLayout;
    m_grid->setHorizontalSpacing(18);
    m_grid->setVerticalSpacing(6);
    content_layout->addLayout(m_grid);

    const QStringList names = { tr("Model type"), tr("Parameters fitted"), tr("Points used"), tr("Degrees of freedom"),
        tr("SAE (sum of absolute errors)"), tr("Error threshold (F-test)"), tr("R<sup>2</sup>"), tr("F value"),
        tr("Mean error"), tr("Variance of the error"), tr("Standard error"), tr("&chi;<sup>2</sup>") };
    QFont value_font = font();
    value_font.setPointSizeF(value_font.pointSizeF() * 1.08);
    value_font.setBold(true);
    for (const QString& name : names) {
        QLabel* label = new QLabel(name);
        label->setForegroundRole(QPalette::PlaceholderText);
        QLabel* value = new QLabel;
        value->setFont(value_font);
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_rows << qMakePair(label, value);
    }

    m_overview = new TextWidget;
    m_overview->setFrameShape(QFrame::NoFrame);
    m_overview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_overview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_overview->viewport()->setAutoFillBackground(false);
    content_layout->addWidget(m_overview);
    content_layout->addStretch();
    content->setLayout(content_layout);

    QScrollArea* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidget(content);

    QVBoxLayout* m_layout = new QVBoxLayout;
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(area);

    connect(m_model.toStrongRef().data(), SIGNAL(StatisticChanged()), this, SLOT(Update()));
    setLayout(m_layout);
    ArrangeRows();
    Update();
}

void StatisticWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    ArrangeRows();
    // the text block follows its content height; the outer scroll area scrolls if needed
    m_overview->document()->setTextWidth(m_overview->viewport()->width());
    m_overview->setFixedHeight(int(m_overview->document()->size().height()) + 8);
}

void StatisticWidget::ArrangeRows()
{
    const int columns = width() >= 900 ? 3 : (width() >= 560 ? 2 : 1);
    if (columns == m_columns)
        return;
    m_columns = columns;
    for (const auto& row : std::as_const(m_rows)) {
        m_grid->removeWidget(row.first);
        m_grid->removeWidget(row.second);
    }
    const int per_column = (m_rows.size() + columns - 1) / columns;
    for (int i = 0; i < m_rows.size(); ++i) {
        const int c = i / per_column, r = i % per_column;
        m_grid->addWidget(m_rows[i].first, r, 2 * c);
        m_grid->addWidget(m_rows[i].second, r, 2 * c + 1);
    }
    for (int c = 0; c < 6; ++c)
        m_grid->setColumnStretch(c, (c % 2 == 1 && c / 2 < columns) ? 1 : 0);
}

StatisticWidget::~StatisticWidget()
{
}

void StatisticWidget::toggleView()
{
    bool hidden = m_subwidget->isHidden();
    m_subwidget->setHidden(!hidden);
}

void StatisticWidget::Update()
{
    QString overview("<table style=\'width:100%\'>");
    overview += "<tr><td>Model Type:</t><td><b>" + Model2Name(m_model.toStrongRef()->SFModel()) + " (" + QString::number(m_model.toStrongRef()->SFModel()) + ") " + "</b></td></tr>\n";
    overview += "<tr><td>Parameter fitted:</t><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Parameter()) + "</b></td></tr>\n";
    overview += "<tr><td>Number of used Points:</t><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Points()) + "</b></td></tr>\n";
    overview += "<tr><td>Degrees of Freedom:</t><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Points() - m_model.toStrongRef().data()->Parameter()) + "</b></td></tr>\n";
    overview += "<tr><td>Error: (squared / absolute)</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->SSE()) + "/" + Print::printDouble(m_model.toStrongRef().data()->SAE()) + "</b></td></tr>\n";
    overview += "<tr><td>Error Threshold (f-Test)</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->ErrorfTestThreshold(qApp->instance()->property("p_value").toDouble())) + "</b></td></tr>\n";
    overview += "<tr><td>R<sup>2</sup></td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->RSquared()) + "</b></td></tr>\n";
    overview += "<tr><td>f-Value</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->finv(qApp->instance()->property("p_value").toDouble())) + "</b></td></tr>\n";
    overview += "<tr><td>Mean Error in Model:</td><td><b> " + Print::printDouble(m_model.toStrongRef().data()->MeanError()) + "</b></td></tr>\n";
    overview += "<tr><td>Variance of Error:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->Variance()) + "</b></td></tr>\n";
    overview += "<tr><td>Standard deviation &sigma;<sub>fit</sub>:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->StdDeviation()) + "</b></td></tr>\n";
    overview += "<tr><td>Standard Error:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->StdError()) + "</b></td></tr>\n";
    overview += "<tr><td>SE<sub>y</sub>:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->SEy()) + "</b></td></tr>\n";
    overview += "<tr><td>&chi;:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->ChiSquared()) + "</b></td></tr>\n";
    //overview += "<tr><td>cov<sub>fit</sub>:</td><td><b>" + Print::printDouble(m_model.toStrongRef().data()->CovFit()) + "</b></td></tr>\n";

    overview += "</table></br>";
    overview += m_model.toStrongRef().data()->ModelInfo() + "</br>";
    // overview += m_model.toStrongRef().data()->AdditionalOutput();

    m_short = overview;
    overview.clear();
    QString moco;

    QJsonObject result = m_model.toStrongRef().data()->getFastConfidence();
    if (!m_model.toStrongRef().data()->getFastConfidence().isEmpty()) {
        moco += "<p><b>Fast Confidence Results:</b></p>\n";
        moco += "<table>\n";

        for (int i = 0; i < result.count() - 1; ++i) {
            QJsonObject data = result.value(QString::number(i)).toObject();
            if (data.isEmpty())
                continue;
            moco += Print::TextFromConfidence(data, result["controller"].toObject());
        }
        moco += "</table>\n";
        overview += moco;
    }
    m_statistics = overview;

    auto model = m_model.toStrongRef();
    const double p = qApp->instance()->property("p_value").toDouble();
    const QStringList values = { Model2Name(model->SFModel()) + " (" + QString::number(model->SFModel()) + ")",
        Print::printDouble(model->Parameter()), Print::printDouble(model->Points()),
        Print::printDouble(model->Points() - model->Parameter()), Print::printDouble(model->SAE()),
        Print::printDouble(model->ErrorfTestThreshold(p)), Print::printDouble(model->RSquared()),
        Print::printDouble(model->finv(p)), Print::printDouble(model->MeanError()),
        Print::printDouble(model->Variance()), Print::printDouble(model->StdError()),
        Print::printDouble(model->ChiSquared()) };
    for (int i = 0; i < m_rows.size() && i < values.size(); ++i)
        m_rows[i].second->setText(values[i]);

    // model-specific text and fast-confidence results below the form (the form holds the numbers)
    const QString extra = model->ModelInfo() + overview;
    m_overview->setText(extra);
    m_overview->setHidden(extra.trimmed().isEmpty());
    m_overview->document()->setTextWidth(m_overview->viewport()->width());
    m_overview->setFixedHeight(int(m_overview->document()->size().height()) + 8);
}

#include "statisticwidget.moc"
