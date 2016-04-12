#include "manager.h"
#include "symbol.h"
#include "record.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QDateTime>
#include <QStringList>
#include <QString>
#include <QTimer>
#include <QCoreApplication>
#include <QEventLoop>
#include <cstring>
#include <QTimeZone>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>


Manager::Manager(QObject *parent)
    : QObject(parent)

{

    m_ibqt = new IBQt(this);

    m_loginTimer = new QTimer(this);
    m_realTimeDataTimer = new QTimer(this);
    m_reqHistoricalDataTimer = new QTimer(this);
    m_reqContractDetailsTimer = new QTimer(this);
//    m_liquidTradingHoursTimer = new QTimer(this);
    m_currentTimeTimer = new QTimer(this);

    m_hdf5OutputFolderPath = QCoreApplication::applicationDirPath();

    connect(m_ibqt, SIGNAL(twsConnected()), this, SLOT(onTwsConnected()));
    connect(m_ibqt, SIGNAL(managedAccounts(QByteArray)), this, SLOT(onManagedAccounts(QByteArray)));
    connect(m_ibqt, SIGNAL(contractDetails(int,ContractDetails)), this, SLOT(onContractDetails(int,ContractDetails)));
    connect(m_ibqt, SIGNAL(contractDetailsEnd(int)), this, SLOT(onContractDetailsEnd(int)));
    connect(m_ibqt, SIGNAL(historicalData(long,QByteArray,double,double,double,double,int,int,double,int)),
            this, SLOT(onHistoricalData(long,QByteArray,double,double,double,double,int,int,double,int)));
    connect(m_ibqt, SIGNAL(error(int,int,QByteArray)), this, SLOT(onError(int,int,QByteArray)));
    connect(m_ibqt, SIGNAL(ibSocketError(QString)), this, SLOT(onIbSocketError(QString)));
    connect(m_ibqt, SIGNAL(tickPrice(long,TickType,double,int)), this, SLOT(onTickPrice(long,TickType,double,int)));
    connect(m_ibqt, SIGNAL(tickSize(long,TickType,int)), this, SLOT(onTickSize(long,TickType,int)));
    connect(m_ibqt, SIGNAL(connectionClosed()), this, SLOT(onConnectionClosed()));

    connect(m_realTimeDataTimer, SIGNAL(timeout()), this, SLOT(onRealTimeDataTimerTimeout()));
    connect(m_reqHistoricalDataTimer, SIGNAL(timeout()), this, SLOT(onRequestedHistoricalDataTimerTimeout()));
    connect(m_reqContractDetailsTimer, SIGNAL(timeout()), this, SLOT(onRequestedContractDetailsTimerTimeout()));
    connect(m_loginTimer, SIGNAL(timeout()), SLOT(onLoginTimerTimeout()));
//    connect(m_liquidTradingHoursTimer, SIGNAL(timeout()), SLOT(onLiquidTradingHoursTimerTimeout()));
    connect(m_currentTimeTimer, SIGNAL(timeout()), this, SLOT(onCurrentTimeTimerTimeout()));

//    m_liquidTradingHoursTimer->start(1000 * 60);
    m_currentTimeTimer->start(1000);

    m_barSizes << "1 secs"
               << "5 secs"
               << "15 secs"
               << "30 secs"
               << "1 min"
               << "2 mins"
               << "3 mins"
               << "5 mins"
               << "15 mins"
               << "30 mins"
               << "1 hour"
               << "1 day";
}

void Manager::login(const QString &url, int port, int clientId)
{
    qDebug() << "In Manager::login()";
    m_ibUrl = url;
    m_ibPort = port;
    m_ibClientId = clientId;

    if (!m_loginTimer->isActive()) {
        qDebug() << "        Starting login-attempt timer for 10 second intervals";
        m_loginTimer->start(1000 * 10);
    }
    ++m_loginAttemptNumber;
    m_attemptingLogIn = true;
    m_isConnected = false;
    m_ibqt->connectToTWS(url.toLatin1(), (quint16)port, clientId);
}

void Manager::logout()
{
    m_ibqt->disconnectTWS();
}

bool Manager::initializeSqlDatabase()
{
    qDebug() << "In initializeSqlDatabase()";
    qDebug() << "host:" << m_sqlHostName;
    qDebug() << "port:" << m_sqlPort;
    qDebug() << "sql password:" << m_sqlPassword;
    qDebug() << "user:" << m_sqlUserName;
    qDebug() << "db:" << m_sqlDatabaseName;

    m_db = QSqlDatabase::addDatabase("QMYSQL");
    m_db.setHostName(m_sqlHostName);
    m_db.setPort(m_sqlPort);
    m_db.setPassword(m_sqlPassword);
    m_db.setUserName(m_sqlUserName);
    m_db.setDatabaseName(m_sqlDatabaseName);

    if (!m_db.open()) {
        qDebug() << "[error] can't open database:" << m_db.lastError();
        return false;
    }
//    qDebug() << "[INFO] Logged into mysql databse:" << m_sqlDatabaseName;

    m_symbolTableModel = new QSqlTableModel(this, m_db);

    qDebug() << "m_symbolTableModel:" << m_symbolTableModel;

    m_symbolTableModel->setTable(m_sqlSymbolTableName);
    m_symbolTableModel->select();
    return true;
}

void Manager::downloadQuotes()
{

    qDebug() << "m_symbolTableModel:" << m_symbolTableModel;

    for (int i=0;i<m_symbolTableModel->rowCount();++i) {
//    for (int i=0;i<1;++i) {
        if (m_db.isOpen() && m_db.databaseName() != m_sqlDatabaseName) {
            m_db.close();
            m_db.setDatabaseName(m_sqlDatabaseName);
            m_db.open();
        }

        QByteArray symbol = m_symbolTableModel->record(i).value("symbol").toByteArray();
        long reqId = m_ibqt->getTickerId();
//        long realId = m_ibqt->getTickerId();

        Symbol* s = new Symbol;
        m_symbolMap[reqId] = s;
//        m_symbolMap[realId] = s;

        s->symbolName       = symbol;
        s->tableName        = QByteArray("stock_") + symbol + QByteArray("_") + m_timeFrameString.replace(' ','_').toLatin1();
        s->currentRow       = 0;
        s->timeFrameString  = m_timeFrameString.split('_')[0] + m_timeFrameString.split('_')[1].toLower();
//        s->realTimeDataId   = realId;
        s->contractDetailsOnly = false;

        if (m_useHdf5) {
            m_hdf5Map[s] = new IbHdf5(s->tableName,
                            m_hdf5OutputFolderPath + QString("/") + s->tableName + QString(".h5"), this);
        }

        Contract* c = new Contract;
        c->symbol = symbol;
        QByteArray currency = m_symbolTableModel->record(i).value("currency").toByteArray();
        if (!currency.isEmpty())
            c->currency = currency;
        QByteArray secType = m_symbolTableModel->record(i).value("sec_type").toByteArray();
        if (!secType.isEmpty())
            c->secType = secType;
        c->exchange = QByteArray("SMART");
        QByteArray primaryExchange = m_symbolTableModel->record(i).value("primary_exchange").toByteArray();
        if (primaryExchange != "SMART")
            c->primaryExchange = primaryExchange;

        m_reqContractDetailsTimer->start(1000 * 15);
        QString msg("Requesting Contract Details for " + QString(c->symbol) + " .. request timeout set for 15 seconds");
        qDebug() << msg;
        emit downloading(msg);
        m_ibqt->reqContractDetails(reqId, *c);

        delete c;

        m_lock = true;
        while (m_lock) {
            if (m_stopButtonClicked) {
                qDebug() << "STOP BUTTON CLICKED .. LEAVING DOWNLOAD QUOTES";
//                m_stopButtonClicked = false;
                return;
            }
            qDebug() << "-";
            delay(100);
        }

        if (m_reqHistoricalDataTimer->isActive())
            m_reqHistoricalDataTimer->stop();

//        convertSqlToHdf5(s);
    }
}

