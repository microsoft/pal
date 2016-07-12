/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implements the physical disk instance pal for statistical information.
    
    \date        2008-04-28 15:20:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/statisticalphysicaldiskinstance.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>

#if defined(aix)
#include <sys/systemcfg.h>
#endif

namespace SCXSystemLib
{

    size_t StatisticalPhysicalDiskInstance::m_currentInstancesCount = 0;
    size_t StatisticalPhysicalDiskInstance::m_instancesCountSinceModuleStart = 0;

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::StatisticalDiskInstance
*/
    StatisticalPhysicalDiskInstance::StatisticalPhysicalDiskInstance(SCXCoreLib::SCXHandle<DiskDepend> deps, bool isTotal /* = false*/) : StatisticalDiskInstance(deps, isTotal)
    {
        m_currentInstancesCount++;
        m_instancesCountSinceModuleStart++;
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.statisticalphysicaldiskinstance");
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetReadsPerSecond
*/
    bool StatisticalPhysicalDiskInstance::GetReadsPerSecond(scxulong& value) const
    {
#if defined(hpux)
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
    bool StatisticalPhysicalDiskInstance::GetWritesPerSecond(scxulong& value) const
    {
#if defined(hpux)
        value = 0;
        return false;
#else
        return StatisticalDiskInstance::GetWritesPerSecond(value);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetBytesPerSecond
*/
    bool StatisticalPhysicalDiskInstance::GetBytesPerSecond(scxulong& read, scxulong& write) const
    {
#if defined(hpux)
        read = write = 0;
        return false;
#else
        return StatisticalDiskInstance::GetBytesPerSecond(read, write);
#endif
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetDiskSize
*/
    bool StatisticalPhysicalDiskInstance::GetDiskSize(scxulong& mbUsed, scxulong& mbFree) const
    {
        mbUsed = mbFree = 0;
        return false;
    }

/*----------------------------------------------------------------------------*/
/**
   \copydoc SCXSystemLib::StatisticalDiskInstance::GetBlockSize
*/
    bool StatisticalPhysicalDiskInstance::GetBlockSize(scxulong& blockSize) const
    {
        blockSize = 0;
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
       \copydoc SCXSystemLib::StatisticalDiskInstance::Sample
    */
    void StatisticalPhysicalDiskInstance::Sample()
    {
#if defined(aix)
        perfstat_id_t id;
        perfstat_disk_t data;

        memset(&id, 0, sizeof(id));

        std::wstring name = L"";
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eInfo);
        if (m_device.substr(0,5) == L"/dev/")
        {
            // Some device paths can have subdirectories inside /dev/, like: /dev/asm/acfs_vol001-41
            // perfstat_disk is expecting id.name to look like "asm/acfs_vol001-41" in this example.
            name = m_device.substr(5);
        }
        else
        {
            std::wstring msg = L"Device path (" + m_device + L") does not begin with /dev/";

            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(msg));
            SCX_LOG(m_log, severity, msg);
            return;
        }

        strncpy(id.name, SCXCoreLib::StrToUTF8(name).c_str(), sizeof(id.name)-1);
        int retval;
        if (1 == (retval = m_deps->perfstat_disk(&id, &data, sizeof(data), 1)))
        {
            m_transfers.AddSample(data.xfers);
            m_rBytes.AddSample(data.rblks * data.bsize);
            m_wBytes.AddSample(data.wblks * data.bsize);
            m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);
            m_tTimes.AddSample(data.time * 1000);

/* XINTFRAC definition now depends on patch level of AIX 6.1 system ... */
#if PF_MAJOR < 7 && !defined(XINTFRAC)

/* See /usr/include/sys/iplcb.h to explain the below */
#define XINTFRAC        (static_cast<double>(_system_configuration.Xint)/static_cast<double>(_system_configuration.Xfrac))

#endif

/* hardware ticks per millisecond */
#define HWTICS2MSECS(x)    ((static_cast<double>(x) * XINTFRAC)/1000000.0)

            m_rTimes.AddSample(HWTICS2MSECS(data.rserv));
            m_wTimes.AddSample(HWTICS2MSECS(data.wserv));
            m_qLengths.AddSample(data.qdepth);
        }
        else
        {
            SCXCoreLib::SCXErrnoException e(L"name = " + name, errno, SCXSRCLOCATION);
            std::wstring msg = L"perfstat_disk failed with retval = " + SCXCoreLib::StrFrom(retval) + L" " + e.What();

            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(msg));
            SCX_LOG(m_log, severity, msg);
            return;
        }
#elif defined(hpux)
        SCXCoreLib::SCXHandle<DeviceInstance> di = m_deps->FindDeviceInstance(m_device);

        if ((0 == di) || (DiskDepend::s_cINVALID_INSTANCE == di->m_instance))
        {
            SCX_LOGERROR(m_log, L"Unable to find disk in device map");
            return;
        }
        m_timeStamp.AddSample(time(0));
        
        struct pst_diskinfo diski;
        memset(&diski, 0, sizeof(diski)); 
        if (1 != m_deps->pstat_getdisk(&diski, sizeof(diski), 1, di->m_instance))
        {
            SCX_LOGERROR(m_log, L"pstat_getdisk failed");
            return;
        }
        // Sanity check of cahed instance id
        if (di->m_devID != ((diski.psd_dev.psd_major << 24) | diski.psd_dev.psd_minor))
        {
            SCX_LOGWARNING(m_log, L"Instance changed");
            di->m_instance = FindDiskInfoByID(di->m_devID);
            return;
        }
        m_transfers.AddSample(diski.psd_dkxfer);
        m_tBytes.AddSample(diski.psd_dkwds * 64);
        m_tTimes.AddSample(diski.psd_dkresp.pst_sec * 1000 + diski.psd_dkresp.pst_usec / 1000);
        m_waitTimes.AddSample(diski.psd_dkwait.pst_sec * 1000 + diski.psd_dkwait.pst_usec / 1000);
        m_qLengths.AddSample(diski.psd_dkqlen_curr);
#elif defined(linux)
        std::vector<std::wstring> parts = m_deps->GetProcDiskStats(m_device);
        for (size_t i=0; parts.size() == 0 && i < m_samplerDevices.size(); ++i)
        {
            parts = m_deps->GetProcDiskStats(m_samplerDevices[i]);
        }
        m_timeStamp.AddSample(time(0));
        if (parts.size() > 11)
        {
            try
            {
                m_reads.AddSample(SCXCoreLib::StrToULong(parts[3]));
                m_writes.AddSample(SCXCoreLib::StrToULong(parts[7]));
                m_rBytes.AddSample(SCXCoreLib::StrToULong(parts[5])*m_sectorSize);
                m_wBytes.AddSample(SCXCoreLib::StrToULong(parts[9])*m_sectorSize);
                m_rTimes.AddSample(SCXCoreLib::StrToULong(parts[6]));
                m_wTimes.AddSample(SCXCoreLib::StrToULong(parts[10]));
                m_transfers.AddSample(m_reads[0] + m_writes[0]);
                m_tBytes.AddSample(m_rBytes[0] + m_wBytes[0]);
                m_qLengths.AddSample(SCXCoreLib::StrToULong(parts[11]));
            }
            catch (const SCXCoreLib::SCXNotSupportedException& e)
            {
                SCX_LOGWARNING(m_log, std::wstring(L"Could not parse line from diskstats: ").append(L" - ").append(e.What()));
            }
        }
#elif defined(sun)
        std::wstringstream out;
        out << L"Sample : Entering";
        SCX_LOGHYSTERICAL(m_log, out.str());

        try
        {
            if ( ! m_deps->ReadKstat(m_kstat, m_device))
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
            m_reads.AddSample(m_kstat->GetValue(L"reads"));
            m_writes.AddSample(m_kstat->GetValue(L"writes"));
            m_transfers.AddSample(m_reads[0] + m_writes[0]); 
            m_rBytes.AddSample(m_kstat->GetValue(L"nread"));
            m_wBytes.AddSample(m_kstat->GetValue(L"nwritten"));
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
    bool StatisticalPhysicalDiskInstance::GetLastMetrics(scxulong& numR, scxulong& numW, scxulong& bytesR, scxulong& bytesW, scxulong& msR, scxulong& msW) const
    {
#if defined(aix) || defined(hpux)
        if (0 == m_transfers.GetNumberOfSamples())
        {
            return false;
        }
        numR = m_transfers[0];
        numW = 0;
#endif
#if defined(hpux)
        if (0 == m_tBytes.GetNumberOfSamples())
        {
            return false;
        }
        bytesR = m_tBytes[0];
        bytesW = 0;
#else
#if ! defined(aix)
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
#endif
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
#endif

#if defined(hpux)
        if (0 == m_tTimes.GetNumberOfSamples() || 0 == m_waitTimes.GetNumberOfSamples())
        {
            return false;
        }
        msW = m_waitTimes[0];
        msR = m_tTimes[0] - msW;
#elif defined(aix) || defined(linux)
        if (0 == m_rTimes.GetNumberOfSamples())
        {
            return false;
        }
        msR = m_rTimes[0];
        if (0 == m_wTimes.GetNumberOfSamples())
        {
            return false;
        }
        msW = m_wTimes[0];
#elif defined(sun)
        msR = 0;
        msW = 0;
#endif
        return true;
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
