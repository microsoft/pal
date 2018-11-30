/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       PAL representation of the operating system

   \date        08-03-04 15:22:00

//////////////////////////////////////////////////////////////////////////
// -- Included per LCA request based on significant commonality to
// OpenPegasus Code...
//
// Licensed to The Open Group (TOG) under one or more contributor license
// agreements.  Refer to the OpenPegasusNOTICE.txt file distributed with
// this work for additional information regarding copyright ownership.
// Each contributor licenses this file to you under the OpenPegasus Open
// Source License; you may not use this file except in compliance with the
// License.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//////////////////////////////////////////////////////////////////////////

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <langinfo.h>

// For stat()
#include <sys/stat.h>

// For getutxent and reading utmp file
#include <utmpx.h>
#include <fcntl.h>

// Not all utmpx.h headers define the UTMPX_FILE macro
#ifndef UTMPX_FILE
    #ifdef UTMP_FILE
        #define UTMPX_FILE UTMP_FILE
    #else
        #define UTMPX_FILE "/etc/utmpx"
    #endif
#endif

#if defined(linux) || defined(sun)
// Regexp includes
#include <regex.h>
// For getrlimit
#include <sys/resource.h>
// Dirent includes
#include <dirent.h>
#endif

#if defined(hpux)
// For gettune()
#include <sys/dyntune.h>
#include <sys/time.h>
#endif

#if defined(sun)
//#include <sys/swap.h>
//#include <sys/sysinfo.h>
#endif

#if defined(linux)
#include <scxcorelib/scxfile.h>
#endif

#include <fstream>
#include <sstream>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/logsuppressor.h>

#include <scxsystemlib/osenumeration.h>
#include <scxsystemlib/osinstance.h>
#include <scxsystemlib/installedsoftwaredepend.h>
#include <scxsystemlib/installedsoftwareinstance.h>

#include <limits.h>

using namespace std;
using namespace SCXCoreLib;

/*
 A table of Windows language/locale codes indexed by ISO 639-1 language code and
 ISO 3166-1 country code.

 The first entry in the table for a language will be taken as the default locale for
 the languages and its entries will become the default country code, default Windows
 locale code and default code page for that language, if no country code is specified.

 Windows language/locale codes contain the Windows language
 code in the lower 10 bits and the locale code in the upper 6 bits.
*/
struct LocaleInfo
{
    char Iso639LanguageCode[4];         // ISO 639-1 two-letter language code
    char Iso3166CountryCode[4];         // ISO 3166 two-letter country code
    unsigned int WindowsLocaleCode;     // Windows language/locale code
    unsigned int CountryCode;           // telephone country code
    unsigned short DefaultCodePage;     // default Windows code page
};

