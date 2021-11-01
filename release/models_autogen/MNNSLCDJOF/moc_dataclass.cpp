/****************************************************************************
** Meta object code from reading C++ file 'dataclass.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/models/dataclass.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dataclass.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DataTable_t {
    const uint offsetsAndSize[2];
    char stringdata0[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_DataTable_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_DataTable_t qt_meta_stringdata_DataTable = {
    {
QT_MOC_LITERAL(0, 9) // "DataTable"

    },
    "DataTable"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DataTable[] = {

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

void DataTable::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject DataTable::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractTableModel::staticMetaObject>(),
    qt_meta_stringdata_DataTable.offsetsAndSize,
    qt_meta_data_DataTable,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_DataTable_t
, QtPrivate::TypeAndForceComplete<DataTable, std::true_type>



>,
    nullptr
} };


const QMetaObject *DataTable::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DataTable::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DataTable.stringdata0))
        return static_cast<void*>(this);
    return QAbstractTableModel::qt_metacast(_clname);
}

int DataTable::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractTableModel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_DataClassPrivateObject_t {
    const uint offsetsAndSize[18];
    char stringdata0[105];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_DataClassPrivateObject_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_DataClassPrivateObject_t qt_meta_stringdata_DataClassPrivateObject = {
    {
QT_MOC_LITERAL(0, 22), // "DataClassPrivateObject"
QT_MOC_LITERAL(23, 22), // "SystemParameterChanged"
QT_MOC_LITERAL(46, 0), // ""
QT_MOC_LITERAL(47, 21), // "SystemParameterLoaded"
QT_MOC_LITERAL(69, 6), // "Update"
QT_MOC_LITERAL(76, 7), // "Message"
QT_MOC_LITERAL(84, 3), // "str"
QT_MOC_LITERAL(88, 8), // "priority"
QT_MOC_LITERAL(97, 7) // "Warning"

    },
    "DataClassPrivateObject\0SystemParameterChanged\0"
    "\0SystemParameterLoaded\0Update\0Message\0"
    "str\0priority\0Warning"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DataClassPrivateObject[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x06,    1 /* Public */,
       3,    0,   57,    2, 0x06,    2 /* Public */,
       4,    0,   58,    2, 0x06,    3 /* Public */,
       5,    2,   59,    2, 0x06,    4 /* Public */,
       5,    1,   64,    2, 0x26,    7 /* Public | MethodCloned */,
       8,    2,   67,    2, 0x06,    9 /* Public */,
       8,    1,   72,    2, 0x26,   12 /* Public | MethodCloned */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    6,    7,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    6,    7,
    QMetaType::Void, QMetaType::QString,    6,

       0        // eod
};

void DataClassPrivateObject::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DataClassPrivateObject *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->SystemParameterChanged(); break;
        case 1: _t->SystemParameterLoaded(); break;
        case 2: _t->Update(); break;
        case 3: _t->Message((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->Message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->Warning((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->Warning((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DataClassPrivateObject::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClassPrivateObject::SystemParameterChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DataClassPrivateObject::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClassPrivateObject::SystemParameterLoaded)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DataClassPrivateObject::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClassPrivateObject::Update)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DataClassPrivateObject::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClassPrivateObject::Message)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DataClassPrivateObject::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClassPrivateObject::Warning)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject DataClassPrivateObject::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_DataClassPrivateObject.offsetsAndSize,
    qt_meta_data_DataClassPrivateObject,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_DataClassPrivateObject_t
, QtPrivate::TypeAndForceComplete<DataClassPrivateObject, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>



>,
    nullptr
} };


