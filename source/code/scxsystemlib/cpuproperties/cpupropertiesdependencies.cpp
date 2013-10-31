/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
  \file        cpupropertiesdependencies.cpp

  \brief       Dependencies needed by the CpuProperties classes

  \date        11-03-18 15:40:00
 */
/*----------------------------------------------------------------------------*/

// Only for solaris sparc platform!
#if defined(sun)  

#include <scxcorelib/scxcmn.h>
#include  "scxsystemlib/cpupropertiesdependencies.h"
#include <sys/sysinfo.h>
#include <string>

using namespace SCXCoreLib;
using namespace std;

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
      Constructor

     */
    CpuPropertiesPALDependencies::CpuPropertiesPALDependencies()
    {
        m_log = SCXLogHandleFactory::GetLogHandle(wstring(L"scx.core.common.pal.system.cpuproperties.cpupropertiesdependencies"));
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesPALDependencies default constructor: "));
    }
    
    /*----------------------------------------------------------------------------*/
    /**
      Destructor

     */
    CpuPropertiesPALDependencies::~CpuPropertiesPALDependencies()
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
      Creates the total CpuPropertiesPALDependencies instance.Get SCXKstat.
     */
    void CpuPropertiesPALDependencies::Init()
    {
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesPALDependencies: "));
        m_kstatHandle = SCXHandle<SCXKstat>(new SCXKstat());
        m_kstatHandle->Update();
    }

    /*----------------------------------------------------------------------------*/
    /**
      Cleanup

     */
    void CpuPropertiesPALDependencies::CleanUp()
    {

    }

    /*----------------------------------------------------------------------------*/
    /**
    Look up kstat model according to module name.
    \param module : module name.
    \param name : kstat name
    \param instance : instance number, -1 means that all instance needed
    \returns     the result if look up sucessfully,when invoking failed, true would be returned, otherwise false.
    \throws      SCXKstatNotFoundException if kstat data could not be found by name
    \throws      SCXKstatErrorException if unable to read KStat data
    */
    bool CpuPropertiesPALDependencies::Lookup(const wstring& module, const wstring& name, int instance)
    {
        SCX_LOGTRACE(m_log, StrAppend(L"CpuPropertiesPALDependencies Lookup: ", &name));
        try
        {
            //
            // Just invoke kstat's interface.
            //
            m_kstatHandle->Lookup(module, name, instance);
        }
        catch(SCXKstatNotFoundException& e)
        {
            SCX_LOGWARNING(m_log, StrAppend(
                StrAppend(wstring(L"Failed to lookup because "), e.What()),
                        e.Where()));
            return false;
        }
       
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Go to the first kstat.

     */
    void CpuPropertiesPALDependencies::ResetInternalIterator()
    {
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesPALDependencies ResetInternalIterator: "));
        m_kstatHandle->ResetInternalIterator();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Go to the next kstat.
        Return: if exist next kstat return true.
    */
    bool CpuPropertiesPALDependencies::AdvanceInternalIterator()
    {
        SCX_LOGTRACE(m_log, wstring(L"CpuPropertiesPALDependencies AdvanceInternalIterator: "));
        kstat_t* pKstat = m_kstatHandle->AdvanceInternalIterator();
        if (pKstat != 0)
        {
            //
            // Verify kstat node returned, if so, it means that cpu info node found out.
            //
            string ksModuleName(pKstat->ks_module);
            SCX_LOGTRACE(m_log, StrAppend(L"CpuPropertiesPALDependencies::AdvanceInternalIterator: pKstat : ", &pKstat));
            if (ksModuleName != StrToUTF8(cModul_Name).c_str()) return false;
            return true;
        }

        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get a named value as an scxulong
    
    \param      statistic - name of named statistic to get.
    \param      value - the extracted value if there is one
    \returns     true if a value was extracted, or false otherwise.
    \throws      SCXNotSupportedException if kstat is not of a known type.
    \throws      SCXKstatStatisticNotFoundException if requested statistic is not found in kstat.
        
    Return a named value from kstat converted to an scxulong.
    */
    bool CpuPropertiesPALDependencies::TryGetValue(const wstring& statistic, scxulong& value) const 
    {
        return m_kstatHandle->TryGetValue(statistic, value);
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get a named value as an scxulong
    
    \param      statistic - name of named statistic to get.
    \param      value - the extracted value if there is one
    \returns     true if a value was extracted, or false otherwise.
    \throws      SCXNotSupportedException if kstat is not of a known type.
    \throws      SCXKstatStatisticNotFoundException if requested statistic is not found in kstat.
        
    Return a named value from kstat converted to an scxulong.
    */
    bool CpuPropertiesPALDependencies::TryGetStringValue(const wstring& statistic, wstring& value) const
    {
        return m_kstatHandle->TryGetStringValue(statistic,value);
    }

}

#endif //sun
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