void Manager::setReconnectOnFailure(bool reconnectOnFailure)
{
    m_reconnectOnFailure = reconnectOnFailure;
}

void Manager::setSqlHostName(const QString &sqlHostName)
{
    m_sqlHostName = sqlHostName;
}

void Manager::setSqlPort(int sqlPort)
{
    m_sqlPort = sqlPort;
}

void Manager::setSqlUserName(const QString &sqlUserName)
{
    m_sqlUserName = sqlUserName;
}

void Manager::setSqlPassword(const QString &sqlPassword)
{
    m_sqlPassword = sqlPassword;
}

void Manager::setSqlDatabaseName(const QString &sqlDatabaseName)
{
    m_sqlDatabaseName = sqlDatabaseName;
}

void Manager::setSqlSymbolTableName(const QString &sqlSymbolTableName)
{
    m_sqlSymbolTableName = sqlSymbolTableName;
}

void Manager::setSqlOutputDatabaseName(const QString &sqlOutputDatabaseName)
{
    m_sqlOutputDatabaseName = sqlOutputDatabaseName;
}

QSqlTableModel *Manager::symbolTableModel() const
{
    return m_symbolTableModel;
}

void Manager::setHdf5OutputFolderPath(const QString &hdf5OutputFolderPath)
{
    m_hdf5OutputFolderPath = hdf5OutputFolderPath;
}

void Manager::setTimeFrame(const TimeFrame &timeFrame)
{
    m_timeFrame = timeFrame;

    switch (timeFrame)
    {
    case SEC_1:
        m_timeFrameString = "1 Sec";
        m_timeFrameInSeconds = 1;
        m_durationStr = "1800 S"; //
        break;
    case SEC_5:
        m_timeFrameString = "5 Sec";
        m_timeFrameInSeconds = 5;
        m_durationStr = "7200 S";
        break;
    case SEC_15:
        m_timeFrameString = "15 Sec";
        m_timeFrameInSeconds = 15;
        m_durationStr = "1440 S";
        break;
    case SEC_30:
        m_timeFrameString = "30 Sec";
        m_timeFrameInSeconds = 30;
        m_durationStr = "28800 S";
        break;
    case MIN_1:
        m_timeFrameString = "1 Min";
        m_timeFrameInSeconds = 60;
        m_durationStr = "1 D";
        break;
    case MIN_2:
        m_timeFrameString = "2 Min";
        m_timeFrameInSeconds = 2 * 60;
        m_durationStr = "2 D";
        break;
    case MIN_3:
        m_timeFrameString = "3 Min";
        m_timeFrameInSeconds = 3 * 60;
        m_durationStr = "1 W";
        break;
    case MIN_5:
        m_timeFrameString = "5 Min";
        m_timeFrameInSeconds = 5 * 60;
        m_durationStr = "1 W";
        break;
    case MIN_15:
        m_timeFrameString = "15 Min";
        m_timeFrameInSeconds = 15 * 60;
        m_durationStr = "2 W";
        break;
    case MIN_30:
        m_timeFrameString = "30 Min";
        m_timeFrameInSeconds = 30 * 60;
        m_durationStr = "21 D";
        break;
    case HOUR_1:
        m_timeFrameString = "1 Hour";
        m_timeFrameInSeconds = 60 * 60;
        m_durationStr = "1 M";
        break;
    case  DAY_1:
        m_timeFrameString = "1 Day";
        m_timeFrameInSeconds = 60 * 60 * 24;
        m_durationStr = "1 Y";
    }
}

void Manager::setNumberOfMonths(int numberOfMonths)
{
    m_numberOfMonths = numberOfMonths;
}

void Manager::setStopButtonClicked(bool stopButtonClicked)
{
    for (int i=0;i<m_realTimeIds.size();++i) {
        m_ibqt->cancelMktData(m_realTimeIds.at(i));
    }
    m_realTimeDataTimer->stop();
    m_stopButtonClicked = stopButtonClicked;
    emit downloading("All Downloading stopped");
}

void Manager::setRealTimeDataEnabled(bool autoDownloadEnabled)
{
    m_realTimeDataEnabled = autoDownloadEnabled;
}

void Manager::setUseSql(bool useSql)
{
    m_useSql = useSql;
}

void Manager::setUseHdf5(bool useHdf5)
{
    m_useHdf5 = useHdf5;
}

void Manager::onManagedAccounts(const QByteArray &accountList)
{
    m_isConnected = true;
    m_attemptingLogIn = false;
    m_loginAttemptNumber = 0;
    if (m_loginTimer->isActive())
        m_loginTimer->stop();
    m_managedAccounts = QString(accountList).split(',', QString::SkipEmptyParts);
    emit connected();
}

void Manager::onContractDetails(int reqId, const ContractDetails &contractDetails)
{
    qDebug() << "In onContractDetails()";
    m_reqContractDetailsTimer->stop();

    Symbol* s = m_symbolMap[reqId];

    if (s == NULL) {
        return;
    }

    qDebug() << "        for symbol:" << s->symbolName;

    s->contractDetails = contractDetails;

    parseLiquidHours(s);

}

