/****************************************************************************
** Meta object code from reading C++ file 'datawidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/mainwindow/datawidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'datawidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DataWidget_t {
    const uint offsetsAndSize[22];
    char stringdata0[116];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_DataWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_DataWidget_t qt_meta_stringdata_DataWidget = {
    {
QT_MOC_LITERAL(0, 10), // "DataWidget"
QT_MOC_LITERAL(11, 11), // "recalculate"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 11), // "NameChanged"
QT_MOC_LITERAL(36, 15), // "ShowContextMenu"
QT_MOC_LITERAL(52, 3), // "pos"
QT_MOC_LITERAL(56, 8), // "switchHG"
QT_MOC_LITERAL(65, 14), // "SetProjectName"
QT_MOC_LITERAL(80, 10), // "setScaling"
QT_MOC_LITERAL(91, 9), // "HidePoint"
QT_MOC_LITERAL(101, 14) // "LinearAnalysis"

    },
    "DataWidget\0recalculate\0\0NameChanged\0"
    "ShowContextMenu\0pos\0switchHG\0"
    "SetProjectName\0setScaling\0HidePoint\0"
    "LinearAnalysis"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DataWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    1 /* Public */,
       3,    0,   63,    2, 0x06,    2 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   64,    2, 0x08,    3 /* Private */,
       6,    0,   67,    2, 0x08,    5 /* Private */,
       7,    0,   68,    2, 0x08,    6 /* Private */,
       8,    0,   69,    2, 0x08,    7 /* Private */,
       9,    0,   70,    2, 0x08,    8 /* Private */,
      10,    0,   71,    2, 0x08,    9 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QPoint,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void DataWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DataWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->recalculate(); break;
        case 1: _t->NameChanged(); break;
        case 2: _t->ShowContextMenu((*reinterpret_cast< const QPoint(*)>(_a[1]))); break;
        case 3: _t->switchHG(); break;
        case 4: _t->SetProjectName(); break;
        case 5: _t->setScaling(); break;
        case 6: _t->HidePoint(); break;
        case 7: _t->LinearAnalysis(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DataWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataWidget::recalculate)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DataWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DataWidget::NameChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject DataWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_DataWidget.offsetsAndSize,
    qt_meta_data_DataWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_DataWidget_t
, QtPrivate::TypeAndForceComplete<DataWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QPoint &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *DataWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DataWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DataWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int DataWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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

// SIGNAL 0
void DataWidget::recalculate()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DataWidget::NameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
