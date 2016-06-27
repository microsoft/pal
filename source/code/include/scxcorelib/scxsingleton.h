/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Contains the definition of the Singleton Template.

    \date        2007-06-19 08:50:28

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXSINGLETON_H
#define SCXSINGLETON_H

#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxhandle.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Provides singleton functionality.

        \date        <2007-06-19 08:50:28>

        Use this class when a singleton is needed.

        To make class A a singleton, let it inherit from SCXSingleton<A>.
        Make Singleton<A> a friend of A, and make the constructors private.

        \ex
        \code
        class A : public SCXSingleton<A>
        {
            friend class SCXSingleton<A>;
                public:
                    std::wstring DumpString() const;
        private:
            A();
        };
        \endcode

        In header file, you'll need to include something like:

        template<> SCXHandle<class> SCXSingleton<class>::s_instance;
        template<> SCXHandle<SCXThreadLockHandle> SCXSingleton<class>::s_lockHandle;
    */
    template<class T> class SCXSingleton
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Returns a reference to the singleton instance.

            If the instance is not yet created, it first creates it.

        */
        static T& Instance();

    protected:
        /** Only instantiated by subclassing. */
        SCXSingleton(int recursiveLock = 1);

    private:
        /** Only instantiated by subclassing. */
        SCXSingleton(const SCXSingleton&);
        /** Only copied by subclassing. */
        SCXSingleton& operator=(const SCXSingleton&);

        static SCXCoreLib::SCXHandle<T> s_instance; //!< Handle to the singleton instance.
        static SCXCoreLib::SCXHandle< SCXThreadLockHandle > s_lockHandle; //!< Used for locking at creation time.
    };
}

// Define some macros to make management of Singletons easier

#define SCXSingleton_Define(SName) \
    template<> SCXCoreLib::SCXHandle<SName> SCXCoreLib::SCXSingleton<SName>::s_instance; \
    template<> SCXCoreLib::SCXHandle<SCXCoreLib::SCXThreadLockHandle> SCXCoreLib::SCXSingleton<SName>::s_lockHandle

#define SCXSingleton_Allocate(SName) \
    template<> SCXCoreLib::SCXHandle<SName> SCXCoreLib::SCXSingleton<SName>::s_instance (0); \
    template<> SCXCoreLib::SCXHandle<SCXCoreLib::SCXThreadLockHandle> SCXCoreLib::SCXSingleton<SName>::s_lockHandle ( \
        new SCXCoreLib::SCXThreadLockHandle(SCXCoreLib::ThreadLockHandleGet (true)) )

#endif /* SCXSINGLETON_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