static LocaleInfo LocaleInfoTable[] =
{
    // English
    { "en", "US", 0x0409,   1, 20127 }, // English-United States
    { "en", "GB", 0x0809,  44, 28591 }, // English-United Kingdom
    { "en", "UK", 0x0809,  44, 28591 }, // English-United Kingdom (non-standard country code)
    { "en", "AU", 0x0C09,  61, 28591 }, // English-Australia
    { "en", "CA", 0x1009,   1, 20127 }, // English-Canada
    { "en", "NZ", 0x1409,  64, 28591 }, // English-New Zealand
    { "en", "IE", 0x1809, 353, 28591 }, // English-Ireland
    { "en", "ZA", 0x1C09,  27, 28591 }, // English-South Africa
    { "en", "JM", 0x2009, 876, 28591 }, // English-Jamaica
    { "en", "BA", 0x2809, 501, 28591 }, // English-Belize
    { "en", "TT", 0x2C09, 868, 28591 }, // English-Trinidad
    { "en", "ZW", 0x3009, 263, 28591 }, // English-Zimbabwe
    { "en", "PH", 0x3409,  63, 28591 }, // English-Phillipines
    { "en", "IN", 0x4009,  91, 28591 }, // English-India
    { "en", "MY", 0x4409,  60, 28591 }, // English-Malaysia
    { "en", "SG", 0x4809,  65, 28591 }, // English-Singapore

    // German
    { "de", "DE", 0x0407,  49, 28591 }, // German-Germany
    { "de", "CH", 0x0807,  41, 28591 }, // German-Switzerland
    { "de", "AT", 0x0C07,  43, 28591 }, // German-Austria
    { "de", "LU", 0x1007, 352, 28591 }, // German-Luxembourg
    { "de", "LI", 0x1407,  49, 28591 }, // German-Liechtenstein

    // French
    { "fr", "FR", 0x040C,  33, 28591 }, // French-France
    { "fr", "BE", 0x080C,  32, 28591 }, // French-Belgium
    { "fr", "CA", 0x0C0C,   1, 28591 }, // French-Canada
    { "fr", "CH", 0x100C,  41, 28591 }, // French-Switzerland
    { "fr", "LU", 0x140C, 352, 28591 }, // French-Luxembourg
    { "fr", "MC", 0x180C, 377, 28591 }, // French-Monaco

    // Chinese
    { "zh", "CN", 0x0804,  86, 65001 }, // Chinese (simplified)-PRC
    { "zh", "TW", 0x0404, 886, 65001 }, // Taiwan
    { "zh", "SG", 0x1004,  65, 65001 }, // Chinese (simplified)-Singapore
    { "zh", "HK", 0x0C04, 852, 65001 }, // Chinese (traditional)-Hong Kong SAR
    { "zh", "MO", 0x1404, 853, 65001 }, // Macao SAR

    // Italian
    { "it", "IT", 0x0010,  39, 28591 }, // Italian-Italy
    { "it", "CH", 0x0810,  41, 28591 }, // Italian-Switzerland

    // Portuguese
    { "pt", "BR", 0x0416,  55, 28591 }, // Portuguese-Brazil
    { "pt", "PT", 0x0816, 351, 28591 }, // Portuguese-Portugal

    // Japanese and Korean
    { "ja", "JP", 0x0411,  81, 65001 }, // Japanese-Japan
    { "jp", "JP", 0x0411,  81, 65001 }, // Japanese-Japan (non-standard language code)
    { "ko", "KR", 0x0412,  82, 65001 }, // Korean-S. Korea
    { "ko", "KP", 0x0812,  82, 65001 }, // Korean-N. Korea

    // Spanish
    { "es", "ES", 0x0C0A,  34, 28591 }, // Spanish-Spain, international sort
    { "es", "MX", 0x080A,  52, 28591 }, // Spanish-Mexico
    { "es", "GT", 0x100A, 502, 28591 }, // Spanish-Guatemala
    { "es", "CR", 0x140A, 506, 28591 }, // Spanish-Costa Rica
    { "es", "PA", 0x180A, 507, 28591 }, // Spanish-Panama
    { "es", "DO", 0x1C0A, 809, 28591 }, // Spanish-Dominican Republic
    { "es", "VE", 0x200A,  58, 28591 }, // Spanish-Venezuela
    { "es", "PE", 0x280A,  51, 28591 }, // Spanish-Peru
    { "es", "AR", 0x2C0A,  54, 28591 }, // Spanish-Argentina
    { "es", "EC", 0x300A, 593, 28591 }, // Spanish-Ecuador
    { "es", "CL", 0x340A,  56, 28591 }, // Spanish-Chile
    { "es", "UY", 0x380A,  56, 28591 }, // Spanish-Uruguay
    { "es", "PY", 0x3C0A, 595, 28591 }, // Spanish-Paraguay
    { "es", "BO", 0x400A, 591, 28591 }, // Spanish-Bolivia
    { "es", "SV", 0x440A, 503, 28591 }, // Spanish-El Salvador
    { "es", "HN", 0x480A, 504, 28591 }, // Spanish-Honduras
    { "es", "NI", 0x4C0A, 505, 28591 }, // Spanish-Nicaragua
    { "es", "PR", 0x500A,   1, 28591 }, // Spanish-Puerto Rico
    { "es", "US", 0x530A,   1, 28591 }, // Spanish-United States

    // Russian
    { "ru", "RU", 0x0419,   7, 65001 }, // Russian-Russia
    { "ru", "MD", 0x0819, 373, 65001 }, // Russian-Moldova

    // Other western European languages
    { "ca", "ES", 0x0403,  34, 28591 }, // Catalan-Spain
    { "da", "DK", 0x0406,  45, 28591 }, // Danish-Denmark
    { "el", "GR", 0x0408,  30, 65001 }, // Greek-Greece
    { "el", "EL", 0x0408,  30, 65001 }, // Greek-Greece (non-standard country code)
    { "fi", "FI", 0x000B, 358, 28591 }, // Finnish-Finland
    { "hu", "HU", 0x040E,  36, 28591 }, // Hungarian-Hungary
    { "is", "IS", 0x040F, 354, 28591 }, // Icelandic-Iceland
    { "nl", "NL", 0x0413,  31, 28591 }, // Dutch-Netherlands
    { "nl", "BE", 0x0813,  31, 28591 }, // Dutch-Belgium
    { "bk", "NO", 0x0414,  47, 28591 }, // Bokmal-Norway
    { "nn", "NO", 0x0814,  47, 28591 }, // Nynorsk-Norway
    { "sv", "SE", 0x041D,  46, 28591 }, // Swedish-Sweden

    // Central European languages
    { "bg", "BG", 0x0402, 359, 65001 }, // Bulgarian defaults to Bulgaria
    { "cs", "CZ", 0x0405,  42, 65001 }, // Czech-Czech Republic
    { "pl", "PL", 0x0415,  48, 65001 }, // Polish-Poland
    { "ro", "RO", 0x0418,  40, 65001 }, // Romanian-Romania
    { "hr", "HR", 0x041A, 385, 65001 }, // Croatian-Croatia
    { "sr", "RS", 0x081A, 381, 65001 }, // Serbian-Serbia (Latin alphabet)
    { "sk", "SK", 0x041B, 421, 28591 }, // Slovak default to Slovakia
    { "sq", "AL", 0x041C, 355, 65001 }, // Albanian-Albania
    { "mk", "MK", 0x042F, 389, 65001 }, // Macedonia, FYRO
    { "be", "BY", 0x0423, 375, 65001 }, // Belarusian-Belarus
    { "sl", "SI", 0x0424, 386, 65001 }, // Slovenian-Sloveniua
    { "et", "EE", 0x0425, 372, 65001 }, // Estonian-Estonia
    { "lv", "LV", 0x0426, 371, 65001 }, // Latvian-Latvia
    { "lt", "LT", 0x0427, 370, 65001 }, // Lithuanian-Lithuania
    { "uk", "UA", 0x0422, 380, 65001 }, // Ukrainian-Ukraine

    // Middle Eastern languages
    { "he", "IL", 0x040D, 972, 65001 }, // Hebrew-Israel
    { "tr", "TR", 0x041F,  90, 65001 }, // Turkish-Turkey

    // Other Asian languages
    { "th", "TH", 0x041E, 668, 65001 }, // Thai-Thailand
    { "ur", "PK", 0x0420,  92, 65001 }, // Urdu-Pakistan
    { "ur", "IN", 0x0820,  91, 65001 }, // Urdu-India
    { "id", "ID", 0x0421,  62, 28561 }, // Indonesian-Indonesia
    { "fa", "IR", 0x0429,  98, 65001 }, // Persian-Iran
    { "vi", "VN", 0x042A,  84, 65001 }, // Vietnamese-Vietnam

    // Arabic
    { "ar", "EG", 0x0C01,  20, 65001 }, // Arabic-Egypt
    { "ar", "SA", 0x0401, 966, 65001 }, // Arabic-Saudi Arabia
    { "ar", "IQ", 0x0801, 964, 65001 }, // Arabic-Iraq
    { "ar", "LY", 0x1001, 218, 65001 }, // Arabic-Libya
    { "ar", "DZ", 0x1401, 213, 65001 }, // Arabic-Algeria
    { "ar", "MA", 0x1801, 212, 65001 }, // Arabic-Morocco
    { "ar", "TN", 0x1C01, 216, 65001 }, // Arabic-Tunisia
    { "ar", "OM", 0x2001, 968, 65001 }, // Arabic-Oman
    { "ar", "YE", 0x2401, 967, 65001 }, // Arabic-Yemen
    { "ar", "SY", 0x2801, 963, 65001 }, // Arabic-Syria
    { "ar", "JO", 0x2C01, 962, 65001 }, // Arabic-Jordan
    { "ar", "LB", 0x3001, 961, 65001 }, // Arabic-Lebanon
    { "ar", "KW", 0x3401, 965, 65001 }, // Arabic-Kuwait
    { "ar", "AE", 0x3801, 971, 65001 }, // Arabic-U.A.E.
    { "ar", "BH", 0x3C01, 973, 65001 }, // Arabic-Bahrain
    { "ar", "QA", 0x4001, 974, 65001 }  // Arabic-Qatar
};

