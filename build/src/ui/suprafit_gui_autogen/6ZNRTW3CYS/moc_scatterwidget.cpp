/****************************************************************************
** Meta object code from reading C++ file 'scatterwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/results/scatterwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'scatterwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ScatterWidget_t {
    const uint offsetsAndSize[28];
    char stringdata0[128];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ScatterWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ScatterWidget_t qt_meta_stringdata_ScatterWidget = {
    {
QT_MOC_LITERAL(0, 13), // "ScatterWidget"
QT_MOC_LITERAL(14, 7), // "Checked"
QT_MOC_LITERAL(22, 0), // ""
QT_MOC_LITERAL(23, 5), // "var_1"
QT_MOC_LITERAL(29, 5), // "var_2"
QT_MOC_LITERAL(35, 7), // "HideBox"
QT_MOC_LITERAL(43, 9), // "parameter"
QT_MOC_LITERAL(53, 17), // "CheckParameterBox"
QT_MOC_LITERAL(71, 12), // "ModelClicked"
QT_MOC_LITERAL(84, 5), // "model"
QT_MOC_LITERAL(90, 12), // "setConverged"
QT_MOC_LITERAL(103, 9), // "converged"
QT_MOC_LITERAL(113, 8), // "setValid"
QT_MOC_LITERAL(122, 5) // "valid"

    },
    "ScatterWidget\0Checked\0\0var_1\0var_2\0"
    "HideBox\0parameter\0CheckParameterBox\0"
    "ModelClicked\0model\0setConverged\0"
    "converged\0setValid\0valid"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ScatterWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   50,    2, 0x06,    1 /* Public */,
       5,    1,   55,    2, 0x06,    4 /* Public */,
       7,    1,   58,    2, 0x06,    6 /* Public */,
       8,    1,   61,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      10,    1,   64,    2, 0x0a,   10 /* Public */,
      12,    1,   67,    2, 0x0a,   12 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    3,    4,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Int,    9,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,   11,
    QMetaType::Void, QMetaType::Bool,   13,

       0        // eod
};

void ScatterWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ScatterWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Checked((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->HideBox((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->CheckParameterBox((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->ModelClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->setConverged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 5: _t->setValid((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ScatterWidget::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScatterWidget::Checked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ScatterWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScatterWidget::HideBox)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ScatterWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScatterWidget::CheckParameterBox)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ScatterWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ScatterWidget::ModelClicked)) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject ScatterWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ScatterWidget.offsetsAndSize,
    qt_meta_data_ScatterWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ScatterWidget_t
, QtPrivate::TypeAndForceComplete<ScatterWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>


>,
    nullptr
} };


const QMetaObject *ScatterWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ScatterWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ScatterWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ScatterWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ScatterWidget::Checked(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ScatterWidget::HideBox(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ScatterWidget::CheckParameterBox(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ScatterWidget::ModelClicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
