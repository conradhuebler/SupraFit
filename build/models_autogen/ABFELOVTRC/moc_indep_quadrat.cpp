/****************************************************************************
** Meta object code from reading C++ file 'indep_quadrat.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/core/models/misc/indep_quadrat.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'indep_quadrat.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Indep_Quadrat_t {
    const uint offsetsAndSize[2];
    char stringdata0[14];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_Indep_Quadrat_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_Indep_Quadrat_t qt_meta_stringdata_Indep_Quadrat = {
    {
QT_MOC_LITERAL(0, 13) // "Indep_Quadrat"

    },
    "Indep_Quadrat"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Indep_Quadrat[] = {

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

void Indep_Quadrat::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject Indep_Quadrat::staticMetaObject = { {
    QMetaObject::SuperData::link<AbstractModel::staticMetaObject>(),
    qt_meta_stringdata_Indep_Quadrat.offsetsAndSize,
    qt_meta_data_Indep_Quadrat,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_Indep_Quadrat_t
, QtPrivate::TypeAndForceComplete<Indep_Quadrat, std::true_type>



>,
    nullptr
} };


const QMetaObject *Indep_Quadrat::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Indep_Quadrat::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_Indep_Quadrat.stringdata0))
        return static_cast<void*>(this);
    return AbstractModel::qt_metacast(_clname);
}

int Indep_Quadrat::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = AbstractModel::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
