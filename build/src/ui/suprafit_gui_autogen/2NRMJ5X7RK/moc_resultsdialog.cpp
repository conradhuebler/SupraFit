/****************************************************************************
** Meta object code from reading C++ file 'resultsdialog.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/dialogs/resultsdialog.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'resultsdialog.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ResultsDialog_t {
    const uint offsetsAndSize[32];
    char stringdata0[159];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_ResultsDialog_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_ResultsDialog_t qt_meta_stringdata_ResultsDialog = {
    {
QT_MOC_LITERAL(0, 13), // "ResultsDialog"
QT_MOC_LITERAL(14, 9), // "LoadModel"
QT_MOC_LITERAL(24, 0), // ""
QT_MOC_LITERAL(25, 6), // "object"
QT_MOC_LITERAL(32, 8), // "AddModel"
QT_MOC_LITERAL(41, 10), // "ShowResult"
QT_MOC_LITERAL(52, 16), // "SupraFit::Method"
QT_MOC_LITERAL(69, 4), // "type"
QT_MOC_LITERAL(74, 5), // "index"
QT_MOC_LITERAL(80, 9), // "Attention"
QT_MOC_LITERAL(90, 10), // "UpdateList"
QT_MOC_LITERAL(101, 17), // "itemDoubleClicked"
QT_MOC_LITERAL(119, 11), // "QModelIndex"
QT_MOC_LITERAL(131, 10), // "RemoveItem"
QT_MOC_LITERAL(142, 4), // "Save"
QT_MOC_LITERAL(147, 11) // "DropRawData"

    },
    "ResultsDialog\0LoadModel\0\0object\0"
    "AddModel\0ShowResult\0SupraFit::Method\0"
    "type\0index\0Attention\0UpdateList\0"
    "itemDoubleClicked\0QModelIndex\0RemoveItem\0"
    "Save\0DropRawData"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ResultsDialog[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   68,    2, 0x06,    1 /* Public */,
       4,    1,   71,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    2,   74,    2, 0x0a,    5 /* Public */,
       9,    0,   79,    2, 0x0a,    8 /* Public */,
      10,    0,   80,    2, 0x08,    9 /* Private */,
      11,    1,   81,    2, 0x08,   10 /* Private */,
      13,    1,   84,    2, 0x08,   12 /* Private */,
      14,    1,   87,    2, 0x08,   14 /* Private */,
      15,    1,   90,    2, 0x08,   16 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QJsonObject,    3,
    QMetaType::Void, QMetaType::QJsonObject,    3,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 6, QMetaType::Int,    7,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 12,    8,
    QMetaType::Void, 0x80000000 | 12,    8,
    QMetaType::Void, 0x80000000 | 12,    8,
    QMetaType::Void, 0x80000000 | 12,    8,

       0        // eod
};

void ResultsDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ResultsDialog *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->LoadModel((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 1: _t->AddModel((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 2: _t->ShowResult((*reinterpret_cast< SupraFit::Method(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->Attention(); break;
        case 4: _t->UpdateList(); break;
        case 5: _t->itemDoubleClicked((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 6: _t->RemoveItem((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 7: _t->Save((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        case 8: _t->DropRawData((*reinterpret_cast< const QModelIndex(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ResultsDialog::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ResultsDialog::LoadModel)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ResultsDialog::*)(const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ResultsDialog::AddModel)) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject ResultsDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_ResultsDialog.offsetsAndSize,
    qt_meta_data_ResultsDialog,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_ResultsDialog_t
, QtPrivate::TypeAndForceComplete<ResultsDialog, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<SupraFit::Method, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QModelIndex &, std::false_type>


>,
    nullptr
} };


const QMetaObject *ResultsDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ResultsDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ResultsDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int ResultsDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void ResultsDialog::LoadModel(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ResultsDialog::AddModel(const QJsonObject & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
