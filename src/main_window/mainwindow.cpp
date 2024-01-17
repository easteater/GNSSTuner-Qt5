/***
 * 该类主要负责主窗口交互动作 和操作逻辑
 *
 * */
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{

    refreshSerialPortTimer = new QTimer(this);
    //reConnectserialPortTimer = new QTimer(this);
    //parseSerialPortDataTimer = new QTimer(this);
    serialPortIdleInterruptTimer= new QTimer(this);

    serialPort = new QSerialPort();


    connect(refreshSerialPortTimer, &QTimer::timeout, this, &MainWindow::refreshSerialPort);
    //connect(reConnectserialPortTimer, &QTimer::timeout, this, &MainWindow::reConnectserialPort);
    //connect(parseSerialPortDataTimer, &QTimer::timeout, this, &MainWindow::parseSerialPortData);
    connect(serialPortIdleInterruptTimer, &QTimer::timeout, this, &MainWindow::serialPortIdleInterrupt);

    connect(serialPort, &QSerialPort::readyRead, this, &MainWindow::serialPortRecvDataCallback);
    connect(serialPort, &QSerialPort::errorOccurred, this, &MainWindow::serialOnBreak);

    ui->setupUi(this);

    ui->serialPortRecvTextBox->setWordWrapMode(QTextOption::NoWrap);
    ui->serialPortRecvTextBox->setLineWrapMode(QPlainTextEdit::NoWrap);

    refreshSerialPortTimer->setInterval(1000);
    refreshSerialPortTimer->start();

    //reConnectserialPortTimer->setInterval(500);
    //parseSerialPortDataTimer->setInterval(100);
    serialPortIdleInterruptTimer->setInterval(100);//空闲时间,自由设置
    // 列出一次串口列表
  //  refreshSerialPort();


    signaltoNoiseRatio.show();
    signaltoNoiseRatio.setWindowFlags(Qt::WindowStaysOnTopHint);
    locateInformation.show();
    locateInformation.setWindowFlags(Qt::WindowStaysOnTopHint);

    //把串口指针直接传递给设置模块
    gpsModuleConfig.serialPort = this->serialPort;



}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::log(QString logInfo)
{

    ui->logTxtBox->appendPlainText(tool.getCurrentDateTime()  + logInfo);
}

void MainWindow::refreshSerialPort()
{

    // 获取最新的串口列表
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    //串口列表没有发生更新
    if (lastPortList.size() == ports.size()) {
        qDebug()<<"端口数量一致,跳过更新";
        //检查一下端口状态如果非人为关闭了,需要打开
        if(userPortAction && !serialPort->isOpen()) {
            openSerialPort();
        }

        return;
    }

    qDebug()<<"端口发现更新" << lastPortList.size()<<"<>" <<ports.size();

    recordLastPort= false;
    // 清空QComboBox的内容 注意会触发lastPort的更新
    ui->serialPortComboBox->clear();

    // 将串口列表添加到QComboBox中
    for (const QSerialPortInfo &portInfo : ports) {
       ui->serialPortComboBox->addItem(portInfo.portName());
    }
    lastPortList = ports;
    recordLastPort = true;
     //最后一次选择的端口 选中这个默认
    if ("" != lastSelectedQSerialPort) {
        int index = ui->serialPortComboBox->findText(lastSelectedQSerialPort);
        qDebug()<<"lastSelectedQSerialPort 查询结果"<<index;
        if (index != -1) {
            ui->serialPortComboBox->setCurrentIndex(index);
            //如果用户手动打开,未关闭,则自动打开
            if(userPortAction && !serialPort->isOpen()) {
                openSerialPort();
            }
        }
    }

}

void MainWindow::reConnectserialPort() {
     on_openSerialPortButton_clicked();
}

//串口空闲中断
void MainWindow::serialPortIdleInterrupt() {
    //qDebug()<<"serialPortIdleInterruptserialPortIdleInterruptserialPortIdleInterruptserialPortIdleInterruptserialPortIdleInterrupt"<<endl;
    serialPortIdleInterruptTimer->stop();//请掉,避免反复触发
    parseSerialPortData() ;//解析
}

/**

  把所有串口中缓存中的代码,格式化成一条条的QString数组
 * @brief MainWindow::formatSerialPortData
 * @return
 */

