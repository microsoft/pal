/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file        cpupropertiesenumeration.cpp

  \brief       Enumeration of CPU Properties. 

  \date        11-10-31 10:45:00
 */
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/cpupropertiesenumeration.h>
#include <scxcorelib/stringaid.h>
#include <sstream>

#if defined(sun)
#include <sys/sysinfo.h>
// For running processes and retrieving output
#include <scxcorelib/scxprocess.h>
#endif
#if defined(hpux)
#include <sys/pstat.h>
#endif

#if defined (linux)
#include <scxcorelib/scxfile.h>
#endif

using namespace SCXCoreLib;
using namespace std;

namespace SCXSystemLib
{

#if defined(linux) 
    /** The type value of Processor structure*/ 
    const int cProcessorInformation = 0x04;
#endif


    /*----------------------------------------------------------------------------*/
    /**
      Constructor. 

     */
#if defined(linux) 
    CpuPropertiesEnumeration::CpuPropertiesEnumeration(SCXCoreLib::SCXHandle<ProcfsCpuInfoReader> cpuinfoTable):
        m_cpuinfoTable(cpuinfoTable)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.CpuPropertiesEnumeration"));
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesEnumeration default constructor: "));
    }
#elif defined(sun) 
    CpuPropertiesEnumeration::CpuPropertiesEnumeration(SCXCoreLib::SCXHandle<CpuPropertiesPALDependencies> deps):
    m_deps(deps)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.CpuPropertiesEnumeration"));
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesEnumeration default constructor"));
    }
#elif defined(aix)  || defined(hpux)
    CpuPropertiesEnumeration::CpuPropertiesEnumeration()
    {
         m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.CpuPropertiesEnumeration"));
         SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesEnumeration default constructor"));
    }
#endif  


    /*----------------------------------------------------------------------------*/
    /**
      Create CpuPropertiesEnumeration instances.
     */
    void CpuPropertiesEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"CpuPropertiesEnumeration Init()");

#if defined(linux) 
        try
        {
            if (m_cpuinfoTable->Load())
            {
			    SCX_LOGTRACE(m_log, wstring(L"CPU info table load successfully."));
            }
			else
			{
               throw SCXCoreLib::SCXInvalidStateException(L"Procfs cpuinfo are unreadable.", SCXSRCLOCATION);
			}
        }
        catch(SCXCoreLib::SCXInternalErrorException e)
        {
            SCX_LOGERROR(m_log, L"Unable to retrieve cpu information. " + e.What());
            throw;
        }
#elif defined(sun) 
        m_deps->Init();
