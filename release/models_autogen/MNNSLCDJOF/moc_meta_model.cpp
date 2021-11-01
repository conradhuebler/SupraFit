/****************************************************************************
** Meta object code from reading C++ file 'meta_model.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/models/meta_model.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QSharedPointer>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'meta_model.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MetaModel_t {
    const uint offsetsAndSize[18];
    char stringdata0[119];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_MetaModel_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_MetaModel_t qt_meta_stringdata_MetaModel = {
    {
QT_MOC_LITERAL(0, 9), // "MetaModel"
QT_MOC_LITERAL(10, 10), // "ModelAdded"
QT_MOC_LITERAL(21, 0), // ""
QT_MOC_LITERAL(22, 29), // "QSharedPointer<AbstractModel>"
QT_MOC_LITERAL(52, 5), // "model"
QT_MOC_LITERAL(58, 12), // "ModelRemoved"
QT_MOC_LITERAL(71, 14), // "ParameterMoved"
QT_MOC_LITERAL(86, 15), // "ParameterSorted"
QT_MOC_LITERAL(102, 16) // "UpdateCalculated"

    },
    "MetaModel\0ModelAdded\0\0"
    "QSharedPointer<AbstractModel>\0model\0"
    "ModelRemoved\0ParameterMoved\0ParameterSorted\0"
    "UpdateCalculated"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MetaModel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   44,    2, 0x06,    1 /* Public */,
       5,    0,   47,    2, 0x06,    3 /* Public */,
       6,    0,   48,    2, 0x06,    4 /* Public */,
       7,    0,   49,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   50,    2, 0x08,    6 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void MetaModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MetaModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ModelAdded((*reinterpret_cast< QSharedPointer<AbstractModel>(*)>(_a[1]))); break;
        case 1: _t->ModelRemoved(); break;
        case 2: _t->ParameterMoved(); break;
        case 3: _t->ParameterSorted(); break;
        case 4: _t->UpdateCalculated(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QSharedPointer<AbstractModel> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MetaModel::*)(QSharedPointer<AbstractModel> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModel::ModelAdded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MetaModel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModel::ModelRemoved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MetaModel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModel::ParameterMoved)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MetaModel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModel::ParameterSorted)) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject MetaModel::staticMetaObject = { {
    QMetaObject::SuperData::link<AbstractModel::staticMetaObject>(),
    qt_meta_stringdata_MetaModel.offsetsAndSize,
    qt_meta_data_MetaModel,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_MetaModel_t
, QtPrivate::TypeAndForceComplete<MetaModel, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QSharedPointer<AbstractModel>, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *MetaModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MetaModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MetaModel.stringdata0))
        return static_cast<void*>(this);
    return AbstractModel::qt_metacast(_clname);
}

int MetaModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = AbstractModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void MetaModel::ModelAdded(QSharedPointer<AbstractModel> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MetaModel::ModelRemoved()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MetaModel::ParameterMoved()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MetaModel::ParameterSorted()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
