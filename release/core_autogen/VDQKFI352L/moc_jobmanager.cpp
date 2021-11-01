/****************************************************************************
** Meta object code from reading C++ file 'jobmanager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/capabilities/jobmanager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'jobmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_JobManager_t {
    const uint offsetsAndSize[38];
    char stringdata0[147];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_JobManager_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_JobManager_t qt_meta_stringdata_JobManager = {
    {
QT_MOC_LITERAL(0, 10), // "JobManager"
QT_MOC_LITERAL(11, 7), // "started"
QT_MOC_LITERAL(19, 0), // ""
QT_MOC_LITERAL(20, 8), // "finished"
QT_MOC_LITERAL(29, 7), // "current"
QT_MOC_LITERAL(37, 3), // "all"
QT_MOC_LITERAL(41, 4), // "time"
QT_MOC_LITERAL(46, 11), // "AllFinished"
QT_MOC_LITERAL(58, 11), // "incremented"
QT_MOC_LITERAL(70, 1), // "t"
QT_MOC_LITERAL(72, 7), // "prepare"
QT_MOC_LITERAL(80, 5), // "count"
QT_MOC_LITERAL(86, 10), // "ShowResult"
QT_MOC_LITERAL(97, 16), // "SupraFit::Method"
QT_MOC_LITERAL(114, 4), // "type"
QT_MOC_LITERAL(119, 5), // "index"
QT_MOC_LITERAL(125, 9), // "Interrupt"
QT_MOC_LITERAL(135, 7), // "Message"
QT_MOC_LITERAL(143, 3) // "str"

    },
    "JobManager\0started\0\0finished\0current\0"
    "all\0time\0AllFinished\0incremented\0t\0"
    "prepare\0count\0ShowResult\0SupraFit::Method\0"
    "type\0index\0Interrupt\0Message\0str"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_JobManager[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,
       3,    3,   63,    2, 0x06,    2 /* Public */,
       7,    0,   70,    2, 0x06,    6 /* Public */,
       8,    1,   71,    2, 0x06,    7 /* Public */,
      10,    1,   74,    2, 0x06,    9 /* Public */,
      12,    2,   77,    2, 0x06,   11 /* Public */,
      16,    0,   82,    2, 0x06,   14 /* Public */,
      17,    1,   83,    2, 0x06,   15 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int,    4,    5,    6,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Int,   11,
    QMetaType::Void, 0x80000000 | 13, QMetaType::Int,   14,   15,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   18,

       0        // eod
};

void JobManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<JobManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->started(); break;
        case 1: _t->finished((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 2: _t->AllFinished(); break;
        case 3: _t->incremented((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->prepare((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->ShowResult((*reinterpret_cast< SupraFit::Method(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->Interrupt(); break;
        case 7: _t->Message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (JobManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::started)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (JobManager::*)(int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::finished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (JobManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::AllFinished)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (JobManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::incremented)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (JobManager::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::prepare)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (JobManager::*)(SupraFit::Method , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::ShowResult)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (JobManager::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::Interrupt)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (JobManager::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&JobManager::Message)) {
                *result = 7;
                return;
            }
        }
    }
}

const QMetaObject JobManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_JobManager.offsetsAndSize,
    qt_meta_data_JobManager,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_JobManager_t
, QtPrivate::TypeAndForceComplete<JobManager, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<SupraFit::Method, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>



>,
    nullptr
} };


const QMetaObject *JobManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *JobManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_JobManager.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int JobManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void JobManager::started()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void JobManager::finished(int _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void JobManager::AllFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void JobManager::incremented(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void JobManager::prepare(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void JobManager::ShowResult(SupraFit::Method _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void JobManager::Interrupt()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void JobManager::Message(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