void Manager::onContractDetailsEnd(int reqId)
{
    // THIS IS TEMPORARY
//    m_useHdf5 = false;

    qDebug() << "In onContractDetailsEnd()";

    Symbol* s = m_symbolMap[reqId];

    if (s == NULL)
        return;

    qDebug() << "        for symbol:" << s->symbolName;

    if (s->liquidHoursStartTime == QDateTime::fromTime_t(0) || s->liquidHoursEndTime == QDateTime::fromTime_t(0)) {
        m_lock = false;
        return;
    }

    if (s->contractDetailsOnly) {
        qDebug() << "s->contractDetailsOnly = true .. leaving onContractDetailsEnd()";
        qDebug() << "\n";
        return;
    }
    else {
        s->contractDetailsOnly = true;
    }

    m_db.close();
    m_db.setDatabaseName(m_sqlOutputDatabaseName);
    m_db.open();
    s->model = new QSqlTableModel(this, m_db);//    QString target("20090507:0930-1600;20090508:CLOSED");

    s->rowCount = 0;

    QDateTime dt;

    if (!m_db.tables().contains(s->tableName)) {

        QSqlQuery q(m_db);

        QString command;

        qDebug() << "DB TABLE NAME:" << s->tableName;

        command = "create table " + m_symbolMap[reqId]->tableName +
                "( "
                "timestamp int(11), "
                "date datetime, "
                "open decimal(10,2), "
                "high decimal(10,2), "
                "low decimal(10,2), "
                "close decimal(10,2), "
                "volume decimal(20,2), "
                "time_frame varchar(5), "
                "primary key (timestamp)"
                " )";

        if (!q.exec(command))
            qDebug() << "[ERROR] QSQlQuery exec DID NOT CREATE DATA TABLE:" << q.lastError();

        q.finish();


        dt = QDateTime::currentDateTime();

    }
    if (dt.isNull()){
        // CHECK TO SEE IF DATABASE IS EMPIRICALLY COMPLETE AND UP TO DATE

        QDateTime edt = empiricalDataComplete(reqId);

        if (!edt.isNull()) {
            qDebug() << "!edt.isNull ... edt:" << edt;
            dt = edt;
        }
    }
    if (dt.isNull()) {
        dt = currentDataComplete(reqId);
    }

    if (dt.isNull() && !m_realTimeDataEnabled) {
        qDebug() << "dt.isNull() && !m_autoDownloadEnabled()";
        m_lock = false;
        return;
    }

    s->model->setTable(s->tableName);
    s->model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    s->model->select();
    s->rowCount = s->model->rowCount();

    if (m_realTimeDataEnabled) {
        long rtId = m_ibqt->getTickerId();
        s->realTimeDataId = rtId;
        m_symbolMap[rtId] = s;
        s->isRealTimeId = true;

        // START RT DATA COLLECTION
        m_realTimeIds.append(rtId);
        m_realTimeDataTimer->start(m_timeFrameInSeconds * 1000);
        m_ibqt->reqMktData(rtId, s->contractDetails.summary, QByteArray(""), false);
    }

    if (!dt.isNull()) {
        // START HIST DATA COLLECTION
        long histId = m_ibqt->getTickerId();
        m_symbolMap[histId] = s;
        m_symbolMap.remove(reqId);
        qDebug() << "calling reqHistoricalData() .. for symbol:" << s->symbolName << " .. reqId:" << histId << ".. dt:" << dt;
//        m_lastReqId = histId;
        reqHistoricalData(histId, s->contractDetails.summary, ibEndDateTimeToString(dt).toLatin1(),
                                  m_durationStr.toLatin1(), m_barSizes.at((int)m_timeFrame).toLatin1(),
                                  QByteArray("TRADES"), 1, 2, QList<TagValue*>());
    }
    else {
        s->insertRealTimeData = true;
        m_lock = false;
    }

    qDebug() << "leaving onContractDetailsEnd()";
}