void MainWindow::parseSerialPortData() {


    // 清掉上次数据
    gnssParser.clearBuffer();
    int endIndex = recvBuffer.indexOf("\r\n");
    while (endIndex != -1)
    {
        // 提取完整的字符串
       QString completeData = recvBuffer.left(endIndex);
       bufferList.append(completeData);
       // 删除已提取的数据及结束标记
       recvBuffer.remove(0, endIndex + 2);
       endIndex = recvBuffer.indexOf("\r\n");

       serialPortRecvTextBoxLastSliderPostion =  ui->serialPortRecvTextBox->verticalScrollBar()->sliderPosition();
       ui->serialPortRecvTextBox->appendPlainText(completeData);
       if (ui->authScrollCheckBox->isChecked()) {
          ui->serialPortRecvTextBox->moveCursor(QTextCursor::End);
       }
       //记录buffer大小
       bufferSize+=completeData.size();
       ui->recvBufferSizeLabel->setText(QString::number(bufferSize));
       //解析最新一条和据
       gnssParser.parseNMea0180Txt(completeData);
    }

    refreshViewGSV();
    refreshViewLocate();



}
QString emptyStringToJsEmptyString(QString old) {
    if (old == "") return "\"\"";
    return "\""+old+"\"";
}
void  MainWindow::refreshViewLocate() {
    QString signalRunJsString =
            R"(
               updateInfo({latitude}, {longitude}, {altitude}, {utcDate}, {utcTime}, {speed}, {isAlready})
            )";
    RMC rmc = gnssParser.getGNSSRuntimeData().rmc;
    QList<int> seriesData;
    QList<QString> xAxisData;

    //直接替换模版
    signalRunJsString = signalRunJsString.replace("{isAlready}", gnssParser.isReady() ? "true" : "false" );
    signalRunJsString = signalRunJsString.replace("{latitude}", emptyStringToJsEmptyString(rmc.latitude)   );
    signalRunJsString = signalRunJsString.replace("{longitude}", emptyStringToJsEmptyString(rmc.longitude) );
    signalRunJsString = signalRunJsString.replace("{utcDate}", emptyStringToJsEmptyString(rmc.utcDate ));
    signalRunJsString = signalRunJsString.replace("{utcTime}", emptyStringToJsEmptyString(rmc.utcTime ));
    signalRunJsString = signalRunJsString.replace("{speed}", emptyStringToJsEmptyString(rmc.groundSpeed ));
    signalRunJsString = signalRunJsString.replace("{altitude}", emptyStringToJsEmptyString(gnssParser.getGNSSRuntimeData().gga.altitude));

    //qDebug()<<"RunJS:"<<signalRunJsString<<endl;
    log("RunJS" + signalRunJsString);
    locateInformation.webView->page()->runJavaScript(signalRunJsString);


}
void  MainWindow::refreshViewGSV() {
    //更新信噪比 生成触发命令
    QString signalRunJsString =
            R"(
            seriesData = [${seriesData}];
            xAxisData = [${xAxisData}];
            isAlready = [${isAlready}];
            viewTotal = ${viewTotal};
            usedTotal = ${usedTotal};
            setChatValue(seriesData, xAxisData, isAlready, viewTotal,usedTotal)

                                 )";
    GSV gsv = gnssParser.getGNSSRuntimeData().gsv;
    QString seriesDataString = "";
    QString xAxisDataString = "";
    QString readyListString = "";


    //计算出所有卫星编号跟所属类型
    for (const auto& key :  gsv.satellites.keys()) {
        SatelliteInfo sateLliteInfo = gsv.satellites.value(key);
        seriesDataString += QString::number(sateLliteInfo.snr) + ",";
        xAxisDataString += "\"" + gnssParser.getSatelliteType(sateLliteInfo.prn) + "\",";
        readyListString += (sateLliteInfo.isValid  ? "true" : "false") ;
        readyListString += ",";

    }
    //去掉拼装的最后一个,
    seriesDataString.chop(1);
    xAxisDataString.chop(1);
    readyListString.chop(1);


    //模版替换格式化
    signalRunJsString = signalRunJsString.replace("${usedTotal}",QString::number(gsv.totalUsed));
    signalRunJsString = signalRunJsString.replace("${viewTotal}",QString::number(gsv.totalSatellites));
    signalRunJsString = signalRunJsString.replace("${seriesData}",seriesDataString);
    signalRunJsString = signalRunJsString.replace("${xAxisData}",xAxisDataString);
    signalRunJsString = signalRunJsString.replace("${isAlready}",readyListString);
    //qDebug()<<"RunJS:"<<signalRunJsString<<endl;

    log("RunJS" + signalRunJsString);

    signaltoNoiseRatio.webView->page()->runJavaScript(signalRunJsString);
}

void MainWindow::serialPortRecvDataCallback() {
    QByteArray data = serialPort->readAll();
    recvBuffer.append(data);
    //空闲中断计时器开始
    serialPortIdleInterruptTimer->stop();
    serialPortIdleInterruptTimer->start();

   //qDebug() << "recvBuffer len  " << recvBuffer.size() <<recvBuffer;
   //qDebug() << "bufferList len " << bufferList.size();


}
void MainWindow::on_commandLinkButton_clicked()
{
    refreshSerialPort();
}


void MainWindow::on_pushButton_2_clicked()
{
    if (!serialPort->isOpen()) {
       qDebug() <<ui->serialPortComboBox->currentText() << " is Not Open";
    }
    on_openSerialPortButton_clicked();
    if (!serialPort->isOpen()) {
       qDebug() <<ui->serialPortComboBox->currentText() << " is Not Open";

       return;
    }

    serialPort->write(ui->serialPortSendTextBox->toPlainText().toLatin1());

}



