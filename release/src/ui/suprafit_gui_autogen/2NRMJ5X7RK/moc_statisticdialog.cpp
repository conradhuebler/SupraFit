/****************************************************************************
** Meta object code from reading C++ file 'statisticdialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/dialogs/statisticdialog.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'statisticdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_RadioButton_t {
    const uint offsetsAndSize[2];
    char stringdata0[12];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_RadioButton_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_RadioButton_t qt_meta_stringdata_RadioButton = {
    {
QT_MOC_LITERAL(0, 11) // "RadioButton"

    },
    "RadioButton"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_RadioButton[] = {

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

void RadioButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject RadioButton::staticMetaObject = { {
    QMetaObject::SuperData::link<QRadioButton::staticMetaObject>(),
    qt_meta_stringdata_RadioButton.offsetsAndSize,
    qt_meta_data_RadioButton,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_RadioButton_t
, QtPrivate::TypeAndForceComplete<RadioButton, std::true_type>



>,
    nullptr
} };


const QMetaObject *RadioButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *RadioButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_RadioButton.stringdata0))
        return static_cast<void*>(this);
    return QRadioButton::qt_metacast(_clname);
}

int RadioButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QRadioButton::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_StatisticDialog_t {
    const uint offsetsAndSize[40];
    char stringdata0[226];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_StatisticDialog_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_StatisticDialog_t qt_meta_stringdata_StatisticDialog = {
    {
QT_MOC_LITERAL(0, 15), // "StatisticDialog"
QT_MOC_LITERAL(16, 14), // "RunCalculation"
QT_MOC_LITERAL(31, 0), // ""
QT_MOC_LITERAL(32, 10), // "controller"
QT_MOC_LITERAL(43, 9), // "Reduction"
QT_MOC_LITERAL(53, 9), // "Interrupt"
QT_MOC_LITERAL(63, 15), // "setMaximumSteps"
QT_MOC_LITERAL(79, 5), // "steps"
QT_MOC_LITERAL(85, 12), // "MaximumSteps"
QT_MOC_LITERAL(98, 16), // "MaximumMainSteps"
QT_MOC_LITERAL(115, 17), // "IncrementProgress"
QT_MOC_LITERAL(133, 4), // "time"
QT_MOC_LITERAL(138, 21), // "IncrementMainProgress"
QT_MOC_LITERAL(160, 10), // "HideWidget"
QT_MOC_LITERAL(171, 10), // "ShowWidget"
QT_MOC_LITERAL(182, 9), // "Attention"
QT_MOC_LITERAL(192, 7), // "Message"
QT_MOC_LITERAL(200, 3), // "str"
QT_MOC_LITERAL(204, 6), // "Update"
QT_MOC_LITERAL(211, 14) // "CalculateError"

    },
    "StatisticDialog\0RunCalculation\0\0"
    "controller\0Reduction\0Interrupt\0"
    "setMaximumSteps\0steps\0MaximumSteps\0"
    "MaximumMainSteps\0IncrementProgress\0"
    "time\0IncrementMainProgress\0HideWidget\0"
    "ShowWidget\0Attention\0Message\0str\0"
    "Update\0CalculateError"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StatisticDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   98,    2, 0x06,    1 /* Public */,
       4,    0,  101,    2, 0x06,    3 /* Public */,
       5,    0,  102,    2, 0x06,    4 /* Public */,
       6,    1,  103,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    1,  106,    2, 0x0a,    7 /* Public */,
       9,    1,  109,    2, 0x0a,    9 /* Public */,
      10,    1,  112,    2, 0x0a,   11 /* Public */,
      12,    0,  115,    2, 0x0a,   13 /* Public */,
      13,    0,  116,    2, 0x0a,   14 /* Public */,
      14,    0,  117,    2, 0x0a,   15 /* Public */,
      15,    0,  118,    2, 0x0a,   16 /* Public */,
      16,    1,  119,    2, 0x0a,   17 /* Public */,
      18,    0,  122,    2, 0x08,   19 /* Private */,
      19,    0,  123,    2, 0x08,   20 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QJsonObject,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void StatisticDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StatisticDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->RunCalculation((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 1: _t->Reduction(); break;
        case 2: _t->Interrupt(); break;
        case 3: _t->setMaximumSteps((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->MaximumSteps((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->MaximumMainSteps((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->IncrementProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->IncrementMainProgress(); break;
        case 8: _t->HideWidget(); break;
        case 9: _t->ShowWidget(); break;
        case 10: _t->Attention(); break;
        case 11: _t->Message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->Update(); break;
        case 13: _t->CalculateError(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StatisticDialog::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StatisticDialog::RunCalculation)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StatisticDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StatisticDialog::Reduction)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (StatisticDialog::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StatisticDialog::Interrupt)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (StatisticDialog::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StatisticDialog::setMaximumSteps)) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject StatisticDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_StatisticDialog.offsetsAndSize,
    qt_meta_data_StatisticDialog,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_StatisticDialog_t
, QtPrivate::TypeAndForceComplete<StatisticDialog, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *StatisticDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StatisticDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StatisticDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int StatisticDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
    return _id;
}

// SIGNAL 0
void StatisticDialog::RunCalculation(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StatisticDialog::Reduction()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void StatisticDialog::Interrupt()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void StatisticDialog::setMaximumSteps(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