/*
 Get the default telephone country code for a given language

 @param[in]     isoLangCode{1,2} - the ISO 639-1 language code

 @returns       the telephone country code
*/
static unsigned int GetDefaultCountryCode(
    char isoLangCode1,
    char isoLangCode2)
{
    for (size_t i = 0; i < sizeof LocaleInfoTable / sizeof (LocaleInfo); i++)
    {
        if (LocaleInfoTable[i].Iso639LanguageCode[0] == isoLangCode1 &&
            LocaleInfoTable[i].Iso639LanguageCode[1] == isoLangCode2)
        {
            return LocaleInfoTable[i].CountryCode;
        }
    }
    return 1;                           // if we can't find the language, default to US/Canada
}

/*
 Get the default code page used by Unix/Linux for a given language

 @param[in]     isoLangCode{1,2} - the ISO 639-1 language code

 @returns       the default code page
*/
static unsigned int GetDefaultCodePage(
    char isoLangCode1,
    char isoLangCode2)
{
    for (size_t i = 0; i < sizeof LocaleInfoTable / sizeof (LocaleInfo); i++)
    {
        if (LocaleInfoTable[i].Iso639LanguageCode[0] == isoLangCode1 &&
            LocaleInfoTable[i].Iso639LanguageCode[1] == isoLangCode2)
        {
            return (unsigned int)LocaleInfoTable[i].DefaultCodePage;
        }
    }
    return 20127;                       // if we can't find the language, use the 7-bit ASCII code page
}

/*
 Get the Windows language code from the ISO 639-1 language code. The Windows language code
 is gotten from the language bits (the lower 10 bits) in the default Windows locale code
 for the language.

 @param[in]     isoLangCode{1,2} - the ISO 639-1 language code

 @returns       the Windows language code
*/
static unsigned int GetWindowsLanguageCode(
    char isoLangCode1,
    char isoLangCode2)
{
    for (size_t i = 0; i < sizeof LocaleInfoTable / sizeof (LocaleInfo); i++)
    {
        if (LocaleInfoTable[i].Iso639LanguageCode[0] == isoLangCode1 &&
            LocaleInfoTable[i].Iso639LanguageCode[1] == isoLangCode2)
        {
            return LocaleInfoTable[i].WindowsLocaleCode & 0x003F;
        }
    }
    return 9;                           // if we can't find the language, default to US English
}

/*
 Get the Windows 16-bit locale/language code from the ISO 639-1 language code and the ISO 3166-1 country code.

 The lower 10 bits of the code are the Windows language code and the upper 6 bits encode the locale.

 @param[in]     isoLangCode{1,2} - the ISO 639-1 language code
 @param[in]     isoCountryCode{1,2} - the ISO 3166-1 alpha-2 country code
*/
static unsigned int GetWindowsLocaleCode(
    char isoLangCode1,
    char isoLangCode2,
    char isoCountryCode1,
    char isoCountryCode2)
{
    // search the language and locale table for a match with the language and country passed in
    for (size_t i = 0; i < sizeof LocaleInfoTable / sizeof (LocaleInfo); i++)
    {
        if (LocaleInfoTable[i].Iso639LanguageCode[0] == isoLangCode1 &&
            LocaleInfoTable[i].Iso639LanguageCode[1] == isoLangCode2 &&
            LocaleInfoTable[i].Iso3166CountryCode[0] == isoCountryCode1 &&
            LocaleInfoTable[i].Iso3166CountryCode[1] == isoCountryCode2)
        {
            return (unsigned int)LocaleInfoTable[i].WindowsLocaleCode;
        }
    }
    return 0x0009;                      // if no other information, return English-no locale
}

/*
 Get the telephone country code from the ISO 3166-1 country code. If the ISO country code is not
 in the table, fall back to getting the default telephone country code for the specified language.

 @param[in]     isoCountryCode{1,2} - the ISO 3166-1 alpha-2 country code
 @param[in]     isoLangCode{1,2} - the ISO 639-1 language code
*/
static unsigned int GetCountryCode(
    char isoCountryCode1,
    char isoCountryCode2,
    char isoLangCode1,
    char isoLangCode2)
{
    // search the locale table for a match with the country passed in
    for (size_t i = 0; i < sizeof LocaleInfoTable / sizeof (LocaleInfo); i++)
    {
        if (LocaleInfoTable[i].Iso3166CountryCode[0] == isoCountryCode1 &&
            LocaleInfoTable[i].Iso3166CountryCode[1] == isoCountryCode2)
        {
            return (unsigned int)LocaleInfoTable[i].CountryCode;
        }
    }

    // if the country wasn't there, return the default country for the language
    return GetDefaultCountryCode(isoLangCode1, isoLangCode2);
}

/*
 Table of code page descriptive names and their Windows numeric code page identifers.

 Code page names with minor standard values (stuff after a period) are compared up to the period only.
 Comparison is done omitting "-" and "_" characters--these are used inconsistently--and without case.
*/
struct CodePageInfo
{
    char CodePageName[12];
    unsigned int WindowsCodePage;
};

