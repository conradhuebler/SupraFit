/****************************************************************************
** Meta object code from reading C++ file 'searchresultwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/results/searchresultwidget.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'searchresultwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SearchResultWidget_t {
    const uint offsetsAndSize[24];
    char stringdata0[120];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SearchResultWidget_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SearchResultWidget_t qt_meta_stringdata_SearchResultWidget = {
    {
QT_MOC_LITERAL(0, 18), // "SearchResultWidget"
QT_MOC_LITERAL(19, 9), // "LoadModel"
QT_MOC_LITERAL(29, 0), // ""
QT_MOC_LITERAL(30, 6), // "object"
QT_MOC_LITERAL(37, 8), // "AddModel"
QT_MOC_LITERAL(46, 11), // "rowSelected"
QT_MOC_LITERAL(58, 11), // "QModelIndex"
QT_MOC_LITERAL(70, 5), // "index"
QT_MOC_LITERAL(76, 12), // "ExportModels"
QT_MOC_LITERAL(89, 11), // "ApplyFilter"
QT_MOC_LITERAL(101, 12), // "ModelClicked"
QT_MOC_LITERAL(114, 5) // "model"

    },
    "SearchResultWidget\0LoadModel\0\0object\0"
    "AddModel\0rowSelected\0QModelIndex\0index\0"
    "ExportModels\0ApplyFilter\0ModelClicked\0"
    "model"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SearchResultWidget[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       4,    1,   53,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    1,   56,    2, 0x08,    5 /* Private */,
       8,    0,   59,    2, 0x08,    7 /* Private */,
       9,    0,   60,    2, 0x08,    8 /* Private */,
      10,    1,   61,    2, 0x08,    9 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QJsonObject,    3,
    QMetaType::Void, QMetaType::QJsonObject,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   11,

       0        // eod
};

void SearchResultWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SearchResultWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->LoadModel((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 1: _t->AddModel((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 2: _t->rowSelected((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 3: _t->ExportModels(); break;
        case 4: _t->ApplyFilter(); break;
        case 5: _t->ModelClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SearchResultWidget::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchResultWidget::LoadModel)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SearchResultWidget::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SearchResultWidget::AddModel)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject SearchResultWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_SearchResultWidget.offsetsAndSize,
    qt_meta_data_SearchResultWidget,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SearchResultWidget_t
, QtPrivate::TypeAndForceComplete<SearchResultWidget, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>


>,
    nullptr
} };


const QMetaObject *SearchResultWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SearchResultWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SearchResultWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int SearchResultWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void SearchResultWidget::LoadModel(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SearchResultWidget::AddModel(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