void MainWindow::on_pushButton_clicked()
{

    gnssParser.clearBuffer();
    gnssParser.parseNMea0180Txt("$GNGSA,A,3,03,09,16,24,28,38,39,,,,,,2.5,0.9,2.3,4*3B");
    gnssParser.parseNMea0180Txt("$GPGSV,2,1,08,05,36,097,27,13,40,044,24,15,,,22,18,,,31,0*6A");
    gnssParser.parseNMea0180Txt("$GPGSV,2,2,08,23,34,309,23,24,49,173,31,195,33,174,28,199,,,25,0*5F");
    gnssParser.parseNMea0180Txt("$BDGSV,3,1,10,01,,,32,02,,,31,03,49,197,37,09,31,208,40,0*77");
    gnssParser.parseNMea0180Txt("$BDGSV,3,2,10,16,53,203,45,24,36,115,37,28,39,258,41,38,77,092,25,0*7A");
    gnssParser.parseNMea0180Txt("$BDGSV,3,3,10,39,61,206,42,42,,,29,0*47");
    gnssParser.parseNMea0180Txt("$GNRMC,115612.000,A,3604.42736,N,12024.40085,E,0.00,0.00,010623,,,A,V*05");
    refreshViewGSV();
    refreshViewLocate();

}


void MainWindow::serialOnBreak(QSerialPort::SerialPortError error)
{
    qDebug() << "serialOnBreak  " << error;



    if (error != QSerialPort::NoError) {
       if (serialPort->isOpen()) {
           serialPort->close();
       }
       ui->openSerialPortButton->setText("打开串口");
    }

    if (error == QSerialPort::ResourceError || error == QSerialPort::DeviceNotFoundError || error == QSerialPort::PermissionError) {

       qDebug() << "ResourceError DeviceNotFoundError PermissionError   !,,,,,now reConnect";
    }





}

void MainWindow::openSerialPort(){

    // 设置串口名称和参数
    serialPort->setPortName(ui->serialPortComboBox->currentText());
    // serialPort->setBaudRate(QSerialPort::Baud9600);
    // serialPort->setDataBits(QSerialPort::Data8);
    // serialPort->setParity(QSerialPort::NoParity);
    // serialPort->setStopBits(QSerialPort::OneStop);


    // 从控件中读取串口参数并设置到 QSerialPort
    serialPort->setBaudRate(static_cast<QSerialPort::BaudRate>(ui->baudRateComboBox->currentText().toInt()));
    serialPort->setDataBits(static_cast<QSerialPort::DataBits>(ui->dataBitsComboBox->currentText().toInt()));
    serialPort->setParity(static_cast<QSerialPort::Parity>(ui->parityComboBox->currentText().toInt()));
    serialPort->setStopBits(static_cast<QSerialPort::StopBits>(ui->stopBitsComboBox->currentText().toInt()));

    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    // 打开串口
    if(!serialPort->open(QIODevice::ReadWrite)) {
        qDebug() <<ui->serialPortComboBox->currentText() << "Failed to open serial port.";
        return;
    }

    qDebug() << ui->serialPortComboBox->currentText() << "Serial port opened successfully.";

    userPortAction = true;
    ui->openSerialPortButton->setText("关闭串口");
}
void MainWindow::on_openSerialPortButton_clicked()
{
    if (ui->openSerialPortButton->text() == "打开串口") {
        openSerialPort();
    } else {
       qDebug() << "Port status " <<serialPort->isOpen() << "Serial port opened successfully.";
       serialPort->close();
       userPortAction = false;
       ui->openSerialPortButton->setText("打开串口");

    }
}


void MainWindow::on_showSNR_stateChanged(int arg1)
{
    if (0 == arg1) {
        signaltoNoiseRatio.close();
    } else {
        signaltoNoiseRatio.webView->reload();
        signaltoNoiseRatio.show();
    }
}

void MainWindow::on_showLocate_stateChanged(int arg1)
{
    if (0 == arg1) {
        locateInformation.close();
    } else {
        locateInformation.webView->reload();
        locateInformation.show();
    }
}

void MainWindow::on_clearRecvDataButton_clicked()
{
    bufferSize = 0;
    bufferList.clear();
    ui->serialPortRecvTextBox->clear();
}

void MainWindow::on_saveRecvToFileButton_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(nullptr, "保存文件", "", "GNSS调试数据 (*.txt)");
    if (!filePath.isEmpty()) {
        // 创建文件对象
        QFile file(filePath);
        qDebug()<<"保存文件:"<<filePath<<endl;
        // 打开文件
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            // 写入内容
            QTextStream stream(&file);
            stream << ui->serialPortRecvTextBox->toPlainText();
            // 关闭文件
            file.close();
        } else {
            // 文件打开失败
            qDebug()<<"打开文件失败,无法保存!"<<endl;
            // 处理错误
        }
    }
}

void MainWindow::on_configPanelCheckBox_stateChanged(int arg1)
{
    if (0 == arg1) {
        gpsModuleConfig.close();
    } else {
        gpsModuleConfig.show();
    }
}


void MainWindow::on_serialPortComboBox_currentTextChanged(const QString &arg1)
{
    if (recordLastPort) {
        qDebug()<<"lastSelectedQSerialPort : "<<arg1;
        lastSelectedQSerialPort = arg1;
    }
}

