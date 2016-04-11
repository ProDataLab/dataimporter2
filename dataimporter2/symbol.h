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
    QVector<RtRecord*>          realTimeData;
    QByteArray                  primaryExchange;
    QByteArray                  secType;
    QByteArray                  currency;
    bool                        insertRealTimeData;
    bool                        contractDetailsOnly;
    bool                        isRealTimeId;
    int                         error505Count;
};

#endif // SYMBOL_H
