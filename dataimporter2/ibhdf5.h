#ifndef IBHDF5_H
#define IBHDF5_H

#include <QObject>
#include <QVector>
#include <hdf5.h>
#include <hdf5_hl.h>
#include "record.h"
#include "symbol.h"

class Symbol;


class IbHdf5 : public QObject
{
    Q_OBJECT
public:
    IbHdf5(const QString & tableName, const QString & filePath, QObject *parent = 0);
//    ~IbHdf5();

    bool writeRecords(Record2 *recArray, int numRecords);

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
};

#endif // IBHDF5_H
