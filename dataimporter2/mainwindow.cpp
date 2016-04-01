#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "manager.h"
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_mgr(new Manager(this))
{
    ui->setupUi(this);
    ui->barsToLoadCheckbox->hide();
    ui->datewiseCheckbox->hide();
    ui->fromGroupbox->hide();
    ui->toGroupbox->hide();
    ui->toolBox->setCurrentIndex(0);
    ui->mySqlCheckBox->setChecked(true);
    ui->mysqlGroupBox->setEnabled(true);
    ui->mySqlCheckBox->hide();
    ui->hdf5CheckBox->setChecked(true);
    ui->hdf5GroupBox->setEnabled(true);
    ui->hdf5CheckBox->hide();

    ui->sqlSymbolTableNameLineEdit->setText("stock_companyInfo");

    connect(m_mgr, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(m_mgr, SIGNAL(downloading(QString)), this, SLOT(onDownloading(QString)));

    readSettings();
}


MainWindow::~MainWindow()
{
    delete ui;
}


// IB_FEED SIGNALS

void MainWindow::on_ibHostLineEdit_textChanged(const QString &arg1)
{
    QSettings m_settings;
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("ib/hostName", QVariant(arg1));
    m_settings.sync();
//    m_settings.endGroup();
}

void MainWindow::on_ibPortSpinBox_valueChanged(int arg1)
{
    QSettings m_settings;
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("ib/port", QVariant(arg1));
    m_settings.sync();
//    m_settings.endGroup();
}

void MainWindow::on_loginButton_clicked()
{
//    qDebug() << "on_loginButton_clicked()";
    ui->statusBar->showMessage("Logging in to IB server.. please wait:");
    m_mgr->login(ui->ibHostLineEdit->text(), ui->ibPortSpinBox->value(), 1);
    m_logoutButtonClicked = false;
}

void MainWindow::on_logoutButton_clicked()
{
    m_mgr->logout();
    ui->logoutButton->setEnabled(false);
    ui->loginButton->setEnabled(true);
    m_logoutButtonClicked = true;
}

void MainWindow::on_reconnectCheckbox_toggled(bool checked)
{
    QSettings m_settings;
    m_mgr->setReconnectOnFailure(checked);
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("ib/reconnectChecked", QVariant(checked));
    m_settings.sync();
//    m_settings.endGroup();
    m_reconnectOnFailure = checked;
}


// DATABASE SIGNALS

void MainWindow::on_mySqlCheckBox_toggled(bool checked)
{
    QSettings s;
//    s.beginGroup(m_sqlSymbolTableName);
    s.setValue("sql/checkBoxToggled", QVariant(checked));
    s.sync();
//    s.endGroup();

    ui->mysqlGroupBox->setEnabled(checked);
}

void MainWindow::on_hdf5CheckBox_toggled(bool checked)
{
    QSettings s;
//    s.beginGroup(m_sqlSymbolTableName);
    s.setValue("hdf5/checkBoxToggled", QVariant(checked));
    s.sync();
//    s.endGroup();

    ui->hdf5GroupBox->setEnabled(checked);
}

void MainWindow::on_sqlHostNameLineEdit_textChanged(const QString &arg1)
{
    QSettings m_settings;
    m_mgr->setSqlHostName(arg1);
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("sql/hostName", QVariant(arg1));
    m_settings.sync();
//    m_settings.endGroup();
}

void MainWindow::on_sqlPortSpinBox_valueChanged(int arg1)
{
    QSettings m_settings;
    m_mgr->setSqlPort(arg1);
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("sql/port", QVariant(arg1));
    m_settings.sync();
//    m_settings.endGroup();
}

void MainWindow::on_sqlUserNameLineEdit_textChanged(const QString &arg1)
{
    QSettings m_settings;
    m_mgr->setSqlUserName(arg1);
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("sql/userName", QVariant(arg1));
    m_settings.sync();
//    m_settings.endGroup();
}

void MainWindow::on_sqlPasswordLineEdit_textChanged(const QString &arg1)
{
    QSettings m_settings;
    m_mgr->setSqlPassword(arg1);
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("sql/password", QVariant(arg1));
    m_settings.sync();
//    m_settings.endGroup();
}

void MainWindow::on_sqlDatabaseNameLineEdit_textChanged(const QString &arg1)
{
    QSettings m_settings;
    m_mgr->setSqlDatabaseName(arg1);
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("sql/databaseName", QVariant(arg1));
    m_settings.sync();
//    m_settings.endGroup();
}

void MainWindow::on_sqlSymbolTableNameLineEdit_textChanged(const QString &arg1)
{

}

void MainWindow::on_sqlOutputDatabaseLineEdit_textChanged(const QString &arg1)
{
    m_mgr->setSqlOutputDatabaseName(arg1);

    QSettings s;
    s.beginGroup("sql");
    s.setValue("outputDatabaseName", QVariant(arg1));
    s.endGroup();
}

void MainWindow::on_loadSymbolTablePushButton_clicked()
{
    m_mgr->setSqlSymbolTableName(m_sqlSymbolTableName);

    if (m_mgr->initializeSqlDatabase()) {
        ui->tableView->setModel((QAbstractItemModel*)m_mgr->symbolTableModel());
        ui->tableView->show();
    }
    else
        qDebug() << "INIT SQL FAILED";
}

void MainWindow::on_hdf5OutputFolderLineEdit_textChanged(const QString &arg1)
{
    QSettings s;
//    s.beginGroup(m_sqlSymbolTableName);
    s.setValue("hdf5/outputFolderLineEdit", QVariant(arg1));
    s.sync();
//    s.endGroup();

    m_mgr->setHdf5OutputFolderPath(arg1);
    ui->hdf5OutputFolderLineEdit->setText(arg1);
}

void MainWindow::on_hdf5OutputFileToolButton_clicked()
{
    QSettings m_settings;
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        m_mgr->setHdf5OutputFolderPath(dir);
        ui->hdf5OutputFolderLineEdit->setText(dir);
//        m_settings.beginGroup(m_sqlSymbolTableName);
        m_settings.setValue("hdf5/outputFolderPath", QVariant(dir));
        m_settings.sync();
//        m_settings.endGroup();
    }
}

