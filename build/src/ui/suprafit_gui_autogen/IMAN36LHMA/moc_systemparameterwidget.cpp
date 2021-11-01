/****************************************************************************
** Meta object code from reading C++ file 'systemparameterwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/systemparameterwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'systemparameterwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SystemParameterWidget_t {
    const uint offsetsAndSize[8];
    char stringdata0[51];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SystemParameterWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SystemParameterWidget_t qt_meta_stringdata_SystemParameterWidget = {
    {
QT_MOC_LITERAL(0, 21), // "SystemParameterWidget"
QT_MOC_LITERAL(22, 12), // "valueChanged"
QT_MOC_LITERAL(35, 0), // ""
QT_MOC_LITERAL(36, 14) // "PrepareChanged"

    },
    "SystemParameterWidget\0valueChanged\0\0"
    "PrepareChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SystemParameterWidget[] = {

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
       1,    0,   26,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       3,    0,   27,    2, 0x08,    2 /* Private */,

 // signals: parameters
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void SystemParameterWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SystemParameterWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->valueChanged(); break;
        case 1: _t->PrepareChanged(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SystemParameterWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SystemParameterWidget::valueChanged)) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject SystemParameterWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QGroupBox::staticMetaObject>(),
    qt_meta_stringdata_SystemParameterWidget.offsetsAndSize,
    qt_meta_data_SystemParameterWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SystemParameterWidget_t
, QtPrivate::TypeAndForceComplete<SystemParameterWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *SystemParameterWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SystemParameterWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SystemParameterWidget.stringdata0))
        return static_cast<void*>(this);
    return QGroupBox::qt_metacast(_clname);
}

int SystemParameterWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void SystemParameterWidget::valueChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
struct qt_meta_stringdata_SPOverview_t {
    const uint offsetsAndSize[2];
    char stringdata0[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SPOverview_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SPOverview_t qt_meta_stringdata_SPOverview = {
    {
QT_MOC_LITERAL(0, 10) // "SPOverview"

    },
    "SPOverview"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SPOverview[] = {

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

void SPOverview::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject SPOverview::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SPOverview.offsetsAndSize,
    qt_meta_data_SPOverview,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SPOverview_t
, QtPrivate::TypeAndForceComplete<SPOverview, std::true_type>



>,
    nullptr
} };


const QMetaObject *SPOverview::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SPOverview::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SPOverview.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SPOverview::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
