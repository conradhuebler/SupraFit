/****************************************************************************
** Meta object code from reading C++ file 'messagedock.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../src/ui/widgets/messagedock.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'messagedock.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MessageDock_t {
    const uint offsetsAndSize[18];
    char stringdata0[64];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_MessageDock_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_MessageDock_t qt_meta_stringdata_MessageDock = {
    {
QT_MOC_LITERAL(0, 11), // "MessageDock"
QT_MOC_LITERAL(12, 9), // "Attention"
QT_MOC_LITERAL(22, 0), // ""
QT_MOC_LITERAL(23, 8), // "Presence"
QT_MOC_LITERAL(32, 6), // "UiInfo"
QT_MOC_LITERAL(39, 7), // "Message"
QT_MOC_LITERAL(47, 3), // "str"
QT_MOC_LITERAL(51, 7), // "Warning"
QT_MOC_LITERAL(59, 4) // "Info"

    },
    "MessageDock\0Attention\0\0Presence\0UiInfo\0"
    "Message\0str\0Warning\0Info"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MessageDock[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x06,    1 /* Public */,
       3,    0,   51,    2, 0x06,    2 /* Public */,
       4,    0,   52,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    1,   53,    2, 0x0a,    4 /* Public */,
       7,    1,   56,    2, 0x0a,    6 /* Public */,
       8,    1,   59,    2, 0x0a,    8 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    6,

       0        // eod
};

void MessageDock::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MessageDock *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Attention(); break;
        case 1: _t->Presence(); break;
        case 2: _t->UiInfo(); break;
        case 3: _t->Message((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->Warning((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->Info((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MessageDock::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MessageDock::Attention)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MessageDock::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MessageDock::Presence)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MessageDock::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MessageDock::UiInfo)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject MessageDock::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_MessageDock.offsetsAndSize,
    qt_meta_data_MessageDock,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_MessageDock_t
, QtPrivate::TypeAndForceComplete<MessageDock, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>


>,
    nullptr
} };


const QMetaObject *MessageDock::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MessageDock::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MessageDock.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MessageDock::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void MessageDock::Attention()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MessageDock::Presence()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MessageDock::UiInfo()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
