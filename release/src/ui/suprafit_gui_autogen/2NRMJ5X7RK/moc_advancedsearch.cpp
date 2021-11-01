/****************************************************************************
** Meta object code from reading C++ file 'advancedsearch.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/dialogs/advancedsearch.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'advancedsearch.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ParameterWidget_t {
    const uint offsetsAndSize[10];
    char stringdata0[49];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ParameterWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ParameterWidget_t qt_meta_stringdata_ParameterWidget = {
    {
QT_MOC_LITERAL(0, 15), // "ParameterWidget"
QT_MOC_LITERAL(16, 12), // "valueChanged"
QT_MOC_LITERAL(29, 0), // ""
QT_MOC_LITERAL(30, 12), // "checkChanged"
QT_MOC_LITERAL(43, 5) // "state"

    },
    "ParameterWidget\0valueChanged\0\0"
    "checkChanged\0state"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ParameterWidget[] = {

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
       1,    0,   26,    2, 0x06,    1 /* Public */,
       3,    1,   27,    2, 0x06,    2 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,

       0        // eod
};

void ParameterWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ParameterWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->valueChanged(); break;
        case 1: _t->checkChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ParameterWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ParameterWidget::valueChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ParameterWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ParameterWidget::checkChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject ParameterWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QGroupBox::staticMetaObject>(),
    qt_meta_stringdata_ParameterWidget.offsetsAndSize,
    qt_meta_data_ParameterWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ParameterWidget_t
, QtPrivate::TypeAndForceComplete<ParameterWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>



>,
    nullptr
} };


const QMetaObject *ParameterWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ParameterWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ParameterWidget.stringdata0))
        return static_cast<void*>(this);
    return QGroupBox::qt_metacast(_clname);
}

int ParameterWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGroupBox::qt_metacall(_c, _id, _a);
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
void ParameterWidget::valueChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ParameterWidget::checkChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_AdvancedSearch_t {
    const uint offsetsAndSize[26];
    char stringdata0[131];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_AdvancedSearch_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_AdvancedSearch_t qt_meta_stringdata_AdvancedSearch = {
    {
QT_MOC_LITERAL(0, 14), // "AdvancedSearch"
QT_MOC_LITERAL(15, 14), // "RunCalculation"
QT_MOC_LITERAL(30, 0), // ""
QT_MOC_LITERAL(31, 10), // "controller"
QT_MOC_LITERAL(42, 8), // "setValue"
QT_MOC_LITERAL(51, 5), // "value"
QT_MOC_LITERAL(57, 9), // "Interrupt"
QT_MOC_LITERAL(67, 12), // "MaximumSteps"
QT_MOC_LITERAL(80, 5), // "steps"
QT_MOC_LITERAL(86, 17), // "IncrementProgress"
QT_MOC_LITERAL(104, 4), // "time"
QT_MOC_LITERAL(109, 12), // "SearchGlobal"
QT_MOC_LITERAL(122, 8) // "MaxSteps"

    },
    "AdvancedSearch\0RunCalculation\0\0"
    "controller\0setValue\0value\0Interrupt\0"
    "MaximumSteps\0steps\0IncrementProgress\0"
    "time\0SearchGlobal\0MaxSteps"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AdvancedSearch[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   56,    2, 0x06,    1 /* Public */,
       4,    1,   59,    2, 0x06,    3 /* Public */,
       6,    0,   62,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,   63,    2, 0x0a,    6 /* Public */,
       9,    1,   66,    2, 0x0a,    8 /* Public */,
      11,    0,   69,    2, 0x08,   10 /* Private */,
      12,    0,   70,    2, 0x08,   11 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QJsonObject,    3,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void AdvancedSearch::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AdvancedSearch *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->RunCalculation((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 1: _t->setValue((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->Interrupt(); break;
        case 3: _t->MaximumSteps((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->IncrementProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->SearchGlobal(); break;
        case 6: _t->MaxSteps(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AdvancedSearch::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AdvancedSearch::RunCalculation)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AdvancedSearch::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AdvancedSearch::setValue)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AdvancedSearch::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AdvancedSearch::Interrupt)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject AdvancedSearch::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_AdvancedSearch.offsetsAndSize,
    qt_meta_data_AdvancedSearch,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_AdvancedSearch_t
, QtPrivate::TypeAndForceComplete<AdvancedSearch, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *AdvancedSearch::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AdvancedSearch::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AdvancedSearch.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int AdvancedSearch::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
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
void AdvancedSearch::RunCalculation(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AdvancedSearch::setValue(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void AdvancedSearch::Interrupt()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