static CodePageInfo CodePageInfoTable[] =
{
    // 7-bit code pages: US-ASCII or the "C" locale
    { "USASCII",    20127 },
    { "ASCII",      20127 },
    { "C",          20127 },
    { "ANSIX3",     20127 },
    { "646",        20127 },
    { "X3",         20127 },

    // Unicode code pages: ways of encoding 20-bit Unicode characters
    { "UTF8",       65001 },
    { "UTF7",       65000 },
    { "UTF16",       1200 },
    { "UTF16LE",     1200 },
    { "UTF16BE",     1201 },
    { "10646",       1200 },
    { "UTF32",      12000 },
    { "UTF32LE",    12000 },
    { "UTF32BE",    12001 },

    // 8-bit ISO code pages. These are the same as 7-bit ASCII up to 0x7F, then add additional printable
    // characters between 0xA0 - 0xFF
    { "ISO88591",   28591 },    // Latin - 1
    { "ISO88592",   28592 },    // Latin - 2
    { "ISO88593",   28593 },    // Latin - 3
    { "ISO88594",   28594 },    // Baltic
    { "ISO88595",   28595 },    // Cyrillic
    { "ISO88596",   28596 },    // Arabic
    { "ISO88597",   28597 },    // Greek
    { "ISO88598",   28598 },    // Hebrew
    { "ISO88599",   28599 },    // Turkish
    { "ISO885913",  28603 },    // Estonian
    { "ISO885915",  28605 },    // Latin - 9, like Latin - 1 but with Euro character and a few other substitutions

    // Windows 8-bit code pages. These are like 7-bit ASCII but with additional printable characters
    // between 0x80 - 0xFF
    { "ANSI1250",    1250 },    // Windows Central European
    { "ANSI1251",    1251 },    // WIndows Cyrillic
    { "ANSI1252",    1252 },    // Windows Western European, like ISO 8859-1 but with printable characters between 0x80 - 0x9F
    { "ANSI1253",    1253 },    // Windows Greek
    { "ANSI1254",    1254 },    // Windows Turkish
    { "ANSI1255",    1255 },    // Windows Hebrew
    { "ANSI1256",    1256 },    // Windows Arabic
    { "ANSI1257",    1257 },    // Windows Baltic
    { "ANSI1258",    1258 },    // Windows Vietnamese

    // 8-bit non-Windows code pages
    { "KOI8R",      20866 },    // Cyrillic w/o Ukrainian letters
    { "KOI8U",      21866 },    // Cyrillic w/ Ukrainian letters
    { "TIS620",       874 },    // Thai

    // Extended Unix multi-byte code pages for Asian languages
    { "SHIFTJIS",     932 },    // Japanese
    { "PKC",          932 },    // Japanese
    { "EUCJP",      51932 },    // Japanese Extended Unix Code
    { "GBK",          936 },    // Simplified Chinese
    { "GB2312",     20936 },    // Simplified Chinese National Standard (Guojia Biaozhun)
    { "GB18030",    54936 },    // Simplified Chinese National Standard (Guojia Biaozhun), characters added to GB2312
    { "EUCGB",      20936 },    // Simplified Chinese Extended Unix Code
    { "CNS11643",   20936 },    // Simplified Chinese
    { "BIG5",         950 },    // Traditional Chinese Big-5
    { "BIG5HK",       950 },    // Traditional Chinese Big-5 as used in Hong Kong SAR
    { "BIG5+HKSCS",   950 }     // Traditional Chinese Big-5 as used in Hong Kong SAR

    // if support is needed for reporting Mac code pages, add names and values for code pages 100xx here

    // if support is needed for reporting IBM code pages, add names and values for code pages < 1000 here
};

/*
 Get the name of a code page from a descriptive string.

 Code page names with minor standard values (stuff after a period) are compared up to the period only.
 Comparison is done omitting "-" and "_" characters--these are used inconsistently--and without case.

 @param[in]     CodePageName - the string that describes the code page

 @return        the Windows code page number that corresponds to the descriptive string
*/
static unsigned int GetCodePage(const std::string& codePageName)
{
    char name[12];
    size_t n = 0;
    char c;

    // eliminate everything after a period or after the first extended or UTF character in the name
    // and upcase the name
    for (size_t i = 0; i < codePageName.size() && i < 11; i++)
    {
        c = codePageName[i];
        if (c == '.' || (signed char)c < 0)
        {                               // the first '.' or extended char. ends the comparison name
            break;
        }
        if (c != '-' && c != '_' && c != ' ')
        {                               // save any character but '-' and space
            if (c >= 'a' && c <= 'z')
            {                           // upcase each letter used in comparison
                c = (char)((unsigned int)c & 0x005F);
            }
            name[n++] = c;
        }
    }
    name[n] = '\0';

    // search for a match in the table
    for (size_t i = 0; i < sizeof CodePageInfoTable / sizeof (CodePageInfo); i++)
    {
        if (strcmp(name, CodePageInfoTable[i].CodePageName) == 0)
        {
            return CodePageInfoTable[i].WindowsCodePage;
        }
    }

    return 0;                           // if not found, flag that to caller
}

namespace SCXSystemLib
{
    const wchar_t *OSInstance::moduleIdentifier = L"scx.core.common.pal.system.os.osinstance";

