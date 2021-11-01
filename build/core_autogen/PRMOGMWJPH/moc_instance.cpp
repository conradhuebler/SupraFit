/****************************************************************************
** Meta object code from reading C++ file 'instance.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/instance.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'instance.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Instance_t {
    const uint offsetsAndSize[8];
    char stringdata0[40];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_Instance_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_Instance_t qt_meta_stringdata_Instance = {
    {
QT_MOC_LITERAL(0, 8), // "Instance"
QT_MOC_LITERAL(9, 20), // "ConfigurationChanged"
QT_MOC_LITERAL(30, 0), // ""
QT_MOC_LITERAL(31, 8) // "instance"

    },
    "Instance\0ConfigurationChanged\0\0instance"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Instance[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   26,    2, 0x06,    1 /* Public */,
       1,    0,   29,    2, 0x26,    3 /* Public | MethodCloned */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,

       0        // eod
};

void Instance::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Instance *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ConfigurationChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->ConfigurationChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (Instance::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&Instance::ConfigurationChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject Instance::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_Instance.offsetsAndSize,
    qt_meta_data_Instance,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_Instance_t
, QtPrivate::TypeAndForceComplete<Instance, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>



>,
    nullptr
} };


const QMetaObject *Instance::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Instance::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Instance.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Instance::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void Instance::ConfigurationChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
