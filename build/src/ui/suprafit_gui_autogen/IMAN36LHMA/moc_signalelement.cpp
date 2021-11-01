/****************************************************************************
** Meta object code from reading C++ file 'signalelement.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/signalelement.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'signalelement.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SignalElement_t {
    const uint offsetsAndSize[34];
    char stringdata0[159];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SignalElement_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SignalElement_t qt_meta_stringdata_SignalElement = {
    {
QT_MOC_LITERAL(0, 13), // "SignalElement"
QT_MOC_LITERAL(14, 10), // "HideSeries"
QT_MOC_LITERAL(25, 0), // ""
QT_MOC_LITERAL(26, 12), // "ToggleSeries"
QT_MOC_LITERAL(39, 1), // "i"
QT_MOC_LITERAL(41, 11), // "chooseColor"
QT_MOC_LITERAL(53, 12), // "ColorChanged"
QT_MOC_LITERAL(66, 5), // "color"
QT_MOC_LITERAL(72, 8), // "ShowLine"
QT_MOC_LITERAL(81, 7), // "setName"
QT_MOC_LITERAL(89, 3), // "str"
QT_MOC_LITERAL(93, 13), // "setMarkerSize"
QT_MOC_LITERAL(107, 5), // "value"
QT_MOC_LITERAL(113, 14), // "setMarkerShape"
QT_MOC_LITERAL(128, 5), // "shape"
QT_MOC_LITERAL(134, 10), // "togglePlot"
QT_MOC_LITERAL(145, 13) // "UnCheckToggle"

    },
    "SignalElement\0HideSeries\0\0ToggleSeries\0"
    "i\0chooseColor\0ColorChanged\0color\0"
    "ShowLine\0setName\0str\0setMarkerSize\0"
    "value\0setMarkerShape\0shape\0togglePlot\0"
    "UnCheckToggle"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SignalElement[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   74,    2, 0x0a,    1 /* Public */,
       3,    1,   75,    2, 0x08,    2 /* Private */,
       5,    0,   78,    2, 0x08,    4 /* Private */,
       6,    1,   79,    2, 0x08,    5 /* Private */,
       8,    1,   82,    2, 0x08,    7 /* Private */,
       9,    1,   85,    2, 0x08,    9 /* Private */,
      11,    1,   88,    2, 0x08,   11 /* Private */,
      13,    1,   91,    2, 0x08,   13 /* Private */,
      15,    0,   94,    2, 0x08,   15 /* Private */,
      16,    1,   95,    2, 0x08,   16 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QColor,    7,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QReal,   12,
    QMetaType::Void, QMetaType::Int,   14,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,

       0        // eod
};

void SignalElement::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SignalElement *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->HideSeries(); break;
        case 1: _t->ToggleSeries((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->chooseColor(); break;
        case 3: _t->ColorChanged((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        case 4: _t->ShowLine((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->setName((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->setMarkerSize((*reinterpret_cast< qreal(*)>(_a[1]))); break;
        case 7: _t->setMarkerShape((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->togglePlot(); break;
        case 9: _t->UnCheckToggle((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject SignalElement::staticMetaObject = { {
    QMetaObject::SuperData::link<QGroupBox::staticMetaObject>(),
    qt_meta_stringdata_SignalElement.offsetsAndSize,
    qt_meta_data_SignalElement,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SignalElement_t
, QtPrivate::TypeAndForceComplete<SignalElement, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<qreal, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>


>,
    nullptr
} };


const QMetaObject *SignalElement::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SignalElement::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SignalElement.stringdata0))
        return static_cast<void*>(this);
    return QGroupBox::qt_metacast(_clname);
}

int SignalElement::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGroupBox::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
