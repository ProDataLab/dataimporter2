#ifndef SYMBOL_H
#define SYMBOL_H

#include "ibqt.h"
#include "record.h"
#include "ibhdf5.h"
#include <QByteArray>
#include <QSqlRelationalTableModel>
#include <QDateTime>


struct Symbol
{
    QByteArray                  symbolName;
    QByteArray                  tableName;
    int                         currentRow;
    ContractDetails             contractDetails;
    QSqlTableModel*             model;
    QSqlQuery*                  query;
    uint                        rowCount;
    QDateTime                   firstDateTime;
    QDateTime                   lastDateTime;
    QVector<Record*>            data;
    bool                        firstDataDownloaded;
    QString                     timeFrameString;
    long                        realTimeDataId;
    long                        contractDetailsId;
    QVector<RtRecord*>          realTimeData;
    QByteArray                  primaryExchange;
    QByteArray                  secType;
    QByteArray                  currency;
    bool                        insertRealTimeData;
    bool                        contractDetailsOnly;
//    bool                        isRealTimeId;
//    bool                        isContractDetailsId;
    int                         error505Count;
    QDateTime                   liquidHoursStartTime;
    QDateTime                   liquidHoursEndTime;
};

#endif // SYMBOL_H
