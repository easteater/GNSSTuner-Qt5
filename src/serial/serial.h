#ifndef SERIAL_H
#define SERIAL_H
#include <QtWidgets/qtwidgetsglobal.h>
#include <QtWidgets/qwidget.h>
#include <QList>

#if defined(Q_OS_WIN) // Windows
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#elif defined(Q_OS_MAC) // macOS
#include <QtSerialPort>
#include <QSerialPortInfo>

#endif
class Serial
{
public:
    Serial();
    QList<QSerialPortInfo>  getSerialPostList();
};

#endif // SERIAL_H

