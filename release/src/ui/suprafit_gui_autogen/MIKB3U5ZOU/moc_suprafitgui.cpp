/****************************************************************************
** Meta object code from reading C++ file 'suprafitgui.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/mainwindow/suprafitgui.h"
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
#error "The header file 'suprafitgui.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DropButton_t {
    const uint offsetsAndSize[8];
    char stringdata0[29];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_DropButton_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_DropButton_t qt_meta_stringdata_DropButton = {
    {
QT_MOC_LITERAL(0, 10), // "DropButton"
QT_MOC_LITERAL(11, 11), // "DataDropped"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 4) // "data"

    },
    "DropButton\0DataDropped\0\0data"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DropButton[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QJsonObject,    3,

       0        // eod
};

void DropButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DropButton *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->DataDropped((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DropButton::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DropButton::DataDropped)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject DropButton::staticMetaObject = { {
    QMetaObject::SuperData::link<QToolButton::staticMetaObject>(),
    qt_meta_stringdata_DropButton.offsetsAndSize,
    qt_meta_data_DropButton,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_DropButton_t
, QtPrivate::TypeAndForceComplete<DropButton, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>



>,
    nullptr
} };


const QMetaObject *DropButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DropButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DropButton.stringdata0))
        return static_cast<void*>(this);
    return QToolButton::qt_metacast(_clname);
}

int DropButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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

// SIGNAL 0
void DropButton::DataDropped(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_LogoLabel_t {
    const uint offsetsAndSize[2];
    char stringdata0[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_LogoLabel_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_LogoLabel_t qt_meta_stringdata_LogoLabel = {
    {
QT_MOC_LITERAL(0, 9) // "LogoLabel"

    },
    "LogoLabel"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LogoLabel[] = {

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

void LogoLabel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject LogoLabel::staticMetaObject = { {
    QMetaObject::SuperData::link<QLabel::staticMetaObject>(),
    qt_meta_stringdata_LogoLabel.offsetsAndSize,
    qt_meta_data_LogoLabel,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_LogoLabel_t
, QtPrivate::TypeAndForceComplete<LogoLabel, std::true_type>



>,
    nullptr
} };


const QMetaObject *LogoLabel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LogoLabel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LogoLabel.stringdata0))
        return static_cast<void*>(this);
    return QLabel::qt_metacast(_clname);
}

int LogoLabel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLabel::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_SupraFitGui_t {
    const uint offsetsAndSize[76];
    char stringdata0[424];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SupraFitGui_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SupraFitGui_t qt_meta_stringdata_SupraFitGui = {
    {
QT_MOC_LITERAL(0, 11), // "SupraFitGui"
QT_MOC_LITERAL(12, 8), // "LoadFile"
QT_MOC_LITERAL(21, 0), // ""
QT_MOC_LITERAL(22, 4), // "file"
QT_MOC_LITERAL(27, 13), // "SpectraEdited"
QT_MOC_LITERAL(41, 5), // "table"
QT_MOC_LITERAL(47, 4), // "data"
QT_MOC_LITERAL(52, 9), // "NewWindow"
QT_MOC_LITERAL(62, 8), // "NewTable"
QT_MOC_LITERAL(71, 8), // "OpenFile"
QT_MOC_LITERAL(80, 14), // "OpenSpectraDir"
QT_MOC_LITERAL(95, 14), // "setWindowTitle"
QT_MOC_LITERAL(110, 17), // "SaveProjectAction"
QT_MOC_LITERAL(128, 19), // "SaveAsProjectAction"
QT_MOC_LITERAL(148, 14), // "SettingsDialog"
QT_MOC_LITERAL(163, 5), // "about"
QT_MOC_LITERAL(169, 11), // "LicenseInfo"
QT_MOC_LITERAL(181, 10), // "FirstStart"
QT_MOC_LITERAL(192, 16), // "UpdateRecentList"
QT_MOC_LITERAL(209, 12), // "AddMetaModel"
QT_MOC_LITERAL(222, 11), // "QModelIndex"
QT_MOC_LITERAL(234, 5), // "index"
QT_MOC_LITERAL(240, 8), // "position"
QT_MOC_LITERAL(249, 19), // "CopySystemParameter"
QT_MOC_LITERAL(269, 6), // "source"
QT_MOC_LITERAL(276, 8), // "SaveData"
QT_MOC_LITERAL(285, 9), // "AddUpData"
QT_MOC_LITERAL(295, 4), // "sign"
QT_MOC_LITERAL(300, 9), // "CopyModel"
QT_MOC_LITERAL(310, 1), // "o"
QT_MOC_LITERAL(312, 5), // "model"
QT_MOC_LITERAL(318, 17), // "TreeDoubleClicked"
QT_MOC_LITERAL(336, 11), // "TreeClicked"
QT_MOC_LITERAL(348, 17), // "TreeRemoveRequest"
QT_MOC_LITERAL(366, 13), // "CloseProjects"
QT_MOC_LITERAL(380, 14), // "ExportAllPlain"
QT_MOC_LITERAL(395, 17), // "ExportAllSupraFit"
QT_MOC_LITERAL(413, 10) // "AddScatter"

    },
    "SupraFitGui\0LoadFile\0\0file\0SpectraEdited\0"
    "table\0data\0NewWindow\0NewTable\0OpenFile\0"
    "OpenSpectraDir\0setWindowTitle\0"
    "SaveProjectAction\0SaveAsProjectAction\0"
    "SettingsDialog\0about\0LicenseInfo\0"
    "FirstStart\0UpdateRecentList\0AddMetaModel\0"
    "QModelIndex\0index\0position\0"
    "CopySystemParameter\0source\0SaveData\0"
    "AddUpData\0sign\0CopyModel\0o\0model\0"
    "TreeDoubleClicked\0TreeClicked\0"
    "TreeRemoveRequest\0CloseProjects\0"
    "ExportAllPlain\0ExportAllSupraFit\0"
    "AddScatter"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SupraFitGui[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      26,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  170,    2, 0x0a,    1 /* Public */,
       4,    2,  173,    2, 0x0a,    3 /* Public */,
       7,    0,  178,    2, 0x08,    6 /* Private */,
       8,    0,  179,    2, 0x08,    7 /* Private */,
       9,    0,  180,    2, 0x08,    8 /* Private */,
      10,    0,  181,    2, 0x08,    9 /* Private */,
      11,    0,  182,    2, 0x08,   10 /* Private */,
      12,    0,  183,    2, 0x08,   11 /* Private */,
      13,    0,  184,    2, 0x08,   12 /* Private */,
      14,    0,  185,    2, 0x08,   13 /* Private */,
      15,    0,  186,    2, 0x08,   14 /* Private */,
      16,    0,  187,    2, 0x08,   15 /* Private */,
      17,    0,  188,    2, 0x08,   16 /* Private */,
      18,    0,  189,    2, 0x08,   17 /* Private */,
      19,    2,  190,    2, 0x08,   18 /* Private */,
      23,    2,  195,    2, 0x08,   21 /* Private */,
      25,    1,  200,    2, 0x08,   24 /* Private */,
      26,    2,  203,    2, 0x08,   26 /* Private */,
      28,    3,  208,    2, 0x08,   29 /* Private */,
      31,    1,  215,    2, 0x08,   33 /* Private */,
      32,    1,  218,    2, 0x08,   35 /* Private */,
      33,    1,  221,    2, 0x08,   37 /* Private */,
      34,    0,  224,    2, 0x08,   39 /* Private */,
      35,    0,  225,    2, 0x08,   40 /* Private */,
      36,    0,  226,    2, 0x08,   41 /* Private */,
      37,    0,  227,    2, 0x08,   42 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QJsonObject, QMetaType::QJsonObject,    5,    6,
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
    QMetaType::Void, 0x80000000 | 20, QMetaType::Int,   21,   22,
    QMetaType::Void, 0x80000000 | 20, QMetaType::Int,   24,   22,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void, 0x80000000 | 20, QMetaType::Bool,   21,   27,
    QMetaType::Void, QMetaType::QJsonObject, QMetaType::Int, QMetaType::Int,   29,    6,   30,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void SupraFitGui::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SupraFitGui *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->LoadFile((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->SpectraEdited((*reinterpret_cast< const QJsonObject(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 2: _t->NewWindow(); break;
        case 3: _t->NewTable(); break;
        case 4: _t->OpenFile(); break;
        case 5: _t->OpenSpectraDir(); break;
        case 6: _t->setWindowTitle(); break;
        case 7: _t->SaveProjectAction(); break;
        case 8: _t->SaveAsProjectAction(); break;
        case 9: _t->SettingsDialog(); break;
        case 10: _t->about(); break;
        case 11: _t->LicenseInfo(); break;
        case 12: _t->FirstStart(); break;
        case 13: _t->UpdateRecentList(); break;
        case 14: _t->AddMetaModel((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 15: _t->CopySystemParameter((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 16: _t->SaveData((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 17: _t->AddUpData((*reinterpret_cast< const QModelIndex(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 18: _t->CopyModel((*reinterpret_cast< const QJsonObject(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 19: _t->TreeDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 20: _t->TreeClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 21: _t->TreeRemoveRequest((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 22: _t->CloseProjects(); break;
        case 23: _t->ExportAllPlain(); break;
        case 24: _t->ExportAllSupraFit(); break;
        case 25: _t->AddScatter(); break;
        default: ;
        }
    }
}

const QMetaObject SupraFitGui::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_SupraFitGui.offsetsAndSize,
    qt_meta_data_SupraFitGui,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SupraFitGui_t
, QtPrivate::TypeAndForceComplete<SupraFitGui, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<bool, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *SupraFitGui::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SupraFitGui::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SupraFitGui.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int SupraFitGui::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
