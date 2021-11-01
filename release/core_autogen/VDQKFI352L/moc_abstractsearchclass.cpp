/****************************************************************************
** Meta object code from reading C++ file 'abstractsearchclass.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/capabilities/abstractsearchclass.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'abstractsearchclass.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AbstractSearchThread_t {
    const uint offsetsAndSize[10];
    char stringdata0[56];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_AbstractSearchThread_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_AbstractSearchThread_t qt_meta_stringdata_AbstractSearchThread = {
    {
QT_MOC_LITERAL(0, 20), // "AbstractSearchThread"
QT_MOC_LITERAL(21, 17), // "IncrementProgress"
QT_MOC_LITERAL(39, 0), // ""
QT_MOC_LITERAL(40, 5), // "msecs"
QT_MOC_LITERAL(46, 9) // "Interrupt"

    },
    "AbstractSearchThread\0IncrementProgress\0"
    "\0msecs\0Interrupt"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AbstractSearchThread[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   26,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    0,   29,    2, 0x0a,    3 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void AbstractSearchThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AbstractSearchThread *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->IncrementProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->Interrupt(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AbstractSearchThread::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractSearchThread::IncrementProgress)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject AbstractSearchThread::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_AbstractSearchThread.offsetsAndSize,
    qt_meta_data_AbstractSearchThread,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_AbstractSearchThread_t
, QtPrivate::TypeAndForceComplete<AbstractSearchThread, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *AbstractSearchThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AbstractSearchThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AbstractSearchThread.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QRunnable"))
        return static_cast< QRunnable*>(this);
    return QObject::qt_metacast(_clname);
}

int AbstractSearchThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void AbstractSearchThread::IncrementProgress(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_AbstractSearchClass_t {
    const uint offsetsAndSize[20];
    char stringdata0[108];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_AbstractSearchClass_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_AbstractSearchClass_t qt_meta_stringdata_AbstractSearchClass = {
    {
QT_MOC_LITERAL(0, 19), // "AbstractSearchClass"
QT_MOC_LITERAL(20, 17), // "IncrementProgress"
QT_MOC_LITERAL(38, 0), // ""
QT_MOC_LITERAL(39, 5), // "msecs"
QT_MOC_LITERAL(45, 15), // "AnalyseFinished"
QT_MOC_LITERAL(61, 15), // "setMaximumSteps"
QT_MOC_LITERAL(77, 8), // "maxsteps"
QT_MOC_LITERAL(86, 7), // "Message"
QT_MOC_LITERAL(94, 3), // "str"
QT_MOC_LITERAL(98, 9) // "Interrupt"

    },
    "AbstractSearchClass\0IncrementProgress\0"
    "\0msecs\0AnalyseFinished\0setMaximumSteps\0"
    "maxsteps\0Message\0str\0Interrupt"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AbstractSearchClass[] = {

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
       4,    0,   47,    2, 0x06,    3 /* Public */,
       5,    1,   48,    2, 0x06,    4 /* Public */,
       7,    1,   51,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       9,    0,   54,    2, 0x0a,    8 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::QString,    8,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void AbstractSearchClass::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AbstractSearchClass *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->IncrementProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->AnalyseFinished(); break;
        case 2: _t->setMaximumSteps((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->Message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->Interrupt(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AbstractSearchClass::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractSearchClass::IncrementProgress)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AbstractSearchClass::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractSearchClass::AnalyseFinished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AbstractSearchClass::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractSearchClass::setMaximumSteps)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AbstractSearchClass::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AbstractSearchClass::Message)) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject AbstractSearchClass::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_AbstractSearchClass.offsetsAndSize,
    qt_meta_data_AbstractSearchClass,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_AbstractSearchClass_t
, QtPrivate::TypeAndForceComplete<AbstractSearchClass, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *AbstractSearchClass::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AbstractSearchClass::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AbstractSearchClass.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AbstractSearchClass::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void AbstractSearchClass::IncrementProgress(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AbstractSearchClass::AnalyseFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void AbstractSearchClass::setMaximumSteps(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AbstractSearchClass::Message(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
