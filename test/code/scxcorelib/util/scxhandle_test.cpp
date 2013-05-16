/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2007-08-06 10:22:00

    Tests for reference counting template class.
    
*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxhandle.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxthread.h>

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif


class HandleDtorCounter {
public:
    HandleDtorCounter(int* pCounter):
        m_pCounter( pCounter ){}
    ~HandleDtorCounter(){ (*m_pCounter)++; }

    int* m_pCounter;
};

class SCXHandleTestClass
{
public:
    scxlong m_value;
};

class ThreadHandleParam : public SCXCoreLib::SCXThreadParam
{
public:
    ThreadHandleParam( SCXCoreLib::SCXHandle<HandleDtorCounter> ptr ):
        m_ptr(ptr)
    {
    }

public:
    SCXCoreLib::SCXHandle<HandleDtorCounter>  m_ptr;
    
};


class SCXHandleTest : public CPPUNIT_NS::TestFixture 
{
    CPPUNIT_TEST_SUITE( SCXHandleTest ); 
    CPPUNIT_TEST( TestAllowNullPointers );
    CPPUNIT_TEST( TestValueIsTheSame );
    CPPUNIT_TEST( TestSingleOwnershipPreventsNewOwner );
    CPPUNIT_TEST( TestSingleOwnershipOnSelfOK );
    CPPUNIT_TEST( TestSingleOwnershipAssertsOnStrayPointer );
    CPPUNIT_TEST( TestSingleOwnershipRemovedWhenAssignedNewValue );
    CPPUNIT_TEST( TestSingleOwnershipCopyConstructorDontCopyOwnership );
    CPPUNIT_TEST( TestSingleOwnershipRemovedWhenPointerSet );
    CPPUNIT_TEST( TestComparison );
    CPPUNIT_TEST( TestConcurrency );
    SCXUNIT_TEST_ATTRIBUTE(TestConcurrency, SLOW);
    CPPUNIT_TEST_SUITE_END();

private:
    
    static void HandleThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        const ThreadHandleParam* pl = dynamic_cast<const ThreadHandleParam*>(param.GetData());

        for( int i = 0; i < 1000000; i++ ){
            SCXCoreLib::SCXHandle<HandleDtorCounter> ptr = pl->m_ptr;
            SCXCoreLib::SCXHandle<HandleDtorCounter> ptr2 = pl->m_ptr;
            // PREfix: Ignore errors on next two lines (no issue because caller
            //         insures Handle's m_pCounter==1 when we're called)
            ptr = 0;
            ptr2 = 0;
        }
    }