// DOWNLOAD SIGNALS

void MainWindow::on_timeFrameComboBox_currentIndexChanged(int index)
{
    m_mgr->setTimeFrame((TimeFrame)index);
    QSettings s;
//    s.beginGroup(m_sqlSymbolTableName);
    s.setValue("download/timeFrameComboBox", QVariant(index));
    s.sync();
//    s.endGroup();
}

void MainWindow::on_numberOfMonthsSpinBox_valueChanged(int arg1)
{
    m_mgr->setNumberOfMonths(arg1);

    QSettings s;
//    s.beginGroup(m_sqlSymbolTableName);
    s.setValue("download/numberOfMonths", QVariant(arg1));
    s.sync();
//    s.endGroup();
}

void MainWindow::on_startButton_clicked()
{
    ui->autoDownloadCheckbox->setEnabled(false);
    if (m_autoDownload)
        ui->statusBar->showMessage("Connected: Autodownload enabled: Starting download:");
    else
        ui->statusBar->showMessage("Connected: Starting download:");

    ui->startButton->setEnabled(false);
    ui->stopButton->setEnabled(true);

    m_mgr->downloadQuotes();


    if (!m_autoDownload) {
        ui->statusBar->showMessage("Connected: Download complete");
    }
    else {
//        m_autoDownloadTimer->start(1000);
        ui->statusBar->showMessage("Connected: Download complete: Autodownload enabled");
    }
    if (!m_autoDownload) {
        ui->startButton->setEnabled(true);
        ui->stopButton->setEnabled(false);
    }
}

void MainWindow::on_stopButton_clicked()
{
//    if (m_autoDownload && m_autoDownloadTimer->isActive())
//        m_autoDownloadTimer->stop();
    m_mgr->setStopButtonClicked(true);
    ui->statusBar->showMessage("Connected: All Downloading stopped.. PLEASE wait..");
    ui->startButton->setEnabled(true);
    ui->stopButton->setEnabled(false);
    ui->autoDownloadCheckbox->setEnabled(true);
}

