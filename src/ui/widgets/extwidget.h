/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2018 - 2019 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/ui/guitools/mime.h"

#include <QtCore/QDebug>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>

#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>

#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QWidget>

class AbstractModel;

class DnDLabel : public QLabel {
    Q_OBJECT

public:
    DnDLabel(const QString& str)
    {
        setText(str);
    }

    inline void setContent(const QByteArray& str) { m_content = str; }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {

        ModelMime* mimeData = new ModelMime;
        mimeData->setData("application/x-suprafitmodel", m_content);
        mimeData->setText("SupraFitSimulation");

        QDrag* drag = new QDrag(this);
        drag->setPixmap(QPixmap(":/misc/suprafit.png"));
        drag->setMimeData(mimeData);
        drag->setHotSpot(event->pos());
        drag->exec();
    }
    void enterEvent(QEvent* ev) override
    {
        QApplication::setOverrideCursor(Qt::OpenHandCursor);
        QTimer::singleShot(1500, this, &QApplication::restoreOverrideCursor);
    }

    void leaveEvent(QEvent* ev) override
    {
        QApplication::restoreOverrideCursor();
    }

private:
    QByteArray m_content;
};

class ClickLabel : public QLabel {
    Q_OBJECT

public:
    void Clicked()
    {
        QString txt = text();
        setText("<font color='red'>Copied to Clipboard ...</font>");
        QTimer::singleShot(1800, this, [this, txt]() { setText(txt); });
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        emit MouseClicked();
        QLabel::mousePressEvent(event);
    }

    void enterEvent(QEvent* ev) override
    {
        QApplication::setOverrideCursor(Qt::PointingHandCursor);
        QTimer::singleShot(1500, this, &QApplication::restoreOverrideCursor);
    }

    void leaveEvent(QEvent* ev) override
    {
        QApplication::restoreOverrideCursor();
    }

signals:
    void MouseClicked();
};

class extWidget : public QWidget {
    Q_OBJECT
public:
    explicit extWidget(QWeakPointer<AbstractModel> model, QWidget* parent = nullptr);

signals:

public slots:

private:
    QWeakPointer<AbstractModel> m_model;

    ClickLabel *m_sse, *m_std, *m_sey;
    DnDLabel *m_ideal, *m_mc_std, *m_mc_sey, *m_mc_user;
    QDoubleSpinBox* m_variance;

private slots:
    void Update();
};
