/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief     Locale initialization

   \date      2008-08-07 16:14:14


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxlocale.h>
// #include <scxcorelib/scxfacets.h>      <-- I want this!
#include <scxcorelib/scxstream.h>         // But, instead I must include this!
#include <scxcorelib/stringaid.h>

#include <locale>
#include <iostream>

#include <locale.h>

#if defined(sun)
#include <langinfo.h>
#include <errno.h>
#include <sstream>
#endif

using namespace std;

namespace SCXCoreLib {

#if defined(sun)
    /**     Saved conversion descriptor for UTF-8 to wchar_t conversion
    */
    iconv_t SCXLocaleContext::m_FromUTF8iconv = NULL;

    /**     Saved conversion descriptor for wchar_t to UTF-8 conversion
    */
    iconv_t SCXLocaleContext::m_ToUTF8iconv = NULL;

    /**     Indicate if we should use iconv for UTF-8/wchar_t conversion
    */
    bool SCXLocaleContext::m_UseIconv = false;

    /**     Indicate if we wanted to use iconv for UTF-8/wchar_t conversion
    */
    bool SCXLocaleContext::m_WantToUseIconv = false;

    /**     Array of errors encountered during initialization.
    */
    std::vector<wstring> SCXLocaleContext::m_Errors;
#endif

    /* Note: One or more of these symbols *must* be referred to in the code,
       or else this whole object file is optimized away by the linker on some 
       platforms. And then the locale never gets initialized. Yes, this really is so!
    */

    /**
        Creates a new SCXLocaleContext setting the current locale to what is supplied
        as argument.
        
        \param[in] lname           Name of the locale to set.
        \param[in] resetAtDestroy  Should we reset to old locale in destructor
    */
    SCXLocaleContext::SCXLocaleContext(const char *lname, bool resetAtDestroy) : m_saved(locale()), m_resetAtDestroy(resetAtDestroy)
    {
        locale::category cat = locale::ctype;   
#if defined(hpux)
        m_SavedPid == getpid(); // Needed to verify validity
        m_SavedCLocale = setlocale(LC_CTYPE, 0);

        // A locale defined by the parameters of the function
        locale base(locale::classic(), lname, cat);
        // A locale that has a translation capable-facet
        std::locale addon(SCXCoreLib::SCXStream::GetLocaleWithSCXDefaultEncodingFacet());
        // And combine the two
        m_current = base.combine<std::codecvt<wchar_t, char, mbstate_t> >(addon);
        locale::global(m_current);

        /* Give the C interface something to work with. This is required for mbrtowc & co. */
        setlocale(LC_CTYPE, lname);

        // Btw, this is how it *should* look
        // locale::global(locale(locale(locale::classic(), lname, cat), 
        //                       new SCXCoreLib::SCXDefaultEncodingFacet()));

#elif defined(macos)
        // Currently MACOS has no support for anything but the "C" locale
        m_current = locale::classic();
        locale::global(m_current);
#else
        m_current = locale(locale::classic(), lname, cat);
        locale::global(m_current);
#endif

#if defined(sun)
        SetIconv();
#endif
    }



#if defined(sun)
    /**
       Get the conversion descriptor for UTF-8 to wchar_t conversion
       \return conversion descriptor for UTF-8 to wchar_t conversion
     */
    iconv_t SCXLocaleContext::GetFromUTF8iconv()
    {
        return m_FromUTF8iconv;
    }

    /**
       Get the conversion descriptor for wchar_t to UTF-8 conversion
       \return conversion descriptor for wchar_t to UTF-8 conversion
     */
    iconv_t SCXLocaleContext::GetToUTF8iconv()
    {
        return m_ToUTF8iconv;
    }

    /**
       Can we use iconv for UTF-8/wchar_t conversion
       \return true if we can use iconv for UTF-8/wchar_t conversion, else false
     */
    bool SCXLocaleContext::UseIconv()
    {
        return m_UseIconv;
    }

    /**
       Do we want to use iconv for UTF-8/wchar_t conversion
       \return true if we want to use iconv for UTF-8/wchar_t conversion, else false
     */
    bool SCXLocaleContext::WantToUseIconv()
    {
        return m_WantToUseIconv;
    }

