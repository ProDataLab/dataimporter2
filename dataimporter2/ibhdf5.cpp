#include "ibhdf5.h"
#include "record.h"
#include <string.h>
#include <QFileInfo>

#define TRUE 1
#define FALSE 0

IbHdf5::IbHdf5(const QString &tableName, const QString &filePath, QObject *parent)
    : m_tableName(tableName)
    , m_filePath(filePath)
    , QObject(parent)
{

//    struct Record2
//    {
//        uint timestamp;
//        char* timeString;
//        double open;
//        double high;
//        double low;
//        double close;
//        uint volume;
//        char* sqlTimeFrame;
//    };

    m_nFields = 8;

//    m_dst_size = sizeof(Record2);

    m_dst_offset[0] = HOFFSET(Record2, timestamp);
    m_dst_offset[1] = HOFFSET(Record2, timeString);
    m_dst_offset[2] = HOFFSET(Record2, open);
    m_dst_offset[3] = HOFFSET(Record2, high);
    m_dst_offset[4] = HOFFSET(Record2, low);
    m_dst_offset[5] = HOFFSET(Record2, close);
    m_dst_offset[6] = HOFFSET(Record2, volume);
    m_dst_offset[7] = HOFFSET(Record2, sqlTimeFrame);


    m_dst_sizes[0] = sizeof(uint);
    m_dst_sizes[1] = 24;
    m_dst_sizes[2] = sizeof(double);
    m_dst_sizes[3] = sizeof(double);
    m_dst_sizes[4] = sizeof(double);
    m_dst_sizes[5] = sizeof(double);
    m_dst_sizes[6] = sizeof(uint);
    m_dst_sizes[7] = 5;


    m_fieldNames[0] = "timestamp";
    m_fieldNames[1] = "timestring";
    m_fieldNames[2] = "open";
    m_fieldNames[3] = "high";
    m_fieldNames[4] = "low";
    m_fieldNames[5] = "close";
    m_fieldNames[6] = "volume";
    m_fieldNames[7] = "timeframe";


    m_chunkSize = 10;

    hid_t timeStringType;
    hid_t timeFrameType;
    m_fillData = NULL;
    m_compress = 0;

    timeStringType = H5Tcopy(H5T_C_S1);
    H5Tset_size(timeStringType, 24);

    timeFrameType = H5Tcopy(H5T_C_S1);
    H5Tset_size(timeFrameType, 5);


    m_fieldType[0] = H5T_NATIVE_UINT;
    m_fieldType[1] = timeStringType;
    m_fieldType[2] = H5T_NATIVE_DOUBLE;
    m_fieldType[3] = H5T_NATIVE_DOUBLE;
    m_fieldType[4] = H5T_NATIVE_DOUBLE;
    m_fieldType[5] = H5T_NATIVE_DOUBLE;
    m_fieldType[6] = H5T_NATIVE_UINT;
    m_fieldType[7] = timeFrameType;

    // CREATE OR OPEN THE FILE
    if (!QFileInfo::exists(m_filePath))
        m_fid = H5Fcreate (m_filePath.toLatin1().constData(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    else
        m_fid = H5Fopen(m_filePath.toLatin1().data(), H5F_ACC_RDWR, H5P_DEFAULT);


}

bool IbHdf5::writeRecords(Record2* recArray, int numRecords)
{

    if (H5Lexists(m_fid, m_tableName.toLatin1().data(), H5P_DEFAULT) == FALSE)
        H5TBmake_table(m_tableName.toLatin1().data(), m_fid, m_tableName.toLatin1().data(),
                   m_nFields, numRecords, sizeof(Record2), m_fieldNames, m_dst_offset,
                   m_fieldType, m_chunkSize, m_fillData, m_compress, recArray);
    else {
        H5TBappend_records(m_fid, m_tableName.toLatin1().data(), (hsize_t)numRecords, sizeof(Record2), m_dst_offset, m_dst_sizes, recArray);
    }
}














