void Manager::onHistoricalData(long reqId, const QByteArray &date, double open, double high, double low, double close, int volume, int barCount, double wap, int hasGaps)
{
    Symbol* s = m_symbolMap[reqId];

    if (s == NULL)
        return;

//    qDebug() << "."/* << "startTime:" << m_liquidHoursStartTime << "endTime:" << m_liquidHoursEndTime*/;
    qDebug() << "." << s->symbolName << "reqId:" << reqId << ".. dt:" << QDateTime::fromTime_t(date.toUInt());
    qDebug() << "    QByteArray(date):" << date;

    static int lastCdtDataSize = 0;


    m_reqHistoricalDataTimer->stop();


    if (s == NULL) {
        qDebug() << "m_symbolMap[reqId] is NULL";
        return;
    }
//    qDebug() << "m_stopButtonClicked is:" << m_stopButtonClicked;

    if (m_stopButtonClicked) {
//        date = "finished";
        m_ibqt->cancelHistoricalData(reqId);
        if (m_reqHistoricalDataTimer->isActive())
            m_reqHistoricalDataTimer->stop();
        m_lock = false;
        return;
//        m_stopButtonClicked = false;
    }

    if (date.contains("finished") || m_stopButtonClicked) {

        qDebug() << "date.contains('finished') for:" << s->tableName << "m_cdtData.size():" << m_cdtData.size();
        qDebug() << "    s->model->rowCount():" << s->model->rowCount();
        qDebug() << "    The timezone for the symbol's exchange is:" << s->contractDetails.timeZoneId;

        // THIS IS A LOGICAL HACK!
        if (s->model->rowCount() == 1) {
            if (m_cdtData.size() > 0) {
                for (int i=0;i<m_cdtData.size();++i) {
                    QSqlRecord r = m_cdtData.at(i);
                    s->model->insertRecord(-1, r);
                }
                m_cdtData.clear();

                // SUBMIT
                sqlSubmit(reqId);

                QDateTime dt = QDateTime::fromTime_t(s->model->record(0).value("timestamp").toUInt());
                // DELAY
                emit downloading(QString("IB server request delay.. 15 seconds for: ") + QString(s->symbolName));

                qDebug() << "DELAY";

                delay(15 * 1000);
                // END DELAY

                long newId = m_ibqt->getTickerId();
                m_symbolMap[newId] = s;
                m_symbolMap.remove(reqId);

//                m_lastReqId = newId;
                reqHistoricalData(newId, s->contractDetails.summary, ibEndDateTimeToString(dt).toLatin1(),
                    m_durationStr.toLatin1(), m_barSizes.at(m_timeFrame).toLatin1(), "TRADES", 1, 2, QList<TagValue*>());
                return;
            }
            if (m_edtData.size() > 0) {

                // THIS IS A STRANGE SITUATION THAT OCCURRED WITH TSLA,
                // I SIMPLY AM ADDING THIS SECTION; REMOVING THE EXISTING ROW AND INSERTING THE NEW EDTDATA.

                s->model->removeRow(0);
                for (int i=m_edtData.size()-1;i>=0;--i) {
                    QSqlRecord r = m_edtData.at(i);
                    s->model->insertRecord(0, r);
                }
                m_edtData.clear();

                // SUBMIT
                sqlSubmit(reqId);

                QDateTime dt = QDateTime::fromTime_t(s->model->record(0).value("timestamp").toUInt());
                // DELAY
                emit downloading(QString("IB server request delay.. 15 seconds for: ") + QString(s->symbolName));

                qDebug() << "DELAY";

                delay(15 * 1000);
                // END DELAY

                long newId = m_ibqt->getTickerId();
                m_symbolMap[newId] = s;
                m_symbolMap.remove(reqId);

//                m_lastReqId = newId;
                reqHistoricalData(newId, s->contractDetails.summary, ibEndDateTimeToString(dt).toLatin1(),
                    m_durationStr.toLatin1(), m_barSizes.at(m_timeFrame).toLatin1(), "TRADES", 1, 2, QList<TagValue*>());
                return;
            }
        }

        // EDT DATA

        if (!m_edtData.isEmpty()) {
            for (int i=m_edtData.size()-1;i>=0;--i) {
                QSqlRecord r = m_edtData.at(i);
                s->model->insertRecord(0, r);
            }

            // SUBMIT EDT DATA
            sqlSubmit(reqId);
            // END SUBMIT

            m_edtData.clear();

            QDateTime edt = empiricalDataComplete(reqId);
            if (!edt.isNull()) {
//                QDateTime dt = QDateTime::fromTime_t(m_edtData.first().value("timestamp").toUInt());
//                QDateTime dt = QDateTime::fromTime_t(s->model->record(0).value("timestamp").toUInt());
                QDateTime dt = edt;
                // DELAY
                emit downloading(QString("IB server request delay.. 15 seconds for: ") + QString(s->symbolName));

                qDebug() << "DELAY";

                delay(15 * 1000);
                // END DELAY

                long newId = m_ibqt->getTickerId();
                m_symbolMap[newId] = s;
                m_symbolMap.remove(reqId);

//                m_lastReqId = newId;
                reqHistoricalData(newId, s->contractDetails.summary, ibEndDateTimeToString(dt).toLatin1(),
                    m_durationStr.toLatin1(), m_barSizes.at(m_timeFrame).toLatin1(), "TRADES", 1, 2, QList<TagValue*>());
                return;
            }
        }

        // END EDT DATA

        // CDT DATA

        if (!m_cdtData.isEmpty()) {
            qDebug() << "!m_cdtData.isEmpty()";

            bool cdtDataIsComplete = false;

            QDateTime ldt = QDateTime::fromTime_t(s->model->record(s->model->rowCount()-1).value("timestamp").toUInt());
//            for (int i=0;i<m_cdtData.size();++i) {
//                QDateTime dt = QDateTime::fromTime_t(m_cdtData.at(i).value("timestamp").toUInt());
//                if (dt <= ldt) {
//                    cdtDataIsComplete = true;
//                    break;
//                }
//            }

            if (m_cdtData.size() != 0 && m_cdtData.size() == lastCdtDataSize)
                cdtDataIsComplete = true;
            lastCdtDataSize = m_cdtData.size();

            if (!cdtDataIsComplete) {
                qDebug() << "!cdtDataIsComplete";
                QDateTime dt = QDateTime::fromTime_t(m_cdtData.first().value("timestamp").toUInt());
                long newId = m_ibqt->getTickerId();
                m_symbolMap[newId] = s;
                m_symbolMap.remove(reqId);
//                for (int i=0;i<m_cdtData.size();++i) {
//                    qDebug() << QDateTime::fromTime_t(m_cdtData.at(i).value("timestamp").toUInt());
//                }
                qDebug() << "calling m_ibqt->reqHistoricalData() for symbol:" << s->symbolName << " .. dt:" << dt;

                // DELAY
                emit downloading(QString("IB server request delay.. 15 seconds for: ") + QString(s->symbolName));

                qDebug() << "DELAY";

                delay(15 * 1000);
                // END DELAY

//                m_lastReqId = newId;
                reqHistoricalData(newId, s->contractDetails.summary, ibEndDateTimeToString(dt).toLatin1(),
                                                               m_durationStr.toLatin1(), m_barSizes.at(m_timeFrame).toLatin1(), "TRADES", 1, 2, QList<TagValue*>());
                return;
            }
            else {
                qDebug() << "writing cdt data to the model";
                for (int i=0;i<m_cdtData.size();++i) {
                    QSqlRecord r = m_cdtData.at(i);
                    QDateTime dt = QDateTime::fromTime_t(r.value("timestamp").toUInt());
                    if (dt > ldt) {
                        s->model->insertRecord(-1, r);
                        ldt = dt;
                    }
                }
                m_cdtData.clear();
                lastCdtDataSize = 0;
            }

        }

        // END CDT DATA

        // SUBMIT ALL CHANGES MADE TO THE MODEL.
        sqlSubmit(reqId);
        // END SUBMIT

        qDebug() << "FINISHED reqId:" << reqId << "ROW COUNT:" << s->model->rowCount(QModelIndex());


        QDateTime dt;
        QDateTime edt = empiricalDataComplete(reqId);
        QDateTime cdt = currentDataComplete(reqId);

        if (!edt.isNull()) {
            dt = edt;
            qDebug() << "dt = edt";
        }
        else if (!cdt.isNull()) {
            dt = cdt;
            qDebug() << "dt = cdt";
        }
//        else {
//            if (m_autoDownloadEnabled) {
//                long rtId = m_ibqt->getTickerId();
//                s->realTimeDataId = rtId;
//                m_symbolMap[rtId] = s;

//                // START RT DATA COLLECTION
//                m_realTimeIds.append(rtId);
//                m_realTimeDataTimer->start(m_timeFrameInSeconds * 1000);
//                m_ibqt->reqMktData(rtId, s->contractDetails.summary, QByteArray(""), false);
//            }
//        }
        if (dt.isNull()){
            qDebug() << "dt for" << s->symbolName << "is ALL CAUGHT UP.. ";
            if (m_useHdf5) {
                qDebug() << "Writing" << s->symbolName << "To HDF5";
                emit downloading(QString("Writing ") + s->symbolName + QString(" to HDF5 Database"));
                Record2 r2[s->model->rowCount()];
                for (int i=0;i<s->model->rowCount();++i) {
                    r2[i].timestamp = s->model->record(i).value(0).toUInt();
                    strcpy(r2[i].timeString, s->model->record(i).value(1).toByteArray().data());
                    r2[i].open = s->model->record(i).value(2).toDouble();
                    r2[i].high = s->model->record(i).value(3).toDouble();
                    r2[i].low = s->model->record(i).value(4).toDouble();
                    r2[i].close = s->model->record(i).value(5).toDouble();
                    r2[i].volume = s->model->record(i).value(6).toUInt();
                    strcpy(r2[i].sqlTimeFrame, s->model->record(i).value(7).toByteArray().data());
                }
                m_hdf5Map[s]->writeRecords(r2, s->model->rowCount());
            }
            qDebug() << "returning the lock";
            s->insertRealTimeData = true;
            m_lock = false;
            return;
        }

        long newId = m_ibqt->getTickerId();
        m_symbolMap[newId] = s;
        m_symbolMap.remove(reqId);

        // DELAY
        emit downloading(QString("IB server request delay.. 15 seconds for: ") + QString(s->symbolName));

        qDebug() << "DELAY";

        delay(15 * 1000);
        // END DELAY

//        qDebug() << "dt:" << dt << "m_durationStr:" << m_durationStr;

        m_lastDt = dt;

        qDebug() << "calling reqHistoricalData() .. edt .. dt:" << dt;

//        m_lastReqId = newId;
        reqHistoricalData(newId, s->contractDetails.summary, ibEndDateTimeToString(dt).toLatin1(),
                                  m_durationStr.toLatin1(), m_barSizes.at(m_timeFrame).toLatin1(), "TRADES", 1, 2, QList<TagValue*>());
        return;
    }

    QDateTime dt = QDateTime::fromTime_t(date.toUInt());
    QString timeString = QDateTime::fromTime_t(date.toUInt()).toString("yyyy-MM-dd hh:mm:ss");
    QString dlStr = s->tableName + " " + timeString;

    // CRAP FILTER

    QDateTime limitDt;
    limitDt.setDate(QDate(2015,1,1));

//    qDebug() << "dt:" << dt << "limitDt:" << limitDt;

    if ((date.toUInt() < 1000000000) || (dt.date().year() == 1969) || (dt < limitDt) || (date.toUInt() < 1) || (open < 1) || (close < 1) || (high < 1) ||
            (low < 1) /*|| (volume < 1)*/)
    {
        qDebug() << "FILTER:" << dlStr << "open:" << open << "high:" << high << "low:" << low << "close:" << close << "volume:" << volume;
//        m_ibqt->cancelHistoricalData(reqId);
//        onHistoricalData(reqId, "finished", 0, 0, 0, 0, 0, 0, 0, 0);
        return;
    }

    // END CRAP FILTER

    QSqlRecord r = s->model->record();

    r.setValue(0, date.toUInt());
    r.setValue(1, timeString);
    r.setValue(2, open);
    r.setValue(3, high);
    r.setValue(4, low);
    r.setValue(5, close);
    r.setValue(6, volume);
    QString num = m_timeFrameString.split('_').at(0);
    QString unit = m_timeFrameString.split('_').at(1).toLower();
    r.setValue(7, num + unit);

    QDateTime fdt;          // empirical datetime
    QDateTime ldt;          // current datetime

    if (s->model->rowCount() > 0) {
        qDebug() << "    rowCount > 0";
        fdt = QDateTime::fromTime_t(s->model->record(0).value("timestamp").toUInt());
        ldt = QDateTime::fromTime_t(s->model->record(s->model->rowCount()-1).value("timestamp").toUInt());

        qDebug() << "    dt:" << dt;
        qDebug() << "    fdt:" << fdt;
        qDebug() << "    ldt:" << ldt;

        if (dt < fdt) {
            qDebug() << "    In onHistoricalData() .. dt < fdt .. m_edtData.append(r) .. m_edtData.size():" << m_edtData.size();
//            s->model->insertRecord(0, r);
            m_edtData.append(r);
            emit downloading(dlStr);

        }
        else if (dt > ldt) {
            qDebug() << "    In onHistoricalData() .. dt > ldt .. m_cdtData.append(r)";
            emit downloading(dlStr);
            if (m_cdtData.isEmpty()) {
                qDebug() << "    m_cdtData.isEmpty() .. m_cdtData.append(r)";
                m_cdtData.append(r);
            }
            QSqlRecord crb;
            for (int i=0;i<m_cdtData.size();++i) {
//                qDebug() << "In m_cdtData FOR LOOP .. m_cdtData.size():" << m_cdtData.size();
                QSqlRecord cr = m_cdtData.at(i);
                QDateTime cdt = QDateTime::fromTime_t(cr.value("timestamp").toUInt());
                if (i==0) {
                    if (dt < cdt)
                        m_cdtData.prepend(r);
                    else if (dt > cdt)
                        m_cdtData.append(r);
                }
                else if (i==m_cdtData.size()-1) {
                    if (dt > cdt)
                        m_cdtData.append(r);
                }
                else {
                    QDateTime cdtb = QDateTime::fromTime_t(crb.value("timestamp").toUInt());
                    if (dt < cdt && dt > cdtb) {
                        m_cdtData.insert(i, r);
                    }
                }
                crb = cr;
            }
        }
        else {
            qDebug() << "    WE CAUGHT A DUPLICATE";
//            return;
        }
    }
    else  {
        qDebug() << "    In onHistoricalData() .. s->model->insertRecord(-1, r)";
        s->model->insertRecord(-1, r);
        emit downloading(dlStr);
    }
}

