#ifndef RECORD_H
#define RECORD_H

#include <QString>

struct Record
{
    uint        timestamp;
    QString     timeString;
    double      open;
    double      high;
    double      low;
    double      close;
    uint        volume;
    QString     sqlTimeFrame;
};

struct Record2
{
    uint    timestamp;
    char    timeString[16];
    double  open;
    double  high;
    double  low;
    double  close;
    uint    volume;
    char    sqlTimeFrame[5];
};

struct RtRecord
{
    uint    timestamp;
    double  last;
    int     size;
    long    reqId;
};

#endif // RECORD_H
