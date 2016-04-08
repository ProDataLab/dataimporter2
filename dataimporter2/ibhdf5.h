#ifndef IBHDF5_H
#define IBHDF5_H

#include <QObject>
#include <QVector>
#include <hdf5.h>
#include <hdf5_hl.h>
#include "record.h"
#include "symbol.h"
#include <QDateTime>

class Symbol;


class IbHdf5 : public QObject
{
    Q_OBJECT
public:
    IbHdf5(const QString & tableName, const QString & filePath, QObject *parent = 0);
//    ~IbHdf5();

    bool writeRecords(Record2 *recArray, int nRecords);
    bool appendRecord(Record2* record);

    hsize_t numRecords() const;

    hsize_t numFields() const;



private:
    QString m_tableName;
    QString m_filePath;
    hsize_t m_nFields;
    size_t m_dst_offset[8];
    size_t m_dst_sizes[8];
    const char* m_fieldNames[16];
    hsize_t m_chunkSize;
    int* m_fillData;
    int m_compress;


    hid_t m_fid;
    hid_t m_fieldType[8];

    hsize_t* m_numRecords;
    hsize_t* m_numFields;

//    QDateTime
};

#endif // IBHDF5_H