void Manager::onTickPrice(const long &tickerId, const TickType &field, const double &price, const int &canAutoExecute)
{
    if (field == LAST) {
        Symbol* s = m_symbolMap[tickerId];
        if (s == NULL)
            return;
        QString msg("TICKPRICE for symbol:" + s->symbolName + "is:" + QString::number(price));
        qDebug() << msg;
        emit downloading(msg);
        RtRecord* r = new RtRecord;
        r->last = price;
        r->timestamp = QDateTime::currentDateTime().toTime_t();
        r->reqId = tickerId;
        r->size = 0;
        s->realTimeData.append(r);
    }
}

void Manager::onTickSize(const long &tickerId, const TickType &field, int size)
{
    if (field == LAST_SIZE) {
        Symbol* s       = m_symbolMap[tickerId];

        if (s == NULL)
            return;

        RtRecord* r     = s->realTimeData.last();

        QString msg("TICKSIZE  for symbol:" + s->symbolName + "is" + QString::number(size));
        qDebug() << msg;
        emit downloading(msg);


//        if (r->size == -1) {
//            r->size = size;
//        }
//        else {
            RtRecord* r2 = new RtRecord;

            r2->timestamp = QDateTime::currentDateTime().toTime_t();
            r2->last = r->last;
            r2->reqId = tickerId;
            r2->size = size;
            s->realTimeData.append(r2);
//        }
    }
}