public:
    void TestValueIsTheSame(void)
    {
        { /* Using simple type pointer */
            SCXCoreLib::SCXHandle<scxlong> h1(new scxlong(42));
            SCXCoreLib::SCXHandle<scxlong> h2(h1);
            SCXCoreLib::SCXHandle<scxlong> h3(new scxlong(4711));

            // Pacify PREfix
            CPPUNIT_ASSERT(NULL != h1);
            CPPUNIT_ASSERT(NULL != h2);
            CPPUNIT_ASSERT(NULL != h3);

            CPPUNIT_ASSERT(42 == *(h1.GetData()));
            CPPUNIT_ASSERT(42 == *(h2.GetData()));
            CPPUNIT_ASSERT(4711 == *(h3.GetData()));
            h2 = h3;
            CPPUNIT_ASSERT(4711 == *(h2.GetData()));
            h2.SetData(new scxlong(17));
            CPPUNIT_ASSERT(17 == *(h2.GetData()));
            CPPUNIT_ASSERT(4711 == *(h3.GetData()));
        }

        { /* Using custom type pointer */
            SCXHandleTestClass* po42 = new SCXHandleTestClass;
            SCXHandleTestClass* po4711 = new SCXHandleTestClass;
            po42->m_value = 42;
            po4711->m_value = 4711;
            
            SCXCoreLib::SCXHandle<SCXHandleTestClass> h1(po42);
            SCXCoreLib::SCXHandle<SCXHandleTestClass> h2(h1);
            SCXCoreLib::SCXHandle<SCXHandleTestClass> h3(po4711);
            CPPUNIT_ASSERT(42 == h1->m_value);
            CPPUNIT_ASSERT(42 == h2->m_value);
            CPPUNIT_ASSERT(4711 == h3->m_value);
            h2 = h3;
            CPPUNIT_ASSERT(4711 == h2->m_value);
            h2->m_value = 17;
            CPPUNIT_ASSERT(17 == h2->m_value);
            CPPUNIT_ASSERT(17 == h3->m_value);
        }
    }

    void TestAllowNullPointers(void)
    {
        SCXUNIT_RESET_ASSERTION();
        {
            SCXCoreLib::SCXHandle<scxlong> h(0);
        }
        SCXUNIT_ASSERTIONS_FAILED(0);
    }

    void TestSingleOwnershipPreventsNewOwner()
    {
        SCXUNIT_RESET_ASSERTION();
        {
            SCXCoreLib::SCXHandle<scxlong> h1(new scxlong(42));
            h1.SetOwner();
            SCXCoreLib::SCXHandle<scxlong> h2 = h1;
            h2.SetOwner();
        }
        SCXUNIT_ASSERTIONS_FAILED(1);
#ifdef NDEBUG
        SCXUNIT_WARNING(L"SCXHandleTest::TestSingleOwnershipPreventsNewOwner cannot verify asserts in release builds");
#endif
    }

    void TestSingleOwnershipOnSelfOK()
    {
        SCXUNIT_RESET_ASSERTION();
        {
            SCXCoreLib::SCXHandle<scxlong> h1(new scxlong(42));
            h1.SetOwner();
            h1.SetOwner();
            SCXCoreLib::SCXHandle<scxlong> h2 = h1;
            h1.SetOwner();
        }
        SCXUNIT_ASSERTIONS_FAILED(0);
#ifdef NDEBUG
        SCXUNIT_WARNING(L"SCXHandleTest::TestSingleOwnershipOnSelfOK cannot verify asserts in release builds");
#endif
    }


    void TestSingleOwnershipAssertsOnStrayPointer()
    {
        SCXUNIT_RESET_ASSERTION();
        {
            SCXCoreLib::SCXHandle<scxlong> h2(0);
            {
                SCXCoreLib::SCXHandle<scxlong> h1(new scxlong(42));
                h1.SetOwner();
                h2 = h1;
            }
        }
        SCXUNIT_ASSERTIONS_FAILED(1);
#ifdef NDEBUG
        SCXUNIT_WARNING(L"SCXHandleTest::TestSingleOwnershipAssertsOnStrayPointer cannot verify asserts in release builds");
#endif
    }

    void TestSingleOwnershipRemovedWhenAssignedNewValue()
    {
        SCXUNIT_RESET_ASSERTION();
        {
            SCXCoreLib::SCXHandle<scxlong> h1(new scxlong(42));
            SCXCoreLib::SCXHandle<scxlong> h2(new scxlong(4711));
            SCXCoreLib::SCXHandle<scxlong> h3(0);
            h1.SetOwner();
            h1 = h2;
            h1.SetOwner(); 
            h1 = h3;           // Should assert since ref count should be 2 and h1 should no longer be "owner"
            h1.SetOwner();
            h3.SetOwner();     // Should assert since ref count should be 2 and h1 should no longer be "owner"
        }
        SCXUNIT_ASSERTIONS_FAILED(2);
#ifdef NDEBUG
        SCXUNIT_WARNING(L"SCXHandleTest::TestSingleOwnershipRemovedWhenAssignedNewValue cannot verify asserts in release builds");
#endif
    }

    void TestSingleOwnershipCopyConstructorDontCopyOwnership()
    {
        SCXUNIT_RESET_ASSERTION();
        {
            SCXCoreLib::SCXHandle<scxlong> h1(new scxlong(42));
            h1.SetOwner();
            SCXCoreLib::SCXHandle<scxlong> h2(h1);
            h2.SetOwner(); // Should assert since h2 should not be owner.
        }
        SCXUNIT_ASSERTIONS_FAILED(1);
#ifdef NDEBUG
        SCXUNIT_WARNING(L"SCXHandleTest::TestSingleOwnershipCopyConstructorDontCopyOwnership cannot verify asserts in release builds");
#endif
    }

    void TestSingleOwnershipRemovedWhenPointerSet()
    {
        SCXUNIT_RESET_ASSERTION();
        {
            SCXCoreLib::SCXHandle<scxlong> h1(new scxlong(42));
            h1.SetOwner();
            h1.SetData(new scxlong(4711));
            SCXCoreLib::SCXHandle<scxlong> h2(h1);
            h1.SetOwner(); 
        }
        SCXUNIT_ASSERTIONS_FAILED(0); // Should not assert since since h2 destroyed first
#ifdef NDEBUG
        SCXUNIT_WARNING(L"SCXHandleTest::TestSingleOwnershipRemovedWhenPointerSet cannot verify asserts in release builds");
#endif
    }

    void TestComparison()
    {
        int* p1 = new int(17);
        int* p2 = new int(19);

        SCXCoreLib::SCXHandle<int> ptr1(p1);
        SCXCoreLib::SCXHandle<int> ptr2(0);

        // compare with NULL:
        CPPUNIT_ASSERT( NULL == ptr2 );
        CPPUNIT_ASSERT( NULL != ptr1 );
        CPPUNIT_ASSERT( ptr2 == NULL );
        CPPUNIT_ASSERT( ptr1 != NULL );

        // pointer comparision
        CPPUNIT_ASSERT( p2 != ptr2 );
        CPPUNIT_ASSERT( ptr2 != p2 );
        CPPUNIT_ASSERT( p1 == ptr1 );
        CPPUNIT_ASSERT( ptr1 == p1 );
        CPPUNIT_ASSERT( ptr1 != ptr2 );

        // assign and re-compare
        // Note: p2 is now delete'd via ptr2's destruction
        ptr2 = p2;
        // pointer comparision
        CPPUNIT_ASSERT( p2 == ptr2 );
        CPPUNIT_ASSERT( ptr2 == p2 );

        CPPUNIT_ASSERT( *ptr1 == 17 );
        CPPUNIT_ASSERT( *ptr2 == 19 );

        // compare content
        int* p1_1 = new int(17);

        CPPUNIT_ASSERT( p1_1 != ptr1 );    // all pointers are different (p1, p2, p1_1)
        CPPUNIT_ASSERT( ptr1 != p1_1 );
        CPPUNIT_ASSERT( p1_1 != ptr2 );
        CPPUNIT_ASSERT( ptr2 != p1_1 );
        CPPUNIT_ASSERT( *p1_1 == *ptr1 );  // values are the same - 17
        CPPUNIT_ASSERT( *p1_1 != *ptr2 );

        delete p1_1;    // have to delete this one manually
    }

    
    void TestConcurrency()
    {
        const int c_nThreads = 3;
        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> aThreads[c_nThreads];
        int nDCounter = 0;
        SCXCoreLib::SCXHandle<HandleDtorCounter>  ptr( new HandleDtorCounter(&nDCounter) );

        for ( int i = 0; i < c_nThreads; i++ ){
            aThreads[i] = new SCXCoreLib::SCXThread( SCXHandleTest::HandleThreadBody,
                new ThreadHandleParam( ptr ) );
        }

        for ( int i = 0; i < c_nThreads; i++ ){
            aThreads[i]->Wait();
            aThreads[i] = 0;
        }

        // object still must exist with exactly one reference
        CPPUNIT_ASSERT_EQUAL( 0, nDCounter );
        
        ptr = 0;
        // ref-counter must be 0. We may already crashed by double-delete 
        // or it can be way much than 0 if handle is not thread-safe
        CPPUNIT_ASSERT_EQUAL( 1, nDCounter );
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXHandleTest ); 
