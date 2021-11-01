/****************************************************************************
** Meta object code from reading C++ file 'configdialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/dialogs/configdialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'configdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_OptimizerDialog_t {
    const uint offsetsAndSize[2];
    char stringdata0[16];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_OptimizerDialog_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_OptimizerDialog_t qt_meta_stringdata_OptimizerDialog = {
    {
QT_MOC_LITERAL(0, 15) // "OptimizerDialog"

    },
    "OptimizerDialog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_OptimizerDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void OptimizerDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject OptimizerDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_OptimizerDialog.offsetsAndSize,
    qt_meta_data_OptimizerDialog,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_OptimizerDialog_t
, QtPrivate::TypeAndForceComplete<OptimizerDialog, std::true_type>



>,
    nullptr
} };


const QMetaObject *OptimizerDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OptimizerDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_OptimizerDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int OptimizerDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_ConfigDialog_t {
    const uint offsetsAndSize[6];
    char stringdata0[31];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ConfigDialog_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ConfigDialog_t qt_meta_stringdata_ConfigDialog = {
    {
QT_MOC_LITERAL(0, 12), // "ConfigDialog"
QT_MOC_LITERAL(13, 16), // "SelectWorkingDir"
QT_MOC_LITERAL(30, 0) // ""

    },
    "ConfigDialog\0SelectWorkingDir\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ConfigDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   20,    2, 0x08,    1 /* Private */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void ConfigDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ConfigDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->SelectWorkingDir(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject ConfigDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ConfigDialog.offsetsAndSize,
    qt_meta_data_ConfigDialog,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ConfigDialog_t
, QtPrivate::TypeAndForceComplete<ConfigDialog, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ConfigDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ConfigDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ConfigDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int ConfigDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
