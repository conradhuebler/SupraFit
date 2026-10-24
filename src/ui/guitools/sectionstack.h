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

#include <QtCore/QHash>
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
 * area sits between the fit controls and the data series; with a single page the list is hidden.
 *
 * A page added with addGroupPage() has sub-pages (e.g. the analyses of the statistic tools). Choosing it
 * replaces the page list by the list of its sub-pages with a "Back" entry on top. Claude Generated. */
class SectionStack : public QWidget {
public:
    explicit SectionStack(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        m_list = makeList();
        m_sub_list = makeList();
        m_nav = new QStackedWidget;
        m_nav->setFixedWidth(150);
        m_nav->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        m_nav->addWidget(m_list);
        m_nav->addWidget(m_sub_list);
        m_stack = new QStackedWidget;

        QHBoxLayout* layout = new QHBoxLayout;
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(6);
        layout->addWidget(m_nav);
        layout->addWidget(m_stack, 1);
        setLayout(layout);

        QObject::connect(m_list, &QListWidget::currentRowChanged, m_stack, [this](int row) {
            if (row >= 0) {
                m_stack->setCurrentIndex(row);
                if (m_changed)
                    m_changed(row);
                if (!m_quiet)
                    enterGroup(row);
            }
        });
        // choosing the group page again after "Back" does not change the row: the click enters it
        QObject::connect(m_list, &QListWidget::itemClicked, m_list, [this](QListWidgetItem*) { enterGroup(m_list->currentRow()); });
        QObject::connect(m_sub_list, &QListWidget::itemClicked, m_sub_list, [this](QListWidgetItem*) {
            const int row = m_sub_list->currentRow();
            if (row == 0)
                leaveGroup();
            else
                chooseSub(row - 1);
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
        if (m_list->currentRow() < 0) {
            m_quiet = true; // the first page is selected, not entered
            m_list->setCurrentRow(0);
            m_quiet = false;
        }
        return m_list->count() - 1;
    }

    /*! \brief Like addPage(), with sub-pages: \p subTitles are listed instead of the pages while this page is
     * shown, \p subChosen(i) is called when the i-th one is picked (it switches whatever \p page shows). */
    int addGroupPage(QWidget* page, const QString& title, const QStringList& subTitles, std::function<void(int)> subChosen)
    {
        const int index = addPage(page, title);
        m_groups.insert(index, Group{ subTitles, subChosen, 0 });
        return index;
    }

    /*! \brief Index of the page called \p title, -1 if there is none. Pages are found by title because the
     * set of pages differs between models, so a stored index would point at another page. */
    int indexOfTitle(const QString& title) const { return m_titles.indexOf(title); }
    QString currentTitle() const { return m_titles.value(m_list->currentRow()); }

    /*! \brief Select the page called \p title; falls back to the first visible page if it is missing or hidden.
     * \p enter also opens the sub-page list of a group page (a request by the user), a restored choice does not. */
    void setCurrentTitle(const QString& title, bool enter = false)
    {
        const int index = indexOfTitle(title);
        m_quiet = true;
        if (index >= 0 && !m_list->item(index)->isHidden())
            m_list->setCurrentRow(index);
        else
            selectFirstVisible();
        m_quiet = false;
        if (enter)
            enterGroup(m_list->currentRow());
    }

    /*! \brief Take a page out of the list (its content may live elsewhere meanwhile) or put it back. */
    void setPageHidden(int index, bool hidden)
    {
        if (index < 0 || index >= m_list->count())
            return;
        m_list->item(index)->setHidden(hidden);
        if (hidden && m_list->currentRow() == index) {
            m_quiet = true;
            selectFirstVisible();
            m_quiet = false;
        }
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
    struct Group {
        QStringList subs;
        std::function<void(int)> chosen;
        int current;
    };

    static QListWidget* makeList()
    {
        QListWidget* list = new QListWidget;
        list->setFrameShape(QFrame::NoFrame);
        list->setSpacing(2);
        list->setBackgroundRole(QPalette::Window);
        list->setWordWrap(true); // long titles ("Cross Validation/Reduction") wrap instead of getting a scroll bar
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list->setTextElideMode(Qt::ElideNone);
        return list;
    }

    void selectFirstVisible()
    {
        for (int i = 0; i < m_list->count(); ++i)
            if (!m_list->item(i)->isHidden()) {
                m_list->setCurrentRow(i);
                return;
            }
    }

    void enterGroup(int row)
    {
        auto it = m_groups.find(row);
        if (it == m_groups.end())
            return;
        m_sub_list->blockSignals(true);
        m_sub_list->clear();
        m_sub_list->addItem(QObject::tr("\u2190 Back"));
        m_sub_list->addItems(it->subs);
        m_sub_list->setCurrentRow(1 + it->current);
        m_sub_list->blockSignals(false);
        m_nav->setCurrentIndex(1);
        m_nav->setVisible(true);
        if (it->chosen)
            it->chosen(it->current);
    }

    void leaveGroup()
    {
        m_nav->setCurrentIndex(0);
        updateListVisibility();
    }

    void chooseSub(int sub)
    {
        auto it = m_groups.find(m_list->currentRow());
        if (it == m_groups.end())
            return;
        it->current = sub;
        if (it->chosen)
            it->chosen(sub);
    }

    // the list is only worth its width when there is something to choose from
    void updateListVisibility()
    {
        int visible = 0;
        for (int i = 0; i < m_list->count(); ++i)
            if (!m_list->item(i)->isHidden())
                ++visible;
        m_nav->setVisible(visible > 1 || m_nav->currentIndex() == 1);
    }

    QStringList m_titles;
    QHash<int, Group> m_groups;
    QListWidget* m_list;
    QListWidget* m_sub_list;
    QStackedWidget* m_nav;
    QStackedWidget* m_stack;
    std::function<void(int)> m_changed;
    bool m_quiet = false; // set while the selection is changed by code that must not open a group
};
