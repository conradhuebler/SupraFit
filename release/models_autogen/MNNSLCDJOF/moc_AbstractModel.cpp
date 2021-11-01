/****************************************************************************
** Meta object code from reading C++ file 'AbstractModel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/models/AbstractModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AbstractModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AbstractModel_t {
    const uint offsetsAndSize[30];
    char stringdata0[155];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_AbstractModel_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_AbstractModel_t qt_meta_stringdata_AbstractModel = {
    {
QT_MOC_LITERAL(0, 13), // "AbstractModel"
QT_MOC_LITERAL(14, 12), // "Recalculated"
QT_MOC_LITERAL(27, 0), // ""
QT_MOC_LITERAL(28, 16), // "StatisticChanged"
QT_MOC_LITERAL(45, 13), // "OptionChanged"
QT_MOC_LITERAL(59, 5), // "index"
QT_MOC_LITERAL(65, 5), // "value"
QT_MOC_LITERAL(71, 12), // "ChartUpdated"
QT_MOC_LITERAL(84, 5), // "chart"
QT_MOC_LITERAL(90, 16), // "ModelNameChanged"
QT_MOC_LITERAL(107, 4), // "name"
QT_MOC_LITERAL(112, 9), // "Calculate"
QT_MOC_LITERAL(122, 15), // "UpdateParameter"
QT_MOC_LITERAL(138, 12), // "UpdateOption"
QT_MOC_LITERAL(151, 3) // "str"

    },
    "AbstractModel\0Recalculated\0\0"
    "StatisticChanged\0OptionChanged\0index\0"
    "value\0ChartUpdated\0chart\0ModelNameChanged\0"
    "name\0Calculate\0UpdateParameter\0"
    "UpdateOption\0str"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AbstractModel[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,
       3,    0,   63,    2, 0x06,    2 /* Public */,
       4,    2,   64,    2, 0x06,    3 /* Public */,
       7,    1,   69,    2, 0x06,    6 /* Public */,
       9,    1,   72,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      11,    0,   75,    2, 0x0a,   10 /* Public */,
      12,    0,   76,    2, 0x0a,   11 /* Public */,
      13,    2,   77,    2, 0x0a,   12 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    5,    6,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   10,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    5,   14,

       0        // eod
};

void AbstractModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AbstractModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Recalculated(); break;
        case 1: _t->StatisticChanged(); break;
        case 2: _t->OptionChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->ChartUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->ModelNameChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->Calculate(); break;
        case 6: _t->UpdateParameter(); break;
        case 7: _t->UpdateOption((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AbstractModel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractModel::Recalculated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AbstractModel::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractModel::StatisticChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AbstractModel::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractModel::OptionChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AbstractModel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractModel::ChartUpdated)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AbstractModel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractModel::ModelNameChanged)) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject AbstractModel::staticMetaObject = { {
    QMetaObject::SuperData::link<DataClass::staticMetaObject>(),
    qt_meta_stringdata_AbstractModel.offsetsAndSize,
    qt_meta_data_AbstractModel,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_AbstractModel_t
, QtPrivate::TypeAndForceComplete<AbstractModel, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>


>,
    nullptr
} };


const QMetaObject *AbstractModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AbstractModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AbstractModel.stringdata0))
        return static_cast<void*>(this);
    return DataClass::qt_metacast(_clname);
}

int AbstractModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = DataClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void AbstractModel::Recalculated()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void AbstractModel::StatisticChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AbstractModel::OptionChanged(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AbstractModel::ChartUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void AbstractModel::ModelNameChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
