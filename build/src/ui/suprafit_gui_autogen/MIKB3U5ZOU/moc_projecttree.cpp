/****************************************************************************
** Meta object code from reading C++ file 'projecttree.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/mainwindow/projecttree.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'projecttree.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ProjectTree_t {
    const uint offsetsAndSize[38];
    char stringdata0[169];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ProjectTree_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ProjectTree_t qt_meta_stringdata_ProjectTree = {
    {
QT_MOC_LITERAL(0, 11), // "ProjectTree"
QT_MOC_LITERAL(12, 12), // "AddMetaModel"
QT_MOC_LITERAL(25, 0), // ""
QT_MOC_LITERAL(26, 11), // "QModelIndex"
QT_MOC_LITERAL(38, 5), // "index"
QT_MOC_LITERAL(44, 8), // "position"
QT_MOC_LITERAL(53, 19), // "CopySystemParameter"
QT_MOC_LITERAL(73, 6), // "source"
QT_MOC_LITERAL(80, 9), // "UiMessage"
QT_MOC_LITERAL(90, 3), // "str"
QT_MOC_LITERAL(94, 9), // "CopyModel"
QT_MOC_LITERAL(104, 1), // "m"
QT_MOC_LITERAL(106, 4), // "data"
QT_MOC_LITERAL(111, 5), // "model"
QT_MOC_LITERAL(117, 8), // "LoadFile"
QT_MOC_LITERAL(126, 4), // "file"
QT_MOC_LITERAL(131, 14), // "LoadJsonObject"
QT_MOC_LITERAL(146, 6), // "object"
QT_MOC_LITERAL(153, 15) // "UpdateStructure"

    },
    "ProjectTree\0AddMetaModel\0\0QModelIndex\0"
    "index\0position\0CopySystemParameter\0"
    "source\0UiMessage\0str\0CopyModel\0m\0data\0"
    "model\0LoadFile\0file\0LoadJsonObject\0"
    "object\0UpdateStructure"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ProjectTree[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   56,    2, 0x06,    1 /* Public */,
       6,    2,   61,    2, 0x06,    4 /* Public */,
       8,    1,   66,    2, 0x06,    7 /* Public */,
      10,    3,   69,    2, 0x06,    9 /* Public */,
      14,    1,   76,    2, 0x06,   13 /* Public */,
      16,    1,   79,    2, 0x06,   15 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      18,    0,   82,    2, 0x0a,   17 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    5,
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    7,    5,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QJsonObject, QMetaType::Int, QMetaType::Int,   11,   12,   13,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void, QMetaType::QJsonObject,   17,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void ProjectTree::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ProjectTree *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->AddMetaModel((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->CopySystemParameter((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 2: _t->UiMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->CopyModel((*reinterpret_cast< const QJsonObject(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 4: _t->LoadFile((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->LoadJsonObject((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 6: _t->UpdateStructure(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ProjectTree::*)(const QModelIndex & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProjectTree::AddMetaModel)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ProjectTree::*)(const QModelIndex & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProjectTree::CopySystemParameter)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ProjectTree::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProjectTree::UiMessage)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ProjectTree::*)(const QJsonObject & , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProjectTree::CopyModel)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ProjectTree::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProjectTree::LoadFile)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ProjectTree::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ProjectTree::LoadJsonObject)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject ProjectTree::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractItemModel::staticMetaObject>(),
    qt_meta_stringdata_ProjectTree.offsetsAndSize,
    qt_meta_data_ProjectTree,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ProjectTree_t
, QtPrivate::TypeAndForceComplete<ProjectTree, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ProjectTree::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ProjectTree::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ProjectTree.stringdata0))
        return static_cast<void*>(this);
    return QAbstractItemModel::qt_metacast(_clname);
}

int ProjectTree::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractItemModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void ProjectTree::AddMetaModel(const QModelIndex & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ProjectTree::CopySystemParameter(const QModelIndex & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ProjectTree::UiMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ProjectTree::CopyModel(const QJsonObject & _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ProjectTree::LoadFile(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ProjectTree::LoadJsonObject(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
