/****************************************************************************
** Meta object code from reading C++ file 'modelelement.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/modelelement.h"
#include <QtCharts/qlineseries.h>
#include <QtGui/qtextcursor.h>
#include <QScreen>
#include <QtCharts/qabstractbarseries.h>
#include <QtCharts/qvbarmodelmapper.h>
#include <QtCharts/qboxplotseries.h>
#include <QtCharts/qcandlestickseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCharts/qpieseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCharts/qboxplotseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCharts/qpieseries.h>
#include <QtCharts/qpieseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCharts/qxyseries.h>
#include <QtCharts/qxyseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCharts/qboxplotseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCharts/qpieseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCharts/qxyseries.h>
#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'modelelement.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ModelElement_t {
    const uint offsetsAndSize[44];
    char stringdata0[224];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ModelElement_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ModelElement_t qt_meta_stringdata_ModelElement = {
    {
QT_MOC_LITERAL(0, 12), // "ModelElement"
QT_MOC_LITERAL(13, 12), // "ValueChanged"
QT_MOC_LITERAL(26, 0), // ""
QT_MOC_LITERAL(27, 8), // "Minimize"
QT_MOC_LITERAL(36, 1), // "i"
QT_MOC_LITERAL(38, 8), // "SetColor"
QT_MOC_LITERAL(47, 19), // "ActiveSignalChanged"
QT_MOC_LITERAL(67, 12), // "ColorChanged"
QT_MOC_LITERAL(80, 5), // "color"
QT_MOC_LITERAL(86, 15), // "LocalCheckState"
QT_MOC_LITERAL(102, 5), // "state"
QT_MOC_LITERAL(108, 6), // "Update"
QT_MOC_LITERAL(115, 12), // "ToggleSeries"
QT_MOC_LITERAL(128, 11), // "ChangeColor"
QT_MOC_LITERAL(140, 11), // "setReadOnly"
QT_MOC_LITERAL(152, 8), // "readonly"
QT_MOC_LITERAL(161, 8), // "setLabel"
QT_MOC_LITERAL(170, 3), // "str"
QT_MOC_LITERAL(174, 11), // "chooseColor"
QT_MOC_LITERAL(186, 10), // "togglePlot"
QT_MOC_LITERAL(197, 12), // "toggleActive"
QT_MOC_LITERAL(210, 13) // "UnCheckToggle"

    },
    "ModelElement\0ValueChanged\0\0Minimize\0"
    "i\0SetColor\0ActiveSignalChanged\0"
    "ColorChanged\0color\0LocalCheckState\0"
    "state\0Update\0ToggleSeries\0ChangeColor\0"
    "setReadOnly\0readonly\0setLabel\0str\0"
    "chooseColor\0togglePlot\0toggleActive\0"
    "UnCheckToggle"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ModelElement[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x06,    1 /* Public */,
       3,    1,  105,    2, 0x06,    2 /* Public */,
       5,    0,  108,    2, 0x06,    4 /* Public */,
       6,    0,  109,    2, 0x06,    5 /* Public */,
       7,    1,  110,    2, 0x06,    6 /* Public */,
       9,    1,  113,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      11,    0,  116,    2, 0x0a,   10 /* Public */,
      12,    1,  117,    2, 0x0a,   11 /* Public */,
      13,    1,  120,    2, 0x0a,   13 /* Public */,
      14,    1,  123,    2, 0x0a,   15 /* Public */,
      16,    1,  126,    2, 0x0a,   17 /* Public */,
      18,    0,  129,    2, 0x08,   19 /* Private */,
      19,    0,  130,    2, 0x08,   20 /* Private */,
      20,    0,  131,    2, 0x08,   21 /* Private */,
      21,    1,  132,    2, 0x08,   22 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QColor,    8,
    QMetaType::Void, QMetaType::Int,   10,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    2,
    QMetaType::Void, QMetaType::QColor,    8,
    QMetaType::Void, QMetaType::Bool,   15,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,

       0        // eod
};

void ModelElement::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ModelElement *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ValueChanged(); break;
        case 1: _t->Minimize((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->SetColor(); break;
        case 3: _t->ActiveSignalChanged(); break;
        case 4: _t->ColorChanged((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        case 5: _t->LocalCheckState((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->Update(); break;
        case 7: _t->ToggleSeries((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->ChangeColor((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        case 9: _t->setReadOnly((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 10: _t->setLabel((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->chooseColor(); break;
        case 12: _t->togglePlot(); break;
        case 13: _t->toggleActive(); break;
        case 14: _t->UnCheckToggle((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ModelElement::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelElement::ValueChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ModelElement::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelElement::Minimize)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ModelElement::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelElement::SetColor)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ModelElement::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelElement::ActiveSignalChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ModelElement::*)(const QColor & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelElement::ColorChanged)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ModelElement::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelElement::LocalCheckState)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject ModelElement::staticMetaObject = { {
    QMetaObject::SuperData::link<QGroupBox::staticMetaObject>(),
    qt_meta_stringdata_ModelElement.offsetsAndSize,
    qt_meta_data_ModelElement,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ModelElement_t
, QtPrivate::TypeAndForceComplete<ModelElement, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>


>,
    nullptr
} };


const QMetaObject *ModelElement::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ModelElement::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ModelElement.stringdata0))
        return static_cast<void*>(this);
    return QGroupBox::qt_metacast(_clname);
}

int ModelElement::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGroupBox::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void ModelElement::ValueChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ModelElement::Minimize(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ModelElement::SetColor()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ModelElement::ActiveSignalChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ModelElement::ColorChanged(const QColor & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ModelElement::LocalCheckState(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
