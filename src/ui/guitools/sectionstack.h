/*
 * Page selector: a slim list on the left, the selected page on the right.
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
 * Claude Generated (UI redesign WP8).
 */

#pragma once

#include <functional>

#include <QtCore/QStringList>

#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

/*! \brief Pages selected from a slim list on the left (instead of a tab bar).
 *
 * Every page gets a header line with its title and optional tool widgets on the right (e.g. load/save
 * buttons), which used to sit on the tab. The list costs no vertical space, which matters when the page
 * area sits between the fit controls and the data series; with a single page the list is hidden. */
class SectionStack : public QWidget {
public:
    explicit SectionStack(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        m_list = new QListWidget;
        m_list->setFrameShape(QFrame::NoFrame);
        m_list->setFixedWidth(150);
        m_list->setSpacing(2);
        m_list->setBackgroundRole(QPalette::Window);
        m_list->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        m_stack = new QStackedWidget;

        QHBoxLayout* layout = new QHBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        layout->addWidget(m_list);
        layout->addWidget(m_stack, 1);
        setLayout(layout);

        QObject::connect(m_list, &QListWidget::currentRowChanged, m_stack, [this](int row) {
            if (row >= 0) {
                m_stack->setCurrentIndex(row);
                if (m_changed)
                    m_changed(row);
            }
        });
    }

    /*! \brief Add \p page under \p title; \p tools (may be null) is shown right-aligned in the page header. */
    int addPage(QWidget* page, const QString& title, QWidget* tools = nullptr)
    {
        QWidget* container = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);
        QHBoxLayout* header = new QHBoxLayout;
        QLabel* label = new QLabel(QString("<b>%1</b>").arg(title));
        header->addWidget(label);
        header->addStretch();
        if (tools)
            header->addWidget(tools);
        layout->addLayout(header);
        layout->addWidget(page, 1);
        container->setLayout(layout);

        m_stack->addWidget(container);
        m_list->addItem(title);
        m_titles << title;
        updateListVisibility();
        if (m_list->currentRow() < 0)
            m_list->setCurrentRow(0);
        return m_list->count() - 1;
    }

    /*! \brief Index of the page called \p title, -1 if there is none. Pages are found by title because the
     * set of pages differs between models, so a stored index would point at another page. Claude Generated. */
    int indexOfTitle(const QString& title) const { return m_titles.indexOf(title); }
    QString currentTitle() const { return m_titles.value(m_list->currentRow()); }

    /*! \brief Select the page called \p title; falls back to the first visible page if it is missing or hidden. */
    void setCurrentTitle(const QString& title)
    {
        const int index = indexOfTitle(title);
        if (index >= 0 && !m_list->item(index)->isHidden())
            m_list->setCurrentRow(index);
        else
            selectFirstVisible();
    }

    /*! \brief Take a page out of the list (its content may live elsewhere meanwhile) or put it back. */
    void setPageHidden(int index, bool hidden)
    {
        if (index < 0 || index >= m_list->count())
            return;
        m_list->item(index)->setHidden(hidden);
        if (hidden && m_list->currentRow() == index)
            selectFirstVisible();
        updateListVisibility();
    }

    int count() const { return m_list->count(); }
    int currentIndex() const { return m_list->currentRow(); }
    void setCurrentIndex(int index)
    {
        if (index >= 0 && index < m_list->count())
            m_list->setCurrentRow(index);
    }
    /*! \brief Called with the new index whenever the user selects another page. */
    void onPageChanged(std::function<void(int)> callback) { m_changed = callback; }

private:
    void selectFirstVisible()
    {
        for (int i = 0; i < m_list->count(); ++i)
            if (!m_list->item(i)->isHidden()) {
                m_list->setCurrentRow(i);
                return;
            }
    }

    // the list is only worth its width when there is something to choose from
    void updateListVisibility()
    {
        int visible = 0;
        for (int i = 0; i < m_list->count(); ++i)
            if (!m_list->item(i)->isHidden())
                ++visible;
        m_list->setVisible(visible > 1);
    }

    QStringList m_titles;
    QListWidget* m_list;
    QStackedWidget* m_stack;
    std::function<void(int)> m_changed;
};
