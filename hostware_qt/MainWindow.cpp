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

    mFifo = new Fifo(MAX_DATAPOINTS);

    connect(ui->actionExit,SIGNAL( triggered() ), qApp, SLOT( quit() ));
    connect(ui->actionAboutThis, SIGNAL(triggered()), this, SLOT(onActionAboutThis()) );
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(onActionSaveFileAs()));
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
        /* stop if firmware did already run before start of hostware */
        port->stopMsrmnt();
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

    /* display the raw characters in a textlabel */
    ui->labelChars->setText( dataText.replace("\n"," ") );
}


/** parser finished. do something with the parsed data */
void MainWindow::onParserDataAvailable(const payloadData *data)
{
    QString dispTime = QString("Elapsed time: %1 sec").arg(data->kernelTime / 1000);
    ui->labelKernelTime ->setText(dispTime);

    totalCounts += data->accuCounts;
    QString dispTotCnts = QString("Total Counts: %1").arg(totalCounts);
    ui->labelTotalCounts ->setText(dispTotCnts);

    QString dispAccuCounts = QString("Counts per interval: %1").arg(data->accuCounts);
    ui->labelAccuCounts->setText(dispAccuCounts);

    double cpm = 1000.0*60.0*(double)(totalCounts)/(double)(data->kernelTime);
    QString dispCPM = "Counts per minute: "+QString::number(cpm, 'f', 1)+" avrg";
    ui->labelCPM->setText(dispCPM);

    mFifo->writeHead(data);
    const int max_xticks = 60;
    int recLen =  mFifo->copyLastN(max_xticks, dataBuffer);
    double tmp[MAX_DATAPOINTS];
    for (int i = 0; i < recLen; i++)
        /* display in counts per minute */
        tmp[i] = (60.0/(double)(timerCountsPerSample))*(double)(dataBuffer[i].accuCounts);
    ui->paintArea->drawCurve(tmp, recLen, max_xticks);
}


void MainWindow::on_pushButton_clicked()
{
    if (port->isOpen()){
        if (msrmntRunning){
            port->stopMsrmnt();
            ui->pushButton->setText("Start");
            msrmntRunning = 0;
            totalCounts = 0;
            ui->comboBox->setEnabled(true);
        }else{
            timerCountsPerSample = ui->comboBox->currentText().toInt();
            ui->comboBox->setDisabled(true);
            port->setTimerCountsPerSample(&timerCountsPerSample);
            port->startMsrmnt();
            ui->pushButton->setText("Stop");
            mFifo->reset();
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


void MainWindow::onActionSaveFileAs()
{
    QString fileName = QFileDialog::getSaveFileName(
                this,
                "Save as",
                "./",
                "Text Files (*.txt);;All Files (*.*)");
    if (!fileName.isEmpty()){
        fileToSave = fileName;
        saveFile();
    }
}


void MainWindow::saveFile()
{
    QFile file(fileToSave);
    if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
        QTextStream outPut(&file);
        int recLen =  mFifo->copyLastN(MAX_DATAPOINTS, dataBuffer);
        for (int i = 0; i < recLen; i++)
            outPut << i + 1 << ";" << dataBuffer[i].timerCounts << endl;
        file.close();
    }else{
        QMessageBox::warning(
                    this,
                    "Save as",
                    tr("Cannot write file %1.\nError: %2")
                    .arg(fileToSave)
                    .arg(file.errorString()));
    }
}
