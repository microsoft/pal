/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file        computersystemdependencies.cpp

  \brief       Dependencies of ComputerSystem 

  \date        11-04-18 14:40:00
 */
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/computersystemdependencies.h>
#include <sys/sysinfo.h>
#include <string>
#include <scxcorelib/stringaid.h>


using namespace SCXCoreLib;
using namespace std;

namespace SCXSystemLib
{
#if FILTERLINUX
     /**The full path of system file including the cpu info */
     const wstring cCPUInfoPath = L"/proc/cpuinfo"; 
#endif

    /*----------------------------------------------------------------------------*/
    /**
      Constructor

     */
    ComputerSystemDependencies::ComputerSystemDependencies()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.computersystem.ComputerSystemDepends"));
        Init();
        SCX_LOGTRACE(m_log, wstring(L"ComputerSystemDependencies default constructor: "));
    }
    
    /*----------------------------------------------------------------------------*/
    /**
      Destructor

     */
    ComputerSystemDependencies::~ComputerSystemDependencies()
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
      Creates the total ComputerSystemDependencies instance.
     */
    void ComputerSystemDependencies::Init()
    {
#if FILTERLINUX
        m_cpuInfoPath.Set(cCPUInfoPath);
        m_cpuInfo.clear();
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
      Cleanup

     */
    void ComputerSystemDependencies::CleanUp()
    {

    }

        /*----------------------------------------------------------------------------*/
        /**
        Get run-level of system as attr "BootupState"
    
        \param      value - the extracted value if there is one
        Return system run-level.
        */
        bool ComputerSystemDependencies::GetSystemRunLevel(wstring &runLevel) const
        {
#if defined FILTERLINUX || defined(sun)
            struct utmpx * tmputmpx = NULL;
            while((tmputmpx = getutxent()) != NULL )
            {
                if (tmputmpx->ut_type == RUN_LVL && tmputmpx->ut_line != 0)
                {
                    runLevel = SCXCoreLib::StrFromUTF8(tmputmpx->ut_line);
                    return true;
                }
            }
            SCX_LOGERROR(m_log, L"Failed to get run level");
            return false;
#elif defined(aix)
            SCXFileHandle f(fopen("/etc/.init.state", "r"));
            if (!f.GetFile())
            {
                SCX_LOGERROR(m_log, L"Unable to open /etc/.init.state");
                return false;
            }
            runLevel = fgetc(f.GetFile());
            return !runLevel.empty();
#else
                        runLevel = L"0";
            return false;
#endif
         }

#if FILTERLINUX
    /*----------------------------------------------------------------------------*/
    /**
      Get CPU Information. 
     */
    const vector<wstring>& ComputerSystemDependencies::GetCpuInfo()
    {
        m_cpuInfo.clear();
        SCX_LOGTRACE(m_log, wstring(L"ComputerSystemDependencies GetCpuInfo(): "));
        SCXCoreLib::SCXHandle<std::wfstream> fs(SCXCoreLib::SCXFile::OpenWFstream(m_cpuInfoPath, std::ios::in));
        fs.SetOwner();
        SCXStream::NLFs nlfs;
        SCXCoreLib::SCXStream::ReadAllLines(*fs, m_cpuInfo, nlfs);
        fs->close(); 
          
        return m_cpuInfo;  
    }

#elif defined(sun)

        /*----------------------------------------------------------------------------*/
        /**
        Get system information by systeminfo() function
    
        \param      value - the extracted value if there is one
        Return  Greater than 0  mains content is OK. Less than 0 mains value is empty.
        */
         int ComputerSystemDependencies::GetSystemInfo(int contentType, wstring &content) const
         {
             char contentVal[cSysinfoValueLength] = {'\0'};
             int retval = sysinfo(contentType, contentVal, cSysinfoValueLength);
             if (retval>0)
             {
                 content = StrFromUTF8(contentVal);
             }

             return retval;
         }


        /*----------------------------------------------------------------------------*/
        /**
        Get time zone of system
    
        \param      value - the extracted value if there is one
        Return system daylight value of time zone.
        */
         bool ComputerSystemDependencies::GetSystemTimeZone(bool &dayLightFlag) const
         {
            //
            //Run get TimeZone function. Get daylight flag from time.h.
            //
            tzset();
            if (daylight >0) 
            {
                dayLightFlag = true;
            }
            else
            {
                dayLightFlag = false;
            }
            return true;
         }

        /*----------------------------------------------------------------------------*/
        /**
        Get "/etc/power.conf" file content about power management configuration.
    
        \param      value - the extracted value if there is one
        Return get system "/etc/power.conf" file content is ok or fail.
        */
         bool ComputerSystemDependencies::GetPowerCfg(std::vector<wstring>& allLines)
         {
             try
             {
                 SCXStream::NLFs nlfs;
                 SCXFile::ReadAllLines(SCXFilePath(cPowerconfPath), allLines, nlfs);
                 return true;
             }
            catch(SCXException& e) 
            {
                SCX_LOGWARNING(m_log, StrAppend(
                    StrAppend(wstring(L"Failed to read power.conf file because "), e.What()),
                           e.Where()));
            }
            return false;
         }
#endif

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
