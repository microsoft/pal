/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

 */
/**
    \file

    \brief     Convenience functions for locale handling

    \date      2008-08-08 08:08:08


 */
/*----------------------------------------------------------------------------*/
#ifndef SCXLOCALE_H
#define SCXLOCALE_H

#include <scxcorelib/scxcmn.h>

#include <locale>
#include <string>
#if defined(hpux)
#include <unistd.h>
#endif

#if defined(sun)
#include <iconv.h>
#include <vector>
#endif

namespace SCXCoreLib {

    /**
       A context for the currently set locale as required in the SCX project.

       When an instance is created, the locale is set as instructed in the constructor.
       When it expires, the locale is reset to the previous value.

       All this is tailored to the needs of the SCX project. For example, only the 
       'ctype' category is ever affected. The other categories remain set to the
       "C" locale. There are also special fixes for certain problematic platforms,
       for instance, on HPUX we replace the codecvt facet with our implementation.
       
       \note Even though an instance can exists in a local context don't be fooled
       into thinking that the effects are local. Creating an instance WILL set
       the global locale setting and thus affect any other running threads.
       For that reason there should only exist one SCXLocaleContext in a multi-threaded
       program. This construct is mostly suitable for testing, that is effectively
       single-threaded, and in which you want to make sure that one test doesn't affect
       the outcome of another.
       
    */
    class SCXLocaleContext
    {
    public:
        /** Creates an uninitialized context that just copies the active locale. */
        SCXLocaleContext() { }

        SCXLocaleContext(const char *lname, bool resetAtDestroy=true);
        ~SCXLocaleContext();

        static std::wstring GetActiveLocale();
        static std::wstring GetCtypeName();

#if defined(sun)
        static iconv_t GetFromUTF8iconv();
        static iconv_t GetToUTF8iconv();
        static bool UseIconv();
        static bool WantToUseIconv();
        /** Get a reference to the array of initialization errors (if any) */
        static const std::vector<std::wstring>& GetErrors();
#endif /* defined(sun) */

        /** The name of the locale that we created. */
        std::string name() const { return m_current.name(); }

    private:
        std::locale m_saved;     //!< Previous locale saved
        bool m_resetAtDestroy;   //!< Should we reset to old locale in destructor
        std::locale m_current;   //!< A copy of the locale we've created
        char *m_SavedCLocale;    //!< Identifies saved locale
#if defined(hpux)
        pid_t m_SavedPid;        //!< Saved pid needed for verification of validity
#endif
#if defined(sun)
        static iconv_t m_FromUTF8iconv; //!< Saved conversion descriptor for UTF-8 to wchar_t conversion
        static iconv_t m_ToUTF8iconv;   //!< Saved conversion descriptor for wchar_t to UTF-8 conversion
        static bool m_UseIconv;         //!< Indicate if we can use iconv for UTF-8/wchar_t conversion
        static bool m_WantToUseIconv;   //!< Indicate if we want to use iconv for UTF-8/wchar_t conversion
        static std::vector<std::wstring> m_Errors; //!< Initialization errors.
        void SetIconv();
#endif /* defined(sun) */
    };
}
   
#endif   /* SCXLOCALE_H */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
