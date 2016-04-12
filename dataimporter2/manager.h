#ifndef MANAGER_H
#define MANAGER_H

#include "ibqt.h"
#include "ibhdf5.h"
#include <QObject>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QMap>

class Symbol;
class Record;
class QSqlTableModel;
class QTimer;

enum TimeFrame
{
    SEC_1,
    SEC_5,
    SEC_15,
    SEC_30,
    MIN_1,
    MIN_2,
    MIN_3,
    MIN_5,
    MIN_15,
    MIN_30,
    HOUR_1,
    DAY_1,
    RAW
};


class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = 0);

    void login(const QString & url, int port, int clientId);

    void logout();

    bool initializeSqlDatabase();

    void downloadQuotes();

    void setReconnectOnFailure(bool reconnectOnFailure);

    void setSqlHostName(const QString &sqlHostName);

    void setSqlPort(int sqlPort);

    void setSqlUserName(const QString &sqlUserName);

    void setSqlPassword(const QString &sqlPassword);

    void setSqlDatabaseName(const QString &sqlDatabaseName);

    void setSqlSymbolTableName(const QString &sqlSymbolTableName);

    void setSqlOutputDatabaseName(const QString &sqlOutputDatabaseName);

    QSqlTableModel *symbolTableModel() const;

    void setHdf5OutputFolderPath(const QString &hdf5OutputFolderPath);

    void setTimeFrame(const TimeFrame &timeFrame);

    void setNumberOfMonths(int numberOfMonths);

    void setStopButtonClicked(bool stopButtonClicked);

    void setRealTimeDataEnabled(bool autoDownloadEnabled);

    void setUseSql(bool useSql);

    void setUseHdf5(bool useHdf5);

    bool isConnected() const;

signals:
    void connected();
    void downloading(const QString & name);

public slots:

private slots:
    void onTwsConnected();
    void onManagedAccounts(const QByteArray & accountList);
    void onContractDetails(int reqId, const ContractDetails & contractDetails);
    void onContractDetailsEnd(int reqId);
    void onHistoricalData(long reqId, const QByteArray & date,
                          double open, double high, double low, double close,
                          int volume, int barCount, double wap, int hasGaps);
    void onTickPrice(const long & tickerId, const TickType & field, const double & price, const int & canAutoExecute);
    void onTickSize(const long & tickerId, const TickType & field, int size);
    void onError(const int id, const int errorCode, const QByteArray errorString);
    void onIbSocketError(const QString & errorString);
    void onConnectionClosed();
    void onLoginTimerTimeout();
    void onRealTimeDataTimerTimeout();
    void onRequestedHistoricalDataTimerTimeout();
    void onRequestedContractDetailsTimerTimeout();

//    void onLiquidTradingHoursTimerTimeout();
    void onCurrentTimeTimerTimeout();


private:
    IBQt*               m_ibqt;
    QString             m_ibUrl;
    int                 m_ibPort;
    int                 m_ibClientId;
    int                 m_loginAttemptNumber;
    bool                m_reconnectOnFailure;
    QString             m_sqlHostName;
    int                 m_sqlPort;
    QString             m_sqlUserName;
    QString             m_sqlPassword;
    QString             m_sqlDatabaseName;
    QString             m_sqlSymbolTableName;
    QString             m_sqlOutputDatabaseName;
    QSqlDatabase        m_db;
    QSqlTableModel*     m_symbolTableModel;
    QString             m_hdf5OutputFolderPath;
    TimeFrame           m_timeFrame;
    QString             m_timeFrameString;
    int                 m_timeFrameInSeconds;
    QString             m_durationStr;
    int                 m_numberOfMonths;
    bool                m_stopButtonClicked;
    bool                m_realTimeDataEnabled;
    bool                m_useSql;
    bool                m_useHdf5;
    QMap<int, Symbol*>  m_symbolMap;
    bool                m_lock;
    QStringList         m_managedAccounts;
    QStringList         m_barSizes;
    QVector<QSqlRecord> m_cdtData;
    QVector<QSqlRecord> m_edtData;
    QTimer*             m_loginTimer;
    QTimer*             m_realTimeDataTimer;
    QTimer*             m_reqHistoricalDataTimer;
    QTimer*             m_reqContractDetailsTimer;
//    QTimer*             m_liquidTradingHoursTimer;
    QTimer*             m_currentTimeTimer;
    QVector<long>       m_realTimeIds;
    QMap<Symbol*,IbHdf5*> m_hdf5Map;
    bool                m_isConnected;
    long                m_lastReqId;
    QDateTime           m_lastDt;
    QDateTime           m_liquidHoursStartTime;
    QDateTime           m_liquidHoursEndTime;
    bool                m_attemptingLogIn;
    QDateTime           m_currentTime;


    void        delay(int milliseconds);
    QDateTime   empiricalDataComplete(long reqId);
    QDateTime   currentDataComplete(long reqId);
    QString     ibEndDateTimeToString(const QDateTime & edt);
    void        sqlSubmit(long reqId);
    void        parseLiquidHours(Symbol *s);
    bool        timeIsInLiquidTradingHours(Symbol *s, const QDateTime & dt);
    bool        timeIsSameTradingDay(Symbol *s, const QDateTime & dt);
    void        reqHistoricalData(long tickerId, const Contract &contract, const QByteArray &endDateTime, const QByteArray &durationStr,
                                  const QByteArray &barSizeSetting, const QByteArray &whatToShow, int useRTH, int formatDate, const QList<TagValue *> &chartOptions);
//    void        convertSqlToHdf5(Symbol* s);
};

#endif // MANAGER_H
