/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implements a simple pattern finder.
    
    \date        2008-01-29 11:17:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxpatternfinder.h>

namespace SCXCoreLib
{
/*----------------------------------------------------------------------------*/
/**
    Default constructor.
*/
    SCXPatternFinder::SCXPatternFinder()
    {
        m_separators = L" \n\t=<>";
        m_mergeMarkers[L"\""] = L"\"";
        m_mergeMarkers[L"'"] = L"'";
        m_parameterIdentifier = L"%";
    }
     
/*----------------------------------------------------------------------------*/
/**
    Virtual destructor
*/
    SCXPatternFinder::~SCXPatternFinder()
    {

    }

/*----------------------------------------------------------------------------*/
/**
    Register a new pattern.
    
    \param       cookie A caller defined cookie to identify the pattern. This
                        cookie is returned when the pattern is matched.
    \param       pattern The pattern string.
    \throws      SCXInternalErrorException if the cookie has already been used or 
                                           if the pattern cannot be tokenized.
    
    The default patterns use white-space and the three chars =, < and > to seperate 
    tokens. " or ' might be used to create token strings. Parameters starts with %.
    Example: Select * from something where value=%parameter
    
*/
    void SCXPatternFinder::RegisterPattern(const SCXPatternCookie& cookie, const std::wstring& pattern)
    {
        if (m_patterns.find(cookie) != m_patterns.end())
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Cookie already exists", SCXSRCLOCATION);
        }
        std::vector<std::wstring> tokens;
        SCXCoreLib::StrTokenize(pattern, tokens, m_separators, false, true, true);
        
        if ( ! SCXCoreLib::StrMergeTokens(tokens, m_mergeMarkers, L""))
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Cannot tokenize pattern: " + pattern, SCXSRCLOCATION);
        }
        m_patterns[cookie] = tokens;
    }

/*----------------------------------------------------------------------------*/
/**
    Attempts to match the given string against registred patterns.
    
    \param       s The string we try to match agains registered patterns.
    \param[out]  cookie The cookie used when registering a matched pattern.
    \param[out]  matches Any parameters matched are returned as string-string
                         value pairs. \note this parameter is not reset by this method.
    \returns     true if a matching pattern was found, otherwise false.
    
    If there is a registred pattern looking like this:
    Select * from something where value=%parameter
    testing for a match with 
    SELECT * FROM something WHERE value=v
    will result in a successful match since matching is case insensitive.
    The matches parameter will hold a "parameter" -> "v" pair.
    
*/
    bool SCXPatternFinder::Match(const std::wstring& s, SCXPatternCookie& cookie, SCXPatternMatch& matches) const
    {
        std::vector<std::wstring> tokens;
        SCXCoreLib::StrTokenize(s, tokens, m_separators, false, true, true);
        if ( ! SCXCoreLib::StrMergeTokens(tokens, m_mergeMarkers, L""))
        {
            return false;
        }
        for (std::map<SCXPatternCookie, std::vector<std::wstring> >::const_iterator pattern = m_patterns.begin();
             pattern != m_patterns.end(); pattern++)
        {
            if (tokens.size() == pattern->second.size())
            {
                bool ok = true;
                for (std::vector<std::wstring>::const_iterator t = tokens.begin(),
                         p = pattern->second.begin();
                     t != tokens.end() && p != pattern->second.end();
                     t++, p++)
                {
                    if (p->substr(0,m_parameterIdentifier.length()) == m_parameterIdentifier)
                    { // Found parameter
                        matches[p->substr(m_parameterIdentifier.length())] = *t;
                    }
                    else if (0 != SCXCoreLib::StrCompare(*t, *p, true))
                    {
                        ok = false;
                    }
                }
                if (ok)
                {
                    cookie = pattern->first;
                    return true;
                }
            }
        }
        return false;
    }

} /*namespace SCXCoreLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