    /*
     Get the language, country and code page information from the Unix/Linux LANG variable.
     The LANG variable is supposed to be of one of the forms:

      la
      la_CO
      la_CO.CodePage

     where la is a two-letter lower-case ISO 639-1 langage identifier or "C",
     CO is a two-letter upper-case ISO 3166-1 country/region identifier,
     and CodePage is a code page name string.

     Because this is a user-set variable, this code will force the cases of
     the identifiers and accept "-" instead of "_" as the first separator. This
     scheme will also accept Internet Explorer language codes, like "en-gb", as
     well as the standard codes, like "en_GB".

     Code page descriptive names are also matched in a case-insensitive way and minor
     standard version numbers, the part after the first period, are ignored, as
     are '-' and '_' characters.
    */
    bool ParseLangVariable(
        const wstring& langSetting,
        wstring& countryCode,
        unsigned int& osLanguage,
        wstring& codeSet)
    {
        std::string localLangSetting = StrToUTF8(langSetting);
        bool codePageSpecified = false;

        // set defaults
        countryCode = L"1";             // US or Canada is the default
        osLanguage = 9;                 // English-no locale is the default
        codeSet = L"20127";             // 7-bit US ASCII, the "C" locale

        if (localLangSetting.size() == 2 || (localLangSetting.size() > 2 && (localLangSetting[2] == '_' || localLangSetting[2] == '-')))
        {
            char isoLangCode[2];

            isoLangCode[0] = (char)((unsigned int)localLangSetting[0] | 0x0020); // cheap lower-case: no need for a better one, because
            isoLangCode[1] = (char)((unsigned int)localLangSetting[1] | 0x0020); // punctuation characters won't match table entries anyway
            if (localLangSetting.size() < 5)
            {                           // handle the "la" form of the LANG variable - language only
                osLanguage = GetWindowsLanguageCode(isoLangCode[0], isoLangCode[1]);
                countryCode = StrFrom(GetDefaultCountryCode(isoLangCode[0], isoLangCode[1]));
                codeSet = StrFrom(GetDefaultCodePage(isoLangCode[0], isoLangCode[1]));
            }
            else
            {
                char isoCountryCode[2];

                isoCountryCode[0] = (char)((unsigned int)localLangSetting[3] & 0x005F);  // cheap upper-case: no need for a better one, because
                isoCountryCode[1] = (char)((unsigned int)localLangSetting[4] & 0x005F);  // punctuation characters won't match table entries anyway
                osLanguage = GetWindowsLocaleCode(isoLangCode[0], isoLangCode[1], isoCountryCode[0], isoCountryCode[1]);
                countryCode = StrFrom(GetCountryCode(isoCountryCode[0], isoCountryCode[1], isoLangCode[0], isoLangCode[1]));
                if (localLangSetting.size() <= 6 || localLangSetting[5] != '.')
                {
                    // If we see 8-bit Cyrillic and Chinese code pages instead of UTF code pages, this will have
                    // to be rewritten to use the country code as well, to distinguish simplified and traditional
                    // Chinese and Turkish/central European languages written with Latin or Cyrillic characters, etc.
                    codeSet = StrFrom(GetDefaultCodePage(isoLangCode[0], isoLangCode[1]));
                    codePageSpecified = false;
                }
                else
                {
                    unsigned int codePage = GetCodePage(localLangSetting.substr(6));
                    if (codePage != 0)
                    {
                        codePageSpecified = true;
                        codeSet = StrFrom(codePage);
                    }
                    else
                    {
                        codeSet = L"20127";
                    }
                }
            }
        }
        return codePageSpecified;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Constructor
    */
    OSInstance::OSInstance() :
        EntityInstance(true)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGTRACE(m_log, L"OSInstance constructor");

        // Initialize OS detail information
        m_osDetailInfo.BootDevice = L"";
        m_osDetailInfo.CodeSet = L"";
        m_osDetailInfo.CountryCode = L"";
        m_osDetailInfo.MUILanguages.clear();
        m_osDetailInfo.OSLanguage = 0;
        m_osDetailInfo.ProductType = ePTMax;

        // Do various initiation that can be done once and for all
#if defined(linux)
        PrecomputeMaxProcesses();
#elif defined(sun) || defined(aix)
        /* Nothing yet */
#elif defined(hpux)
        /* Get information the system static variables (guaranteed constant until reboot) */
        m_psts_isValid = true;
        if (pstat_getstatic(&m_psts, sizeof(m_psts), 1, 0) < 0) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not do pstat_getstatic(). errno = ", errno));
            m_psts_isValid = false;
        }

