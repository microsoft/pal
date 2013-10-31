/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Defines the statistical disk information instance PAL for logical disks.
    
    \date        2008-04-28 15:20:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef STATISTICALLOGICALDISKINSTANCE_H
#define STATISTICALLOGICALDISKINSTANCE_H

#include <scxsystemlib/statisticaldiskinstance.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Represents a single statistical logical disk instance.
    */
    class StatisticalLogicalDiskInstance : public StatisticalDiskInstance
    {
        friend class StatisticalLogicalDiskEnumeration;
    public:
        enum eDiskstatsStandardColumns
        {
            eDeviceMajorId = 0,              //!< column  1: The device major ID number
            eDeviceMinorId,                  //!< column  2: The device minor ID number
            eDeviceName                      //!< column  3: The device name
        };

        enum eDiskstatsDiskColumns
        {
            eNumberOfReadsCompleted = 3,     //!< column  4, Field  1: This is the total number of reads completed successfully.
            eNumberOfReadsMerged,            //!< column  5, Field  2: Reads which are adjacent to each other may be merged for efficiency.
            eNumberOfSectorsRead,            //!< column  6, Field  3: This is the total number of sectors read successfully.
            eMillisecondsSpentReading,       //!< column  7, Field  4: This is the total number of milliseconds spent by all reads.
            eNumberOfWritesCompleted,        //!< column  8, Field  5: This is the total number of writes completed successfully.
            eNumberOfWritesMerged,           //!< column  9, Field  6: Writes which are adjacent to each other may be merged for efficiency.
            eNumberOfSectorsWritten,         //!< column 10, Field  7: This is the total number of sectors written successfully.
            eMillisecondsSpentWriting,       //!< column 11, Field  8: This is the total number of milliseconds spent by all writes.
            eNumberOfInProgressIO,           //!< column 12, Field  9: The only field that should go to zero (all start at zero).
            eMillisecondsSpentInIO,          //!< column 13, Field 10: This field is increases so long as field 9 is nonzero.
            eWeightedIOTime,                 //!< column 14, Field 11: This can provide an easy measure of both I/O completion time and the backlog that may be accumulating.
            eNumberOfDiskColumns             //!< this is the number of columns for a disk
        };

        enum eDiskstatsPartitionColumns
        {
            eNumberOfReadsIssued = 3,        //!< column  4, Field  1: This is the total number of reads issued to this partition.
            eNumberOfReadSectorRequests,     //!< column  5, Field  2: This is the total number of sectors requested to be read from this partition.
            eNumberOfWritesIssued,           //!< column  6, Field  3: This is the total number of writes issued to this partition.
            eNumberOfWriteSectorRequests,    //!< column  7, Field  4: This is the total number of sectors requested to be written to this partition.
            eNumberOfPartitionColumns        //!< this is the number of columns for a partition
        };

        StatisticalLogicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps, bool isTotal = false);
        virtual bool GetReadsPerSecond(scxulong& value) const;
        virtual bool GetWritesPerSecond(scxulong& value) const;
        virtual bool GetTransfersPerSecond(scxulong& value) const;
        virtual bool GetBytesPerSecond(scxulong& read, scxulong& write) const;
        virtual bool GetBytesPerSecondTotal(scxulong& total) const;
        virtual bool GetIOTimes(double& read, double& write) const;
        virtual bool GetIOTimesTotal(double& total) const;
        virtual bool GetDiskQueueLength(double& value) const;

        virtual void Sample();

        virtual bool GetLastMetrics(scxulong& numR, scxulong& numW, scxulong& bytesR, scxulong& bytesW, scxulong& msR, scxulong& msW) const;
    private:
        int m_NrOfFailedFinds; //!< Number of consecutive failed calls to FindDeviceInstance.
    };
}
#endif /* STATISTICALLOGICALDISKINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
