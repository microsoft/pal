/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation for a log severity filter.

    \date        2008-07-24 15:43:21

*/
/*----------------------------------------------------------------------------*/

#include "scxlogseverityfilter.h"
#include <scxcorelib/scxlogitem.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Default constructor.
    */
    SCXLogSeverityFilter::SCXLogSeverityFilter() :
        m_DefaultSeverity(eNotSet)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Determine if log item passes the severity filter.
        \param[in] item Item to filter.
        \returns true if item passes the filter.
    */
    bool SCXLogSeverityFilter::IsLogable(const SCXLogItem& item) const
    {
        if (eNotSet == item.GetSeverity())
        {
            return false;
        }
        SCXLogSeverity threshold = GetSeverityThreshold(item.GetModule());
        if (eNotSet == threshold)
        {
            return false;
        }
        
        return item.GetSeverity() >= threshold;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get severity threshold for a module.
        The severity is either explicitly stated for this module, or inherited
        from a parent module. If no severity is set that affects this module,
        then eNotSet is returned.

        \param[in] module Module to get severity threshold for.
        \returns severity The severity threshold for the module.
    */
    SCXLogSeverity SCXLogSeverityFilter::GetSeverityThreshold(const std::wstring& module) const
    {
        /* Example: To get the the effective severity for the module
           scx.core.common.pal.system.common.entityenumeration, first look for
           scx.core.common.pal.system.common.entityenumeration in the map,
           then try scx.core.common.pal.system.common, then try
           scx.core.common.pal.system, then scx.core.common.pal, and so on until we
           reach the root element.
        */
        SeverityMap::const_iterator pos;
        std::wstring effective_module(module);
        std::wstring::size_type dotpos;

        for (;;) {
            pos = m_ModuleMap.find(effective_module);
            if (pos != m_ModuleMap.end()) {
                // Found an effective severity. This is probably it. However,
                // there is an exception to the inheritance rule. If the severity
                // is eHysterical and this is not the original module that 
                // the method was invoked on, then we have to go higher in the
                // inheritance structure.
                if (eHysterical != pos->second || effective_module == module ) { return pos->second; }
            }

            dotpos = effective_module.rfind(L'.');
            if (dotpos == std::wstring::npos) {
                // We've just tested the top level, "scx" in the example.
                break;
            }
            
            // Erase part from dot to end of string
            effective_module.erase(dotpos);
        }
        // We've tried our module name and all superior modules without result
        // Now return the default
        return m_DefaultSeverity;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set severity threshold for a module.

        \param[in] module Module to set severity threshold for.
        \param[in] severity The severity threshold to set.
        \returns true if the severity filter was actually changed as a result of
        this method call.
    */
    bool SCXLogSeverityFilter::SetSeverityThreshold(const std::wstring& module, SCXLogSeverity severity)
    {
        if (module.empty())
        {
            if (m_DefaultSeverity != severity && eHysterical != severity)
            {
                m_DefaultSeverity = severity;
                return true;
            }
        }
        else
        {
            SeverityMap::iterator iter = m_ModuleMap.lower_bound(module);
            if ((m_ModuleMap.end() != iter) && !(m_ModuleMap.key_comp()(module, iter->first)))
            {
                if (iter->second != severity)
                {
                    iter->second = severity;
                    return true;
                }
            }
            else
            {
                m_ModuleMap.insert(iter, std::pair<const std::wstring, SCXLogSeverity>(module, severity));
                return true;
            }
        }
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Unset severity threshold for a module.

        \param[in] module Module to clear severity threshold for.
        \returns true if the severity filter was actually changed as a result of
        this method call.
    */
    bool SCXLogSeverityFilter::ClearSeverityThreshold(const std::wstring& module)
    {
        if (module.empty())
        {
            if (m_DefaultSeverity != eNotSet)
            {
                m_DefaultSeverity = eNotSet;
                return true;
            }
        }
        else
        {
            return (1 == m_ModuleMap.erase(module));
        }
        return false;
    }

    /**
        Get the minimum log severity threshold used for any module in this filter.
        \returns Minimum severity threshold used for this filter.
    */
    SCXLogSeverity SCXLogSeverityFilter::GetMinActiveSeverityThreshold() const
    {
        SCXLogSeverity s = m_DefaultSeverity;

        for (SeverityMap::const_iterator iter = m_ModuleMap.begin();
             iter != m_ModuleMap.end();
             ++iter)
        {
            if (iter->second < s)
            {
                s = iter->second;
            }
        }
        return s;
    }
} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
