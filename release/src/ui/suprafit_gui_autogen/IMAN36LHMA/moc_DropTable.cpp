/****************************************************************************
** Meta object code from reading C++ file 'DropTable.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/DropTable.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DropTable.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_DropTable_t {
    const uint offsetsAndSize[6];
    char stringdata0[18];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_DropTable_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_DropTable_t qt_meta_stringdata_DropTable = {
    {
QT_MOC_LITERAL(0, 9), // "DropTable"
QT_MOC_LITERAL(10, 6), // "Edited"
QT_MOC_LITERAL(17, 0) // ""

    },
    "DropTable\0Edited\0"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_DropTable[] = {

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
       1,    0,   20,    2, 0x06,    1 /* Public */,

 // signals: parameters
    QMetaType::Void,

       0        // eod
};

void DropTable::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DropTable *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Edited(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DropTable::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&DropTable::Edited)) {
                *result = 0;
                return;
            }
        }
    }
    (void)_a;
}

const QMetaObject DropTable::staticMetaObject = { {
    QMetaObject::SuperData::link<QTableView::staticMetaObject>(),
    qt_meta_stringdata_DropTable.offsetsAndSize,
    qt_meta_data_DropTable,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_DropTable_t
, QtPrivate::TypeAndForceComplete<DropTable, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>



>,
    nullptr
} };


const QMetaObject *DropTable::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DropTable::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DropTable.stringdata0))
        return static_cast<void*>(this);
    return QTableView::qt_metacast(_clname);
}

int DropTable::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTableView::qt_metacall(_c, _id, _a);
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
void DropTable::Edited()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
