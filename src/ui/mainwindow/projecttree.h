/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2020 - 2022 Conrad Hübler <Conrad.Huebler@gmx.net>
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

#include "src/core/models/dataclass.h"

#include "src/ui/guitools/mime.h"

#include <QtCore/QAbstractItemModel>

class ProjectTree : public QAbstractItemModel {
    Q_OBJECT
public:
    inline ProjectTree(QVector<QWeakPointer<DataClass>>* data_list, QObject* parent)
        : QAbstractItemModel(parent)
    {
        m_data_list = data_list;
        QUuid uuid;
        m_instance = uuid.createUuid().toString();
    }

    inline virtual ~ProjectTree() override {}

    virtual Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        Q_UNUSED(index);
        Qt::ItemFlags flags;

        flags = QAbstractItemModel::flags(index);
        flags = flags | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled;

        return flags;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            if (section == 0)
                return tr("Project Name");
            else
                return tr("# Models");
        } else
            return QAbstractItemModel::headerData(section, orientation, role);
    }

    virtual int columnCount(const QModelIndex& p = QModelIndex()) const override;

    virtual int rowCount(const QModelIndex& p = QModelIndex()) const override;

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;

    virtual QModelIndex parent(const QModelIndex& child) const override;

    virtual Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction | Qt::MoveAction;
    }

    virtual QMimeData* mimeData(const QModelIndexList& indexes) const override;

    virtual bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    virtual bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;

    QString UUID(const QModelIndex& index) const;

public slots:
    void UpdateStructure();

private:
    QVector<QWeakPointer<DataClass>>* m_data_list;

    QStringList m_uuids;
    QList<void*> m_ptr_uuids;

    QString m_instance;

signals:
    void AddMetaModel(const QModelIndex& index, int position);
    void CopySystemParameter(const QModelIndex& source, int position);
    void UiMessage(const QString& str);
    void CopyModel(const QJsonObject& m, int data, int model);
    void LoadFile(const QString& file);
    void LoadJsonObject(const QJsonObject& object);
};
