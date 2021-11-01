/****************************************************************************
** Meta object code from reading C++ file 'modelactions.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/modelactions.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'modelactions.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PushButton_t {
    const uint offsetsAndSize[2];
    char stringdata0[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_PushButton_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_PushButton_t qt_meta_stringdata_PushButton = {
    {
QT_MOC_LITERAL(0, 10) // "PushButton"

    },
    "PushButton"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PushButton[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void PushButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject PushButton::staticMetaObject = { {
    QMetaObject::SuperData::link<QPushButton::staticMetaObject>(),
    qt_meta_stringdata_PushButton.offsetsAndSize,
    qt_meta_data_PushButton,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_PushButton_t
, QtPrivate::TypeAndForceComplete<PushButton, std::true_type>



>,
    nullptr
} };


const QMetaObject *PushButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PushButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PushButton.stringdata0))
        return static_cast<void*>(this);
    return QPushButton::qt_metacast(_clname);
}

int PushButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPushButton::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_ModelActions_t {
    const uint offsetsAndSize[36];
    char stringdata0[225];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ModelActions_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ModelActions_t qt_meta_stringdata_ModelActions = {
    {
QT_MOC_LITERAL(0, 12), // "ModelActions"
QT_MOC_LITERAL(13, 13), // "LocalMinimize"
QT_MOC_LITERAL(27, 0), // ""
QT_MOC_LITERAL(28, 17), // "OptimizerSettings"
QT_MOC_LITERAL(46, 15), // "ImportConstants"
QT_MOC_LITERAL(62, 15), // "ExportConstants"
QT_MOC_LITERAL(78, 18), // "OpenAdvancedSearch"
QT_MOC_LITERAL(97, 12), // "TogglePlot3D"
QT_MOC_LITERAL(110, 10), // "TogglePlot"
QT_MOC_LITERAL(121, 21), // "ToggleStatisticDialog"
QT_MOC_LITERAL(143, 9), // "Save2File"
QT_MOC_LITERAL(153, 12), // "ToggleSearch"
QT_MOC_LITERAL(166, 8), // "NewGuess"
QT_MOC_LITERAL(175, 14), // "ExportSimModel"
QT_MOC_LITERAL(190, 7), // "Restore"
QT_MOC_LITERAL(198, 8), // "Detailed"
QT_MOC_LITERAL(207, 6), // "Charts"
QT_MOC_LITERAL(214, 10) // "ToggleMore"

    },
    "ModelActions\0LocalMinimize\0\0"
    "OptimizerSettings\0ImportConstants\0"
    "ExportConstants\0OpenAdvancedSearch\0"
    "TogglePlot3D\0TogglePlot\0ToggleStatisticDialog\0"
    "Save2File\0ToggleSearch\0NewGuess\0"
    "ExportSimModel\0Restore\0Detailed\0Charts\0"
    "ToggleMore"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ModelActions[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      15,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  110,    2, 0x06,    1 /* Public */,
       3,    0,  111,    2, 0x06,    2 /* Public */,
       4,    0,  112,    2, 0x06,    3 /* Public */,
       5,    0,  113,    2, 0x06,    4 /* Public */,
       6,    0,  114,    2, 0x06,    5 /* Public */,
       7,    0,  115,    2, 0x06,    6 /* Public */,
       8,    0,  116,    2, 0x06,    7 /* Public */,
       9,    0,  117,    2, 0x06,    8 /* Public */,
      10,    0,  118,    2, 0x06,    9 /* Public */,
      11,    0,  119,    2, 0x06,   10 /* Public */,
      12,    0,  120,    2, 0x06,   11 /* Public */,
      13,    0,  121,    2, 0x06,   12 /* Public */,
      14,    0,  122,    2, 0x06,   13 /* Public */,
      15,    0,  123,    2, 0x06,   14 /* Public */,
      16,    0,  124,    2, 0x06,   15 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      17,    0,  125,    2, 0x08,   16 /* Private */,

 // signals: parameters
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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void ModelActions::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ModelActions *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->LocalMinimize(); break;
        case 1: _t->OptimizerSettings(); break;
        case 2: _t->ImportConstants(); break;
        case 3: _t->ExportConstants(); break;
        case 4: _t->OpenAdvancedSearch(); break;
        case 5: _t->TogglePlot3D(); break;
        case 6: _t->TogglePlot(); break;
        case 7: _t->ToggleStatisticDialog(); break;
        case 8: _t->Save2File(); break;
        case 9: _t->ToggleSearch(); break;
        case 10: _t->NewGuess(); break;
        case 11: _t->ExportSimModel(); break;
        case 12: _t->Restore(); break;
        case 13: _t->Detailed(); break;
        case 14: _t->Charts(); break;
        case 15: _t->ToggleMore(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::LocalMinimize)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::OptimizerSettings)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::ImportConstants)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::ExportConstants)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::OpenAdvancedSearch)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::TogglePlot3D)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::TogglePlot)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::ToggleStatisticDialog)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::Save2File)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::ToggleSearch)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::NewGuess)) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::ExportSimModel)) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::Restore)) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::Detailed)) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (ModelActions::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelActions::Charts)) {
                *result = 14;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject ModelActions::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ModelActions.offsetsAndSize,
    qt_meta_data_ModelActions,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ModelActions_t
, QtPrivate::TypeAndForceComplete<ModelActions, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ModelActions::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ModelActions::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ModelActions.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ModelActions::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void ModelActions::LocalMinimize()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ModelActions::OptimizerSettings()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ModelActions::ImportConstants()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ModelActions::ExportConstants()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ModelActions::OpenAdvancedSearch()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ModelActions::TogglePlot3D()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ModelActions::TogglePlot()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ModelActions::ToggleStatisticDialog()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void ModelActions::Save2File()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void ModelActions::ToggleSearch()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void ModelActions::NewGuess()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void ModelActions::ExportSimModel()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}

// SIGNAL 12
void ModelActions::Restore()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void ModelActions::Detailed()
{
    QMetaObject::activate(this, &staticMetaObject, 13, nullptr);
}

// SIGNAL 14
void ModelActions::Charts()
{
    QMetaObject::activate(this, &staticMetaObject, 14, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