void Manager::onError(const int id, const int errorCode, const QByteArray errorString)
{

    if (errorCode == 2104 || errorCode == 2106 || errorCode == 2108) {
        qDebug() << "[IBQT INFO]" << id << errorCode << errorString;
    }
    else if (errorCode == 505) {
        qDebug() << "[IBQT WARN]" << id << errorCode << errorString
                 << "            CLEARING MESSAGE AND RE-REQUESTING.. 15 sec delay";

        Symbol* s = m_symbolMap[id];
        if (s == NULL) {
            return;
        }

        ++s->error505Count;

        if (s->error505Count == 5) {
            s->error505Count = 0;
            m_lock = false;
            return;
        }

        long newId = m_ibqt->getTickerId();
        m_symbolMap[newId] = s;

//        m_symbolMap.remove(m_lastReqId);
        m_ibqt->cancelHistoricalData(id);
        m_symbolMap.remove(id);
        delay(15 * 1000);

//        m_lastReqId = newId;
        reqHistoricalData(newId, s->contractDetails.summary, ibEndDateTimeToString(m_lastDt).toLatin1(),
                                  m_durationStr.toLatin1(), m_barSizes.at(m_timeFrame).toLatin1(), "TRADES", 1, 2, QList<TagValue*>());
    }
    else if (errorCode == 162 || errorCode == 200) {
        m_lock == false;
    }
    else if (errorCode == 504) {
        if (m_reconnectOnFailure) {
            qDebug() << "[ERROR-IbError" << QDateTime::currentDateTime().toString() << "] errorString:" << errorString;
            qDebug() << "        Attempting to login again...";
            m_isConnected = false;
            login(m_ibUrl, m_ibPort, m_ibClientId);
        }
    }
    else if (errorCode == 321) {
        qDebug() << "[IBQT ERROR]" << id << errorCode << errorString;
        qDebug() << "             This is probably a 'primary_exchange' issue... SKIPPING THE SYMBOL THAT CAUSED THE ERROR.";
        m_lock = false;
    }
    else
        qDebug() << "[IBQT ERROR]" << id << errorCode << errorString;
}

void Manager::onIbSocketError(const QString &errorString)
{

    if (m_reconnectOnFailure) {
        m_isConnected = false;
        if (!m_attemptingLogIn) {
            qDebug() << "[ERROR-IbSocketError" << QDateTime::currentDateTime().toString() << "] errorString:" << errorString;
            qDebug() << "        Attempting to login again...";
            login(m_ibUrl, m_ibPort, m_ibClientId);
        }
    }
}

void Manager::onConnectionClosed()
{
    qDebug() << "[ERROR-onConnectionClosed" << QDateTime::currentDateTime().toString() << "]";

    if (m_reconnectOnFailure) {
        m_isConnected = false;
        qDebug() << "        Attempting to login again...";
        login(m_ibUrl, m_ibPort, m_ibClientId);
    }
}

void Manager::onLoginTimerTimeout()
{
    QString msg("Attempt number " + QString::number(++m_loginAttemptNumber) + " to login into IB");
    emit downloading(msg);
    qDebug() << msg;

    if (m_isConnected) {
        m_loginTimer->stop();
        return;
    }
    login(m_ibUrl, m_ibPort, m_ibClientId);
}

void Manager::onRealTimeDataTimerTimeout()
{
    qDebug() << "\n";
    qDebug() << "In onRealTimeDataTimerTimeout()";

    QDateTime cdt = QDateTime::currentDateTime();

    // we are collecting data for more than one symbol, probably
    for (int i=0;i<m_realTimeIds.size();++i) {
        long rtId = m_realTimeIds.at(i);
        Symbol* s = m_symbolMap[rtId];

        if (s == NULL)
            return;

        if (!(timeIsInLiquidTradingHours(s, cdt)))
            continue;

        if (!s->insertRealTimeData) {
            RtRecord r = *(s->realTimeData.last());
            qDeleteAll(s->realTimeData);
            s->realTimeData.clear();
            s->realTimeData.append(new RtRecord);
            s->realTimeData.at(0)->last = r.last;
            s->realTimeData.at(0)->reqId = r.reqId;
            s->realTimeData.at(0)->size = 0;
            s->realTimeData.at(0)->timestamp = r.timestamp;
            qDebug() << "_NOT_ Inserting Real Time Data Yet";
            continue;
        }

        qDebug() << "Inserting Real Time Data Now";

        QDateTime ldt = QDateTime::fromTime_t(s->model->record(s->model->rowCount()-1).value("timestamp").toUInt());
        QDateTime ndt;
        double open = 0;
        double high = 0;
        double low = 9999;
        double close = 0;
        double volume = 0;
        qDebug() << "symbol:" << s->symbolName << "has a size of" << s->realTimeData.size() << "realtime entries";
        for (int j=0;j<s->realTimeData.size();++j) {

            double last = s->realTimeData.at(j)->last;
            int size = s->realTimeData.at(j)->size;
            qDebug() << "size:" << size;
            volume += (double)size;
            qDebug() << "volume:" << volume;

            if (j==0)
                open = last;
            if (last > high)
                high = last;
            if (last < low)
                low = last;
            if (j==s->realTimeData.size()-1) {
                close = last;
                RtRecord r = *(s->realTimeData.last());
                qDeleteAll(s->realTimeData);
                s->realTimeData.clear();
                s->realTimeData.append(new RtRecord);
                s->realTimeData.at(0)->last = r.last;
                s->realTimeData.at(0)->reqId = r.reqId;
                s->realTimeData.at(0)->size = 0;
                s->realTimeData.at(0)->timestamp = r.timestamp;
            }
        }


        if (!timeIsSameTradingDay(s, ldt)
                && cdt > s->liquidHoursStartTime
                /*&& cdt < m_liquidHoursEndTime.addSecs(m_timeFrameInSeconds)*/)
        {
            ndt = s->liquidHoursStartTime;
        }
        else {
            ndt = ldt.addSecs(m_timeFrameInSeconds);
        }
                                            // 2016-04-04 13:57:00
        QString timeString  = ndt.toString("yyyy-MM-dd hh:mm:ss");

        qDebug() << "ndt:" << ndt;
        qDebug() << "timeString:" << timeString;

        QSqlRecord  r = s->model->record();
        r.setValue(0, ndt.toTime_t());
        r.setValue("timeString", timeString);
        r.setValue(2, open);
        r.setValue(3, high);
        r.setValue(4, low);
        r.setValue(5, close);
        r.setValue(6, volume);
        QString num = m_timeFrameString.split('_').at(0);
        QString unit = m_timeFrameString.split('_').at(1).toLower();
        r.setValue(7, num + unit);

        qDebug() << "s->model->rowCount():" << s->model->rowCount();
        QString oldName = m_db.databaseName();
        qDebug() << "old db name:" << oldName;
        m_db.close();
        m_db.setDatabaseName(m_sqlDatabaseName);
        m_db.open();

        qDebug() << "timestamp:" << r.value("timestamp").toString();
        qDebug() << "timeString:" << timeString;
        qDebug() << "open:" << r.value("open").toString();
        qDebug() << "high:" << r.value("high").toString();
        qDebug() << "low:" << r.value("low").toString();
        qDebug() << "close:" << r.value("close").toString();
        qDebug() << "volume:" << r.value("volume").toString();
        qDebug() << "timeFrame:" << r.value(7).toString();

        QSqlQuery q;

        // USING SQL INSERT COMMAND INSTEAD OF SQL TABLE MODEL
        if (!q.exec(QString("insert into ") + s->tableName + QString(" values")
            + QString("(")
            + QString::number(ndt.toTime_t()) + QString(", ")
            + QString("'") + timeString + QString("', ")
            + QString::number(open) + QString(", ")
            + QString::number(high) + QString(", ")
            + QString::number(low) + QString(", ")
            + QString::number(close) + QString(", ")
            + QString::number(volume) + QString(", ")
            + QString("'") + s->timeFrameString + QString("'")
            + QString(")")))
        {
            qDebug() << "q->exec (insert into) failed for table:" << s->tableName << "at" << i << "q->exec error:" << q.lastError();
        }
        else {
            q.finish();
            emit downloading(QString("Sending record: ") + QString::number(i) + QString(" of ") + QString::number(s->data.size()) + QString(" to ") + s->tableName + QString(" in MySql."));
        }

        // END USING SQL INSERT COMMAND


        s->model->insertRecord(-1, r);

        m_db.close();
        m_db.setDatabaseName(oldName);
        m_db.open();

        // HDF5
        if (m_useHdf5) {
            QString msg = QString("Writing ") + s->symbolName + QString("to HDF5");
            qDebug() << msg;
            emit downloading(msg);
            Record2 r2[1];
            r2[0].timestamp = r.value(0).toUInt();
            strcpy(r2[0].timeString, r.value(1).toByteArray().data());
            r2[0].open  = r.value(2).toDouble();
            r2[0].high  = r.value(2).toDouble();
            r2[0].low   = r.value(2).toDouble();
            r2[0].close = r.value(2).toDouble();
            r2[0].volume = r.value(6).toUInt();
            strcpy(r2[0].sqlTimeFrame, r.value(7).toByteArray().data());

            m_hdf5Map[s]->appendRecord(r2);
        }
    }
    qDebug() << "Leaving onRealTimeDataTimerTimeout()";
    qDebug() << "\n";
}

