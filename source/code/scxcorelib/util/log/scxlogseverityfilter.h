/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Contains the definition of the log severity filter class.

    \date        2008-07-24 15:21:39

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOGSEVERITYFILTER_H
#define SCXLOGSEVERITYFILTER_H

#include <map>
#include <scxcorelib/scxlog.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        An SCXLogSeverityFilter is used to filter log items according to
        severity thresholds set in the module hierarchy.
    */
    class SCXLogSeverityFilter
    {
        typedef std::map<std::wstring, SCXLogSeverity> SeverityMap; //!< Type for mapping
    public:
        SCXLogSeverityFilter();
        virtual bool IsLogable(const SCXLogItem& item) const;
        virtual SCXLogSeverity GetSeverityThreshold(const std::wstring& module) const;
        virtual bool SetSeverityThreshold(const std::wstring& module, SCXLogSeverity severity);
        virtual bool ClearSeverityThreshold(const std::wstring& module);
        virtual SCXLogSeverity GetMinActiveSeverityThreshold() const;

        /**
            Virtual destructor.
         */
        virtual ~SCXLogSeverityFilter() {};
    private:
        SCXLogSeverity m_DefaultSeverity; //!< Severity of root module.
        SeverityMap m_ModuleMap; //!< Module severity mapping.
    };
} /* namespace SCXCoreLib */
#endif /* SCXLOGSEVERITYFILTER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
