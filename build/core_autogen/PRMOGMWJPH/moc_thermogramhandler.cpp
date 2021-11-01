/****************************************************************************
** Meta object code from reading C++ file 'thermogramhandler.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/thermogramhandler.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'thermogramhandler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ThermogramHandler_t {
    const uint offsetsAndSize[18];
    char stringdata0[127];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ThermogramHandler_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ThermogramHandler_t qt_meta_stringdata_ThermogramHandler = {
    {
QT_MOC_LITERAL(0, 17), // "ThermogramHandler"
QT_MOC_LITERAL(18, 21), // "ThermogramInitialised"
QT_MOC_LITERAL(40, 0), // ""
QT_MOC_LITERAL(41, 17), // "ThermogramChanged"
QT_MOC_LITERAL(59, 15), // "BaseLineChanged"
QT_MOC_LITERAL(75, 18), // "CalibrationChanged"
QT_MOC_LITERAL(94, 16), // "PeakRulesChanged"
QT_MOC_LITERAL(111, 7), // "Message"
QT_MOC_LITERAL(119, 7) // "message"

    },
    "ThermogramHandler\0ThermogramInitialised\0"
    "\0ThermogramChanged\0BaseLineChanged\0"
    "CalibrationChanged\0PeakRulesChanged\0"
    "Message\0message"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ThermogramHandler[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x06,    1 /* Public */,
       3,    0,   51,    2, 0x06,    2 /* Public */,
       4,    0,   52,    2, 0x06,    3 /* Public */,
       5,    0,   53,    2, 0x06,    4 /* Public */,
       6,    0,   54,    2, 0x06,    5 /* Public */,
       7,    1,   55,    2, 0x06,    6 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,

       0        // eod
};

void ThermogramHandler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ThermogramHandler *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ThermogramInitialised(); break;
        case 1: _t->ThermogramChanged(); break;
        case 2: _t->BaseLineChanged(); break;
        case 3: _t->CalibrationChanged(); break;
        case 4: _t->PeakRulesChanged(); break;
        case 5: _t->Message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ThermogramHandler::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ThermogramHandler::ThermogramInitialised)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ThermogramHandler::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ThermogramHandler::ThermogramChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ThermogramHandler::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ThermogramHandler::BaseLineChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ThermogramHandler::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ThermogramHandler::CalibrationChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ThermogramHandler::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ThermogramHandler::PeakRulesChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ThermogramHandler::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ThermogramHandler::Message)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject ThermogramHandler::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ThermogramHandler.offsetsAndSize,
    qt_meta_data_ThermogramHandler,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ThermogramHandler_t
, QtPrivate::TypeAndForceComplete<ThermogramHandler, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>



>,
    nullptr
} };


const QMetaObject *ThermogramHandler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ThermogramHandler::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ThermogramHandler.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ThermogramHandler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ThermogramHandler::ThermogramInitialised()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ThermogramHandler::ThermogramChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ThermogramHandler::BaseLineChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ThermogramHandler::CalibrationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ThermogramHandler::PeakRulesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ThermogramHandler::Message(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
