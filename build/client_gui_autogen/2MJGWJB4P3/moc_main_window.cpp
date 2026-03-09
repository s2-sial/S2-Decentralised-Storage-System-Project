/****************************************************************************
** Meta object code from reading C++ file 'main_window.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/client/main_window.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_MainWindow_t {
    uint offsetsAndSizes[38];
    char stringdata0[11];
    char stringdata1[13];
    char stringdata2[1];
    char stringdata3[13];
    char stringdata4[16];
    char stringdata5[11];
    char stringdata6[5];
    char stringdata7[6];
    char stringdata8[8];
    char stringdata9[14];
    char stringdata10[13];
    char stringdata11[14];
    char stringdata12[11];
    char stringdata13[17];
    char stringdata14[8];
    char stringdata15[4];
    char stringdata16[5];
    char stringdata17[24];
    char stringdata18[19];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_MainWindow_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
        QT_MOC_LITERAL(0, 10),  // "MainWindow"
        QT_MOC_LITERAL(11, 12),  // "onPutClicked"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 12),  // "onGetClicked"
        QT_MOC_LITERAL(38, 15),  // "onRepairClicked"
        QT_MOC_LITERAL(54, 10),  // "onProgress"
        QT_MOC_LITERAL(65, 4),  // "done"
        QT_MOC_LITERAL(70, 5),  // "total"
        QT_MOC_LITERAL(76, 7),  // "message"
        QT_MOC_LITERAL(84, 13),  // "onPutFinished"
        QT_MOC_LITERAL(98, 12),  // "manifestPath"
        QT_MOC_LITERAL(111, 13),  // "onGetFinished"
        QT_MOC_LITERAL(125, 10),  // "outputPath"
        QT_MOC_LITERAL(136, 16),  // "onRepairFinished"
        QT_MOC_LITERAL(153, 7),  // "onError"
        QT_MOC_LITERAL(161, 3),  // "log"
        QT_MOC_LITERAL(165, 4),  // "text"
        QT_MOC_LITERAL(170, 23),  // "onDownloadButtonClicked"
        QT_MOC_LITERAL(194, 18)   // "refreshPeerMonitor"
    },
    "MainWindow",
    "onPutClicked",
    "",
    "onGetClicked",
    "onRepairClicked",
    "onProgress",
    "done",
    "total",
    "message",
    "onPutFinished",
    "manifestPath",
    "onGetFinished",
    "outputPath",
    "onRepairFinished",
    "onError",
    "log",
    "text",
    "onDownloadButtonClicked",
    "refreshPeerMonitor"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_MainWindow[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   80,    2, 0x08,    1 /* Private */,
       3,    0,   81,    2, 0x08,    2 /* Private */,
       4,    0,   82,    2, 0x08,    3 /* Private */,
       5,    3,   83,    2, 0x08,    4 /* Private */,
       9,    1,   90,    2, 0x08,    8 /* Private */,
      11,    1,   93,    2, 0x08,   10 /* Private */,
      13,    0,   96,    2, 0x08,   12 /* Private */,
      14,    1,   97,    2, 0x08,   13 /* Private */,
      15,    1,  100,    2, 0x08,   15 /* Private */,
      17,    0,  103,    2, 0x08,   17 /* Private */,
      18,    0,  104,    2, 0x08,   18 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    6,    7,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.offsetsAndSizes,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_MainWindow_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'onPutClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGetClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onRepairClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onPutFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onGetFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onRepairFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'log'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onDownloadButtonClicked'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshPeerMonitor'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onPutClicked(); break;
        case 1: _t->onGetClicked(); break;
        case 2: _t->onRepairClicked(); break;
        case 3: _t->onProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 4: _t->onPutFinished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->onGetFinished((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->onRepairFinished(); break;
        case 7: _t->onError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->log((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->onDownloadButtonClicked(); break;
        case 10: _t->refreshPeerMonitor(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
