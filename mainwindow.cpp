#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QCloseEvent>
#include <QThread>

#include "mainwindow.h"
#include "ui_mainwindow.h"


void set_iface_mode(const QString& iface, const QString& mode)
{
    // find better solution
    system(QString("ifconfig " + iface + " down").toStdString().c_str());
    system(QString("iwconfig " + iface + " mode " + mode).toStdString().c_str());
    system(QString("ifconfig " + iface + " up").toStdString().c_str());
}


MainWindow::MainWindow(const QString& iface, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mIface(iface)
{
    ui->setupUi(this);
    set_iface_mode(mIface, "monitor");
    QThread::msleep(500);

    mWS = new WifiSniffer(mIface, this);

    connect(mWS, SIGNAL(accessPointAdded(AccessPoint)), this, SLOT(onAPAdded(AccessPoint)));
    connect(mWS, SIGNAL(assocStationAdded(AssocStation)), this, SLOT(onAssocStatAdded(AssocStation)));
    connect(mWS, SIGNAL(channelChanged(int)), this, SLOT(onChannelChanged(int)));

    mAPModel = new QStandardItemModel();
    mAPModel->setHorizontalHeaderItem(0, new QStandardItem("SSID"));
    mAPModel->setHorizontalHeaderItem(1, new QStandardItem("BSSID"));
    mAPModel->setHorizontalHeaderItem(2, new QStandardItem("Vendor"));

    mAssocModel = new QStandardItemModel();
    mAssocModel->setHorizontalHeaderItem(0, new QStandardItem("AP SSID"));
    mAssocModel->setHorizontalHeaderItem(1, new QStandardItem("AP BSSID"));
    mAssocModel->setHorizontalHeaderItem(2, new QStandardItem("Client MAC"));
    mAssocModel->setHorizontalHeaderItem(3, new QStandardItem("Client Vendor"));

    ui->ap_table->setModel(mAPModel);
    ui->ap_table->setColumnWidth(0, 250);
    ui->ap_table->setColumnWidth(1, 160);
    ui->ap_table->setColumnWidth(2, 280);

    ui->as_table->setModel(mAssocModel);
    ui->as_table->setColumnWidth(0, 250);
    ui->as_table->setColumnWidth(1, 160);
    ui->as_table->setColumnWidth(2, 160);
    ui->as_table->setColumnWidth(3, 280);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onAPAdded(const AccessPoint &ap)
{
    int row = mAPModel->rowCount();
    mAPModel->setItem(row, 0, new QStandardItem(ap.ssid));
    mAPModel->setItem(row, 1, new QStandardItem(ap.bssid));
    mAPModel->setItem(row, 2, new QStandardItem(ap.vendor));
}

void MainWindow::onAssocStatAdded(const AssocStation &as)
{
    int row = mAssocModel->rowCount();
    mAssocModel->setItem(row, 0, new QStandardItem(as.ap.ssid));
    mAssocModel->setItem(row, 1, new QStandardItem(as.ap.bssid));
    mAssocModel->setItem(row, 2, new QStandardItem(as.mac));
    mAssocModel->setItem(row, 3, new QStandardItem(as.vendor));
}

void MainWindow::onChannelChanged(int channel)
{
    ui->statusBar->showMessage("Channel " + QString::number(channel));
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    if (QMessageBox::question(this, "Exit?", "Are you sure want to exit?")
            == QMessageBox::Yes) {

        set_iface_mode(mIface, "managed");
    }
    else {
        ev->ignore();
    }

}

void MainWindow::on_saveListAsBtn_clicked()
{
    QString fname = QFileDialog::getSaveFileName(this, "Save List to file as");
    if (fname.isEmpty())
        return;

    QVector<AccessPoint> aps = mWS->getAPList();
    QVector<AssocStation> assoc = mWS->getAssocStationList();

    QFile file(fname);
    if (file.open(QIODevice::WriteOnly)) {
        file.write("==============[    Access Point    ]==============\n");
        for (AccessPoint ap : aps) {
            QString str = ap.ssid + "\t" + ap.bssid + "\t" + ap.vendor + "\n";
            file.write(str.toStdString().c_str());
        }

        file.write("\n\n==============[ Associated Stations ]==============\n");
        for (AssocStation as : assoc) {
            QString str = as.ap.ssid + "\t" + as.ap.bssid + "\t" + as.mac + "\t" + as.vendor + "\n";
            file.write(str.toStdString().c_str());
        }

        file.close();
    }
}