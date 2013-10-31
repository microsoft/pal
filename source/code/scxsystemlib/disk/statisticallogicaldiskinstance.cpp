/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implements the logical disk instance pal for statistical information.
    
    \date        2008-04-28 15:20:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/statisticallogicaldiskinstance.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::StatisticalDiskInstance
*/
    StatisticalLogicalDiskInstance::StatisticalLogicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps, bool isTotal /* = false*/)
        : StatisticalDiskInstance(deps, isTotal),
          m_NrOfFailedFinds(0)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.statisticallogicaldiskinstance");
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetReadsPerSecond
*/
    bool StatisticalLogicalDiskInstance::GetReadsPerSecond(scxulong& value) const
    {
#if defined(aix)
        value = 0;
        return false;
#else
        return StatisticalDiskInstance::GetReadsPerSecond(value);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetWritesPerSecond
*/
    bool StatisticalLogicalDiskInstance::GetWritesPerSecond(scxulong& value) const
    {
#if defined(aix)
        value = 0;
        return false;
#else
        return StatisticalDiskInstance::GetWritesPerSecond(value);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetTransfersPerSecond
*/
    bool StatisticalLogicalDiskInstance::GetTransfersPerSecond(scxulong& value) const
    {
#if defined(aix)
        value = 0;
        return false;
#else
        return StatisticalDiskInstance::GetTransfersPerSecond(value);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetBytesPerSecond
*/
    bool StatisticalLogicalDiskInstance::GetBytesPerSecond(scxulong& read, scxulong& write) const
    {
#if defined(aix)
        read = write = 0;
        return false;
#else
        return StatisticalDiskInstance::GetBytesPerSecond(read, write);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetBytesPerSecondTotal
*/
    bool StatisticalLogicalDiskInstance::GetBytesPerSecondTotal(scxulong& total) const
    {
#if defined(aix)
        total = 0;
        return false;
#else
        return StatisticalDiskInstance::GetBytesPerSecondTotal(total);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetIOTimes
*/
    bool StatisticalLogicalDiskInstance::GetIOTimes(double& read, double& write) const
    {
#if defined(aix) || defined(linux)
        read = write = 0;
        return false;
#else
        return StatisticalDiskInstance::GetIOTimes(read, write);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetIOTimesTotal
*/
    bool StatisticalLogicalDiskInstance::GetIOTimesTotal(double& total) const
    {
#if defined(aix) || defined(linux)
        total = 0;
        return false;
#else
        return StatisticalDiskInstance::GetIOTimesTotal(total);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetDiskQueueLength
*/
    bool StatisticalLogicalDiskInstance::GetDiskQueueLength(double& queueLength) const
    {
#if defined(sun)
        return StatisticalDiskInstance::GetDiskQueueLength(queueLength);
#else
        queueLength = 0;
        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::StatisticalDiskInstance::Sample
    */
    void StatisticalLogicalDiskInstance::Sample()
    {
#if defined(hpux)
        SCXCoreLib::SCXHandle<DeviceInstance> di = m_deps->FindDeviceInstance(m_device);

        if ((0 == di) || (DiskDepend::s_cINVALID_INSTANCE == di->m_instance))
        {
            if (m_NrOfFailedFinds < 10)
            {
                SCX_LOGERROR(m_log, SCXCoreLib::StrAppend(L"Unable to find disk in device map: ", m_device));
                ++m_NrOfFailedFinds;
            }
            else if (m_NrOfFailedFinds == 10)
            {
                SCX_LOGERROR(m_log, SCXCoreLib::StrAppend(L"Unable to find disk in device map: ", m_device)
                             .append(L" This has happened 10 times in a row for this device and will not be reported again."));
                ++m_NrOfFailedFinds;
            }
            return;
        }
        m_NrOfFailedFinds = 0;
            
        m_timeStamp.AddSample(time(0));

        struct pst_lvinfo lvi;
        memset(&lvi, 0, sizeof(lvi));
        if (di->m_instance < 0 || 1 != m_deps->pstat_getlv(&lvi, sizeof(lvi), 1, di->m_instance))
        { // Check if the struct has moved
            di->m_instance = FindLVInfoByID(di->m_devID);
            if (di->m_instance < 0 || 1 != m_deps->pstat_getlv(&lvi, sizeof(lvi), 1, di->m_instance))
            {
                SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"No instance for: ", m_device));
                return;
            }
        }
        // Sanity check of cahed instance id
        if (di->m_devID != ((lvi.psl_dev.psd_major << 24) | lvi.psl_dev.psd_minor))
        {
            SCX_LOGWARNING(m_log, L"Instance changed");
            di->m_instance = FindLVInfoByID(di->m_devID);
            return;
        }
        m_reads.AddSample(lvi.psl_rxfer);
        m_writes.AddSample(lvi.psl_wxfer);
        m_rBytes.AddSample(lvi.psl_rcount);
        m_wBytes.AddSample(lvi.psl_wcount);
        m_transfers.AddSample(m_reads[0] + m_writes[0]);
        m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);
#elif defined(linux)
        std::wstring device = m_device;
        std::vector<std::wstring> parts;
        std::wstringstream out;

        if (!m_samplerDevices.empty())
        {
            // this is an LVM partition
            //
            // Note: m_samplerDevices is a vector due to the original v1 implementation.
            //       After fixing the LVM support for CU5, it could be changed to
            //       a simple string, but it was left as is to reduce total code
            //       change.
            SCXASSERT( 1 == m_samplerDevices.size() );

            device = m_samplerDevices[0];
        }

        parts = m_deps->GetProcDiskStats(device);

        m_timeStamp.AddSample(time(0));

        if (parts.size() == eNumberOfDiskColumns)
        {
            // this looks like a diskstats entry for a disk-type device
            try
            {
                m_reads.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfReadsCompleted]));
                m_writes.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfWritesCompleted]));
                m_rBytes.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfSectorsRead])*m_sectorSize);
                m_wBytes.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfSectorsWritten])*m_sectorSize);
                m_transfers.AddSample(m_reads[0] + m_writes[0]);
                m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);
            }
            catch (const SCXCoreLib::SCXNotSupportedException& e)
        {
                out.str(L"");

                out << L"Could not parse disk device line from diskstats for device \"" << device << L"\" - " << e.What();
                SCX_LOGWARNING(m_log, out.str());
            }
        }
        else if (parts.size() == eNumberOfPartitionColumns)
        {
            // this looks like a diskstats entry for a partition-type device
            try
            {
                m_reads.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfReadsIssued]));
                m_writes.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfWritesIssued]));
                m_rBytes.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfReadSectorRequests])*m_sectorSize);
                m_wBytes.AddSample(SCXCoreLib::StrToULong(parts[eNumberOfWriteSectorRequests])*m_sectorSize);
                m_transfers.AddSample(m_reads[0] + m_writes[0]);
                m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);
            }
            catch (const SCXCoreLib::SCXNotSupportedException& e)
            {
                out.str(L"");

                out << L"Could not parse partition device line from diskstats for device \"" << device << L"\" - " << e.What();
                SCX_LOGWARNING(m_log, out.str());
            }
        }
        else
        {
            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);

            out.str(L"");

            // Note: If this message shows up in the logs and the device in question
            //       shouldn't even be getting enumerated, then maybe the device
            //       type needs to be added to the list of ignored device types in
            //       diskdepend (see WI 33450).
            out << L"The diskstats map does not contain a key matching the device named \"" << device << L"\", or only " << parts.size() << L" columns were found";
            SCX_LOG(m_log, suppressor.GetSeverity(out.str()), out.str());
        }
