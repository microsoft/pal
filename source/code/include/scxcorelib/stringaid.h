/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief     A collection of string functions assisting the std::wstring class

    \date      07-05-14 12:00:00

    String helper functions, support for working with std::wstring
*/
/*----------------------------------------------------------------------------*/
#ifndef STRINGAID_H
#define STRINGAID_H

#include <string>
#include <vector>
#include <map>

#include <scxcorelib/scxexception.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Append an integral datatype to a string

        \param  str    string to compute the hashcode for.
        \returns       unsigned int hashcode.

        \remarks This method will not work properly for wstrings.
    */
    inline unsigned int HashCode(const std::string& str)
    {
        unsigned int hashcode = 19;
        for (int i = (int)str.length() - 1; i >= 0; i-- )
        {
            hashcode = (37 * hashcode) + (unsigned char)str[i];
        }

        return hashcode;
    }

    std::wstring StrFromUTF8(const std::string& str);
    std::string StrToUTF8(const std::wstring& str);

    std::wstring DumpString(const std::exception &e);

    std::wstring StrFromMultibyte(const std::string& str, bool useDefaultLocale = false);
    std::wstring StrFromMultibyteNoThrow(const std::string& str);
// #ifdef USE_STR_TO_MULTIBYTE
    std::string StrToMultibyte(const std::wstring& str, bool useDefaultLocale = false);
// #endif

    std::wstring StrTrimL(const std::wstring& str);
    std::wstring StrTrimR(const std::wstring& str);
    std::wstring StrTrim(const std::wstring& str);

    std::wstring StrStripL(const std::wstring& str, const std::wstring& what);
    std::wstring StrStripR(const std::wstring& str, const std::wstring& what);
    std::wstring StrStrip(const std::wstring& str, const std::wstring& what);

    unsigned int UtfToUpper(unsigned int ch);
    unsigned int UtfToLower(unsigned int ch);

    // These templates are defined below in this file. Usable for at least all integral types.
    template <typename T> std::wstring StrAppend(const std::wstring& str, T i);
    template <typename T> std::wstring StrFrom(T v);

    unsigned int StrToUInt(const std::wstring& str);
    double StrToDouble(const std::wstring& str);
    scxlong StrToLong(const std::wstring& str);
    scxulong StrToULong(const std::wstring& str);

    std::wstring StrToUpper(const std::wstring& str);
    std::wstring StrToLower(const std::wstring& str);

    void StrReplaceAll(std::wstring &str, const std::wstring &what, const std::wstring &with);

    int StrCompare(const std::wstring& str1, const std::wstring& str2, bool ci=false);

    void StrTokenize(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters = L" \n", bool trim=true, bool emptyTokens=false, bool keepDelimiters = false);
    void StrTokenizeStr(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiter, bool trim=true, bool emptyTokens=false);
    void StrTokenizeQuoted(const std::wstring& str, std::vector<std::wstring>& tokens, const std::wstring& delimiters, bool emptyTokens=false);
    bool StrMergeTokens(std::vector<std::wstring>& tokens, const std::map<std::wstring,std::wstring>& mergePairs, const std::wstring& glue);

    bool StrIsPrefix(const std::wstring& str, const std::wstring& prefix, bool ci=false);

    /*----------------------------------------------------------------------------*/
    /**
       Utility function to convert a String to a unsigned char vector

       \param [in] input Input String
       \return Corresponding unsigned char vector
    */
    static inline std::vector<unsigned char> ToUnsignedCharVector(const std::string& input)
    {
        std::basic_string<unsigned char> unsignedStr(reinterpret_cast<const unsigned char*>(input.c_str()));
        std::vector<unsigned char> output(unsignedStr.begin(), unsignedStr.end());
        return output;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Utility function to convert a unsigned char vector to string

       \param [in] input The input vector
       \return The corresponding string 
    */
    static inline std::string FromUnsignedCharVector(const std::vector<unsigned char>& input)
    {
        std::string output(reinterpret_cast<const char*>(&(input[0])), input.size());
        return output;
    }

    /** Exception for failed multibyte conversion*/
    class SCXStringConversionException : public SCXException {
    public:
        //! Ctor
        SCXStringConversionException(const SCXCodeLocation& l)
            : SCXException(l)
        {};

        std::wstring What() const { return L"Multibyte string conversion failed"; }
    };

    /*----------------------------------------------------------------------------*/
    /**
        Append an integral datatype to a string

        \param  str    String to append to
        \param  i      Integral datatype to append
        \returns       The appended string

        \date          2007-10-02 14:02:00

        \remarks This template started its life as a number of separate functions
        that took an explicitly typed second arguments. That eventually clashed with
        how size_t was defined on separated platforms. You may think that it is ugly
        to include the definition in the header file and that we should use export
        instead, the so called separation model. Well, that's not possible, export
        is not supported by the compiler we use. What about explicit instantiation
        then? That would have some very bad consequences in shape of wild includes
        and dependencies. This is because for every type we use, there must be very
        explicit instantiation. Since we feed types from pegasus into this function
        that will be very bad.
    */
    template <typename T>
        std::wstring StrAppend(const std::wstring& str, T i)
    {
        std::wstringstream ss;
        ss << str << i;
        return ss.str();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Convert an integral datatype to a string

        \param    v     Integral datatype to convert
        \returns        The integral datatype as a string

        \date           2007-10-02 14:02:00

    */
    template <typename T>
        std::wstring StrFrom(T v)
    {
        std::wstringstream ss;
        ss << v;
        return ss.str();
    }
}


#endif /* STRINGAID_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
