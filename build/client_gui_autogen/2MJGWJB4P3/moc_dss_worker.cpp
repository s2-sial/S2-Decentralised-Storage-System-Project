/****************************************************************************
** Meta object code from reading C++ file 'dss_worker.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/client/dss_worker.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dss_worker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_DssWorker_t {
    uint offsetsAndSizes[44];
    char stringdata0[10];
    char stringdata1[9];
    char stringdata2[1];
    char stringdata3[5];
    char stringdata4[6];
    char stringdata5[8];
    char stringdata6[12];
    char stringdata7[13];
    char stringdata8[12];
    char stringdata9[11];
    char stringdata10[15];
    char stringdata11[6];
    char stringdata12[8];
    char stringdata13[10];
    char stringdata14[12];
    char stringdata15[9];
    char stringdata16[10];
    char stringdata17[9];
    char stringdata18[8];
    char stringdata19[7];
    char stringdata20[16];
    char stringdata21[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_DssWorker_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_DssWorker_t qt_meta_stringdata_DssWorker = {
    {
        QT_MOC_LITERAL(0, 9),  // "DssWorker"
        QT_MOC_LITERAL(10, 8),  // "progress"
        QT_MOC_LITERAL(19, 0),  // ""
        QT_MOC_LITERAL(20, 4),  // "done"
        QT_MOC_LITERAL(25, 5),  // "total"
        QT_MOC_LITERAL(31, 7),  // "message"
        QT_MOC_LITERAL(39, 11),  // "putFinished"
        QT_MOC_LITERAL(51, 12),  // "manifestPath"
        QT_MOC_LITERAL(64, 11),  // "getFinished"
        QT_MOC_LITERAL(76, 10),  // "outputPath"
        QT_MOC_LITERAL(87, 14),  // "repairFinished"
        QT_MOC_LITERAL(102, 5),  // "error"
        QT_MOC_LITERAL(108, 7),  // "putFile"
        QT_MOC_LITERAL(116, 9),  // "trackerIp"
        QT_MOC_LITERAL(126, 11),  // "trackerPort"
        QT_MOC_LITERAL(138, 8),  // "filePath"
        QT_MOC_LITERAL(147, 9),  // "chunkSize"
        QT_MOC_LITERAL(157, 8),  // "replicas"
        QT_MOC_LITERAL(166, 7),  // "getFile"
        QT_MOC_LITERAL(174, 6),  // "repair"
        QT_MOC_LITERAL(181, 15),  // "desiredReplicas"
        QT_MOC_LITERAL(197, 5)   // "batch"
    },
    "DssWorker",
    "progress",
    "",
    "done",
    "total",
    "message",
    "putFinished",
    "manifestPath",
    "getFinished",
    "outputPath",
    "repairFinished",
    "error",
    "putFile",
    "trackerIp",
    "trackerPort",
    "filePath",
    "chunkSize",
    "replicas",
    "getFile",
    "repair",
    "desiredReplicas",
    "batch"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_DssWorker[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    3,   62,    2, 0x06,    1 /* Public */,
       6,    1,   69,    2, 0x06,    5 /* Public */,
       8,    1,   72,    2, 0x06,    7 /* Public */,
      10,    0,   75,    2, 0x06,    9 /* Public */,
      11,    1,   76,    2, 0x06,   10 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    5,   79,    2, 0x0a,   12 /* Public */,
      18,    4,   90,    2, 0x0a,   18 /* Public */,
      19,    4,   99,    2, 0x0a,   23 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    3,    4,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::QString, QMetaType::ULongLong, QMetaType::Int,   13,   14,   15,   16,   17,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::QString, QMetaType::QString,   13,   14,    7,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int, QMetaType::Int,   13,   14,   20,   21,

       0        // eod
};

Q_CONSTINIT const QMetaObject DssWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_DssWorker.offsetsAndSizes,
    qt_meta_data_DssWorker,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_DssWorker_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DssWorker, std::true_type>,
        // method 'progress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'putFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'repairFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'error'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'putFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'getFile'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'repair'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>
    >,
    nullptr
} };

void DssWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DssWorker *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->progress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 1: _t->putFinished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->getFinished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->repairFinished(); break;
        case 4: _t->error((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->putFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<quint64>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[5]))); break;
        case 6: _t->getFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4]))); break;
        case 7: _t->repair((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DssWorker::*)(int , int , const QString & );
            if (_t _q_method = &DssWorker::progress; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DssWorker::*)(const QString & );
            if (_t _q_method = &DssWorker::putFinished; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (DssWorker::*)(const QString & );
            if (_t _q_method = &DssWorker::getFinished; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (DssWorker::*)();
            if (_t _q_method = &DssWorker::repairFinished; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (DssWorker::*)(const QString & );
            if (_t _q_method = &DssWorker::error; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *DssWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DssWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_DssWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DssWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void DssWorker::progress(int _t1, int _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void DssWorker::putFinished(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void DssWorker::getFinished(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void DssWorker::repairFinished()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void DssWorker::error(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