        // Compute the boot time once and for all
        SetBootTime();
#else
        #error "Not implemented for this platform."
#endif

#if defined(linux) || defined(sun)
        m_LangSetting = L"";
#endif

    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    OSInstance::~OSInstance()
    {
        SCX_LOGTRACE(m_log, L"OSInstance destructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Updates instance with latest data in preparation for read of individual
       properties.
    */
    void OSInstance::Update()
    {
        SCX_LOGTRACE(m_log, L"OSInstance Update()");

        // Get current time
        m_now = SCXCalendarTime::CurrentLocal();

#if defined(hpux)
        m_unameIsValid = !((uname(&m_unameInfo) < 0) && (errno != EOVERFLOW));
        // Meaning: Ok if no error, or if errno is EOVERFLOW
#else
        m_unameIsValid = !(uname(&m_unameInfo) < 0);
#endif
        if (!m_unameIsValid) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not do uname(). errno = ", errno));
        }

        SetBootTime();
        SetUptime();

#if defined(hpux)
        /* Get information about the system dynamic variables */
        m_pstd_isValid = true;
        if (pstat_getdynamic(&m_pstd, sizeof(m_pstd), 1, 0) != 1) {
            SCX_LOGERROR(m_log, StrAppend(L"Could not do pstat_getdynamic(). errno = ",errno));
            m_pstd_isValid = false;
        }
#endif

        //
        // Get system language/locale information from LANG envrionment variable and perhaps nl_langinfo
        //
        bool codePageSpecified = false;
        m_osDetailInfo.CountryCode = L"1";   // US or Canada is the default
        m_osDetailInfo.OSLanguage = 9;       // English-no locale is the default
        m_osDetailInfo.CodeSet = L"20127";   // 7-bit US ASCII, the "C" locale
        if (getOSLangSetting(m_LangSetting))
        {
            codePageSpecified = ParseLangVariable(m_LangSetting,
                                                  m_osDetailInfo.CountryCode,
                                                  m_osDetailInfo.OSLanguage,
                                                  m_osDetailInfo.CodeSet);
        }
        if (!codePageSpecified)
        {
            //
            // Get CodeSet from nl_langinfo
            //
            std::string codeSet = nl_langinfo(CODESET);

            // For Linux:
            // The CodeSet "ANSI_X3.4-1968" means the same as 7-bit US-ASCII
            //
            m_osDetailInfo.CodeSet = StrFrom(GetCodePage(codeSet));
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Clean up the instance. Closes the thread.
    */
    void OSInstance::CleanUp()
    {
        SCX_LOGTRACE(m_log, L"OSInstance CleanUp()");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Dump object as string (for logging).

       \returns     The object represented as a string suitable for logging.
    */
    const wstring OSInstance::DumpString() const
    {
        wstringstream ss;
        ss << L"OSInstance";
        return ss.str();
    }

    /*----------------------------------------------------------------------------*/
    /* Platform specific utility functions */

#if defined(linux)
    /**
       Computes the kernel-configured maximum number
       of processes.

       Since this is not likely to change until reboot we
       compute this number once and for all.
    */
    void OSInstance::PrecomputeMaxProcesses(void)
    {
        // VERY inspired by the Pegasus code.
        //-- prior to 2.4.* kernels, this will not work.  also, this is
        //   technically the maximum number of threads allowed; since
        //   linux has no notion of kernel-level threads, this is the
        //   same as the total number of processes allowed.  should
        //   this change, the algorithm will need to change.
        const char proc_file[] = "/proc/sys/kernel/threads-max";
        const size_t MAXPATHLEN = 80;
        char buffer[MAXPATHLEN];

        m_MaxProcesses = 0;
        FILE* vf = fopen(proc_file, "r");
        if (vf)
        {
            if (fgets(buffer, MAXPATHLEN, vf) != NULL)
                sscanf(buffer, "%u", &m_MaxProcesses);
            fclose(vf);
        }
    }

#endif /* linux */

    /**
     *  Sets the time related member variables.
     *  Information is read from the utmp file
     */
    void OSInstance::SetBootTime(void)
    {
        m_system_boot_isValid = false;

        int fd;
        struct utmpx record;
        int reclen = sizeof(struct utmpx);

        fd = open(UTMPX_FILE, O_RDONLY);
        if (fd == -1){
            SCX_LOGERROR(m_log, StrAppend(L"Could not open UTMP file. errno = ", errno));
            return;
        }

        while (read(fd, &record, reclen) == reclen)
        {
            if (strcmp(record.ut_line, "system boot") == 0 || // Aix
                strcmp(record.ut_user, "reboot") == 0 ||      // Linux
                strcmp(record.ut_id, "si") == 0)              // Suse
            {
                time_t boot_time = record.ut_tv.tv_sec;
                SCX_LOGTRACE(m_log, StrAppend(L"Read utmp system boot time = ", boot_time));
                try
                {
                    m_system_boot = SCXCalendarTime::FromPosixTime((scxulong)boot_time);
                    m_system_boot.MakeLocal();
                    m_system_boot_isValid = true;
                }
                catch (const SCXNotSupportedException& e)
                {
                    SCX_LOGERROR(m_log, std::wstring(L"Error converting timestamp")
                                 .append(L" - ").append(e.What()));
                    break;
                }
                break;
            }
        }
        close (fd);
    }

    void OSInstance::SetUptime()
    {
        m_upsec_isValid = false;
#if defined(linux)
        // Read seconds since boot
        FILE *uptimeFile = fopen("/proc/uptime", "r");
        if (uptimeFile)
        {
            long unsigned int tmp_upsec;
            int success = fscanf(uptimeFile, "%lu", &tmp_upsec);
            if (success == 1)
            {
                m_upsec = static_cast<scxulong>(tmp_upsec);
                m_upsec_isValid = true;
            }
            else
            {
                SCX_LOGERROR(m_log, StrAppend(L"Could not read /proc/uptime. errno = ", errno));
            }
        }
        else
        {
            SCX_LOGERROR(m_log, StrAppend(L"Could not open /proc/uptime. errno = ", errno));
        }
        fclose(uptimeFile);
#else
        if (m_system_boot_isValid)
        {
            time_t nowTime = time(0);   // get time now
            time_t bootTime = m_system_boot.ToPosixTime();

            m_upsec = nowTime - bootTime;
            m_upsec_isValid = (m_upsec > 0);
        }
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
    Get OS lang setting.

    \param      value - the extracted value if there is one
    Return false if can not get the value of "LANG" SETTING
    */
    bool OSInstance::getOSLangSetting(wstring& lang) const
    {
        //
        // Read system LANG setting
        char *sRet = getenv("LANG");
        if (sRet != NULL)
        {
            lang = StrFromUTF8(sRet);
            if (!lang.empty())
            {
                return true;
            }
        }

        return false;
    }

        /*====================================================================================*/
        /* Properties of SCXCM_OperatingSystem                                                */
        /*====================================================================================*/
        /*
         * Get Boot Device
         * Parameters: bd - RETURN: Boot Device of OS instance
         * Retval:     true if a value is supported by this platform
         * Throw:      SCXNotSupportedException - For not implemented platform
         */
        bool OSInstance::GetBootDevice(wstring& bd) const
        {
                bd = m_osDetailInfo.BootDevice;
#if defined(linux) && defined(PF_DISTRO_REDHAT)
                return true;
#elif defined(linux) && defined(PF_DISTRO_ULINUX)
                if (bd != L"")
                    return true;
#endif

#if defined(SCX_UNIX)
                return false;
#else
                throw SCXNotSupportedException(L"Boot Device", SCXSRCLOCATION);
#endif
        }

    /*
         * Get Code Set
         * Parameters: cs - RETURN: Code Set of OS instance
         * Retval:     true if a value is supported by this platform
         * Throw:      SCXNotSupportedException - For not implemented platform
         */
        bool OSInstance::GetCodeSet(wstring& cs) const
        {
                cs = m_osDetailInfo.CodeSet;
#if defined(linux) || defined(sun)
        if(!cs.empty())
        {
            return true;
        }
        else
        {
            return false;
        }
#elif defined(aix) || defined(hpux) || defined(macos)
            return false;
#else
            throw SCXNotSupportedException(L"Code Set", SCXSRCLOCATION);
#endif
        }

    /*
     * Get Country Code
     * Parameters: cc - RETURN: Country Code of OS instance
     * Retval:     true if a value is supported by this platform
     * Throw:      SCXNotSupportedException - For not implemented platform
         */
    bool OSInstance::GetCountryCode(wstring& cc) const
    {
        cc = m_osDetailInfo.CountryCode;
#if defined(linux) || defined(sun)
        if(!cc.empty())
        {
            return true;
        }
        else
        {
            return false;
        }
#elif defined(aix) || defined(hpux) || defined(macos)
        return false;
#else
        throw SCXNotSupportedException(L"Country Code", SCXSRCLOCATION);
#endif
    }

    /*
     * Get OSLanguage
     * Parameters: lang - RETURN: OSLanguage of OS instance
     * Retval:     true if a value is supported by this platform
     * Throw:      SCXNotSupportedException - For not implemented platform
     */
    bool OSInstance::GetOSLanguage(unsigned int& lang) const
    {
        lang = m_osDetailInfo.OSLanguage;
#if defined(linux)  || defined(sun)
        if (m_osDetailInfo.OSLanguage != 0)
        {
           lang = m_osDetailInfo.OSLanguage;
           return true;
        }
        return false;

#elif defined(aix) || defined(hpux) || defined(macos)
        return false;
#else
        throw SCXNotSupportedException(L"OSLanguage", SCXSRCLOCATION);
#endif
    }

    /*
     * Get MUI(Multiling User Interface) Pack Languages
     * Parameters: langs - RETURN: MUILanguages of OS instance
     * Retval:     true if a value is supported by this platform
     * Throw:      SCXNotSupportedException - For not implemented platform
     */
    bool OSInstance::GetMUILanguages(vector<wstring>& langs) const
    {
        langs = m_osDetailInfo.MUILanguages;
#if defined(linux) && defined(PF_DISTRO_REDHAT)
        return true;
#elif defined(linux) && defined(PF_DISTRO_ULINUX)
        if (langs.size() > 0)
            return true;
#endif

#if defined(SCX_UNIX)
        return false;
#else
        throw SCXNotSupportedException(L"MUILanguages", SCXSRCLOCATION);
#endif
    }

    /*
     * Get Product Type  TODO: How to determine the type in future releases??
     * Parameters: pt - RETURN: Product Type of OS instance
     * Retval:     true if a value is supported by this platform
     * Throw:      SCXNotSupportedException - For not implemented platform
     */
        bool OSInstance::GetProductType(unsigned int& pt) const
    {
#if defined(SCX_UNIX)
        pt = ePTServer;
        return true;
#else
        #error Platform not supported
#endif

    }

    /*
     * Get Manufacturer
     * Parameters: manufacturer - RETURN: Manufacturer
     * Retval:     true if a value is supported by this platform
     */
    bool OSInstance::GetManufacturer(wstring& manufacturer) const
    {
        manufacturer = m_osInfo.GetManufacturer();
        return !manufacturer.empty();
    }

    /*====================================================================================*/
    /* Properties of CIM_OperatingSystem                                                  */
    /*====================================================================================*/

    /**
       Gets the OSType
       \param[out]  ost
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       A integer indicating the type of OperatingSystem.
    */
    bool OSInstance::GetOSType(unsigned short& ost) const
    {
#if defined(linux)
        ost = LINUX;
        return true;
#elif defined(aix)
        ost = AIX;
        return true;
#elif defined(hpux)
        ost = HP_UX;
        return true;
#elif defined(macos)
        ost = MACOS;
        return true;
#elif defined(sun)
        ost = Solaris;
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the OtherTypeDescription
       \param[out]  otd
       \returns     true if this value is supported by the implementation

       \note Linux: Note that the implementation is just plain wrong with
       regard to how this property is defined by the CIM model.

       According to the CIM model:
       A string describing the manufacturer and OperatingSystem
       type - used when the OperatingSystem property, OSType, is
       set to 1 or 59 (\"Other\" or \"Dedicated\"). The format of
       the string inserted in OtherTypeDescription should be
       similar in format to the Values strings defined for OSType.
       OtherTypeDescription should be set to NULL when OSType is
       any value other than 1 or 59.
    */
    bool OSInstance::GetOtherTypeDescription(wstring& otd) const
    {
#if defined(SCX_UNIX)
        if (!m_unameIsValid)
        {
            return false;
        }
#else
        #error Platform not supported
#endif

        wstring tmp( StrFromUTF8(m_unameInfo.release) );
        tmp.append(L" ");
        tmp.append( StrFromUTF8(m_unameInfo.version) );
        tmp.append(L" ");
        tmp.append( StrFromUTF8(m_unameInfo.machine) );
        tmp.append(L" ");
        tmp.append(m_osInfo.GetProcessor());
        tmp.append(L" ");
        tmp.append(m_osInfo.GetHardwarePlatform());
        tmp.append(L" ");
        tmp.append(m_osInfo.GetOperatingSystem());

        otd.assign(tmp);
        return true;
    }


    /**
       Gets the Version
       \param[out]  ver
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       A string describing the Operating System's version
       number. The format of the version information is as follows:
       [Major Number].[Minor Number].[Revision] or
       [Major Number].[Minor Number].[Revision Letter].
    */
    bool OSInstance::GetVersion(wstring& ver) const
    {
        bool fRet = false;
#if defined(SCX_UNIX)
        if (m_unameIsValid)
        {
#if defined(hpux) || (defined(linux) && !defined(PF_DISTRO_ULINUX))
            ostringstream oss(stringstream::out);
            oss << PF_MAJOR;
            oss << ".";
            oss << PF_MINOR;
            ver.assign(StrFromUTF8(oss.str()));
            fRet = true;

#elif defined(PF_DISTRO_ULINUX)
            ver = m_osInfo.GetOSVersion();
            fRet = !ver.empty();
#elif defined(sun)
            const char *sminv = strrchr(m_unameInfo.release, '.');
            if (sminv != NULL && sminv[1] != '\0')
            {
                ver.assign(StrFromUTF8(&sminv[1]));
            }
            else
            {
                ver.assign(StrFromUTF8(m_unameInfo.release));
            }
            fRet = true;
#elif defined(aix)
            scxulong release = 0;
            scxulong version = 0;
            wstringstream ssv(StrFromUTF8(m_unameInfo.version));
            wstringstream ssr(StrFromUTF8(m_unameInfo.release));
            wstringstream ss;
            if ((ssv >> version) && (ssr >> release))
            {
                ss << version << "." << release;
            } else
            {
                ss << m_unameInfo.version << " " << m_unameInfo.release;
            }
            ver.assign(ss.str());
            fRet = true;
#else
            #error Platform not supported
#endif
        }

#else
        #error Platform not supported
#endif
        return fRet;
    }

    /**
       Gets the BuildNumber
       \param[out]  buildNumber
       \returns     true if this value is supported by the implementation

       According to the Win32_OperatingSystem model:
       Build number of an operating system. It can be used for more precise
       version information than product release version numbers
    */
    bool OSInstance::GetBuildNumber(wstring& buildNumber) const
    {
        bool fRet = false;
#if defined(SCX_UNIX)
        if (m_unameIsValid)
        {
            buildNumber.assign(StrFromUTF8(m_unameInfo.release));
            fRet = true;
        }
#else
        #error Platform not supported
#endif
        return fRet;
    }

    /**
       Gets the LastBootUpTime
       \param[out]  lbut
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Time when the OperatingSystem was last booted.
    */
    bool OSInstance::GetLastBootUpTime(SCXCalendarTime& lbut) const
    {
#if defined(linux) || defined(hpux) || defined(aix) || defined(sun)
        // Same basic solution as Pegasus, but we have a time PAL to use instead
        // The m_system_boot is set once by Update().
        if (!m_system_boot_isValid) { return false; }
        lbut = m_system_boot;
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the LocalDateTime
       \param[out]  ldt
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       OperatingSystem's notion of the local date and time of day.
    */
    bool OSInstance::GetLocalDateTime(SCXCalendarTime& ldt) const
    {
#if defined(SCX_UNIX)
        ldt = m_now;
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the CurrentTimeZone
       \param[out]  ctz
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       CurrentTimeZone indicates the number of minutes the
       OperatingSystem is offset from Greenwich Mean Time.
       Either the number is positive, negative or zero.
    */
    bool OSInstance::GetCurrentTimeZone(signed short& ctz) const
    {
#if defined(SCX_UNIX)
        ctz = static_cast<signed short>(m_now.GetOffsetFromUTC().GetMinutes());
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the NumberOfLicensedUsers
       \param[out]  nolu
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Number of user licenses for the OperatingSystem.
       If unlimited, enter 0.
    */
    bool OSInstance::GetNumberOfLicensedUsers(unsigned int& nolu) const
    {
#if defined(hpux)
        // Taken from the Pegasus implementation
        if (!m_unameIsValid) { return false; }
        // For HP-UX, the number of licensed users is returned in the version
        // field of uname result.
        switch (m_unameInfo.version[0]) {
        case 'A' : { nolu = 2; break; }
        case 'B' : { nolu = 16; break; }
        case 'C' : { nolu = 32; break; }
        case 'D' : { nolu = 64; break; }
        case 'E' : { nolu = 8; break; }
        case 'U' : {
            // U could be 128, 256, or unlimited
            // need to find test system with 128 or 256 user license
            // to determine if uname -l has correct value
            // for now, return 0 = unlimited
            nolu = 0;
            break;
        }
        default : return false;
        }
        return true;
#elif defined(sun)
        // According to the Pegasus Solaris implementation they don't know how
        // to determine this number, but they still return 0 for unlimited.
        nolu = 0;
        return true;
#else
        nolu = 0;
        return true;
#endif
    }

    /**
       Gets the NumberOfUsers
       \param[out]  numberOfUsers
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Number of user sessions for which the OperatingSystem is
       currently storing state information.
    */
    bool OSInstance::GetNumberOfUsers(unsigned int& numberOfUsers) const
    {
#if defined(SCX_UNIX)
        // This is the Pegasus code, straight up.
        // Note that getutxent() isn't thread safe, but that's no problem
        // since access here is protected.
        struct utmpx * utmpp;

        numberOfUsers = 0;

        while ((utmpp = getutxent()) != NULL)
        {
            if (utmpp->ut_type == USER_PROCESS)
            {
                numberOfUsers++;
            }
        }

        endutxent();
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the MaxNumberOfProcesses
       \param[out]  mp
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Maximum number of process contexts the OperatingSystem can
       support. If there is no fixed maximum, the value should be 0.
       On systems that have a fixed maximum, this object can help
       diagnose failures that occur when the maximum is reached.
    */
    bool OSInstance::GetMaxNumberOfProcesses(unsigned int& mp) const
    {
#if defined(linux)
        mp = m_MaxProcesses;
        return m_MaxProcesses != 0;
#elif defined(hpux)
        mp = m_psts.max_proc;
        return m_psts_isValid;
#elif defined(sun) | defined(aix)
        // Not supported by the Pegasus Solaris or AIX implementation
        (void) mp;
        return false;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the MaxProcessMemorySize
       \param[out]  mpms
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       Maximum number of Kbytes of memory that can be allocated
       to a Process. For Operating Systems with no virtual memory,
       this value is typically equal to the total amount of
       physical Memory minus memory used by the BIOS and OS. For
       some Operating Systems, this value may be infinity - in
       which case, 0 should be entered. In other cases, this value
       could be a constant - for example, 2G or 4G.
    */
    bool OSInstance::GetMaxProcessMemorySize(scxulong& mpms) const
    {
#if defined(linux) || defined(sun) || defined (aix)
        struct rlimit rls;

        int res = getrlimit(RLIMIT_AS, &rls);
        if (0 == res)
        {
            if (RLIM_INFINITY == rls.rlim_max)
            {
                mpms = 0;
            }
            else
            {
                mpms = rls.rlim_max / 1024;
            }
        }
        return res == 0;
#elif defined(hpux)
        // Pegasus implements a very elaborate scheme that supports many
        // different versions of HP/UX through various mechanisms to get
        // kernel configuration data. Since we only support 11v3 and on
        // we can cut out all the older stuff and just reuse what we need.
        // But corrected to return the output in Kilobytes.

        const static char *maxsiz[3][2] = { { "maxdsiz", "maxdsiz_64bit" },
                                            { "maxssiz", "maxssiz_64bit" },
                                            { "maxtsiz", "maxtsiz_64bit" }};
        int is64 = sysconf(_SC_KERNEL_BITS) == 64 ? 1 : 0;
        uint64_t data = 0, sum = 0;
        for (int i = 0; i < 3; i++) {
            if (gettune(maxsiz[i][is64], &data) != 0) { return false; }
            sum += data;
        }
        mpms = sum / 1024;
        return true;
#else
        #error Platform not supported
#endif
    }

    /**
       Gets the MaxProcessesPerUser
       \param[out]  mppu
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       A value that indicates the maximum processes that a user
       can have associate with it.
    */
    bool OSInstance::GetMaxProcessesPerUser(unsigned int& mppu) const
    {
#if defined(linux) || defined(sun) || defined(aix)
        // Not supported on Solaris, for some reason. We reuse the Linux code.
        // Here's the Pegasus implementation for Linux. Spot the problem?
        // return sysconf(_SC_CHILD_MAX);
        long res = sysconf(_SC_CHILD_MAX);

        if (res == -1 && errno == 0)
        {
            res = INT_MAX;
        }

        mppu = static_cast<unsigned int>(res);

        return (res >= 0);
#elif defined(hpux)
        // We could use the same sysconf() call as on Linux,

        // but Pegasus uses gettune(), and so do we.
        uint64_t maxuprc = 0;
        if (gettune("maxuprc", &maxuprc) != 0)
        {
            return false;
        }
        mppu = maxuprc;
        return true;
#else
        #error Platform not supported
#endif
    }

    /*====================================================================================*/
    /* Properties of SCX_OperatingSystem (These comes from PG_OperatingSystem)            */
    /*====================================================================================*/

    /**
       Gets the SystemUpTime
       \param[out]  sut
       \returns     true if this value is supported by the implementation

       According to the CIM model:
       The elapsed time, in seconds, since the OS was booted.
       A convenience property, versus having to calculate
       the time delta from LastBootUpTime to LocalDateTime.
    */
    bool OSInstance::GetSystemUpTime(scxulong& sut) const
    {
#if defined(linux) || defined(sun) || defined(aix) || defined(hpux)
        sut = m_upsec;
        return m_upsec_isValid;
#else
        #error Platform not supported
#endif
    }

}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