const QMetaObject *DataClassPrivateObject::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DataClassPrivateObject::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DataClassPrivateObject.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DataClassPrivateObject::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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
void DataClassPrivateObject::SystemParameterChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DataClassPrivateObject::SystemParameterLoaded()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void DataClassPrivateObject::Update()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void DataClassPrivateObject::Message(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 5
void DataClassPrivateObject::Warning(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
struct qt_meta_stringdata_DataClass_t {
    const uint offsetsAndSize[32];
    char stringdata0[181];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_DataClass_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_DataClass_t qt_meta_stringdata_DataClass = {
    {
QT_MOC_LITERAL(0, 9), // "DataClass"
QT_MOC_LITERAL(10, 8), // "RowAdded"
QT_MOC_LITERAL(19, 0), // ""
QT_MOC_LITERAL(20, 20), // "ActiveSignalsChanged"
QT_MOC_LITERAL(41, 10), // "QList<int>"
QT_MOC_LITERAL(52, 14), // "active_signals"
QT_MOC_LITERAL(67, 21), // "SystemParameterLoaded"
QT_MOC_LITERAL(89, 22), // "SystemParameterChanged"
QT_MOC_LITERAL(112, 19), // "ProjectTitleChanged"
QT_MOC_LITERAL(132, 4), // "name"
QT_MOC_LITERAL(137, 6), // "Update"
QT_MOC_LITERAL(144, 7), // "Deleted"
QT_MOC_LITERAL(152, 7), // "Message"
QT_MOC_LITERAL(160, 3), // "str"
QT_MOC_LITERAL(164, 8), // "priority"
QT_MOC_LITERAL(173, 7) // "Warning"

    },
    "DataClass\0RowAdded\0\0ActiveSignalsChanged\0"
    "QList<int>\0active_signals\0"
    "SystemParameterLoaded\0SystemParameterChanged\0"
    "ProjectTitleChanged\0name\0Update\0Deleted\0"
    "Message\0str\0priority\0Warning"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DataClass[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       9,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   68,    2, 0x06,    1 /* Public */,
       3,    1,   69,    2, 0x06,    2 /* Public */,
       6,    0,   72,    2, 0x06,    4 /* Public */,
       7,    0,   73,    2, 0x06,    5 /* Public */,
       8,    1,   74,    2, 0x06,    6 /* Public */,
      10,    0,   77,    2, 0x06,    8 /* Public */,
      11,    0,   78,    2, 0x06,    9 /* Public */,
      12,    2,   79,    2, 0x06,   10 /* Public */,
      15,    2,   84,    2, 0x06,   13 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   13,   14,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   13,   14,

       0        // eod
};

void DataClass::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DataClass *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->RowAdded(); break;
        case 1: _t->ActiveSignalsChanged((*reinterpret_cast< QList<int>(*)>(_a[1]))); break;
        case 2: _t->SystemParameterLoaded(); break;
        case 3: _t->SystemParameterChanged(); break;
        case 4: _t->ProjectTitleChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->Update(); break;
        case 6: _t->Deleted(); break;
        case 7: _t->Message((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 8: _t->Warning((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<int> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DataClass::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::RowAdded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DataClass::*)(QList<int> );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::ActiveSignalsChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DataClass::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::SystemParameterLoaded)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DataClass::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::SystemParameterChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DataClass::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::ProjectTitleChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (DataClass::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::Update)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (DataClass::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::Deleted)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (DataClass::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::Message)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (DataClass::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataClass::Warning)) {
                *result = 8;
                return;
            }
        }
    }
}

const QMetaObject DataClass::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_DataClass.offsetsAndSize,
    qt_meta_data_DataClass,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_DataClass_t
, QtPrivate::TypeAndForceComplete<DataClass, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QList<int>, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>



>,
    nullptr
} };


const QMetaObject *DataClass::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DataClass::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DataClass.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DataClass::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void DataClass::RowAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DataClass::ActiveSignalsChanged(QList<int> _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DataClass::SystemParameterLoaded()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void DataClass::SystemParameterChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void DataClass::ProjectTitleChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void DataClass::Update()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void DataClass::Deleted()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void DataClass::Message(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void DataClass::Warning(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
