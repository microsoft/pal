/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
    \file

    \brief     Regex implementation

    \date      2008-08-20 14:19:15

 */
/*----------------------------------------------------------------------------*/


#include <scxcorelib/scxregex.h>
#include <scxcorelib/stringaid.h>

namespace SCXCoreLib {
    /*--------------------------------------------------------------*/
    /**
        Initializes and compiles a new instance of the Regex class
        for the specified regular expression.
        Note: we'll be using Extended Reg Expressions

        \param[in] expression Regular expression to compile.
        \throws SCXInvalidRegexException if compilation of regex fails.
    */
    SCXRegex::SCXRegex(const std::wstring& expression) : m_Expression(expression), m_fCompiled(0)
    {
        m_fCompiled = regcomp(&m_Preq, StrToUTF8(expression).c_str(), REG_EXTENDED);  //|REG_NOSUB);
        if (m_fCompiled != 0)
        {
            throw SCXInvalidRegexException(expression, m_fCompiled, &m_Preq, SCXSRCLOCATION);
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Indicates whether the regular expression specified in the
        SCXRegex constructor finds a match in the input string.
        
        \param[in] text Input string to match.
        \returns true if a match is found.
     */
    bool SCXRegex::IsMatch(const std::wstring& text) const
    {
        if (m_fCompiled == 0)
        {
            // Compiled successfully
            return (0 == regexec(&m_Preq, StrToUTF8(text).c_str(), 0, 0, 0));
        }
        else
        {
            return false;
        }
    }

    /*--------------------------------------------------------------*/
    /**
        Returns a vector of matched strings from the given input text.
    
        \param[in] text: Input string to match.
        \param[out] matches: Vector of matched strings, the first contains the original text.
                    If match fails, this vector contains the regex error msg.
        \param[in] flags: Flags to pass into regexec, such as REG_NOTBOL or REG_NOTEOL.
        \returns true: if a match is found, and results are in vector matches.
                    false: no match found, matches[0] contains regerr message.
    */
    bool SCXRegex::ReturnMatch(const std::wstring& text, std::vector<std::wstring>& matches, int flags)
    {
        std::vector<SCXRegExMatch> matchesFound;
        matches.clear();
        if(ReturnMatch(text, matchesFound, 32, flags, true))
        {
            size_t i;
            for(i = 0; i < matchesFound.size(); i++)
            {
                matches.push_back(matchesFound[i].matchString);
            }
            return true;
        }
        return false;
    }

    /*--------------------------------------------------------------*/
    /**
        Returns a vector of matched strings from the given input text as well as the vector returning exactly
        what string in the vector of matched strings was successfuly matched.
    
        \param[in] text: Input string to match.
        \param[out] matches: Vector of matched strings, the first contains the original text.
                    If match fails, this vector contains the regex error msg.
        \param[in] requestedMatchCt: Number of requested matches to be returned by matches array, unless it is
                                     truncated by stopWhenNoMatch parameter.
        \param[in] flags: Flags to pass into regexec, such as REG_NOTBOL or REG_NOTEOL.
        \param[in] stopWhenNoMatch: if true then method stops returning matches in the matches vector when first
                   not matched element is encountered.
        \returns true: if a match is found, and results are in vector matches.
                    false: no match found, matches[0] contains regerr message.
    */
    bool SCXRegex::ReturnMatch(const std::wstring& text, std::vector<SCXRegExMatch>& matches,
                               unsigned int requestedMatchCt, int flags, bool stopWhenNoMatch /* = false*/)
    {
        bool rc = false;
        matches.clear();
        
        //See if we've already had a failure in compiling the pattern.
        if (m_fCompiled != 0)
        {
            matches.push_back(SCXRegExMatch(L"Compile of Regexec", false));
            return false;
        }
        
        std::vector<regmatch_t> allMatches;
        allMatches.resize(requestedMatchCt);
        for (unsigned int idx=0; idx < allMatches.size(); idx++)
        {
            allMatches[idx].rm_so = -1;
            allMatches[idx].rm_eo = -1;
        }
        
        rc = regexec(&m_Preq, StrToUTF8(text).c_str(), requestedMatchCt, &allMatches[0], flags);
        
        if (rc != 0)
        {
            // Have an error in finding a match
            char errmsg[0x0200];
            regerror(rc, &m_Preq, errmsg, 0x200);
            std::wstring fullregerr(StrFromUTF8(errmsg));
            matches.push_back(SCXRegExMatch(fullregerr, false));
            return false;
        }
        
        for (unsigned int idx=0; idx < allMatches.size(); idx++)
        {
            if (allMatches[idx].rm_so == -1 || allMatches[idx].rm_eo == -1) 
            {
                if(stopWhenNoMatch == true)
                {
                    break;
                }
                matches.push_back(SCXRegExMatch(std::wstring(L""),false));
                continue;
            }
        
            //Looks like we have a matching substring, let's write it to our vector
            std::wstring resultStr;
            size_t strSize = allMatches[idx].rm_eo - allMatches[idx].rm_so;
            if(strSize > 0)
            {
                resultStr = text.substr(allMatches[idx].rm_so, strSize);
            }
            matches.push_back(SCXRegExMatch(resultStr,true));
        }
        
        return true;
    }


    /*--------------------------------------------------------------*/
    /**
       Get the regular expression in wstring type

       \returns Regular expression used when generating the object
    */
    std::wstring SCXRegex::Get() const
    {
        return m_Expression;
    }

    /*--------------------------------------------------------------*/
    /**
        Destructor
     */
    SCXRegex::~SCXRegex()
    {
        regfree(&m_Preq);
    }

    /*--------------------------------------------------------------*/
    /**
        Creates a new regex exception

        \param[in] expression Regular expression that failed to compile
        \param[in] errcode    System error code
        \param[in] preq       Pattern buffer storage area
        \param[in] l          Source code location object

    */
    SCXInvalidRegexException::SCXInvalidRegexException(const std::wstring& expression,
                                                       int errcode, const regex_t* preq,
                                                       const SCXCodeLocation& l) :
        SCXException(l),
        m_Expression(expression),
        m_Errcode(errcode)
    {
        char buf[80];
        regerror(m_Errcode, preq, buf, sizeof(buf));
        m_Errtext = buf;
    }

    std::wstring SCXInvalidRegexException::What() const {
        std::wostringstream txt;
        txt << L"Compiling " << m_Expression << L" returned an error code = " << m_Errcode << L" (" << m_Errtext.c_str() << L")";
        return txt.str();
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
