/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Dependancies of Processor: Load CPU info structure from kstat

   \date        11-03-11 16:00:00

   \date        11-03-11 16:28:00
*/
/*----------------------------------------------------------------------------*/
#ifndef PROCESSORDEPENDENCIES_H
#define PROCESSORDEPENDENCIES_H

#if !defined(sun) || !defined(sparc)
#error this file is only meaningful on the Solaris sparc platform
#endif

#include <scxcorelib/scxlog.h>
#include <scxsystemlib/scxkstat.h>
#include <sys/types.h>

namespace SCXSystemLib
{
    /* Kstat module name */
    const std::wstring cModul_Name = L"cpu_info";

    /** Instances number, -1 mains all */
    const int cInstancesNum = -1;

    /** Kstat attr name chip_id */
    const std::wstring cAttrName_ChipID = L"chip_id";

    /** Kstat attr name clock_MHz */
    const std::wstring cAttrName_ClockMHz = L"clock_MHz";

    /** Kstat attr name current_clock_Hz */
    const std::wstring cAttrName_CurrentClockHz = L"current_clock_Hz";

    /** Kstat attr name implementation */
    const std::wstring cAttrName_Implementation = L"implementation";

    /*----------------------------------------------------------------------------*/
    /**
       Class representing all external dependencies from the Processor PAL.

    */
    class ProcessorPALDependencies
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor
        */
        ProcessorPALDependencies();

        /*----------------------------------------------------------------------------*/
        /**
           Virtual destructor
        */
        virtual ~ProcessorPALDependencies();

        /*----------------------------------------------------------------------------*/
        /**
        Init running context.
        */
        void Init();

        /*----------------------------------------------------------------------------*/
        /**
        Clean up running context.
        */
        void CleanUp();

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
        virtual bool Lookup(const std::wstring& module, const std::wstring& name, int instance = -1);

        /**
        Reset the internal iterator.
        */
        virtual void ResetInternalIterator();

        /*----------------------------------------------------------------------------*/
        /**
        Advance the internal iterator.
        */
        virtual bool AdvanceInternalIterator();

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
        virtual bool TryGetValue(const std::wstring& statistic, scxulong& value) const;

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
        virtual bool TryGetStringValue(const std::wstring& statistic, std::wstring& value) const;

    private:
        SCXCoreLib::SCXHandle<SCXKstat> m_kstatHandle;  // kstat handle
        SCXCoreLib::SCXLogHandle m_log;                 // log handle
    };
}


#endif /* PROCESSORDEPENDENCIES_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
