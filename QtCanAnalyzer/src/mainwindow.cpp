/****************************************************************************
**
** This Code Base Qt example CAN Bus example, Terminal Example
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingsdialog.h"
#include "commoncan.h"
#include "canframe.h"

#include <QLabel>
#include <QMessageBox>
#include <QDesktopServices>

#include <QTimerEvent>

#include <QDebug>

#include "SerialCan.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_ui(new Ui::MainWindow),
    m_status(new QLabel),
    m_settings(new SettingsDialog),
    mStatus(0),
    timerID(0) {
    m_ui->setupUi(this);
    QWidget::setWindowTitle("CanAnalyzer Tool");
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionQuit->setEnabled(true);
    m_ui->actionConfigure->setEnabled(true);
    m_ui->actionClear->setEnabled(true);
    m_ui->statusBar->addWidget(m_status);

    SerialCan::getInstance();
    initActionsConnections();
}

MainWindow::~MainWindow() {
    SerialCan::destroyInstance();
    delete m_settings;
    delete m_ui;
}

void MainWindow::openSerialPort() {
    const SettingsDialog::Settings p = m_settings->settings();
    QString name = p.name;
    qint32 baudRate = p.baudRate;
    if (SerialCan::getInstance().serialOpen(name, baudRate)){
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));

        // Add CAN Device init
        auto canDevice = m_settings->getCanDeviceInfo();
        for (auto data : canDevice) {
            data.printInfo();
            /*
             *    | STX  | TYPE | CMD  | LSB |  MSB |  CAN(num)   | TYPE  | MODE  | BUAD | BUADDATA | CRC  | EOF
             *    | 0xF0 | 0x02 | 0x80 | 0x5 |   0  |  0x01, 0x02 |
             * N  |  0   |   1  |  2   |  3  |   4  |    5        |   6   |   7   |  8       9      |      |
             */
            SerialCan::getInstance().canDeviceInit(data);
        }

    } else {
        showStatusMessage(tr("Open error"));
    }
}

void MainWindow::closeSerialPort() {
    SerialCan::getInstance().serialClose();
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}

void MainWindow::about() {
    QMessageBox::about(this, tr("안녕하세요."),
                       tr("전자공작 까페에서 Can FD Analyzer 관련 개인취미생활을 진행하려고 만들게 되었습니다."
                          "반갑습니다. Qt 예제와 오픈소스를 참고로 진행 예정입니다."
                          "<td><a href=\"https://cafe.naver.com/circuitsmanual\">전자공작</a></td>"
                          "<a href=\"https://www.notion.so/CAN-FD-Analyzer-0ea49aa3d1664fd88a1eb4d8518fdae4\">스터디</a>"
                          ));
}

void MainWindow::writeData(const QByteArray &data) {
    SerialCan::getInstance().writePacket(data);
}

void MainWindow::clear() {
    m_ui->text_recv->clear();
    SerialCan::getInstance().clearBuffer();
}

void MainWindow::sendFrame(const QCanBusFrame &frame) const {
    QString hexvalue = QString("%1").arg(frame.frameId(), 8, 16, QLatin1Char( '0' ));
    QString data = "Send ID: " + hexvalue + " DATA: " + frame.payload().toHex();
    m_ui->text_recv->append(data);
}

void MainWindow::recvFrame(const qint8 &channel, const QCanBusFrame &frame) {
    QString hexvalue = QString("%1").arg(frame.frameId(), 8, 16, QLatin1Char( '0' ));
    if (channel == CanPacketCMD::CAN_RECV_CAN1 || channel == CanPacketCMD::CAN_SEND_CAN1) {
        QString data = "Channel: CAN1 Recv ID: " + hexvalue + " DATA: " + frame.payload().toHex();
        m_ui->text_recv->append(data);
    } else if (channel == CanPacketCMD::CAN_RECV_CAN2 || channel == CanPacketCMD::CAN_SEND_CAN2) {
        QString data = "Channel: CAN2 Recv ID: " + hexvalue + " DATA: " + frame.payload().toHex();
        m_ui->text_recv->append(data);
    }
}

//void MainWindow::recvFrame(const QCanBusFrame &frame) {
//    QString hexvalue = QString("%1").arg(frame.frameId(), 8, 16, QLatin1Char( '0' ));
//    QString data = "Recv ID: " + hexvalue + " DATA: " + frame.payload().toHex();
//    m_ui->text_recv->append(data);
//}

void MainWindow::initActionsConnections() {
    SerialCan& ptr = SerialCan::getInstance();
    connect(m_ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(m_ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(m_ui->actionConfigure, &QAction::triggered, m_settings, &SettingsDialog::show);
    connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    connect(m_ui->sendFrameBox, &SendFrameBox::sendFrame, this, &MainWindow::sendFrame);
    connect(&ptr, &SerialCan::recvCanFrame, this, &MainWindow::recvFrame);

    connect(m_ui->actionClear, &QAction::triggered, this, &MainWindow::clear);
}

char MainWindow::makeCRC(const QByteArray &data) {
    char checkSum = 0;
    for (auto cData : data) {
        checkSum ^= cData;
    }
    return checkSum;
}

void MainWindow::timerEvent(QTimerEvent *) {
//    qDebug() << __FUNCTION__ << "write total" << mDataArray.size();

    for (const auto& data : mDataArray) {
        SerialCan::getInstance().writePacket(data);
    }
}

void MainWindow::showStatusMessage(const QString &message) {
    m_status->setText(message);
}

void MainWindow::on_btn_send_clicked() {
    int address = 0x44332211;

    QString text = m_ui->input_text->text();
    QByteArray data = QByteArray::fromHex(text.toLatin1()); // RawData

    CanFrame frame;
    frame.GenerateWriteData(address, data);
    SerialCan::getInstance().writePacket(data);

/*
    if (timerID == 0) {
        mDataArray.emplace_back(data);
        mDataArray.emplace_back(data);
        mDataArray.emplace_back(data);
        mDataArray.emplace_back(data);
        mDataArray.emplace_back(data);
        timerID = startTimer(50);
    } else {
        killTimer(timerID);
        timerID = 0;
        mDataArray.clear();
        SerialCan::getInstance().clearBuffer();
    }
*/
    m_ui->text_recv->append(QString(data.toHex()).toUpper());
}
