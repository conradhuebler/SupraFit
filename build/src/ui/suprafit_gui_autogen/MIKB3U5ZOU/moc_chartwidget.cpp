/****************************************************************************
** Meta object code from reading C++ file 'chartwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/mainwindow/chartwidget.h"
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
#error "The header file 'chartwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ChartDockTitleBar_t {
    const uint offsetsAndSize[30];
    char stringdata0[146];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ChartDockTitleBar_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ChartDockTitleBar_t qt_meta_stringdata_ChartDockTitleBar = {
    {
QT_MOC_LITERAL(0, 17), // "ChartDockTitleBar"
QT_MOC_LITERAL(18, 5), // "close"
QT_MOC_LITERAL(24, 0), // ""
QT_MOC_LITERAL(25, 12), // "ThemeChanged"
QT_MOC_LITERAL(38, 18), // "QChart::ChartTheme"
QT_MOC_LITERAL(57, 5), // "theme"
QT_MOC_LITERAL(63, 16), // "AnimationChanged"
QT_MOC_LITERAL(80, 9), // "animation"
QT_MOC_LITERAL(90, 9), // "ChartFlip"
QT_MOC_LITERAL(100, 4), // "flip"
QT_MOC_LITERAL(105, 7), // "setSize"
QT_MOC_LITERAL(113, 4), // "size"
QT_MOC_LITERAL(118, 11), // "ThemeChange"
QT_MOC_LITERAL(130, 8), // "QAction*"
QT_MOC_LITERAL(139, 6) // "action"

    },
    "ChartDockTitleBar\0close\0\0ThemeChanged\0"
    "QChart::ChartTheme\0theme\0AnimationChanged\0"
    "animation\0ChartFlip\0flip\0setSize\0size\0"
    "ThemeChange\0QAction*\0action"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChartDockTitleBar[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x06,    1 /* Public */,
       3,    1,   51,    2, 0x06,    2 /* Public */,
       6,    1,   54,    2, 0x06,    4 /* Public */,
       8,    1,   57,    2, 0x06,    6 /* Public */,
      10,    1,   60,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    1,   63,    2, 0x08,   10 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 4,    5,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::Int,   11,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 13,   14,

       0        // eod
};

void ChartDockTitleBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartDockTitleBar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->close(); break;
        case 1: _t->ThemeChanged((*reinterpret_cast< QChart::ChartTheme(*)>(_a[1]))); break;
        case 2: _t->AnimationChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->ChartFlip((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->setSize((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->ThemeChange((*reinterpret_cast< QAction*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAction* >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChartDockTitleBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartDockTitleBar::close)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChartDockTitleBar::*)(QChart::ChartTheme );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartDockTitleBar::ThemeChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChartDockTitleBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartDockTitleBar::AnimationChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChartDockTitleBar::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartDockTitleBar::ChartFlip)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChartDockTitleBar::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ChartDockTitleBar::setSize)) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject ChartDockTitleBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ChartDockTitleBar.offsetsAndSize,
    qt_meta_data_ChartDockTitleBar,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ChartDockTitleBar_t
, QtPrivate::TypeAndForceComplete<ChartDockTitleBar, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QChart::ChartTheme, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QAction *, std::false_type>


>,
    nullptr
} };


const QMetaObject *ChartDockTitleBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChartDockTitleBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChartDockTitleBar.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ChartDockTitleBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ChartDockTitleBar::close()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChartDockTitleBar::ThemeChanged(QChart::ChartTheme _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ChartDockTitleBar::AnimationChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ChartDockTitleBar::ChartFlip(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChartDockTitleBar::setSize(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
struct qt_meta_stringdata_ChartWidget_t {
    const uint offsetsAndSize[24];
    char stringdata0[133];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ChartWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ChartWidget_t qt_meta_stringdata_ChartWidget = {
    {
QT_MOC_LITERAL(0, 11), // "ChartWidget"
QT_MOC_LITERAL(12, 10), // "formatAxis"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 7), // "Repaint"
QT_MOC_LITERAL(32, 8), // "updateUI"
QT_MOC_LITERAL(41, 11), // "updateTheme"
QT_MOC_LITERAL(53, 18), // "QChart::ChartTheme"
QT_MOC_LITERAL(72, 5), // "theme"
QT_MOC_LITERAL(78, 12), // "setAnimation"
QT_MOC_LITERAL(91, 9), // "animation"
QT_MOC_LITERAL(101, 14), // "stopAnimiation"
QT_MOC_LITERAL(116, 16) // "restartAnimation"

    },
    "ChartWidget\0formatAxis\0\0Repaint\0"
    "updateUI\0updateTheme\0QChart::ChartTheme\0"
    "theme\0setAnimation\0animation\0"
    "stopAnimiation\0restartAnimation"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ChartWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x08,    1 /* Private */,
       3,    0,   57,    2, 0x08,    2 /* Private */,
       4,    0,   58,    2, 0x08,    3 /* Private */,
       5,    1,   59,    2, 0x08,    4 /* Private */,
       8,    1,   62,    2, 0x08,    6 /* Private */,
      10,    0,   65,    2, 0x08,    8 /* Private */,
      11,    0,   66,    2, 0x08,    9 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ChartWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChartWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->formatAxis(); break;
        case 1: _t->Repaint(); break;
        case 2: _t->updateUI(); break;
        case 3: _t->updateTheme((*reinterpret_cast< QChart::ChartTheme(*)>(_a[1]))); break;
        case 4: _t->setAnimation((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->stopAnimiation(); break;
        case 6: _t->restartAnimation(); break;
        default: ;
        }
    }
}

const QMetaObject ChartWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ChartWidget.offsetsAndSize,
    qt_meta_data_ChartWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ChartWidget_t
, QtPrivate::TypeAndForceComplete<ChartWidget, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QChart::ChartTheme, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ChartWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChartWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ChartWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ChartWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
