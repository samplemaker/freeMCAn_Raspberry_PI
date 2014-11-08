/** \file mainwindow.cpp
* \brief A GUI interface to control the kernel firmware
*
* \author Copyright (C) 2014 samplemaker
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1
* of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301 USA
*
* @{
*/

#include <QMessageBox>
#include <QFileDialog>
#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>
#include <QShortcut>
#include "MainWindow.h"
#include "ui_MainWindow.h"


/** setup the GUI API */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->actionExit,SIGNAL( triggered() ), qApp, SLOT( quit() ));
    connect(ui->actionAboutThis, SIGNAL(triggered()), this, SLOT(onActionAboutThis()) );
    QShortcut *startStopSc = new QShortcut(QKeySequence("Ctrl+s"), this);
    connect(startStopSc, SIGNAL( activated() ), this, SLOT( on_pushButton_clicked() ));
    QShortcut *exitSc = new QShortcut(QKeySequence("Ctrl+x"), this );
    connect(exitSc, SIGNAL( activated() ), qApp, SLOT( quit() ));

    msrmntRunning = 0;

    mParser = new Parser();

    port = new QcharDev();
    if (port->isOpen())
        port->close();
    /* unbuffered implies qint64 maxSize in QcharDev::readData() is used */
    port->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    if (port->isOpen()){
        connect(port, SIGNAL(readyRead()), this, SLOT(onDataAvailable()));
        statusBar()->showMessage("Connection established",0);
        ui->pushButton->setText("START");
        connect(mParser, SIGNAL( parserDataReady(const payloadData *) ),
                this, SLOT( onParserDataAvailable(const payloadData *) ));
    }else{
        disconnect(port, SIGNAL(readyRead()), this, SLOT(onDataAvailable()));
        statusBar()->showMessage("Error - cannot open device",0);
        ui->pushButton->setText("NAN");
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}


/** proceed the raw character stream from the character device */
void MainWindow::onDataAvailable()
{
    /* qint64 len = port->bytesAvailable();
     qWarning() << "bytes minimum available:" << len; */

    QByteArray dataArray = port->readAll();
    QString dataText(dataArray);
    const int ret = mParser->doParse(dataArray.constData(), dataArray.size());
    if (ret < 0)
        statusBar()->showMessage("error parsing", 0);
    else
        statusBar()->clearMessage();

    /* display the raw characters in a textlable */
    ui->labelChars->setText( dataText.replace("\n"," ") );

 /*
    char out[50];
    qint64 ret_val = port->read(out, sizeof(out));
    QByteArray dataArray = QByteArray::fromRawData(out, strlen(out));
    QString dataText(dataArray);
    qWarning() << "bytes read:" << ret_val;
    qWarning() << "data:" << dataText;
*/
}


/** parser finished. do something with the parsed data */
void MainWindow::onParserDataAvailable(const payloadData *data)
{
    ui->labelAccuCounts->setText(QString::number(data->accuCounts));

    QString dispTime = QString("Elapsed time: %1 sec").arg(data->kernelTime / 1000);
    ui->labelKernelTime ->setText(dispTime);

    totalCounts += data->accuCounts;
    QString dispTotCnts = QString("Total Counts: %1").arg(totalCounts);
    ui->labelTotalCounts ->setText(dispTotCnts);
}


void MainWindow::on_pushButton_clicked()
{
    if (port->isOpen()){
        if (msrmntRunning){
            port->stopMsrmnt();
            ui->pushButton->setText("Start");
            msrmntRunning = 0;
            totalCounts = 0;
        }else{
            port->startMsrmnt();
            ui->pushButton->setText("Stop");
            msrmntRunning = 1;
        }
    }
}


void MainWindow::onActionAboutThis()
{
    QMessageBox::about(this, tr("About application"),
                 tr("<p><b>Hotkeys:</b><br>" \
                    "<p><b>Exit:</b> Ctrl-x" \
                    "<p><b>Start/Stop:</b> Ctrl-s <br>"));
}