void MainWindow::on_autoDownloadCheckbox_toggled(bool checked)
{
    QSettings m_settings;
    m_mgr->setAutoDownloadEnabled(checked);
//    m_settings.beginGroup(m_sqlSymbolTableName);
    m_settings.setValue("download/autoDownloadChecked", QVariant(checked));
    m_settings.sync();
//    m_settings.endGroup();
    m_autoDownload = checked;
    if (checked) {
        ui->statusBar->showMessage("Connected: Autodownload enabled");
    }
    else {
        ui->statusBar->showMessage("Connected:");
//        if (m_autoDownloadTimer->isActive())
//            m_autoDownloadTimer->stop();
    }
}


// PRIVATE METHODS

void MainWindow::readSettings()
{
    qDebug() << "In readSettings()";

    QSettings m_settings;

    QString sval;
    bool bval;
    QStringList slval;
    int ival;


//    m_settings.clear();


    sval = m_settings.value("sql/symbolTableName", QVariant("stock_companyInfo")).toString();
    qDebug() << "SVAL2:" << sval;
    m_mgr->setSqlSymbolTableName(sval);
    ui->sqlSymbolTableNameLineEdit->setText(sval);

    m_sqlSymbolTableName = sval;


//    m_settings.beginGroup(m_sqlSymbolTableName.toLower());


    // MAINWINDOW
    m_settings.beginGroup("MainWindow");

    resize(m_settings.value("size", QSize(400, 400)).toSize());
    move(m_settings.value("pos", QPoint(200, 200)).toPoint());

    m_settings.endGroup();


    // IB
    m_settings.beginGroup("ib");

    sval = m_settings.value("hostName").toString();
    qDebug() << "hostName:" << sval;
    ui->ibHostLineEdit->setText(sval);


    ival = m_settings.value("port").toInt();
    ui->ibPortSpinBox->setValue(ival);

    bval = m_settings.value("reconnectChecked").toBool();
    ui->reconnectCheckbox->setChecked(bval);
    m_mgr->setReconnectOnFailure(bval);

    m_settings.endGroup();


    // SQL
    m_settings.beginGroup("sql");

    sval = m_settings.value("hostName").toString();
    m_mgr->setSqlHostName(sval);
    ui->sqlHostNameLineEdit->setText(sval);

    sval = m_settings.value("password").toString();
    m_mgr->setSqlPassword(sval);
    ui->sqlPasswordLineEdit->setText(sval);

    sval = m_settings.value("databaseName", "Stockz").toString();
    m_mgr->setSqlDatabaseName(sval);
    ui->sqlDatabaseNameLineEdit->setText(sval);

    sval = m_settings.value("outputDatabaseName", "StockIntradayData").toString();
    qDebug() << "SVAL1:" << sval;
    m_mgr->setSqlOutputDatabaseName(sval);
    ui->sqlOutputDatabaseLineEdit->setText(sval);

    ival = m_settings.value("port").toInt();
    m_mgr->setSqlPort(ival);
    ui->sqlPortSpinBox->setValue(ival);

    sval = m_settings.value("userName").toString();
    m_mgr->setSqlUserName(sval);
    ui->sqlUserNameLineEdit->setText(sval);

//    bval = m_settings.value("checkBoxToggled").toBool();
//    ui->mySqlCheckBox->setChecked(bval);
//    m_mgr->setUseSql(bval);

    m_settings.endGroup();


    // HDF5
    m_settings.beginGroup("hdf5");

    sval = m_settings.value("outputFolderPath").toString();
    m_mgr->setHdf5OutputFolderPath(sval);
    ui->hdf5OutputFolderLineEdit->setText(sval);

//    bval = m_settings.value("checkBoxToggled").toBool();
//    ui->hdf5CheckBox->setChecked(bval);
//    m_mgr->setUseHdf5(bval);

    m_settings.endGroup();

    m_settings.beginGroup("download");

//    slval = m_settings.value("symbolsToDownload").toStringList();
//    m_mgr->setSymbolsToDownload(slval);

    ival = m_settings.value("timeFrameComboBox", 4).toInt();
    ui->timeFrameComboBox->setCurrentIndex(ival);
    m_mgr->setTimeFrame((TimeFrame)ival);



    ival = m_settings.value("numberOfMonths").toInt();
    ui->numberOfMonthsSpinBox->setValue(ival);
    m_mgr->setNumberOfMonths(ival);

    bval = m_settings.value("autoDownloadChecked").toBool();
    ui->autoDownloadCheckbox->setChecked(bval);
    m_mgr->setAutoDownloadEnabled(bval);

    m_settings.endGroup(); // download

//    m_settings.endGroup(); // m_sqlSymbolTableName;
}


