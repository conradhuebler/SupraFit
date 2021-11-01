/****************************************************************************
** Meta object code from reading C++ file 'metamodelwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/mainwindow/metamodelwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'metamodelwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MetaModelWidget_t {
    const uint offsetsAndSize[48];
    char stringdata0[265];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_MetaModelWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_MetaModelWidget_t qt_meta_stringdata_MetaModelWidget = {
    {
QT_MOC_LITERAL(0, 15), // "MetaModelWidget"
QT_MOC_LITERAL(16, 9), // "Interrupt"
QT_MOC_LITERAL(26, 0), // ""
QT_MOC_LITERAL(27, 8), // "Finished"
QT_MOC_LITERAL(36, 17), // "IncrementProgress"
QT_MOC_LITERAL(54, 7), // "progess"
QT_MOC_LITERAL(62, 12), // "MaximumSteps"
QT_MOC_LITERAL(75, 5), // "steps"
QT_MOC_LITERAL(81, 7), // "Message"
QT_MOC_LITERAL(89, 3), // "str"
QT_MOC_LITERAL(93, 8), // "priority"
QT_MOC_LITERAL(102, 7), // "Warning"
QT_MOC_LITERAL(110, 8), // "LoadJson"
QT_MOC_LITERAL(119, 6), // "object"
QT_MOC_LITERAL(126, 9), // "Calculate"
QT_MOC_LITERAL(136, 8), // "Minimize"
QT_MOC_LITERAL(145, 18), // "OpenAdvancedSearch"
QT_MOC_LITERAL(164, 8), // "NewGuess"
QT_MOC_LITERAL(173, 15), // "ImportConstants"
QT_MOC_LITERAL(189, 15), // "ExportConstants"
QT_MOC_LITERAL(205, 21), // "ToggleStatisticDialog"
QT_MOC_LITERAL(227, 10), // "TogglePlot"
QT_MOC_LITERAL(238, 8), // "Detailed"
QT_MOC_LITERAL(247, 17) // "OptimizerSettings"

    },
    "MetaModelWidget\0Interrupt\0\0Finished\0"
    "IncrementProgress\0progess\0MaximumSteps\0"
    "steps\0Message\0str\0priority\0Warning\0"
    "LoadJson\0object\0Calculate\0Minimize\0"
    "OpenAdvancedSearch\0NewGuess\0ImportConstants\0"
    "ExportConstants\0ToggleStatisticDialog\0"
    "TogglePlot\0Detailed\0OptimizerSettings"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MetaModelWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  116,    2, 0x06,    1 /* Public */,
       3,    0,  117,    2, 0x06,    2 /* Public */,
       4,    1,  118,    2, 0x06,    3 /* Public */,
       6,    1,  121,    2, 0x06,    5 /* Public */,
       8,    2,  124,    2, 0x06,    7 /* Public */,
      11,    2,  129,    2, 0x06,   10 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    1,  134,    2, 0x0a,   13 /* Public */,
      14,    0,  137,    2, 0x08,   15 /* Private */,
      15,    0,  138,    2, 0x08,   16 /* Private */,
      16,    0,  139,    2, 0x08,   17 /* Private */,
      17,    0,  140,    2, 0x08,   18 /* Private */,
      18,    0,  141,    2, 0x08,   19 /* Private */,
      19,    0,  142,    2, 0x08,   20 /* Private */,
      20,    0,  143,    2, 0x08,   21 /* Private */,
      21,    0,  144,    2, 0x08,   22 /* Private */,
      22,    0,  145,    2, 0x08,   23 /* Private */,
      23,    0,  146,    2, 0x08,   24 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    9,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    9,   10,

 // slots: parameters
    QMetaType::Void, QMetaType::QJsonObject,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MetaModelWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MetaModelWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Interrupt(); break;
        case 1: _t->Finished(); break;
        case 2: _t->IncrementProgress((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->MaximumSteps((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->Message((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->Warning((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->LoadJson((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 7: _t->Calculate(); break;
        case 8: _t->Minimize(); break;
        case 9: _t->OpenAdvancedSearch(); break;
        case 10: _t->NewGuess(); break;
        case 11: _t->ImportConstants(); break;
        case 12: _t->ExportConstants(); break;
        case 13: _t->ToggleStatisticDialog(); break;
        case 14: _t->TogglePlot(); break;
        case 15: _t->Detailed(); break;
        case 16: _t->OptimizerSettings(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MetaModelWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModelWidget::Interrupt)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MetaModelWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModelWidget::Finished)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MetaModelWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModelWidget::IncrementProgress)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MetaModelWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModelWidget::MaximumSteps)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MetaModelWidget::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModelWidget::Message)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MetaModelWidget::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MetaModelWidget::Warning)) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject MetaModelWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_MetaModelWidget.offsetsAndSize,
    qt_meta_data_MetaModelWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_MetaModelWidget_t
, QtPrivate::TypeAndForceComplete<MetaModelWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *MetaModelWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MetaModelWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MetaModelWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MetaModelWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void MetaModelWidget::Interrupt()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MetaModelWidget::Finished()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MetaModelWidget::IncrementProgress(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MetaModelWidget::MaximumSteps(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void MetaModelWidget::Message(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void MetaModelWidget::Warning(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
