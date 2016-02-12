/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-06-19 11:31:53

    Test singleton template class.
    
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxsingleton.h>
#include <testutils/scxunit.h>
#include <iostream>
#include <sstream>

#if defined(WIN32)
#include <windows.h>
#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
#include <pthread.h>
#include <signal.h>
#else
#error "Not implemented for this plattform"
#endif

extern "C"
{
    typedef void* (*pThreadFn)(void *);
}

class A : public SCXCoreLib::SCXSingleton<A>
{
    friend class SCXCoreLib::SCXSingleton<A>;
public:
    int getI() { return m_i; }
    void setI(int i) { m_i = i; }
    virtual ~A() {}
    virtual std::wstring DumpString()
    {
        return L"Class A";
    }

private:
    int m_i;
    A() : m_i(0) {}
    A(const A&) : SCXCoreLib::SCXSingleton<A>(), m_i(0) {}
    A& operator=(const A&) { m_i = 0; return *this; }
};

template<> SCXCoreLib::SCXHandle<A> SCXCoreLib::SCXSingleton<A>::s_instance (0);
template<> SCXCoreLib::SCXHandle<SCXCoreLib::SCXThreadLockHandle> SCXCoreLib::SCXSingleton<A>::s_lockHandle (
    new SCXCoreLib::SCXThreadLockHandle(SCXCoreLib::ThreadLockHandleGet (true)) );

class B : public SCXCoreLib::SCXSingleton<B>
{
    friend class SCXCoreLib::SCXSingleton<B>;
public:
    virtual ~B() {}
    virtual std::wstring DumpString() const
    {
        return L"Class B";
    }

    // For counting the number of times the constructor is called.
    static int s_constructed;

private:
    B()
    {
        ++s_constructed;
#if defined(WIN32)
        Sleep(1000);
#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix)
        usleep(1000000);
#endif
    }
    B(const B&) : SCXCoreLib::SCXSingleton<B>() {}
    B& operator=(const B&) { return *this; }
};

template<> SCXCoreLib::SCXHandle<B> SCXCoreLib::SCXSingleton<B>::s_instance (0);
template<> SCXCoreLib::SCXHandle<SCXCoreLib::SCXThreadLockHandle> SCXCoreLib::SCXSingleton<B>::s_lockHandle (
    new SCXCoreLib::SCXThreadLockHandle(SCXCoreLib::ThreadLockHandleGet (true)) );

int B::s_constructed = 0;

#include <scxcorelib/scxsingleton-defs.h>

class SCXSingletonTest : public CPPUNIT_NS::TestFixture
{
public:
    CPPUNIT_TEST_SUITE( SCXSingletonTest );
    CPPUNIT_TEST( TestSameInstance );
    SCXUNIT_TEST( TestConstructor, 0 );
    SCXUNIT_TEST_ATTRIBUTE( TestConstructor, SLOW );
    CPPUNIT_TEST_SUITE_END();

private:
#if defined(WIN32)
    typedef HANDLE thread_handle_t;

    static thread_handle_t StartThread(void* fn, void* param, int* id)
    {
        return CreateThread(NULL, 0, static_cast<LPTHREAD_START_ROUTINE>(fn), param, 0, reinterpret_cast<LPDWORD>(id));
    }

#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
    typedef pthread_t thread_handle_t;

    static thread_handle_t StartThread(void*(*fn)(void*), void* param, int* /*id*/)
    {
        thread_handle_t h;
        int r = pthread_create(&h, NULL, (pThreadFn) fn, param);
        if (0 != r)
            return 0;
        return h;
    }
#endif

#if defined(WIN32)
    static int TestThread(void*)
#elif defined(linux) || defined(sun) || defined(hpux) || defined(aix) || defined(macos)
    static void* TestThread(void*)
#endif
    {
        B& b = B::Instance();
        b.DumpString();
        return 0;
    }

public:
    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestSameInstance(void)
    {
        A& a1 = A::Instance();
        a1.setI(1);

        A& a2 = A::Instance();
        CPPUNIT_ASSERT(a1.getI() == 1);
        CPPUNIT_ASSERT(a2.getI() == 1);

        a2.setI(2);
        CPPUNIT_ASSERT(a1.getI() == 2);
        CPPUNIT_ASSERT(a2.getI() == 2);
    }

    void TestConstructor(void)
    {
        StartThread(TestThread, 0, 0);

        B& b = B::Instance();

        CPPUNIT_ASSERT(b.s_constructed == 1);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXSingletonTest );
