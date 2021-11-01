/****************************************************************************
** Meta object code from reading C++ file 'spinbox.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/buttons/spinbox.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'spinbox.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LineEdit_t {
    const uint offsetsAndSize[2];
    char stringdata0[9];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_LineEdit_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_LineEdit_t qt_meta_stringdata_LineEdit = {
    {
QT_MOC_LITERAL(0, 8) // "LineEdit"

    },
    "LineEdit"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LineEdit[] = {

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

void LineEdit::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject LineEdit::staticMetaObject = { {
    QMetaObject::SuperData::link<QLineEdit::staticMetaObject>(),
    qt_meta_stringdata_LineEdit.offsetsAndSize,
    qt_meta_data_LineEdit,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_LineEdit_t
, QtPrivate::TypeAndForceComplete<LineEdit, std::true_type>



>,
    nullptr
} };


const QMetaObject *LineEdit::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LineEdit::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LineEdit.stringdata0))
        return static_cast<void*>(this);
    return QLineEdit::qt_metacast(_clname);
}

int LineEdit::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QLineEdit::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_SpinBox_t {
    const uint offsetsAndSize[12];
    char stringdata0[59];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_SpinBox_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_SpinBox_t qt_meta_stringdata_SpinBox = {
    {
QT_MOC_LITERAL(0, 7), // "SpinBox"
QT_MOC_LITERAL(8, 20), // "valueChangedNotBySet"
QT_MOC_LITERAL(29, 0), // ""
QT_MOC_LITERAL(30, 3), // "val"
QT_MOC_LITERAL(34, 8), // "setValue"
QT_MOC_LITERAL(43, 15) // "On_valueChanged"

    },
    "SpinBox\0valueChangedNotBySet\0\0val\0"
    "setValue\0On_valueChanged"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SpinBox[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   32,    2, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   35,    2, 0x0a,    3 /* Public */,
       5,    1,   38,    2, 0x08,    5 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Double,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::Double,    3,
    QMetaType::Void, QMetaType::Double,    3,

       0        // eod
};

void SpinBox::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SpinBox *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->valueChangedNotBySet((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 1: _t->setValue((*reinterpret_cast< double(*)>(_a[1]))); break;
        case 2: _t->On_valueChanged((*reinterpret_cast< double(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SpinBox::*)(double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SpinBox::valueChangedNotBySet)) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject SpinBox::staticMetaObject = { {
    QMetaObject::SuperData::link<QDoubleSpinBox::staticMetaObject>(),
    qt_meta_stringdata_SpinBox.offsetsAndSize,
    qt_meta_data_SpinBox,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_SpinBox_t
, QtPrivate::TypeAndForceComplete<SpinBox, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>


>,
    nullptr
} };


const QMetaObject *SpinBox::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SpinBox::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SpinBox.stringdata0))
        return static_cast<void*>(this);
    return QDoubleSpinBox::qt_metacast(_clname);
}

int SpinBox::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDoubleSpinBox::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void SpinBox::valueChangedNotBySet(double _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
