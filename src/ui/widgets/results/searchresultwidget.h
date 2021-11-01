/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2017 - 2018 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include <QtGui/QPainter>
#include <QtGui/QTextDocument>

#include <QtWidgets/QHeaderView>
#include <QtWidgets/QWidget>

#include "src/capabilities/globalsearch.h"

class AbstractModel;
class GlobalSearch;

class ChartView;
class ScientificBox;
class ScatterWidget;

class QCheckBox;
class QSortFilterProxyModel;
class QPushButton;
class QTableView;
class QTabWidget;

class HeaderView : public QHeaderView {
public:
    HeaderView(Qt::Orientation orientation, QWidget* parent = nullptr)
        : QHeaderView(orientation, parent)
    {
        setSectionsClickable(true);
        setHighlightSections(true);
    }

protected:
    virtual void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override
    {
        QHeaderView::paintSection(painter, rect, logicalIndex);

        QStyleOptionHeader opt;
        initStyleOption(&opt);

        opt.rect = rect;
        opt.section = logicalIndex;
        opt.text = this->model()->headerData(logicalIndex, this->orientation(), Qt::DisplayRole).toString();
        opt.textAlignment = Qt::AlignCenter;

        // the section position

        int visual = visualIndex(logicalIndex);

        Q_ASSERT(visual != -1);

        if (count() == 1)
            opt.position = QStyleOptionHeader::OnlyOneSection;

        else if (visual == 0)
            opt.position = QStyleOptionHeader::Beginning;

        else if (visual == count() - 1)
            opt.position = QStyleOptionHeader::End;

        else
            opt.position = QStyleOptionHeader::Middle;

        QTextDocument TextDoc;
        QString tmp = opt.text;
        opt.text = ""; //IMPORTANT!

        // draw the section
        style()->drawControl(QStyle::CE_Header, &opt, painter, this);

        painter->save();
        QRect textRect = style()->subElementRect(QStyle::SE_HeaderLabel, &opt, this);

        painter->translate(textRect.topLeft());

        TextDoc.setDocumentMargin(0);
        TextDoc.setHtml(tmp);
        TextDoc.drawContents(painter, QRect(QPoint(0, 0), textRect.size()));

        painter->restore();
    }

private:
    QTextDocument TextDoc;
};

class SearchResultWidget : public QWidget {
    Q_OBJECT

public:
    SearchResultWidget(const QJsonObject& results, const QSharedPointer<AbstractModel> model, QWidget* parent);
    ~SearchResultWidget();

private:
    QTableView* BuildList();
    ScatterWidget* BuildScatter();

    QList<QJsonObject> m_models;
    QTableView* m_table;
    ScatterWidget* m_contour;
    QTabWidget* m_central_widget;
    QWeakPointer<AbstractModel> m_model;
    QCheckBox *m_valid, *m_converged;
    ScientificBox* m_threshold;
    QPushButton* m_export;
    QVector<QList<qreal>> m_input;

    QJsonObject m_results;
    QSortFilterProxyModel* m_proxyModel;

private slots:
    void rowSelected(const QModelIndex& index);
    void ExportModels();
    void ApplyFilter();
    void ModelClicked(int model);

signals:
    void LoadModel(const QJsonObject& object);
    void AddModel(const QJsonObject& object);
};
