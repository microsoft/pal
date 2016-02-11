/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Atomic operation tests

    \author      Emil Gustafsson <emilg@microsoft.com> 
    \date        2008-01-14 11:08:50

*/
/*----------------------------------------------------------------------------*/
#include <iostream>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxatomic.h> 
#include <scxcorelib/scxthread.h>
#include <testutils/scxunit.h>

using namespace std;

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif

class ThreadAtomicParam : public SCXCoreLib::SCXThreadParam
{
public:
    ThreadAtomicParam( scx_atomic_t* pCounter, const int* pMode, bool* pDecToZero ):
        m_pCounter( pCounter ),
        m_pMode( pMode ),
        m_pDecToZero(pDecToZero)
    {
    }

public:
    scx_atomic_t*   m_pCounter;
    const int*      m_pMode;
    bool*           m_pDecToZero;
};


class SCXAtomicTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXAtomicTest ); 
    CPPUNIT_TEST( TestIncrement );
    CPPUNIT_TEST( TestDecrement );
    CPPUNIT_TEST( TestIncDecPairs );
    CPPUNIT_TEST( TestConcurrency );
    SCXUNIT_TEST_ATTRIBUTE( TestConcurrency, SLOW );
    CPPUNIT_TEST_SUITE_END();

private:
    scx_atomic_t m_zero;
    scx_atomic_t m_one;
    scx_atomic_t m_two;

public:
    void setUp()
    {
        m_zero = 0;
        m_one = 1;
        m_two = 2;
    }

    void TestIncrement()
    {
        CPPUNIT_ASSERT_NO_THROW(scx_atomic_increment(&m_zero));
        CPPUNIT_ASSERT_NO_THROW(scx_atomic_increment(&m_one));
        CPPUNIT_ASSERT_NO_THROW(scx_atomic_increment(&m_two));
    }

    static void AtomicThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        const ThreadAtomicParam* pl = dynamic_cast<const ThreadAtomicParam*>(param.GetData());
        CPPUNIT_ASSERT_MESSAGE("No thread parameters", NULL != pl );

        scx_atomic_t* res = pl->m_pCounter;

        for( int i = 0; i < 10000000; i++ ){
            switch ( *pl->m_pMode){
                case 0: // atomic
                    scx_atomic_increment(res);
                    scx_atomic_decrement_test(res);
                    break;

                case 1: // ++/--
                    (*res)++;
                    (*res)--;
                    break;

            }
        }
        *pl->m_pDecToZero = scx_atomic_decrement_test(res);
    }

    void TestDecrement()
    {
        CPPUNIT_ASSERT_MESSAGE("Decrementing zero should not equal zero", ! scx_atomic_decrement_test(&m_zero));
        CPPUNIT_ASSERT_MESSAGE("Decrementing one should equal zero", scx_atomic_decrement_test(&m_one));
        CPPUNIT_ASSERT_MESSAGE("Decrementing two should not equal zero", ! scx_atomic_decrement_test(&m_two));
    }

    void TestIncDecPairs()
    {
        scx_atomic_increment(&m_zero);
        scx_atomic_increment(&m_zero);
        CPPUNIT_ASSERT_MESSAGE("zero incremented twice and then decremented once should not be zero", ! scx_atomic_decrement_test(&m_zero));
        CPPUNIT_ASSERT_MESSAGE("zero incremented twice and then decremented twice should be zero", scx_atomic_decrement_test(&m_zero));
        
        scx_atomic_increment(&m_one);
        scx_atomic_increment(&m_one);
        CPPUNIT_ASSERT_MESSAGE("one incremented twice and then decremented once should not be zero", ! scx_atomic_decrement_test(&m_one));
        CPPUNIT_ASSERT_MESSAGE("one incremented twice and then decremented twice should not be zero", ! scx_atomic_decrement_test(&m_one));
        CPPUNIT_ASSERT_MESSAGE("one incremented twice and then decremented three times should be zero", scx_atomic_decrement_test(&m_one));

        scx_atomic_increment(&m_two);
        scx_atomic_increment(&m_two);
        CPPUNIT_ASSERT_MESSAGE("two incremented twice and then decremented once should not be zero", ! scx_atomic_decrement_test(&m_two));
        CPPUNIT_ASSERT_MESSAGE("two incremented twice and then decremented twice should not be zero", ! scx_atomic_decrement_test(&m_two));
        CPPUNIT_ASSERT_MESSAGE("two incremented twice and then decremented three times should not be zero", ! scx_atomic_decrement_test(&m_two));
        CPPUNIT_ASSERT_MESSAGE("two incremented twice and then decremented four times should be zero", scx_atomic_decrement_test(&m_two));
    }

    void TestConcurrency()
    {
        const int c_nThreads = 3;
        int     nMode = 0;         // atomic
        bool    aWaitRes[c_nThreads];
        
        scx_atomic_t res(c_nThreads);

        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> aThreads[c_nThreads];

        for ( int i = 0; i < c_nThreads; i++ ){
            aWaitRes[i] = false;
            aThreads[i] = new SCXCoreLib::SCXThread( SCXAtomicTest::AtomicThreadBody,
                new ThreadAtomicParam( &res, &nMode, &aWaitRes[i] ) );
        }

        int nWaitCounter = 0;
        for ( int i = 0; i < c_nThreads; i++ ){
            aThreads[i]->Wait();
            aThreads[i] = 0;
            
            if ( aWaitRes[i] )
               nWaitCounter ++;
        }

        // if atomic works we have to get 0!
        CPPUNIT_ASSERT( res == 0 );

        // we also must get only one "true" from scx_atomic_decrement_test
        CPPUNIT_ASSERT( nWaitCounter == 1 );
        
        // regular ++/-- mode
        nMode = 1;
        res = c_nThreads;
        
        for ( int i = 0; i < c_nThreads; i++ ){
            aWaitRes[i] = false;
            aThreads[i] = new SCXCoreLib::SCXThread( SCXAtomicTest::AtomicThreadBody,
                new ThreadAtomicParam( &res, &nMode, &aWaitRes[i] ) );
        }

        for ( int i = 0; i < c_nThreads; i++ ){
            aThreads[i]->Wait();
            aThreads[i] = 0;
        }

        if ( res == 0 ){
            // With true concurrency it is very unlikely res should be 0, so the recommendation is to run the test
            // at a multi-cpu host. It may or may not happen anyway depending on number of CPU, timing etc
            SCXUNIT_WARNING(L"Regular (unprotected) ++/-- returns 0 (no mismatch); it's recommended to run atomic test on mutliple-CPU machine to get a realistic test environment");
        }
        else {
            wstringstream str(L"");
            str << L"simple ++/-- returns " << (int)res;
            SCXUNIT_LOG_STREAM(str);
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXAtomicTest ); 