void MainWindow::writeSettings()
{
    qDebug() << "In writeSettings";

    QSettings m_settings;
    m_settings.setValue(m_sqlSymbolTableName, "blah");

//    m_settings.beginGroup(m_sqlSymbolTableName);

    m_settings.beginGroup("MainWindow");
    m_settings.setValue("size", size());
    m_settings.setValue("pos", pos());
    m_settings.endGroup();


    m_settings.setValue("sql/userName", QVariant(ui->sqlUserNameLineEdit->text()));
    m_settings.setValue("download/timeFrameComboBox", QVariant(ui->timeFrameComboBox->currentIndex()));
    qDebug() << "VAL:" << ui->sqlSymbolTableNameLineEdit->text();
    m_settings.setValue("sql/symbolTableName", QVariant(ui->sqlSymbolTableNameLineEdit->text()));
    m_settings.sync();

    m_settings.sync();
//    m_settings.endGroup();
}


void MainWindow::onConnected()
{
    if (m_autoDownload)
        ui->statusBar->showMessage(QString("Connected: Autodownload enabled:"));
    else
        ui->statusBar->showMessage(QString("Connected:"));
    ui->loginButton->setEnabled(false);
    ui->logoutButton->setEnabled(true);
    ui->startButton->setEnabled(true);
//    m_reconnectTimer->start(1000);
}


void MainWindow::closeEvent(QCloseEvent* event)
{
    if (userReallyWantsToQuit()) {
        writeSettings();
        if (m_mgr->isConnected()) {
//            if (ui->mySqlCheckBox->isEnabled())
//                m_mgr->sendDataToSql(m_mgr->getLastReqId());
//            if (ui->hdf5CheckBox->isEnabled())
//                m_mgr->sendDataToHdf5(m_fcqt->getLastReqId());
            m_mgr->logout();
        }
        event->accept();
    } else {
        event->ignore();
    }
}


bool MainWindow::userReallyWantsToQuit()
{
    if (m_mgr->isConnected()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Data Importer"),
                     tr("You are still connected to the server.\n"
                        "Do you want to save the application state and exit now?"),
                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return true;
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;

}


void MainWindow::on_sqlSymbolTableNameLineEdit_editingFinished()
{
    qDebug() << "In on_sqlSymbolTableNameLineEdit";

    m_sqlSymbolTableName = ui->sqlSymbolTableNameLineEdit->text();
    m_mgr->setSqlSymbolTableName(m_sqlSymbolTableName);

    QSettings s;
//    s.beginGroup(m_sqlSymbolTableName);
    s.setValue("sql/symbolTableName", QVariant(ui->sqlSymbolTableNameLineEdit->text()));
    s.sync();
//    s.endGroup();
}

void MainWindow::on_hdf5OutputFolderLineEdit_editingFinished()
{
    qDebug() << "hdf5outputfolder blah";

    QString arg1 = ui->hdf5OutputFolderLineEdit->text();

    QSettings s;
//    s.beginGroup(m_sqlSymbolTableName);
    s.setValue("hdf5/outputFolderLineEdit", QVariant(arg1));
    s.sync();
//    s.endGroup();

    m_mgr->setHdf5OutputFolderPath(arg1);
//    ui->hdf5OutputFolderLineEdit->setText(arg1);
}


void MainWindow::onDownloading(const QString & name)
{
    ui->statusBar->showMessage(name);
}
