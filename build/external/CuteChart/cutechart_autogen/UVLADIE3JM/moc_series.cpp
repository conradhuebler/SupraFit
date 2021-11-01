/****************************************************************************
** Meta object code from reading C++ file 'series.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../external/CuteChart/src/series.h"
#include <QtGui/qtextcursor.h>
#include <QScreen>
#include <QtCharts/qlineseries.h>
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
#error "The header file 'series.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LineSeries_t {
    const uint offsetsAndSize[28];
    char stringdata0[107];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_LineSeries_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_LineSeries_t qt_meta_stringdata_LineSeries = {
    {
QT_MOC_LITERAL(0, 10), // "LineSeries"
QT_MOC_LITERAL(11, 8), // "setColor"
QT_MOC_LITERAL(20, 0), // ""
QT_MOC_LITERAL(21, 5), // "color"
QT_MOC_LITERAL(27, 14), // "setDashDotLine"
QT_MOC_LITERAL(42, 7), // "dashdot"
QT_MOC_LITERAL(50, 7), // "setSize"
QT_MOC_LITERAL(58, 4), // "size"
QT_MOC_LITERAL(63, 9), // "LineWidth"
QT_MOC_LITERAL(73, 8), // "ShowLine"
QT_MOC_LITERAL(82, 5), // "state"
QT_MOC_LITERAL(88, 7), // "setName"
QT_MOC_LITERAL(96, 4), // "name"
QT_MOC_LITERAL(101, 5) // "Color"

    },
    "LineSeries\0setColor\0\0color\0setDashDotLine\0"
    "dashdot\0setSize\0size\0LineWidth\0ShowLine\0"
    "state\0setName\0name\0Color"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LineSeries[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x0a,    1 /* Public */,
       4,    1,   65,    2, 0x0a,    3 /* Public */,
       6,    1,   68,    2, 0x0a,    5 /* Public */,
       8,    0,   71,    2, 0x10a,    7 /* Public | MethodIsConst  */,
       9,    1,   72,    2, 0x0a,    8 /* Public */,
       9,    1,   75,    2, 0x0a,   10 /* Public */,
      11,    1,   78,    2, 0x0a,   12 /* Public */,
      13,    0,   81,    2, 0x10a,   14 /* Public | MethodIsConst  */,

 // slots: parameters
    QMetaType::Void, QMetaType::QColor,    3,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::QReal,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::Bool,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::QColor,

       0        // eod
};

void LineSeries::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<LineSeries *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->setColor((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        case 1: _t->setDashDotLine((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 2: _t->setSize((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: { qreal _r = _t->LineWidth();
            if (_a[0]) *reinterpret_cast< qreal*>(_a[0]) = std::move(_r); }  break;
        case 4: _t->ShowLine((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->ShowLine((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 6: _t->setName((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: { QColor _r = _t->Color();
            if (_a[0]) *reinterpret_cast< QColor*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

const QMetaObject LineSeries::staticMetaObject = { {
    QMetaObject::SuperData::link<QLineSeries::staticMetaObject>(),
    qt_meta_stringdata_LineSeries.offsetsAndSize,
    qt_meta_data_LineSeries,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_LineSeries_t
, QtPrivate::TypeAndForceComplete<LineSeries, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<qreal, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<QColor, std::false_type>


>,
    nullptr
} };


const QMetaObject *LineSeries::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LineSeries::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LineSeries.stringdata0))
        return static_cast<void*>(this);
    return QLineSeries::qt_metacast(_clname);
}

int LineSeries::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineSeries::qt_metacall(_c, _id, _a);
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
struct qt_meta_stringdata_ScatterSeries_t {
    const uint offsetsAndSize[18];
    char stringdata0[76];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ScatterSeries_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ScatterSeries_t qt_meta_stringdata_ScatterSeries = {
    {
QT_MOC_LITERAL(0, 13), // "ScatterSeries"
QT_MOC_LITERAL(14, 11), // "NameChanged"
QT_MOC_LITERAL(26, 0), // ""
QT_MOC_LITERAL(27, 3), // "str"
QT_MOC_LITERAL(31, 14), // "visibleChanged"
QT_MOC_LITERAL(46, 5), // "state"
QT_MOC_LITERAL(52, 8), // "setColor"
QT_MOC_LITERAL(61, 5), // "color"
QT_MOC_LITERAL(67, 8) // "ShowLine"

    },
    "ScatterSeries\0NameChanged\0\0str\0"
    "visibleChanged\0state\0setColor\0color\0"
    "ShowLine"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScatterSeries[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   38,    2, 0x06,    1 /* Public */,
       4,    1,   41,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    1,   44,    2, 0x0a,    5 /* Public */,
       8,    1,   47,    2, 0x0a,    7 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Int,    5,

 // slots: parameters
    QMetaType::Void, QMetaType::QColor,    7,
    QMetaType::Void, QMetaType::Int,    5,

       0        // eod
};

void ScatterSeries::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScatterSeries *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->NameChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->visibleChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->setColor((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        case 3: _t->ShowLine((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScatterSeries::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScatterSeries::NameChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScatterSeries::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScatterSeries::visibleChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject ScatterSeries::staticMetaObject = { {
    QMetaObject::SuperData::link<QScatterSeries::staticMetaObject>(),
    qt_meta_stringdata_ScatterSeries.offsetsAndSize,
    qt_meta_data_ScatterSeries,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ScatterSeries_t
, QtPrivate::TypeAndForceComplete<ScatterSeries, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>


>,
    nullptr
} };


const QMetaObject *ScatterSeries::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScatterSeries::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScatterSeries.stringdata0))
        return static_cast<void*>(this);
    return QScatterSeries::qt_metacast(_clname);
}

int ScatterSeries::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QScatterSeries::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void ScatterSeries::NameChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScatterSeries::visibleChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
struct qt_meta_stringdata_BoxPlotSeries_t {
    const uint offsetsAndSize[12];
    char stringdata0[49];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_BoxPlotSeries_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_BoxPlotSeries_t qt_meta_stringdata_BoxPlotSeries = {
    {
QT_MOC_LITERAL(0, 13), // "BoxPlotSeries"
QT_MOC_LITERAL(14, 8), // "setColor"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 5), // "color"
QT_MOC_LITERAL(30, 10), // "setVisible"
QT_MOC_LITERAL(41, 7) // "visible"

    },
    "BoxPlotSeries\0setColor\0\0color\0setVisible\0"
    "visible"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BoxPlotSeries[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   26,    2, 0x0a,    1 /* Public */,
       4,    1,   29,    2, 0x0a,    3 /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::QColor,    3,
    QMetaType::Void, QMetaType::Bool,    5,

       0        // eod
};

void BoxPlotSeries::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BoxPlotSeries *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->setColor((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        case 1: _t->setVisible((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject BoxPlotSeries::staticMetaObject = { {
    QMetaObject::SuperData::link<QBoxPlotSeries::staticMetaObject>(),
    qt_meta_stringdata_BoxPlotSeries.offsetsAndSize,
    qt_meta_data_BoxPlotSeries,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_BoxPlotSeries_t
, QtPrivate::TypeAndForceComplete<BoxPlotSeries, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>


>,
    nullptr
} };


const QMetaObject *BoxPlotSeries::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BoxPlotSeries::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BoxPlotSeries.stringdata0))
        return static_cast<void*>(this);
    return QBoxPlotSeries::qt_metacast(_clname);
}

int BoxPlotSeries::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QBoxPlotSeries::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
