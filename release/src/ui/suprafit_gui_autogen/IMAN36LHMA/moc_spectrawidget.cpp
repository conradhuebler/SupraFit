/****************************************************************************
** Meta object code from reading C++ file 'spectrawidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/spectrawidget.h"
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
#error "The header file 'spectrawidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SpectraWidget_t {
    const uint offsetsAndSize[20];
    char stringdata0[103];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SpectraWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SpectraWidget_t qt_meta_stringdata_SpectraWidget = {
    {
QT_MOC_LITERAL(0, 13), // "SpectraWidget"
QT_MOC_LITERAL(14, 13), // "UpdateSpectra"
QT_MOC_LITERAL(28, 0), // ""
QT_MOC_LITERAL(29, 10), // "UpdateData"
QT_MOC_LITERAL(40, 18), // "PointDoubleClicked"
QT_MOC_LITERAL(59, 5), // "point"
QT_MOC_LITERAL(65, 12), // "UpdateXRange"
QT_MOC_LITERAL(78, 6), // "point1"
QT_MOC_LITERAL(85, 6), // "point2"
QT_MOC_LITERAL(92, 10) // "SaveToFile"

    },
    "SpectraWidget\0UpdateSpectra\0\0UpdateData\0"
    "PointDoubleClicked\0point\0UpdateXRange\0"
    "point1\0point2\0SaveToFile"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SpectraWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x0a,    1 /* Public */,
       3,    0,   45,    2, 0x0a,    2 /* Public */,
       4,    1,   46,    2, 0x08,    3 /* Private */,
       6,    2,   49,    2, 0x08,    5 /* Private */,
       9,    0,   54,    2, 0x08,    8 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QPointF,    5,
    QMetaType::Void, QMetaType::QPointF, QMetaType::QPointF,    7,    8,
    QMetaType::Void,

       0        // eod
};

void SpectraWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SpectraWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->UpdateSpectra(); break;
        case 1: _t->UpdateData(); break;
        case 2: _t->PointDoubleClicked((*reinterpret_cast< const QPointF(*)>(_a[1]))); break;
        case 3: _t->UpdateXRange((*reinterpret_cast< const QPointF(*)>(_a[1])),(*reinterpret_cast< const QPointF(*)>(_a[2]))); break;
        case 4: _t->SaveToFile(); break;
        default: ;
        }
    }
}

const QMetaObject SpectraWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SpectraWidget.offsetsAndSize,
    qt_meta_data_SpectraWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SpectraWidget_t
, QtPrivate::TypeAndForceComplete<SpectraWidget, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<const QPointF &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *SpectraWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SpectraWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SpectraWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SpectraWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
