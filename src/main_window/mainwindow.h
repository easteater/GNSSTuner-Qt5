#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QScrollBar>

#include "signaltonoiseratio.h"
#include "locateinformation.h"
#include "gnssparser.h"
#include "tool.h"
#include "gpsmoduleconfig.h"

#if defined(Q_OS_WIN) // Windows
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#elif defined(Q_OS_MAC) // macOS
#include <QtSerialPort>
#endif

/**

  第一次使用QT写功能性程序, 逻辑安排,目录结构等较欠缺,望海涵

 * @brief The GPSInfo struct
 */



QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void refreshSerialPort();
    void log(QString logInfo);
    void reConnectserialPort();
    void on_commandLinkButton_clicked();
    void serialPortRecvDataCallback();
    void on_pushButton_2_clicked();
    void on_pushButton_clicked();
    void serialOnBreak(QSerialPort::SerialPortError error);
    void on_openSerialPortButton_clicked();
    void on_showSNR_stateChanged(int arg1);
    void on_showLocate_stateChanged(int arg1);
    void parseSerialPortData();

    void on_clearRecvDataButton_clicked();
    //渲染相关的
    void   refreshViewGSV();
    void   refreshViewLocate();

    void on_saveRecvToFileButton_clicked();
    void serialPortIdleInterrupt();

    void on_configPanelCheckBox_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
    QTimer *refreshSerialPortTimer ;
    QTimer *reConnectserialPortTimer ;
   // QTimer *parseSerialPortDataTimer ;
    QTimer *serialPortIdleInterruptTimer ;//串口中断计时器


    QSerialPort *serialPort;
    long serialPortRecvTextBoxLastSliderPostion;
    QString recvBuffer;
    QList<QString> bufferList;
    SignaltoNoiseRatio signaltoNoiseRatio;
    LocateInformation locateInformation;
    long bufferSize = 0;
    GNSSParser gnssParser;;
    Tool tool;
    GpsModuleConfig gpsModuleConfig;



};
#endif // MAINWINDOW_H