#elif defined(aix) 
        int rc = perfstat_partition_total(NULL, &m_partTotal, sizeof(perfstat_partition_total_t), 1);
        if (rc == 0)
        {
            throw SCXCoreLib::SCXInvalidStateException(L"perfstat partition is unavailable.", SCXSRCLOCATION);
        }

        rc = perfstat_cpu_total(NULL, &m_cpuTotal, sizeof(perfstat_cpu_total_t), 1);
        if (rc == 0)
        {
            throw SCXCoreLib::SCXInvalidStateException(L"perfstat cpu is unavailable.", SCXSRCLOCATION);
        }
 #endif
        //
        //Clear all instances first,then load the latest ones.
        //
        Clear();
        Update(false);
    }

    /*----------------------------------------------------------------------------*/
    /**
      Update all the Processor instances.  
      
      This is done collectively for all instances by using a platform dependent 
     */
    void CpuPropertiesEnumeration::Update(bool updateInstances)
    {
        SCX_LOGTRACE(m_log, L"CpuPropertiesEnumeration Update");

        CreateCpuPropertiesInstances();

        if (updateInstances)
        {
            UpdateInstances();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
      Cleanup

     */
    void CpuPropertiesEnumeration::CleanUp()
    {

        SCX_LOGTRACE(m_log, L"CpuPropertiesEnumeration CleanUp()");

    }

    /*----------------------------------------------------------------------------*/
    /**
      Destructor

     */
    CpuPropertiesEnumeration::~CpuPropertiesEnumeration()
    {
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesEnumeration default destructor: "));
    }

#if defined(linux) 
    void CpuPropertiesEnumeration::CreateCpuPropertiesInstances()
    {
	    set<wstring> deviceIds;
        unsigned short physicalCPU;

	    for(ProcfsCpuInfoReader::const_iterator cit = m_cpuinfoTable->begin(); cit != m_cpuinfoTable->end(); cit++)
        {
		    SCX_LOGTRACE(m_log, wstring(L"Reading cpu info table"));
		    //get physical cpuid
			if (cit->PhysicalId(physicalCPU))
			{
			    wstring thisID = StrAppend(wstring(L"CPU "),physicalCPU);
				set<wstring>::iterator itr = deviceIds.find(thisID);

				//new physical ID
			    if (itr == deviceIds.end())	
				{
				    deviceIds.insert(StrAppend(L"CPU ", physicalCPU));
					SCX_LOGTRACE(m_log, L"Added physical cpu for " + thisID);
					AddInstance(SCXCoreLib::SCXHandle<CpuPropertiesInstance>(new CpuPropertiesInstance(thisID, *cit)));
				}
			}
            else //physical id not exist in cpuinfo table
            {
                wstring thisID = cit->CpuKey();
                AddInstance(SCXCoreLib::SCXHandle<CpuPropertiesInstance>(new CpuPropertiesInstance(thisID, *cit)));
            }
        }
    }
#elif defined(sun) 
    /*----------------------------------------------------------------------------*/
    /**
      Read Processor number for creating processor instances.
     */
    void CpuPropertiesEnumeration::CreateCpuPropertiesInstances()
    {
        SCX_LOGTRACE(m_log, L"CpuPropertiesEnumeration CreateCpuPropertiesInstances()");

        scxulong chipId = 999999999;

        unsigned int cpuIndex =0;

        while(1)
        {
            wstringstream cpuInfoName; 
            cpuInfoName << cModul_Name.c_str() << cpuIndex;

            if (!m_deps->Lookup(cModul_Name, cpuInfoName.str(), cInstancesNum)) break;
            scxulong newChipId = 0;
            if (!m_deps->TryGetValue(cAttrName_ChipID, newChipId))
            {
                throw SCXNotSupportedException(L"Chip Id does not exist", SCXSRCLOCATION);
            }
            //
            // Check ChipId. If equal, the Kstat belongs to another core and they are in same processor.
            //
            if (newChipId != chipId)
            {
                chipId = newChipId;
                SCX_LOGTRACE(m_log, StrAppend(L"CpuPropertiesEnumeration Update() - cpuInfoName: ", cpuInfoName.str()));
                AddInstance(SCXCoreLib::SCXHandle<CpuPropertiesInstance>(new CpuPropertiesInstance(cpuInfoName.str(), m_deps)));
            }
            
            cpuIndex++;
        }
        

    }
#elif defined(aix)
    /**
      Add one instance for every physical cpu.
     */
    void CpuPropertiesEnumeration::CreateCpuPropertiesInstances()
    {
        for (unsigned int proc = 0; proc < m_partTotal.online_cpus ; proc++)
        {
            SCXCoreLib::SCXHandle<CpuPropertiesInstance> cpuinst = SCXCoreLib::SCXHandle<CpuPropertiesInstance>(new CpuPropertiesInstance(m_cpuTotal, m_partTotal));
            if (NULL != cpuinst)
            {
                cpuinst->SetId(StrAppend(wstring(L"CPU "), StrFrom(proc)));
                AddInstance(cpuinst);
            }
        }
    }
#elif defined(hpux)
    /**
      Add one instance for every physical cpu.
     */
    void CpuPropertiesEnumeration::CreateCpuPropertiesInstances()
    {
        struct pst_processor psp[PST_MAX_PROCS] = { { 0 } };
		set<wstring> uniquePhysicalIDs;
        struct pst_dynamic psd;

        errno = 0;
        int rc = pstat_getprocessor(psp, sizeof(struct pst_processor), PST_MAX_PROCS, 0);

        if (-1 == rc)
        {
            wstring msg(L"pstat_getprocessor failed. errno ");
            msg.append(StrFrom(errno));
            SCX_LOGTRACE(m_log, msg);
            throw SCXCoreLib::SCXInvalidStateException(msg, SCXSRCLOCATION);
        }

        m_cpuTotal = rc;

        rc = pstat_getdynamic(&psd, sizeof(psd), (size_t)1, 0);

        if (-1 == rc)
        {
            wstring msg(L"pstat_getdynamic failed. errno ");
            msg.append(StrFrom(errno));
            SCX_LOGTRACE(m_log, msg);
            throw SCXCoreLib::SCXInvalidStateException(msg, SCXSRCLOCATION);
        }

        unsigned int cpuId = 0;
        for (unsigned int proc = 0; proc < m_cpuTotal; proc++)
        {
#if (PF_MINOR >= 31) 
		    wstring thisID = StrFrom(psp[proc].psp_socket_id);
            set<wstring>::iterator itr = uniquePhysicalIDs.find(thisID);

            //new physical ID
		    if (itr == uniquePhysicalIDs.end())
            {
                uniquePhysicalIDs.insert(thisID);
				SCX_LOGTRACE(m_log, L"Added physical cpu for socket : " + thisID);
				SCXCoreLib::SCXHandle<CpuPropertiesInstance> cpuinst = 
				                  SCXCoreLib::SCXHandle<CpuPropertiesInstance>(new CpuPropertiesInstance(thisID, psp[proc], psd));
		        if (NULL != cpuinst)
				{
                    cpuinst->SetId(StrAppend(wstring(L"CPU "), cpuId++));
				    AddInstance(cpuinst);
				}
			}
#else // PF_MINOR == 23
            wstring thisID = StrFrom(proc);
            SCXCoreLib::SCXHandle<CpuPropertiesInstance> cpuinst = SCXCoreLib::SCXHandle<CpuPropertiesInstance>(new CpuPropertiesInstance(thisID, psp[proc], psd));
            if (NULL != cpuinst)
            {
                cpuinst->SetId(StrAppend(wstring(L"CPU "), StrFrom(proc)));
                AddInstance(cpuinst);
            }

#endif
        }
    }
#endif


#if defined(sun) 
    /*----------------------------------------------------------------------------*/
    /**
      Retrieve the number of physical CPUs present using the 'psrinfo' command.
      \param       numCpus - The count of the number of physical CPUs returned by psrinfo cmd.
      \returns     true if value was set, otherwise false.
    */
    bool CpuPropertiesEnumeration::GetCpuCount(unsigned int& numCpus)
    {
        SCX_LOGTRACE(m_log, L"CpuPropertiesEnumeration GetCpuCount()");
#if PF_MAJOR == 5 && (PF_MINOR  == 9 || PF_MINOR == 10)
        wstring cmdStringPsrinfo = L"/usr/sbin/psrinfo -p";
#elif PF_MAJOR == 5 && PF_MINOR  == 11
        wstring cmdStringPsrinfo = L"/sbin/psrinfo -p";
#else
#error "Platform not supported"
#endif
        std::istringstream processInputPsrinfo;
        std::ostringstream processOutputPsrinfo;
        std::ostringstream processErrPsrinfo;
        wstring psrinfoResultStr;
        numCpus = 0;
        bool retVal = false;

        try 
        {
            SCXCoreLib::SCXProcess::Run(cmdStringPsrinfo, processInputPsrinfo, processOutputPsrinfo, processErrPsrinfo, 15000);
            psrinfoResultStr = StrFromUTF8(processOutputPsrinfo.str());

            wstring errOutPsr = StrFromUTF8(processErrPsrinfo.str());
            if (errOutPsr.size() > 0)
            {
                SCX_LOGERROR(m_log, StrAppend(wstring(L"Got this error string from psrinfo command: "),errOutPsr));
                return retVal;  //Return F
            }

        }
        catch(SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, L"Unable to retrieve cpu information from OS using 'psrinfo'..." + e.What());
            return retVal;  //Return F
        }


        // Let's check the results
        if (psrinfoResultStr.size() > 0) 
        {
            // Now let's convert the result to an int:
            numCpus = SCXCoreLib::StrToUInt(psrinfoResultStr);
            retVal = true;
        }
        else
        {
            SCX_LOGERROR(m_log, L"Empty results returned from 'psrinfo'");
        }

        // Return success or failure:
        return retVal;
    }

#endif
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