void Manager::onRequestedHistoricalDataTimerTimeout()
{
    qDebug() << "In onRequestedHistoricalDataTimerTimeout() .. unlocking m_lock now";
    m_lock = false;
}

void Manager::onRequestedContractDetailsTimerTimeout()
{
    qDebug() << "In onRequestedContractDetailsTimerTimeout() .. unlocking m_lock now";
    m_lock = false;
}

void Manager::onCurrentTimeTimerTimeout()
{
    static bool updating = false;

    m_currentTime = QDateTime::currentDateTime();

    if (updating)
        return;

    QList<int> reqIdList;
    for (int i=0;i<m_symbolMap.keys().size();++i) {
        int reqId = m_symbolMap.keys().at(i);
        Symbol* s = m_symbolMap[reqId];
        if (!reqIdList.contains(reqId) && !s->isRealTimeId) {
            reqIdList.append(reqId);
        }
    }
    for (int i=0;i<reqIdList.size();++i) {
        updating = true;
        int reqId = reqIdList.at(i);
        Symbol* s = m_symbolMap[reqId];
        QDateTime currentExchangeTime = m_currentTime.toTimeZone(QTimeZone(s->contractDetails.timeZoneId));
        QDateTime currentliquidHoursStart = s->liquidHoursStartTime;

        if (currentExchangeTime.date().day() != currentliquidHoursStart.date().day()) {
            m_ibqt->reqContractDetails(reqId, s->contractDetails.summary);
            delay(1000);
        }
    }
    updating = false;
}

//void Manager::onLiquidTradingHoursTimerTimeout()
//{
//    qDebug() << "\n\n";
//    qDebug() << "In onLiquidTradingHoursTimerTimeout()";

//    qDebug() << "        m_symbolMap.size():" << m_symbolMap.size();

//    QList<int>     reqIdList;
//    reqIdList = QSet<int>::fromList(m_symbolMap.keys()).toList();
//    for (int i=0;i<reqIdList.size();++i) {
//        long reqId  = reqIdList.at(i);
//        Symbol* s   = m_symbolMap[reqId];

//        if (s == NULL)
//            continue;

//        qDebug() << "In onLiquidTradingHoursTimerTimeout()";

//        qDebug() << "        m_symbolMap symbol name is:" << s->symbolName;

////        if (/*s->isRealTimeId || */symbols.contains(s)) {
////            qDebug() << "        ... this symbol is a repeat.. ignoring and moving to the next symbol";
////            continue;
////        }
////        symbols.append(s);

//        long nId = m_ibqt->getTickerId();
//        m_symbolMap[nId] = s;
//        m_symbolMap.remove(reqId);

//        qDebug() << "        reqContractDetails for symbol:" << s->symbolName;

//        m_ibqt->reqContractDetails(nId, s->contractDetails.summary);
//    }
//}


bool Manager::isConnected() const
{
    return m_isConnected;
}

void Manager::onTwsConnected()
{
//    m_loginAttemptNumber = 0;
//    m_loginTimer->stop();
}

void Manager::delay(int milliseconds)
{
    QTime dieTime = QTime::currentTime().addMSecs( milliseconds);
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}


QDateTime Manager::empiricalDataComplete(long reqId)
{
    Symbol* s = m_symbolMap[reqId];

    if (s == NULL)
        return QDateTime();

    QDateTime fdt = QDateTime::fromTime_t(s->model->record(0).value("timestamp").toUInt());

    qDebug() << "In empericalDataComplete() .. fdt:" << fdt << "cdt:" << QDateTime::currentDateTime();

    if (fdt.addMonths(m_numberOfMonths) < QDateTime::currentDateTime())
        return QDateTime();
    return fdt;
}


