/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/
/**
    \file        

    \brief       Contains the implementation of the log file configurator.

    \date        2008-07-21 15:10:29

*/
/*----------------------------------------------------------------------------*/

#include "scxlogconfigreader.h"

#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxlogpolicy.h>
#include <scxcorelib/stringaid.h>
#include <vector>

namespace SCXCoreLib
{

    
    /*----------------------------------------------------------------------------*/
    /**
        Given a string, translates it to an SCXLogSeverity.
        Recognized strings are:

        \li HYSTERICAL
        \li TRACE
        \li INFO
        \li WARNING
        \li ERROR
        \li SUPPRESS

        If the string does not match any of these, eNotSet is returned.

        \param[in] severityString String to translate.
        \returns SCXLogSeverity representing the severity string supplied.
    */
    SCXLogSeverity SCXLogConfigReader_TranslateSeverityString(const std::wstring& severityString)
    {
        if (L"HYSTERICAL" == severityString) {
            return eHysterical;
        } else if (L"TRACE" == severityString) {
            return eTrace;
        } else if (L"INFO" == severityString) {
            return eInfo;
        } else if (L"WARNING" == severityString) {
            return eWarning;
        } else if (L"ERROR" == severityString) {
            return eError;
        } else if (L"SUPPRESS" == severityString) {
            return eSuppress;
        } else {
            return eNotSet;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Given a log severity, translates it to an string.

        \li NOTSET
        \li HYSTERICAL
        \li TRACE
        \li INFO
        \li WARNING
        \li ERROR
        \li SUPPRESS

        \param[in] severity Severity to translate.
        \returns String representing the severity supplied.
    */
    std::wstring SCXLogConfigReader_SeverityToString(SCXLogSeverity severity)
    {
        switch (severity)
        {
        case eHysterical:
            return L"HYSTERICAL";
        case eTrace:
            return L"TRACE";
        case eInfo:
            return L"INFO";
        case eWarning:
            return L"WARNING";
        case eError:
            return L"ERROR";
        case eSuppress:
            return L"SUPPRESS";
        case eNotSet:
        case eSeverityMax:
        default:
            return L"NOTSET";
        }
    }


} /* namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
