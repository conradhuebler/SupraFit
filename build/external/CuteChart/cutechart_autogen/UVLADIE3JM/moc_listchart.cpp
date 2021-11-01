/****************************************************************************
** Meta object code from reading C++ file 'listchart.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../external/CuteChart/src/listchart.h"
#include <QtCharts/qlineseries.h>
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'listchart.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ListChart_t {
    const uint offsetsAndSize[38];
    char stringdata0[214];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ListChart_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ListChart_t qt_meta_stringdata_ListChart = {
    {
QT_MOC_LITERAL(0, 9), // "ListChart"
QT_MOC_LITERAL(10, 17), // "itemDoubleClicked"
QT_MOC_LITERAL(28, 0), // ""
QT_MOC_LITERAL(29, 16), // "QListWidgetItem*"
QT_MOC_LITERAL(46, 4), // "item"
QT_MOC_LITERAL(51, 14), // "LastDirChanged"
QT_MOC_LITERAL(66, 3), // "dir"
QT_MOC_LITERAL(70, 10), // "formatAxis"
QT_MOC_LITERAL(81, 8), // "setColor"
QT_MOC_LITERAL(90, 5), // "index"
QT_MOC_LITERAL(96, 5), // "color"
QT_MOC_LITERAL(102, 10), // "HideSeries"
QT_MOC_LITERAL(113, 24), // "ApplyConfigurationChange"
QT_MOC_LITERAL(138, 17), // "SeriesListClicked"
QT_MOC_LITERAL(156, 16), // "NamesListClicked"
QT_MOC_LITERAL(173, 11), // "ContextMenu"
QT_MOC_LITERAL(185, 3), // "pos"
QT_MOC_LITERAL(189, 12), // "RenameSeries"
QT_MOC_LITERAL(202, 11) // "ChangeColor"

    },
    "ListChart\0itemDoubleClicked\0\0"
    "QListWidgetItem*\0item\0LastDirChanged\0"
    "dir\0formatAxis\0setColor\0index\0color\0"
    "HideSeries\0ApplyConfigurationChange\0"
    "SeriesListClicked\0NamesListClicked\0"
    "ContextMenu\0pos\0RenameSeries\0ChangeColor"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ListChart[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   80,    2, 0x06,    1 /* Public */,
       5,    1,   83,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    0,   86,    2, 0x0a,    5 /* Public */,
       8,    2,   87,    2, 0x0a,    6 /* Public */,
      11,    1,   92,    2, 0x0a,    9 /* Public */,
      12,    0,   95,    2, 0x0a,   11 /* Public */,
      13,    1,   96,    2, 0x08,   12 /* Private */,
      14,    1,   99,    2, 0x08,   14 /* Private */,
      15,    1,  102,    2, 0x08,   16 /* Private */,
      17,    0,  105,    2, 0x08,   18 /* Private */,
      18,    0,  106,    2, 0x08,   19 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QColor,    9,   10,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QPoint,   16,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void ListChart::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ListChart *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->itemDoubleClicked((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        case 1: _t->LastDirChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->formatAxis(); break;
        case 3: _t->setColor((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QColor(*)>(_a[2]))); break;
        case 4: _t->HideSeries((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->ApplyConfigurationChange(); break;
        case 6: _t->SeriesListClicked((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        case 7: _t->NamesListClicked((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        case 8: _t->ContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 9: _t->RenameSeries(); break;
        case 10: _t->ChangeColor(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ListChart::*)(QListWidgetItem * );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ListChart::itemDoubleClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ListChart::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ListChart::LastDirChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject ListChart::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ListChart.offsetsAndSize,
    qt_meta_data_ListChart,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ListChart_t
, QtPrivate::TypeAndForceComplete<ListChart, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QListWidgetItem *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ListChart::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ListChart::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ListChart.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ListChart::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}

// SIGNAL 0
void ListChart::itemDoubleClicked(QListWidgetItem * _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ListChart::LastDirChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