QDateTime Manager::currentDataComplete(long reqId)
{
    Symbol* s = m_symbolMap[reqId];

    if (s == NULL)
        return QDateTime();

    QDateTime ldt = QDateTime::fromTime_t(s->model->record(s->model->rowCount()-1).value("timestamp").toUInt());
    QDateTime cdt = QDateTime::currentDateTime();

    qDebug() << "In currentDataComplete() .. ldt:" << ldt << "cdt:" << cdt;

    if (timeIsInLiquidTradingHours(s, ldt) && ldt.addSecs(m_timeFrameInSeconds) > cdt)
        return QDateTime();

    qDebug() << "In currentDataComplete: ldt + m_timeFrameInSeconds:" << ldt.time().addSecs(m_timeFrameInSeconds);
    qDebug() << "In currentDataComplete: m_liquidEnd.time:" << m_liquidHoursEndTime.time();

    if (timeIsSameTradingDay(s, ldt) && ldt.time().addSecs(m_timeFrameInSeconds) == m_liquidHoursEndTime.time()) {
        return QDateTime();
    }

    if (s->liquidHoursStartTime.isNull()
            && (cdt.date().dayOfWeek() == 6 && ldt.date().addDays(1) == cdt.date())
            || (cdt.date().dayOfWeek() == 7 && ldt.date().addDays(2) == cdt.date())
            || (cdt.date().dayOfWeek() == 8 && ldt.date().addDays(3) == cdt.date()))
    {
        return QDateTime();
    }

    return cdt;
}

QString Manager::ibEndDateTimeToString(const QDateTime &edt)
{
    return edt.toUTC().toString("yyyyMMdd hh:mm:ss 'GMT'");
}

void Manager::sqlSubmit(long reqId)
{
    if (m_symbolMap[reqId] == NULL)
        return;

    QSqlTableModel* model = m_symbolMap[reqId]->model;

    bool isDirty = model->isDirty();
    if (!isDirty)
        return;

    qDebug() << "In sqlSubmit: isDirty:" << isDirty;

    if (isDirty) {
        model->database().transaction();
        if (model->submitAll()) {
            model->database().commit();
        } else {
            model->database().rollback();
            qDebug() << "[WARN] The sql database reported an error:" << model->lastError().text();
        }
    }
//    m_lastReqId = reqId;
}

void Manager::parseLiquidHours(Symbol* s)
{
    qDebug() << "In parseLiquidHours()";

//    if (timeZone != QByteArray("EST5EDT")) {
//        qDebug() << "[ERROR] TimeZone of this security is not yet supported! tz:" << timeZone;
//        return;
//    }

//    The liquid trading hours of the product. For example, 20090507:0930-1600;20090508:CLOSED.


    QRegularExpression re("^\\d{8}:((\\d{4}-\\d{4})|(CLOSED));\\d{8}:((\\d{4}-\\d{4})|(CLOSED))");

    QRegularExpressionMatch m = re.match(s->contractDetails.liquidHours);

    if (!m.hasMatch()) {
        s->liquidHoursEndTime = QDateTime::fromTime_t(0);
        s->liquidHoursStartTime = QDateTime::fromTime_t(0);
        return;
    }

    QByteArray timeZone = s->contractDetails.timeZoneId;

    QString lhs(s->contractDetails.liquidHours);
    QString today = lhs.split(';').at(0);
    qDebug() << "today:" << today;
    QString date = today.split(':').at(0);
    qDebug() << "date:" << date;
    QString times = today.split(':').at(1);
    qDebug() << "times:" << times;
    QStringList timesList = times.split('-', QString::SkipEmptyParts);
    QString startTime = timesList.at(0);
    qDebug() << "startTime:" << startTime;

    qDebug() << "timeZone:" << timeZone;

    if (startTime != "CLOSED") {
        QString endTime = times.split('-', QString::SkipEmptyParts).at(1);
        qDebug() << "endTime:" << endTime;

        s->liquidHoursStartTime = QDateTime::fromString(date+startTime, "yyyyMMddhhmm");
        s->liquidHoursStartTime.setTimeZone(QTimeZone(timeZone));
        s->liquidHoursStartTime = s->liquidHoursStartTime.toTimeSpec(Qt::LocalTime);

        s->liquidHoursEndTime   = QDateTime::fromString(date+endTime, "yyyyMMddhhmm");
        s->liquidHoursEndTime.setTimeZone(QTimeZone(timeZone));
        s->liquidHoursEndTime = s->liquidHoursEndTime.toTimeSpec(Qt::LocalTime);

        qDebug() << "s->liquidHoursStartTime:" << s->liquidHoursStartTime;
        qDebug() << "s->liquidHoursEndTime:" << s->liquidHoursEndTime;
    }
    else {
        s->liquidHoursStartTime = QDateTime();
        s->liquidHoursEndTime = QDateTime();
    }
}

bool Manager::timeIsInLiquidTradingHours(Symbol*s, const QDateTime & dt)
{
    qDebug() << "s->liquidHoursStartTime:" << s->liquidHoursStartTime << "s->liquidHoursEndTime:" << s->liquidHoursEndTime;

    bool ret = !(s->liquidHoursStartTime.isNull() || s->liquidHoursEndTime.isNull());

    if (ret)
        ret = (dt > s->liquidHoursStartTime && dt < s->liquidHoursEndTime);

//    qDebug() << "In timeIsInLiquidTradingHours() returning:" <<  ret;

    return ret;
}

bool Manager::timeIsSameTradingDay(Symbol* s, const QDateTime &dt)
{
    QDate nd = dt.addDays(1).date();
    QTime nt = s->liquidHoursStartTime.time();
    QDateTime ndt(nd, nt);

    QDateTime cdt = QDateTime::currentDateTime();

    bool ret = false;

    if (dt.date().year() != cdt.date().year())
        ret = false;
    if (dt.date().day() == cdt.date().day())
        ret = true;
    if (cdt < ndt)
        ret = true;

//    qDebug() << "In timeIsSameTradingDay() .. returning:" << ret;

    return ret;
}

void Manager::reqHistoricalData(long tickerId, const Contract &contract, const QByteArray &endDateTime, const QByteArray &durationStr,
                                const QByteArray &barSizeSetting, const QByteArray &whatToShow, int useRTH, int formatDate, const QList<TagValue*> & chartOptions)
{
    m_reqHistoricalDataTimer->start(1000 * 15);
    emit downloading("Requesting historical data for " + contract.symbol + " .. request timeout set for 15 seconds");
    m_ibqt->reqHistoricalData(tickerId, contract, endDateTime, durationStr, barSizeSetting, whatToShow, useRTH, formatDate, chartOptions);
}

//void Manager::convertSqlToHdf5(Symbol *s)
//{
//    QSqlTableModel* model = s->model;

//    QSqlRecord firstRecord = model->record(0);
//    QSqlRecord lastRecord  = model->record(model->rowCount()-1);

//    QDateTime fdts = QDateTime::fromTime_t(firstRecord.value("timestamp").toUInt());
//    QDateTime ldts = QDateTime::fromTime_t(lastRecord.value("timestamp").toUInt());

//    IbHdf5* ibh5 = m_hdf5Map[s];

////    herr_t H5TBread_fields_name ( hid_t loc_id, const char *table_name, const char * field_names, hsize_t start,
////                                  hsize_t nrecords, size_t type_size,  const size_t *field_offset, const size_t *dst_sizes, void *data)

////    QDateTime fdth =
//}
