    /**
       Try to create the iconv conversion descriptors
     */
    void SCXLocaleContext::SetIconv()
    {
        std::string code(nl_langinfo(CODESET));

        if (m_FromUTF8iconv != NULL)
        {
            if (iconv_close(m_FromUTF8iconv) != 0)
            {
                wstringstream os;
                os << L"SCXLocaleContext::SetIconv: iconv_close(m_FromUTF8iconv) failed: " << errno << endl;
                m_Errors.push_back(os.str());
            }
            m_FromUTF8iconv = NULL;
        }

        if (m_ToUTF8iconv != NULL)
        {
            if (iconv_close(m_ToUTF8iconv) != 0)
            {
                wstringstream os;
                os << L"SCXLocaleContext::SetIconv: iconv_close(m_ToUTF8iconv) failed: " << errno << endl;
                m_Errors.push_back(os.str());
            }
            m_ToUTF8iconv = NULL;
        }

        if (code == "UTF-8" || code == "C" || code == "646")
        {
            m_UseIconv = false;
            m_WantToUseIconv = false;
            return;
        }

        m_UseIconv = true;
        m_WantToUseIconv = true;

        m_FromUTF8iconv = iconv_open(code.c_str(), "UTF-8");

        if (m_FromUTF8iconv == (iconv_t)-1)
        {
            wstringstream os;
            os << L"SCXLocaleContext::SetIconv: iconv_open(" << SCXCoreLib::StrFromUTF8(code) << L", UTF-8) failed: " << errno << endl;
            m_Errors.push_back(os.str());
            m_FromUTF8iconv = NULL;
            m_UseIconv = false;
            return;
        }

        m_ToUTF8iconv = iconv_open("UTF-8", code.c_str());

        if (m_ToUTF8iconv == (iconv_t)-1)
        {
            wstringstream os;
            os << L"SCXLocaleContext::SetIconv: iconv_open(UTF-8, " << SCXCoreLib::StrFromUTF8(code) << L") failed: " << errno << endl;
            m_Errors.push_back(os.str());
            m_ToUTF8iconv = NULL;
            m_UseIconv = false;
            return;
        }
    }

    const std::vector<wstring>& SCXLocaleContext::GetErrors()
    {
        return m_Errors; 
    }

#endif /* defined(sun) */

    SCXLocaleContext::~SCXLocaleContext()
    {
        if (m_resetAtDestroy)
        {
            // Reset locale to original setting.
            // In theory this can throw, so lets protect ourselves.
            try {
                locale::global(m_saved);
            } catch(exception&) {
                // We're not really interested in what may have gone wrong.
                // (And no logging please. Think about it....)
            }
#if defined (hpux)
            /* On HPUX we must also reset the locale for the C-library. */
            // This may look funny, but the string is only valid within the same process
            if (m_SavedPid == getpid()) {
                setlocale(LC_CTYPE, m_SavedCLocale);
            }
#endif
#if defined(sun)
            SetIconv();
#endif
        }
    }
    
    /**
       Get the name string for the global locale setting.
       \return Locale name string.
     */
    wstring SCXLocaleContext::GetActiveLocale()
    {
        return SCXCoreLib::StrFromUTF8(std::locale().name());
    }

    /**
       Gets name string for the ctype/LC_CTYPE category.
       That's the only category whose locale-features we're interested in.
       This name is not dependable for C++, and only included here because
       there is no C++ counterpart.
       \return Locale name string for the LC_CTYPE category.
    */
    wstring SCXLocaleContext::GetCtypeName()
    {
        string tmp(setlocale(LC_CTYPE, 0)); // The is no C++ interface to this
        return SCXCoreLib::StrFromUTF8(tmp);
    }
}

/** Defines the SCXLocaleInit class and its global object. This must be defined
    after all SCXLocalContext definitions to avoid constructor ordering errors. 
    According to the C++ standard, objects are constructed in the same order 
    they are declared in the source file (from top to bottom).
*/
namespace { // put it in anonymous namespace
    
    /** 
        SCXLocaleInit initializes the locale settings for the scx module on creation.
        It should be created as a global object and is effectively a singleton.
        (We could wrap it in a SCXSingleton template, but since it hasn't got
        global access, that level of protection isn't neccessary.)
        
        \note We can't rely on anything to be running, so no logging from here.
        And don't call any other SCX class that may do logging.
    */
    class SCXLocaleInit {
    private:
        SCXCoreLib::SCXLocaleContext *ctx;    //!< Locale context that we've initiated
        SCXLocaleInit( const SCXLocaleInit &obj);  // Disable copy
    public:
        /** The constructor sets the global locale. */
        SCXLocaleInit()
        {
            // We use locale defined by the environment, but only the LC_CTYPE part of it.
            // Then compose a our own locale out of the "C" locale and the ctype-
            // category from the current LANG environment variable.
            // (This can be overridden by setting LC_CTYPE, or LC_ALL.)
            // All this is now handled by the SCXLocaleContext class.
            try {
                // We use new here to avoid creating a temporary object that must be
                // destroyed and thus run the destructor, but at the same time run it
                // inside a try-catch. Is there another way of doing that without
                // using a new? And no ScxHandle here, that's risky.
                ctx = new SCXCoreLib::SCXLocaleContext("", false);
            } catch(exception& e) {
                /* OK, locale init failed. We can't log it, but we can try to write
                   on stderr in case someone sees it. At least this isn't so critical
                   that the app will crash.
                */
                cerr << "[scx.core.common.pal.os.scxlocale] SCXLocaleInit::Initialization "
                    "failed. What = " << e.what() << endl;
            }
        }

        ~SCXLocaleInit()
        {
            // Reset locale to original setting.
            // In theory this can throw, so lets protect ourselves.
            try {
                delete ctx;
                ctx = 0;
            } catch(exception&) {
                // We're not really interested in what may have gone wrong.
            }
        }
    };

    /** This global instance only exists so that the constructor of SCXLocaleInit
        is run when the library is loaded and before any calls are made.
    */
    static SCXLocaleInit global_instance;
}
