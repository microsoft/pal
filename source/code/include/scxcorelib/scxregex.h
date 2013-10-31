/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

 */
/**
    \file

    \brief     Regular expression class

    \date      2008-08-20 14:08:17

 */
/*----------------------------------------------------------------------------*/
#ifndef SCXREGEX_H
#define SCXREGEX_H
 
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>

#include <string>
#include <sys/types.h>
#include <regex.h>
#include <vector>

namespace SCXCoreLib {
    /*----------------------------------------------------------------------------*/
    /**
        The SCXRegExMatch struct contains one return entry from the call to the SCXRegex::ReturnMatch() method.
     */
    struct SCXRegExMatch
    {
        std::wstring matchString;      //!< If match is found, string containing the match.
        bool matchFound;            //!< Indicates if match is found and matchString contains the matched value.

        /*--------------------------------------------------------------*/
        /**
            Constructor.
            \param[in] matchStringInit: Initial value for matchString.
            \param[in] matchFoundInit: Initial value for matchFound.
        */
        SCXRegExMatch(const std::wstring &matchStringInit, bool matchFoundInit):
            matchString(matchStringInit), matchFound(matchFoundInit){}

        /*--------------------------------------------------------------*/
        /**
            Default constructor.
        */
        SCXRegExMatch():matchFound(false){}
    };

    /*----------------------------------------------------------------------------*/
    /**
        The SCXRegex class represents an immutable (read-only) regular expression.
        It also contains static methods that allow use of other regular expression
        classes without explicitly creating instances of the other classes.
     */
    class SCXRegex
    {
    public:
        /*--------------------------------------------------------------*/
        /**
            Initializes and compiles a new instance of the Regex class
            for the specified regular expression.
            Note: we'll be using Extended Reg Expressions

            \param[in] expression Regular expression to compile.
            \throws SCXInvalidRegexException if compilation of regex fails.
        */
        SCXRegex(const std::wstring& expression); 

        /*--------------------------------------------------------------*/
        /**
            Indicates whether the regular expression specified in the
            SCXRegex constructor finds a match in the input string.
            Sets m_fCompiled to indicate whether the class successfully
            compiled and is useable or not.
        
            \param[in] text Input string to match.
            \returns true if a match is found.
        */
        bool IsMatch(const std::wstring& text) const;

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
        bool ReturnMatch(const std::wstring& text, std::vector<std::wstring>& matches, int flags);

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
        bool ReturnMatch(const std::wstring& text, std::vector<SCXRegExMatch>& matches,
                         unsigned int requestedMatchCt, int flags, bool stopWhenNoMatch  = false);

        /*----------------------------------------------------------------------------*/
        /**
           Destructor.
        */
        ~SCXRegex();
        std::wstring Get() const;
    private:
        std::wstring m_Expression;
        regex_t m_Preq; //!< Pattern buffer storage area.
        int m_fCompiled; //!< Indicates successfully compiled
    };


    /*----------------------------------------------------------------------------*/
    /**
        This exception is thrown by the constructor of SCXRegex when regular
        expression can't be compiled.

        \param[in] expression Regular expression that failed to compile
        \param[in] errcode    System error code
        \param[in] preq       Pattern buffer storage area
        \param[in] l          Source code location object

     */
    class SCXInvalidRegexException : public SCXException
    {
    public:
        SCXInvalidRegexException(const std::wstring& expression,
                                 int errcode, const regex_t* preq,
                                 const SCXCodeLocation& l);
        std::wstring What() const;    //!< Method to print the exception text
    protected:
        std::wstring m_Expression;    //!< Regular expression that caused exception.
        int m_Errcode;             //!< System error code.
        std::string  m_Errtext;    //!< Text describing the error.
    };

    /**
       Helper structure to hold a regular expression together with its index
    */

    struct SCXRegexWithIndex
    {
        size_t index;     //!< index for regular expression
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXRegex> regex;  //!< the regular expression
    };
}

#endif  /* SCXFILE_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
