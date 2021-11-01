/****************************************************************************
** Meta object code from reading C++ file 'fl_1_1_1_2_Model.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/models/titrations/fluorescence/fl_1_1_1_2_Model.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fl_1_1_1_2_Model.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_fl_ItoI_ItoII_Model_t {
    const uint offsetsAndSize[2];
    char stringdata0[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_fl_ItoI_ItoII_Model_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_fl_ItoI_ItoII_Model_t qt_meta_stringdata_fl_ItoI_ItoII_Model = {
    {
QT_MOC_LITERAL(0, 19) // "fl_ItoI_ItoII_Model"

    },
    "fl_ItoI_ItoII_Model"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_fl_ItoI_ItoII_Model[] = {

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

void fl_ItoI_ItoII_Model::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject fl_ItoI_ItoII_Model::staticMetaObject = { {
    QMetaObject::SuperData::link<AbstractTitrationModel::staticMetaObject>(),
    qt_meta_stringdata_fl_ItoI_ItoII_Model.offsetsAndSize,
    qt_meta_data_fl_ItoI_ItoII_Model,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_fl_ItoI_ItoII_Model_t
, QtPrivate::TypeAndForceComplete<fl_ItoI_ItoII_Model, std::true_type>



>,
    nullptr
} };


const QMetaObject *fl_ItoI_ItoII_Model::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *fl_ItoI_ItoII_Model::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_fl_ItoI_ItoII_Model.stringdata0))
        return static_cast<void*>(this);
    return AbstractTitrationModel::qt_metacast(_clname);
}

int fl_ItoI_ItoII_Model::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = AbstractTitrationModel::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
