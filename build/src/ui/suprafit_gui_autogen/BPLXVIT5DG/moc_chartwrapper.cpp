/****************************************************************************
** Meta object code from reading C++ file 'chartwrapper.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/guitools/chartwrapper.h"
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
#error "The header file 'chartwrapper.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChartWrapper_t {
    const uint offsetsAndSize[28];
    char stringdata0[154];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ChartWrapper_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ChartWrapper_t qt_meta_stringdata_ChartWrapper = {
    {
QT_MOC_LITERAL(0, 12), // "ChartWrapper"
QT_MOC_LITERAL(13, 12), // "ModelChanged"
QT_MOC_LITERAL(26, 0), // ""
QT_MOC_LITERAL(27, 14), // "stopAnimiation"
QT_MOC_LITERAL(42, 16), // "restartAnimation"
QT_MOC_LITERAL(59, 10), // "ShowSeries"
QT_MOC_LITERAL(70, 1), // "i"
QT_MOC_LITERAL(72, 16), // "ModelTransformed"
QT_MOC_LITERAL(89, 11), // "SeriesAdded"
QT_MOC_LITERAL(101, 11), // "UpdateModel"
QT_MOC_LITERAL(113, 10), // "MakeSeries"
QT_MOC_LITERAL(124, 10), // "showSeries"
QT_MOC_LITERAL(135, 10), // "SetBlocked"
QT_MOC_LITERAL(146, 7) // "blocked"

    },
    "ChartWrapper\0ModelChanged\0\0stopAnimiation\0"
    "restartAnimation\0ShowSeries\0i\0"
    "ModelTransformed\0SeriesAdded\0UpdateModel\0"
    "MakeSeries\0showSeries\0SetBlocked\0"
    "blocked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChartWrapper[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   74,    2, 0x06,    1 /* Public */,
       3,    0,   75,    2, 0x06,    2 /* Public */,
       4,    0,   76,    2, 0x06,    3 /* Public */,
       5,    1,   77,    2, 0x06,    4 /* Public */,
       7,    0,   80,    2, 0x06,    6 /* Public */,
       8,    1,   81,    2, 0x06,    7 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       9,    0,   84,    2, 0x0a,    9 /* Public */,
      10,    0,   85,    2, 0x0a,   10 /* Public */,
      11,    1,   86,    2, 0x0a,   11 /* Public */,
      12,    1,   89,    2, 0x0a,   13 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Int,   13,

       0        // eod
};

void ChartWrapper::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartWrapper *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ModelChanged(); break;
        case 1: _t->stopAnimiation(); break;
        case 2: _t->restartAnimation(); break;
        case 3: _t->ShowSeries((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->ModelTransformed(); break;
        case 5: _t->SeriesAdded((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->UpdateModel(); break;
        case 7: _t->MakeSeries(); break;
        case 8: _t->showSeries((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 9: _t->SetBlocked((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChartWrapper::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartWrapper::ModelChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChartWrapper::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartWrapper::stopAnimiation)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChartWrapper::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartWrapper::restartAnimation)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChartWrapper::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartWrapper::ShowSeries)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChartWrapper::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartWrapper::ModelTransformed)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ChartWrapper::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartWrapper::SeriesAdded)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject ChartWrapper::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ChartWrapper.offsetsAndSize,
    qt_meta_data_ChartWrapper,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ChartWrapper_t
, QtPrivate::TypeAndForceComplete<ChartWrapper, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>


>,
    nullptr
} };


const QMetaObject *ChartWrapper::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChartWrapper::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChartWrapper.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChartWrapper::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void ChartWrapper::ModelChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChartWrapper::stopAnimiation()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChartWrapper::restartAnimation()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ChartWrapper::ShowSeries(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChartWrapper::ModelTransformed()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ChartWrapper::SeriesAdded(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
