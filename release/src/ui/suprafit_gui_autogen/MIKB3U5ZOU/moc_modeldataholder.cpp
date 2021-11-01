/****************************************************************************
** Meta object code from reading C++ file 'modeldataholder.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/mainwindow/modeldataholder.h"
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
#error "The header file 'modeldataholder.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ToolButton_t {
    const uint offsetsAndSize[8];
    char stringdata0[30];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ToolButton_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ToolButton_t qt_meta_stringdata_ToolButton = {
    {
QT_MOC_LITERAL(0, 10), // "ToolButton"
QT_MOC_LITERAL(11, 11), // "ChangeColor"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 5) // "color"

    },
    "ToolButton\0ChangeColor\0\0color"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ToolButton[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   20,    2, 0x0a,    1 /* Public */,

 // slots: parameters
    QMetaType::Void, QMetaType::QColor,    3,

       0        // eod
};

void ToolButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ToolButton *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ChangeColor((*reinterpret_cast< const QColor(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject ToolButton::staticMetaObject = { {
    QMetaObject::SuperData::link<QToolButton::staticMetaObject>(),
    qt_meta_stringdata_ToolButton.offsetsAndSize,
    qt_meta_data_ToolButton,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ToolButton_t
, QtPrivate::TypeAndForceComplete<ToolButton, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QColor &, std::false_type>


>,
    nullptr
} };


const QMetaObject *ToolButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ToolButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ToolButton.stringdata0))
        return static_cast<void*>(this);
    return QToolButton::qt_metacast(_clname);
}

int ToolButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QToolButton::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_TabWidget_t {
    const uint offsetsAndSize[2];
    char stringdata0[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_TabWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_TabWidget_t qt_meta_stringdata_TabWidget = {
    {
QT_MOC_LITERAL(0, 9) // "TabWidget"

    },
    "TabWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TabWidget[] = {

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

void TabWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject TabWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QTabWidget::staticMetaObject>(),
    qt_meta_stringdata_TabWidget.offsetsAndSize,
    qt_meta_data_TabWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_TabWidget_t
, QtPrivate::TypeAndForceComplete<TabWidget, std::true_type>



>,
    nullptr
} };


const QMetaObject *TabWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TabWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TabWidget.stringdata0))
        return static_cast<void*>(this);
    return QTabWidget::qt_metacast(_clname);
}

int TabWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTabWidget::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_MDHDockTitleBar_t {
    const uint offsetsAndSize[18];
    char stringdata0[95];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_MDHDockTitleBar_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_MDHDockTitleBar_t qt_meta_stringdata_MDHDockTitleBar = {
    {
QT_MOC_LITERAL(0, 15), // "MDHDockTitleBar"
QT_MOC_LITERAL(16, 8), // "AddModel"
QT_MOC_LITERAL(25, 0), // ""
QT_MOC_LITERAL(26, 8), // "CloseAll"
QT_MOC_LITERAL(35, 14), // "ShowStatistics"
QT_MOC_LITERAL(50, 11), // "OptimizeAll"
QT_MOC_LITERAL(62, 7), // "Compare"
QT_MOC_LITERAL(70, 8), // "EditData"
QT_MOC_LITERAL(79, 15) // "PrepareAddModel"

    },
    "MDHDockTitleBar\0AddModel\0\0CloseAll\0"
    "ShowStatistics\0OptimizeAll\0Compare\0"
    "EditData\0PrepareAddModel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MDHDockTitleBar[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   56,    2, 0x06,    1 /* Public */,
       3,    0,   57,    2, 0x06,    2 /* Public */,
       4,    0,   58,    2, 0x06,    3 /* Public */,
       5,    0,   59,    2, 0x06,    4 /* Public */,
       6,    0,   60,    2, 0x06,    5 /* Public */,
       7,    0,   61,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   62,    2, 0x08,    7 /* Private */,

 // signals: parameters
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

void MDHDockTitleBar::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MDHDockTitleBar *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->AddModel(); break;
        case 1: _t->CloseAll(); break;
        case 2: _t->ShowStatistics(); break;
        case 3: _t->OptimizeAll(); break;
        case 4: _t->Compare(); break;
        case 5: _t->EditData(); break;
        case 6: _t->PrepareAddModel(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MDHDockTitleBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MDHDockTitleBar::AddModel)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MDHDockTitleBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MDHDockTitleBar::CloseAll)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MDHDockTitleBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MDHDockTitleBar::ShowStatistics)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MDHDockTitleBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MDHDockTitleBar::OptimizeAll)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MDHDockTitleBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MDHDockTitleBar::Compare)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MDHDockTitleBar::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MDHDockTitleBar::EditData)) {
                *result = 5;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject MDHDockTitleBar::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_MDHDockTitleBar.offsetsAndSize,
    qt_meta_data_MDHDockTitleBar,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_MDHDockTitleBar_t
, QtPrivate::TypeAndForceComplete<MDHDockTitleBar, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *MDHDockTitleBar::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MDHDockTitleBar::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MDHDockTitleBar.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MDHDockTitleBar::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void MDHDockTitleBar::AddModel()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MDHDockTitleBar::CloseAll()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MDHDockTitleBar::ShowStatistics()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MDHDockTitleBar::OptimizeAll()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MDHDockTitleBar::Compare()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MDHDockTitleBar::EditData()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
struct qt_meta_stringdata_ModelDataHolder_t {
    const uint offsetsAndSize[74];
    char stringdata0[371];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ModelDataHolder_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ModelDataHolder_t qt_meta_stringdata_ModelDataHolder = {
    {
QT_MOC_LITERAL(0, 15), // "ModelDataHolder"
QT_MOC_LITERAL(16, 10), // "ModelAdded"
QT_MOC_LITERAL(27, 0), // ""
QT_MOC_LITERAL(28, 12), // "ModelRemoved"
QT_MOC_LITERAL(41, 10), // "MessageBox"
QT_MOC_LITERAL(52, 3), // "str"
QT_MOC_LITERAL(56, 8), // "priority"
QT_MOC_LITERAL(65, 11), // "InsertModel"
QT_MOC_LITERAL(77, 5), // "model"
QT_MOC_LITERAL(83, 6), // "active"
QT_MOC_LITERAL(90, 11), // "nameChanged"
QT_MOC_LITERAL(102, 11), // "recalculate"
QT_MOC_LITERAL(114, 7), // "Message"
QT_MOC_LITERAL(122, 7), // "Warning"
QT_MOC_LITERAL(130, 13), // "SpectraEdited"
QT_MOC_LITERAL(144, 5), // "table"
QT_MOC_LITERAL(150, 4), // "data"
QT_MOC_LITERAL(155, 14), // "AddToWorkspace"
QT_MOC_LITERAL(170, 6), // "object"
QT_MOC_LITERAL(177, 18), // "LoadCurrentProject"
QT_MOC_LITERAL(196, 9), // "RemoveTab"
QT_MOC_LITERAL(206, 1), // "i"
QT_MOC_LITERAL(208, 8), // "CloseAll"
QT_MOC_LITERAL(217, 14), // "CloseAllForced"
QT_MOC_LITERAL(232, 8), // "AddModel"
QT_MOC_LITERAL(241, 17), // "SetProjectTabName"
QT_MOC_LITERAL(259, 7), // "RunJobs"
QT_MOC_LITERAL(267, 3), // "job"
QT_MOC_LITERAL(271, 11), // "OptimizeAll"
QT_MOC_LITERAL(283, 10), // "CompareAIC"
QT_MOC_LITERAL(294, 9), // "CompareCV"
QT_MOC_LITERAL(304, 16), // "CompareReduction"
QT_MOC_LITERAL(321, 9), // "CompareMC"
QT_MOC_LITERAL(331, 8), // "EditData"
QT_MOC_LITERAL(340, 14), // "HideSubWindows"
QT_MOC_LITERAL(355, 5), // "index"
QT_MOC_LITERAL(361, 9) // "Interrupt"

    },
    "ModelDataHolder\0ModelAdded\0\0ModelRemoved\0"
    "MessageBox\0str\0priority\0InsertModel\0"
    "model\0active\0nameChanged\0recalculate\0"
    "Message\0Warning\0SpectraEdited\0table\0"
    "data\0AddToWorkspace\0object\0"
    "LoadCurrentProject\0RemoveTab\0i\0CloseAll\0"
    "CloseAllForced\0AddModel\0SetProjectTabName\0"
    "RunJobs\0job\0OptimizeAll\0CompareAIC\0"
    "CompareCV\0CompareReduction\0CompareMC\0"
    "EditData\0HideSubWindows\0index\0Interrupt"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ModelDataHolder[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      26,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      10,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  170,    2, 0x06,    1 /* Public */,
       3,    0,  171,    2, 0x06,    2 /* Public */,
       4,    2,  172,    2, 0x06,    3 /* Public */,
       7,    2,  177,    2, 0x06,    6 /* Public */,
       7,    1,  182,    2, 0x06,    9 /* Public */,
      10,    0,  185,    2, 0x06,   11 /* Public */,
      11,    0,  186,    2, 0x06,   12 /* Public */,
      12,    2,  187,    2, 0x06,   13 /* Public */,
      13,    2,  192,    2, 0x06,   16 /* Public */,
      14,    2,  197,    2, 0x06,   19 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      17,    1,  202,    2, 0x0a,   22 /* Public */,
      19,    1,  205,    2, 0x0a,   24 /* Public */,
      20,    1,  208,    2, 0x0a,   26 /* Public */,
      22,    0,  211,    2, 0x0a,   28 /* Public */,
      23,    0,  212,    2, 0x0a,   29 /* Public */,
      24,    0,  213,    2, 0x08,   30 /* Private */,
      25,    0,  214,    2, 0x08,   31 /* Private */,
      26,    1,  215,    2, 0x08,   32 /* Private */,
      28,    0,  218,    2, 0x08,   34 /* Private */,
      29,    0,  219,    2, 0x08,   35 /* Private */,
      30,    0,  220,    2, 0x08,   36 /* Private */,
      31,    0,  221,    2, 0x08,   37 /* Private */,
      32,    0,  222,    2, 0x08,   38 /* Private */,
      33,    0,  223,    2, 0x08,   39 /* Private */,
      34,    1,  224,    2, 0x08,   40 /* Private */,
      36,    0,  227,    2, 0x08,   42 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    5,    6,
    QMetaType::Void, QMetaType::QJsonObject, QMetaType::Int,    8,    9,
    QMetaType::Void, QMetaType::QJsonObject,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    5,    6,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    5,    6,
    QMetaType::Void, QMetaType::QJsonObject, QMetaType::QJsonObject,   15,   16,

 // slots: parameters
    QMetaType::Void, QMetaType::QJsonObject,   18,
    QMetaType::Void, QMetaType::QJsonObject,   18,
    QMetaType::Void, QMetaType::Int,   21,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QJsonObject,   27,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   35,
    QMetaType::Void,

       0        // eod
};

void ModelDataHolder::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ModelDataHolder *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->ModelAdded(); break;
        case 1: _t->ModelRemoved(); break;
        case 2: _t->MessageBox((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->InsertModel((*reinterpret_cast< const QJsonObject(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->InsertModel((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 5: _t->nameChanged(); break;
        case 6: _t->recalculate(); break;
        case 7: _t->Message((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 8: _t->Warning((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 9: _t->SpectraEdited((*reinterpret_cast< const QJsonObject(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 10: _t->AddToWorkspace((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 11: _t->LoadCurrentProject((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 12: _t->RemoveTab((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->CloseAll(); break;
        case 14: _t->CloseAllForced(); break;
        case 15: _t->AddModel(); break;
        case 16: _t->SetProjectTabName(); break;
        case 17: _t->RunJobs((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 18: _t->OptimizeAll(); break;
        case 19: _t->CompareAIC(); break;
        case 20: _t->CompareCV(); break;
        case 21: _t->CompareReduction(); break;
        case 22: _t->CompareMC(); break;
        case 23: _t->EditData(); break;
        case 24: _t->HideSubWindows((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 25: _t->Interrupt(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ModelDataHolder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::ModelAdded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::ModelRemoved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::MessageBox)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)(const QJsonObject & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::InsertModel)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::InsertModel)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::nameChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::recalculate)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::Message)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::Warning)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (ModelDataHolder::*)(const QJsonObject & , const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ModelDataHolder::SpectraEdited)) {
                *result = 9;
                return;
            }
        }
    }
}

const QMetaObject ModelDataHolder::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_ModelDataHolder.offsetsAndSize,
    qt_meta_data_ModelDataHolder,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ModelDataHolder_t
, QtPrivate::TypeAndForceComplete<ModelDataHolder, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *ModelDataHolder::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ModelDataHolder::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ModelDataHolder.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int ModelDataHolder::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 26)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 26;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 26)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 26;
    }
    return _id;
}

// SIGNAL 0
void ModelDataHolder::ModelAdded()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ModelDataHolder::ModelRemoved()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ModelDataHolder::MessageBox(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ModelDataHolder::InsertModel(const QJsonObject & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ModelDataHolder::InsertModel(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ModelDataHolder::nameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ModelDataHolder::recalculate()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ModelDataHolder::Message(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void ModelDataHolder::Warning(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void ModelDataHolder::SpectraEdited(const QJsonObject & _t1, const QJsonObject & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
