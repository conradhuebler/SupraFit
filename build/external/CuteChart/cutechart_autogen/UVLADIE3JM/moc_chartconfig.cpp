/****************************************************************************
** Meta object code from reading C++ file 'chartconfig.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../external/CuteChart/src/chartconfig.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chartconfig.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChartConfigDialog_t {
    const uint offsetsAndSize[24];
    char stringdata0[142];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ChartConfigDialog_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ChartConfigDialog_t qt_meta_stringdata_ChartConfigDialog = {
    {
QT_MOC_LITERAL(0, 17), // "ChartConfigDialog"
QT_MOC_LITERAL(18, 13), // "ConfigChanged"
QT_MOC_LITERAL(32, 0), // ""
QT_MOC_LITERAL(33, 11), // "ChartConfig"
QT_MOC_LITERAL(45, 11), // "chartconfig"
QT_MOC_LITERAL(57, 9), // "ScaleAxis"
QT_MOC_LITERAL(67, 15), // "ResetFontConfig"
QT_MOC_LITERAL(83, 7), // "Changed"
QT_MOC_LITERAL(91, 12), // "setTicksFont"
QT_MOC_LITERAL(104, 11), // "setKeysFont"
QT_MOC_LITERAL(116, 12), // "setLabelFont"
QT_MOC_LITERAL(129, 12) // "setTitleFont"

    },
    "ChartConfigDialog\0ConfigChanged\0\0"
    "ChartConfig\0chartconfig\0ScaleAxis\0"
    "ResetFontConfig\0Changed\0setTicksFont\0"
    "setKeysFont\0setLabelFont\0setTitleFont"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChartConfigDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x06,    1 /* Public */,
       5,    0,   65,    2, 0x06,    3 /* Public */,
       6,    0,   66,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    0,   67,    2, 0x08,    5 /* Private */,
       8,    0,   68,    2, 0x08,    6 /* Private */,
       9,    0,   69,    2, 0x08,    7 /* Private */,
      10,    0,   70,    2, 0x08,    8 /* Private */,
      11,    0,   71,    2, 0x08,    9 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ChartConfigDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartConfigDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ConfigChanged((*reinterpret_cast< ChartConfig(*)>(_a[1]))); break;
        case 1: _t->ScaleAxis(); break;
        case 2: _t->ResetFontConfig(); break;
        case 3: _t->Changed(); break;
        case 4: _t->setTicksFont(); break;
        case 5: _t->setKeysFont(); break;
        case 6: _t->setLabelFont(); break;
        case 7: _t->setTitleFont(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChartConfigDialog::*)(ChartConfig );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartConfigDialog::ConfigChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChartConfigDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartConfigDialog::ScaleAxis)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChartConfigDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartConfigDialog::ResetFontConfig)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject ChartConfigDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ChartConfigDialog.offsetsAndSize,
    qt_meta_data_ChartConfigDialog,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ChartConfigDialog_t
, QtPrivate::TypeAndForceComplete<ChartConfigDialog, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<ChartConfig, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ChartConfigDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChartConfigDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChartConfigDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int ChartConfigDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void ChartConfigDialog::ConfigChanged(ChartConfig _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ChartConfigDialog::ScaleAxis()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChartConfigDialog::ResetFontConfig()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
