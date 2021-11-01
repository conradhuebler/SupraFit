/****************************************************************************
** Meta object code from reading C++ file 'chartviewprivate.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../external/CuteChart/src/chartviewprivate.h"
#include <QtCharts/qlineseries.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chartviewprivate.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChartViewPrivate_t {
    const uint offsetsAndSize[64];
    char stringdata0[305];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ChartViewPrivate_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ChartViewPrivate_t qt_meta_stringdata_ChartViewPrivate = {
    {
QT_MOC_LITERAL(0, 16), // "ChartViewPrivate"
QT_MOC_LITERAL(17, 8), // "LockZoom"
QT_MOC_LITERAL(26, 0), // ""
QT_MOC_LITERAL(27, 10), // "UnLockZoom"
QT_MOC_LITERAL(38, 11), // "ZoomChanged"
QT_MOC_LITERAL(50, 7), // "scaleUp"
QT_MOC_LITERAL(58, 9), // "scaleDown"
QT_MOC_LITERAL(68, 7), // "AddRect"
QT_MOC_LITERAL(76, 6), // "point1"
QT_MOC_LITERAL(83, 6), // "point2"
QT_MOC_LITERAL(90, 8), // "ZoomRect"
QT_MOC_LITERAL(99, 18), // "PointDoubleClicked"
QT_MOC_LITERAL(118, 5), // "point"
QT_MOC_LITERAL(124, 16), // "EscapeSelectMode"
QT_MOC_LITERAL(141, 8), // "RightKey"
QT_MOC_LITERAL(150, 7), // "LeftKey"
QT_MOC_LITERAL(158, 18), // "UpdateVerticalLine"
QT_MOC_LITERAL(177, 1), // "x"
QT_MOC_LITERAL(179, 10), // "UpdateView"
QT_MOC_LITERAL(190, 3), // "min"
QT_MOC_LITERAL(194, 3), // "max"
QT_MOC_LITERAL(198, 22), // "setVerticalLineEnabled"
QT_MOC_LITERAL(221, 7), // "enabled"
QT_MOC_LITERAL(229, 7), // "setZoom"
QT_MOC_LITERAL(237, 5), // "x_min"
QT_MOC_LITERAL(243, 5), // "x_max"
QT_MOC_LITERAL(249, 5), // "y_min"
QT_MOC_LITERAL(255, 5), // "y_max"
QT_MOC_LITERAL(261, 10), // "UpdateZoom"
QT_MOC_LITERAL(272, 12), // "setSelectBox"
QT_MOC_LITERAL(285, 7), // "topleft"
QT_MOC_LITERAL(293, 11) // "bottomright"

    },
    "ChartViewPrivate\0LockZoom\0\0UnLockZoom\0"
    "ZoomChanged\0scaleUp\0scaleDown\0AddRect\0"
    "point1\0point2\0ZoomRect\0PointDoubleClicked\0"
    "point\0EscapeSelectMode\0RightKey\0LeftKey\0"
    "UpdateVerticalLine\0x\0UpdateView\0min\0"
    "max\0setVerticalLineEnabled\0enabled\0"
    "setZoom\0x_min\0x_max\0y_min\0y_max\0"
    "UpdateZoom\0setSelectBox\0topleft\0"
    "bottomright"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChartViewPrivate[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  116,    2, 0x06,    1 /* Public */,
       3,    0,  117,    2, 0x06,    2 /* Public */,
       4,    0,  118,    2, 0x06,    3 /* Public */,
       5,    0,  119,    2, 0x06,    4 /* Public */,
       6,    0,  120,    2, 0x06,    5 /* Public */,
       7,    2,  121,    2, 0x06,    6 /* Public */,
      10,    2,  126,    2, 0x06,    9 /* Public */,
      11,    1,  131,    2, 0x06,   12 /* Public */,
      13,    0,  134,    2, 0x06,   14 /* Public */,
      14,    0,  135,    2, 0x06,   15 /* Public */,
      15,    0,  136,    2, 0x06,   16 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      16,    1,  137,    2, 0x0a,   17 /* Public */,
      18,    2,  140,    2, 0x0a,   19 /* Public */,
      21,    1,  145,    2, 0x0a,   22 /* Public */,
      23,    4,  148,    2, 0x0a,   24 /* Public */,
      28,    0,  157,    2, 0x0a,   29 /* Public */,
      29,    2,  158,    2, 0x0a,   30 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPointF,    8,    9,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPointF,    8,    9,
    QMetaType::Void, QMetaType::QPointF,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::Double,   17,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,   19,   20,
    QMetaType::Void, QMetaType::Bool,   22,
    QMetaType::Void, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal, QMetaType::QReal,   24,   25,   26,   27,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPointF,   30,   31,

       0        // eod
};

void ChartViewPrivate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartViewPrivate *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->LockZoom(); break;
        case 1: _t->UnLockZoom(); break;
        case 2: _t->ZoomChanged(); break;
        case 3: _t->scaleUp(); break;
        case 4: _t->scaleDown(); break;
        case 5: _t->AddRect((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        case 6: _t->ZoomRect((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        case 7: _t->PointDoubleClicked((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 8: _t->EscapeSelectMode(); break;
        case 9: _t->RightKey(); break;
        case 10: _t->LeftKey(); break;
        case 11: _t->UpdateVerticalLine((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 12: _t->UpdateView((*reinterpret_cast< double(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 13: _t->setVerticalLineEnabled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 14: _t->setZoom((*reinterpret_cast< qreal(*)>(_a[1])),(*reinterpret_cast< qreal(*)>(_a[2])),(*reinterpret_cast< qreal(*)>(_a[3])),(*reinterpret_cast< qreal(*)>(_a[4]))); break;
        case 15: _t->UpdateZoom(); break;
        case 16: _t->setSelectBox((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::LockZoom)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::UnLockZoom)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::ZoomChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::scaleUp)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::scaleDown)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)(const QPointF & , const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::AddRect)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)(const QPointF & , const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::ZoomRect)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)(const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::PointDoubleClicked)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::EscapeSelectMode)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::RightKey)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (ChartViewPrivate::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartViewPrivate::LeftKey)) {
                *result = 10;
                return;
            }
        }
    }
}

const QMetaObject ChartViewPrivate::staticMetaObject = { {
    QMetaObject::SuperData::link<QChartView::staticMetaObject>(),
    qt_meta_stringdata_ChartViewPrivate.offsetsAndSize,
    qt_meta_data_ChartViewPrivate,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ChartViewPrivate_t
, QtPrivate::TypeAndForceComplete<ChartViewPrivate, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<qreal, std::false_type>, QtPrivate::TypeAndForceComplete<qreal, std::false_type>, QtPrivate::TypeAndForceComplete<qreal, std::false_type>, QtPrivate::TypeAndForceComplete<qreal, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>


>,
    nullptr
} };


const QMetaObject *ChartViewPrivate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChartViewPrivate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChartViewPrivate.stringdata0))
        return static_cast<void*>(this);
    return QChartView::qt_metacast(_clname);
}

int ChartViewPrivate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QChartView::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void ChartViewPrivate::LockZoom()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChartViewPrivate::UnLockZoom()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChartViewPrivate::ZoomChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ChartViewPrivate::scaleUp()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ChartViewPrivate::scaleDown()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ChartViewPrivate::AddRect(const QPointF & _t1, const QPointF & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void ChartViewPrivate::ZoomRect(const QPointF & _t1, const QPointF & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ChartViewPrivate::PointDoubleClicked(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void ChartViewPrivate::EscapeSelectMode()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void ChartViewPrivate::RightKey()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void ChartViewPrivate::LeftKey()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
