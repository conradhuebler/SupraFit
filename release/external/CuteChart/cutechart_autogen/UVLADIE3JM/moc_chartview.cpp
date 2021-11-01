/****************************************************************************
** Meta object code from reading C++ file 'chartview.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../external/CuteChart/src/chartview.h"
#include <QtCharts/qlineseries.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'chartview.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChartView_t {
    const uint offsetsAndSize[76];
    char stringdata0[430];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ChartView_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ChartView_t qt_meta_stringdata_ChartView = {
    {
QT_MOC_LITERAL(0, 9), // "ChartView"
QT_MOC_LITERAL(10, 11), // "AxisChanged"
QT_MOC_LITERAL(22, 0), // ""
QT_MOC_LITERAL(23, 12), // "ChartCleared"
QT_MOC_LITERAL(36, 20), // "ConfigurationChanged"
QT_MOC_LITERAL(57, 14), // "LastDirChanged"
QT_MOC_LITERAL(72, 3), // "dir"
QT_MOC_LITERAL(76, 18), // "PointDoubleClicked"
QT_MOC_LITERAL(95, 5), // "point"
QT_MOC_LITERAL(101, 11), // "ZoomChanged"
QT_MOC_LITERAL(113, 7), // "scaleUp"
QT_MOC_LITERAL(121, 9), // "scaleDown"
QT_MOC_LITERAL(131, 7), // "AddRect"
QT_MOC_LITERAL(139, 6), // "point1"
QT_MOC_LITERAL(146, 6), // "point2"
QT_MOC_LITERAL(153, 16), // "EscapeSelectMode"
QT_MOC_LITERAL(170, 8), // "RightKey"
QT_MOC_LITERAL(179, 7), // "LeftKey"
QT_MOC_LITERAL(187, 12), // "setSelectBox"
QT_MOC_LITERAL(200, 7), // "topleft"
QT_MOC_LITERAL(208, 11), // "bottomright"
QT_MOC_LITERAL(220, 10), // "formatAxis"
QT_MOC_LITERAL(231, 18), // "QtNiceNumbersScale"
QT_MOC_LITERAL(250, 10), // "SpaceScale"
QT_MOC_LITERAL(261, 8), // "setXAxis"
QT_MOC_LITERAL(270, 3), // "str"
QT_MOC_LITERAL(274, 8), // "setYAxis"
QT_MOC_LITERAL(283, 8), // "setTitle"
QT_MOC_LITERAL(292, 24), // "ApplyConfigurationChange"
QT_MOC_LITERAL(317, 8), // "ZoomRect"
QT_MOC_LITERAL(326, 12), // "PlotSettings"
QT_MOC_LITERAL(339, 9), // "ExportPNG"
QT_MOC_LITERAL(349, 14), // "setChartConfig"
QT_MOC_LITERAL(364, 11), // "ChartConfig"
QT_MOC_LITERAL(376, 11), // "chartconfig"
QT_MOC_LITERAL(388, 15), // "forceformatAxis"
QT_MOC_LITERAL(404, 15), // "ResetFontConfig"
QT_MOC_LITERAL(420, 9) // "Configure"

    },
    "ChartView\0AxisChanged\0\0ChartCleared\0"
    "ConfigurationChanged\0LastDirChanged\0"
    "dir\0PointDoubleClicked\0point\0ZoomChanged\0"
    "scaleUp\0scaleDown\0AddRect\0point1\0"
    "point2\0EscapeSelectMode\0RightKey\0"
    "LeftKey\0setSelectBox\0topleft\0bottomright\0"
    "formatAxis\0QtNiceNumbersScale\0SpaceScale\0"
    "setXAxis\0str\0setYAxis\0setTitle\0"
    "ApplyConfigurationChange\0ZoomRect\0"
    "PlotSettings\0ExportPNG\0setChartConfig\0"
    "ChartConfig\0chartconfig\0forceformatAxis\0"
    "ResetFontConfig\0Configure"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChartView[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      28,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      12,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  182,    2, 0x06,    1 /* Public */,
       3,    0,  183,    2, 0x06,    2 /* Public */,
       4,    0,  184,    2, 0x06,    3 /* Public */,
       5,    1,  185,    2, 0x06,    4 /* Public */,
       7,    1,  188,    2, 0x06,    6 /* Public */,
       9,    0,  191,    2, 0x06,    8 /* Public */,
      10,    0,  192,    2, 0x06,    9 /* Public */,
      11,    0,  193,    2, 0x06,   10 /* Public */,
      12,    2,  194,    2, 0x06,   11 /* Public */,
      15,    0,  199,    2, 0x06,   14 /* Public */,
      16,    0,  200,    2, 0x06,   15 /* Public */,
      17,    0,  201,    2, 0x06,   16 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      18,    2,  202,    2, 0x0a,   17 /* Public */,
      21,    0,  207,    2, 0x0a,   20 /* Public */,
      22,    0,  208,    2, 0x0a,   21 /* Public */,
      23,    0,  209,    2, 0x0a,   22 /* Public */,
      24,    1,  210,    2, 0x0a,   23 /* Public */,
      26,    1,  213,    2, 0x0a,   25 /* Public */,
      27,    1,  216,    2, 0x0a,   27 /* Public */,
      28,    1,  219,    2, 0x0a,   29 /* Public */,
      28,    0,  222,    2, 0x2a,   31 /* Public | MethodCloned */,
      29,    2,  223,    2, 0x0a,   32 /* Public */,
      30,    0,  228,    2, 0x08,   35 /* Private */,
      31,    0,  229,    2, 0x08,   36 /* Private */,
      32,    1,  230,    2, 0x08,   37 /* Private */,
      35,    0,  233,    2, 0x08,   39 /* Private */,
      36,    0,  234,    2, 0x08,   40 /* Private */,
      37,    0,  235,    2, 0x08,   41 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QPointF,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPointF,   13,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPointF,   19,   20,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void, QMetaType::QString,   25,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPointF,   13,   14,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 33,   34,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ChartView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartView *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->AxisChanged(); break;
        case 1: _t->ChartCleared(); break;
        case 2: _t->ConfigurationChanged(); break;
        case 3: _t->LastDirChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->PointDoubleClicked((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 5: _t->ZoomChanged(); break;
        case 6: _t->scaleUp(); break;
        case 7: _t->scaleDown(); break;
        case 8: _t->AddRect((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        case 9: _t->EscapeSelectMode(); break;
        case 10: _t->RightKey(); break;
        case 11: _t->LeftKey(); break;
        case 12: _t->setSelectBox((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        case 13: _t->formatAxis(); break;
        case 14: _t->QtNiceNumbersScale(); break;
        case 15: _t->SpaceScale(); break;
        case 16: _t->setXAxis((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 17: _t->setYAxis((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 18: _t->setTitle((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 19: _t->ApplyConfigurationChange((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->ApplyConfigurationChange(); break;
        case 21: _t->ZoomRect((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        case 22: _t->PlotSettings(); break;
        case 23: _t->ExportPNG(); break;
        case 24: _t->setChartConfig((*reinterpret_cast< const ChartConfig(*)>(_a[1]))); break;
        case 25: _t->forceformatAxis(); break;
        case 26: _t->ResetFontConfig(); break;
        case 27: _t->Configure(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::AxisChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::ChartCleared)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::ConfigurationChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChartView::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::LastDirChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChartView::*)(const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::PointDoubleClicked)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::ZoomChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::scaleUp)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::scaleDown)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (ChartView::*)(const QPointF & , const QPointF & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::AddRect)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::EscapeSelectMode)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::RightKey)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (ChartView::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartView::LeftKey)) {
                *result = 11;
                return;
            }
        }
    }
}

const QMetaObject ChartView::staticMetaObject = { {
    QMetaObject::SuperData::link<QScrollArea::staticMetaObject>(),
    qt_meta_stringdata_ChartView.offsetsAndSize,
    qt_meta_data_ChartView,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ChartView_t
, QtPrivate::TypeAndForceComplete<ChartView, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const ChartConfig &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ChartView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChartView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChartView.stringdata0))
        return static_cast<void*>(this);
    return QScrollArea::qt_metacast(_clname);
}

int ChartView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QScrollArea::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 28;
    }
    return _id;
}

// SIGNAL 0
void ChartView::AxisChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChartView::ChartCleared()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChartView::ConfigurationChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ChartView::LastDirChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChartView::PointDoubleClicked(const QPointF & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ChartView::ZoomChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ChartView::scaleUp()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ChartView::scaleDown()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void ChartView::AddRect(const QPointF & _t1, const QPointF & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void ChartView::EscapeSelectMode()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void ChartView::RightKey()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void ChartView::LeftKey()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