#elif defined(sun)
        std::wstringstream out;
        out << L"Sample : Entering";
        SCX_LOGHYSTERICAL(m_log, out.str());

        try
        {
            if ( ! m_deps->ReadKstat(m_kstat, m_device, m_mountPoint))
            {
                out.str(L"");
                out << L"Sample : Failed : Unable to determine kstat parameters for device " << m_device;

                SCX_LOGTRACE(m_log, out.str());
                return;
            }
        }
        catch (SCXCoreLib::SCXException& exception)
        {
            out.str(L"");
            out << L"Sample : Error : An unexpected exception prevented reading kstat for device " << m_device
                << L" : " << typeid(exception).name()
                << L" : " << exception.What()
                << L" : " << exception.Where();

            SCX_LOGERROR(m_log, out.str());
            return;
        }

        try
        {
            SCXKstatFSSample sample = m_kstat->GetFSSample();

            m_reads.AddSample(sample.GetNumReadOps());
            m_writes.AddSample(sample.GetNumWriteOps());
            m_transfers.AddSample(m_reads[0] + m_writes[0]);
            m_rBytes.AddSample(sample.GetBytesRead());
            m_wBytes.AddSample(sample.GetBytesWritten());
            m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);

            out.str(L"");
            out << L"Sample : Succeeded : Got kstat sample for device " << m_device
                << L", nR: " << m_reads[0]
                << L", nw: " << m_writes[0]
                << L", bR: " << m_rBytes[0]
                << L", bW: " << m_wBytes[0];
            SCX_LOGHYSTERICAL(m_log, out.str());
        }
        catch (SCXKstatException& exception)
        {
            out.str(L"");
            out << L"Sample : Error : An unexpected exception prevented sampling the kstat data for device " << m_device
                << L" : " << typeid(exception).name()
                << L" : " << exception.What()
                << L" : " << exception.Where();

            SCX_LOGERROR(m_log, out.str());
        }
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetLastMetrics
*/
    bool StatisticalLogicalDiskInstance::GetLastMetrics(scxulong& numR, scxulong& numW, scxulong& bytesR, scxulong& bytesW, scxulong& msR, scxulong& msW) const
    {
#if defined(aix)
        numR = numW = bytesR = bytesW = msR = msW = 0;
        return true;
#else
        if (0 == m_reads.GetNumberOfSamples())
        {
            return false;
        }
        numR = m_reads[0];
        if (0 == m_writes.GetNumberOfSamples())
        {
            return false;
        }
        numW = m_writes[0];
        if (0 == m_rBytes.GetNumberOfSamples())
        {
            return false;
        }
        bytesR = m_rBytes[0];
        if (0 == m_wBytes.GetNumberOfSamples())
        {
            return false;
        }
        bytesW = m_wBytes[0];
#if defined(linux) || defined(hpux) || defined(solaris)
        msR = 0;
        msW = 0;
#else
        if (0 == m_runTimes.GetNumberOfSamples())
        {
            return false;
        }
        msR = m_runTimes[0];
        if (0 == m_waitTimes.GetNumberOfSamples())
        {
            return false;
        }
        msW = m_waitTimes[0];
#endif
        return true;
#endif
    }
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
