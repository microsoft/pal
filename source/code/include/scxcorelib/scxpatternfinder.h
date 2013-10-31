/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Defines a simple pattern finding object.
    
    \date        2008-01-29 11:15:45

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXPATTERFINDER_H
#define SCXPATTERFINDER_H

#include <string>
#include <map>
#include <vector>

namespace SCXCoreLib
{
/*----------------------------------------------------------------------------*/
/**
    A simple pattern finder class.
    
    The user of this object can register a number of patterns using cookies 
    to identify the patterns. patterns are case insensitive and may contain
    parameters. See documentation for Registerpattern and Match for examples.
    
*/
    class SCXPatternFinder
    {
    public:
        /** The pattern cookie is used to identify cookies when matching patterns. */
        typedef scxulong SCXPatternCookie;
        /** Represents parameter matches after a successful pattern match */
        typedef std::map<std::wstring,std::wstring> SCXPatternMatch;

        SCXPatternFinder();
        virtual ~SCXPatternFinder();

        void RegisterPattern(const SCXPatternCookie& cookie, const std::wstring& pattern);

        bool Match(const std::wstring& s, SCXPatternCookie& cookie, SCXPatternMatch& matches) const;

    private:
        std::wstring m_separators; //!< Holds separators used when tokenizing a pattern.
        std::map<std::wstring,std::wstring> m_mergeMarkers; //!< Holds merge markers used when merging tokens.
        std::wstring m_parameterIdentifier; //!< Start string of parameters.
        std::map<SCXPatternCookie, std::vector<std::wstring> > m_patterns; //!< registered patterns
    };

} /*namespace SCXCoreLib */

#endif /* SCXPATTERFINDER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
