/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2014-06-13 12:20:00

    unique_ptr class unit tests.
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>

#include "unique_ptr.h"

using namespace util;

namespace
{

//
// Test support code
//

enum Result
{
    SUCCESS = 0,
    DELETER_FAILED = 1 << 0,
    CTOR_FAILED = 1 << 1,
    OPERATOR_BOOL_FAILED = 1 << 2,
    RELEASE_FAILED = 1 << 3,
    SWAP_FAILED = 1 << 4,
    MOVE_FAILED = 1 << 5,
    MOVE_OBJ_FAILED = 1 << 6,
    MOVE_CTOR_FAILED = 1 << 7,
    SPLAT_OP_FAILED = 1 << 8,
    ARROW_OP_FAILED = 1 << 9,
    INDEX_OP_FAILED = 1 << 10,
    REL_OP_FAILED = 1 << 11
};

typedef unsigned int result_type;

class TestObj
{
public:
    static size_t s_count;

    /*ctor*/ TestObj () { ++s_count; }
    virtual /*dtor*/ ~TestObj () { --s_count; }
};


class DerivedTestObj : public TestObj
{
public:
    virtual /*dtor*/ ~DerivedTestObj () {}
};


class ValueObj
{
public:
    /*ctor*/ ValueObj (int val) : m_Val (val) {}

    int m_Val;
};


struct TestObjDeleter
{
    static size_t s_execution_count;

    void operator () (TestObj*& pObj)
    {
        ++s_execution_count;
        if (0 != pObj)
        {
            delete pObj;
            pObj = 0;
        }
    }
};

struct TestObjArrDeleter
{
    static size_t s_execution_count;

    void operator () (TestObj*& pObjArr)
    {
        ++s_execution_count;
        if (0 != pObjArr)
        {
            delete[] pObjArr;
            pObjArr = 0;
        }
    }
};


size_t TestObj::s_count = 0;
size_t TestObjDeleter::s_execution_count = 0;
size_t TestObjArrDeleter::s_execution_count = 0;
size_t s_testObjDel_execution_count = 0;
size_t s_testObjArrDel_execution_count = 0;

size_t const ARRAY_SIZE = 10;
void * const nullptr_t = 0;


struct NoOpDeleter
{
    template<typename T>
    void operator () (T*&) {}
};


void testObjDeleteFn (TestObj*& pObj)
{
    ++s_testObjDel_execution_count;
    if (0 != pObj)
    {
        delete pObj;
        pObj = 0;
    }
}

void testObjArrDeleteFn (TestObj*& pObjArr)
{
    ++s_testObjArrDel_execution_count;
    if (0 != pObjArr)
    {
        delete[] pObjArr;
        pObjArr = 0;
    }
}

template<typename T, typename D>
typename unique_ptr<T, D>::move_type
returnMoveType ()
{
    unique_ptr<T, D> ptr (new typename unique_ptr<T, D>::element_type);
    return ptr.move ();
}

template<typename T, typename D>
typename unique_ptr<T, D>::move_type
returnMoveTypeArray ()
{
    unique_ptr<T, D> ptr (new typename unique_ptr<T, D>::element_type[ARRAY_SIZE]);
    return ptr.move ();
}

template<typename T, typename D>
result_type passMoveType (
    typename unique_ptr<T, D>::move_type moveObj,
    size_t const expectedCount,
    typename unique_ptr<T, D>::pointer const pExpected)
{
    unique_ptr<T, D> pObj1 (moveObj);
    unique_ptr<T, D> pObj2 (moveObj);
    return (expectedCount == TestObj::s_count &&
            pExpected == pObj1.get () &&
            0 == pObj2.get ()) ?
                SUCCESS : MOVE_OBJ_FAILED;
}


}

//
// Unit tests follow
//

class Unique_Ptr_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( Unique_Ptr_Test );

    CPPUNIT_TEST( TestDefaultDelete );
    CPPUNIT_TEST( TestDefaultDeleteArray );
    CPPUNIT_TEST( TestEmptyConstructor );
    CPPUNIT_TEST( TestEmptyConstructorArray );
    CPPUNIT_TEST( TestBasicValueCtor_and_Dtor );
    CPPUNIT_TEST( TestBasicValueCtor_and_Dtor_Array );
    CPPUNIT_TEST( TestCompleteValueCtor_and_Dtor );
    CPPUNIT_TEST( TestCompleteValueCtor_and_Dtor_Array );
    CPPUNIT_TEST( TestBool );
    CPPUNIT_TEST( TestBoolArray );
    CPPUNIT_TEST( TestRelease );
    CPPUNIT_TEST( TestReleaseArray );
    CPPUNIT_TEST( TestReset );
    CPPUNIT_TEST( TestResetArray );
    CPPUNIT_TEST( TestSwap );
    CPPUNIT_TEST( TestSwapArray );
    CPPUNIT_TEST( TestMove );
    CPPUNIT_TEST( TestMoveArray );
    CPPUNIT_TEST( TestUniquePtrMove );
    CPPUNIT_TEST( TestUniquePtrMoveArray );
    CPPUNIT_TEST( TestMoveConstructor );
    CPPUNIT_TEST( TestMoveConstructorArray );
    CPPUNIT_TEST( TestOperator_Pointer ); // test operator *
    CPPUNIT_TEST( TestOperator_Deref ); // test operator ->
    CPPUNIT_TEST( TestOperator_Array ); // test operator []
    CPPUNIT_TEST( TestOperator_Equals ); // test operator ==
    CPPUNIT_TEST( TestOperator_NotEqual ); // test operator !=
    CPPUNIT_TEST( TestOperator_LessThan ); // test operator <
    CPPUNIT_TEST( TestOperator_GreaterThan ); // test operator >
    CPPUNIT_TEST( TestOperator_LessThanOrEqual ); // test operator <=
    CPPUNIT_TEST( TestOperator_GreaterThanOrEqual ); // test operator >=

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestDefaultDelete()
    {
        result_type result = SUCCESS;

        {
            int* pInt = 0;
            default_delete<int> intDeleter;
            intDeleter (pInt);
            if (0 != pInt)
            {
                result |= DELETER_FAILED;
            }
            pInt = new int;
            intDeleter (pInt);
            if (0 != pInt)
            {
                result |= DELETER_FAILED;
            }
            TestObj* pTestObj = 0;
            default_delete<TestObj> testObjDeleter;
            testObjDeleter (pTestObj);
            if (0 != pTestObj)
            {
                result |= DELETER_FAILED;
            }
            pTestObj = new TestObj;
            size_t count = TestObj::s_count;
            testObjDeleter (pTestObj);
            if (0 != pTestObj ||
                (count - 1) != TestObj::s_count)
            {
                result |= DELETER_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestDefaultDeleteArray()
    {
        result_type result = SUCCESS;
        {
            int* pInt = 0;
            default_delete<int[]> intDeleter;
            intDeleter (pInt);
            if (0 != pInt)
            {
                result |= DELETER_FAILED;
            }
            pInt = new int[ARRAY_SIZE];
            intDeleter (pInt);
            if (0 != pInt)
            {
                result |= DELETER_FAILED;
            }
            TestObj* pTestObj = 0;
            default_delete<TestObj[]> testObjDeleter;
            testObjDeleter (pTestObj);
            if (0 != pTestObj)
            {
                result |= DELETER_FAILED;
            }
            pTestObj = new TestObj[ARRAY_SIZE];
            size_t count = TestObj::s_count;
            testObjDeleter (pTestObj);
            if (0 != pTestObj ||
                (count - ARRAY_SIZE) != TestObj::s_count)
            {
                result |= DELETER_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestEmptyConstructor()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        {
            unique_ptr<int> pInt;
            if (0 != pInt.get ())
            {
                result = CTOR_FAILED;
            }
            unique_ptr<TestObj> pObj;
            if (0 != pObj.get () ||
                count != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
        }
        if (count != TestObj::s_count)
        {
            result = CTOR_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestEmptyConstructorArray()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        {
            unique_ptr<int[]> pInt;
            if (0 != pInt.get ())
            {
                result = CTOR_FAILED;
            }
            unique_ptr<TestObj[]> pObj;
            if (0 != pObj.get () ||
                count != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
        }
        if (count != TestObj::s_count)
        {
            result = CTOR_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestBasicValueCtor_and_Dtor()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        {
            unique_ptr<int> ptrInt1 (0);
            if (0 != ptrInt1.get ())
            {
                result = CTOR_FAILED;
            }
            int* const pInt2 = new int;
            unique_ptr<int> ptrInt2 (pInt2);
            if (pInt2 != ptrInt2.get ())
            {
                result = CTOR_FAILED;
            }
            unique_ptr<TestObj> ptrObj1 (0);
            if (0 != ptrObj1.get () ||
                (count) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }

            TestObj* const pObj2 = new TestObj;
            unique_ptr<TestObj> ptrObj2 (pObj2);
            if (pObj2 != ptrObj2.get () ||
                (count + 1) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            DerivedTestObj* const pObj3 = new DerivedTestObj;
            unique_ptr<TestObj> ptrObj3 (pObj3);
            if (pObj3 != ptrObj3.get () ||
                (count + 2) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
        }
        if (count != TestObj::s_count)
        {
            result = CTOR_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestBasicValueCtor_and_Dtor_Array()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        {
            unique_ptr<int[]> ptrInt1 (0);
            if (0 != ptrInt1.get ())
            {
                result = CTOR_FAILED;
            }
            int* const pInt2 = new int[ARRAY_SIZE];
            unique_ptr<int[]> ptrInt2 (pInt2);
            if (pInt2 != ptrInt2.get ())
            {
                result = CTOR_FAILED;
            }
            unique_ptr<TestObj[]> ptrObj1 (0);
            if (0 != ptrObj1.get () ||
                (count) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObj* const pObj2 = new TestObj[ARRAY_SIZE];
            unique_ptr<TestObj[]> ptrObj2 (pObj2);
            if (pObj2 != ptrObj2.get () ||
                (count + ARRAY_SIZE) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
        }
        if (count != TestObj::s_count)
        {
            result = CTOR_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }


    void TestCompleteValueCtor_and_Dtor()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        size_t totalDeleterCount = TestObjDeleter::s_execution_count;
        size_t totalDeleteFnCount = s_testObjDel_execution_count;
        {
            unique_ptr<TestObj, TestObjDeleter> ptrObj1 (0);
            if (0 != ptrObj1.get () ||
                (count) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObj* const pObj2 = new TestObj;
            unique_ptr<TestObj, TestObjDeleter> ptrObj2 (pObj2);
            if (pObj2 != ptrObj2.get () ||
                (count + 1) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObjDeleter deleter;
            unique_ptr<TestObj, TestObjDeleter> ptrObj3 (0, deleter);
            if (0 != ptrObj3.get () ||
                (count + 1) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObj* const pObj4 = new TestObj;
            unique_ptr<TestObj, TestObjDeleter> ptrObj4 (pObj4, deleter);
            if (pObj4 != ptrObj4.get () ||
                (count + 2) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            typedef void (*DeleteFn_t)(TestObj*&);
            unique_ptr<TestObj, DeleteFn_t> ptrObj5 (0, testObjDeleteFn);
            if (0 != ptrObj5.get () ||
                (count + 2) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObj* const pObj6 = new TestObj;
            unique_ptr<TestObj, DeleteFn_t> ptrObj6 (pObj6, testObjDeleteFn);
            if (pObj6 != ptrObj6.get () ||
                (count + 3) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
        }
        if ((4 + totalDeleterCount) != TestObjDeleter::s_execution_count ||
            (2 + totalDeleteFnCount) != s_testObjDel_execution_count ||
            count != TestObj::s_count)
        {
            result = CTOR_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestCompleteValueCtor_and_Dtor_Array()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        size_t totalDeleterCount = TestObjArrDeleter::s_execution_count;
        size_t totalDeleteFnCount = s_testObjArrDel_execution_count;
        {
            unique_ptr<TestObj[], TestObjArrDeleter> ptrObj1 (0);
            if (0 != ptrObj1.get () ||
                (count) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObj* const pObj2 = new TestObj[ARRAY_SIZE];
            unique_ptr<TestObj[], TestObjArrDeleter> ptrObj2 (pObj2);
            if (pObj2 != ptrObj2.get () ||
                (count + ARRAY_SIZE) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObjArrDeleter deleter;
            unique_ptr<TestObj[], TestObjArrDeleter> ptrObj3 (0, deleter);
            if (0 != ptrObj3.get () ||
                (count + ARRAY_SIZE) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObj* const pObj4 = new TestObj[ARRAY_SIZE];
            unique_ptr<TestObj[], TestObjArrDeleter> ptrObj4 (pObj4, deleter);
            if (pObj4 != ptrObj4.get () ||
                (count + 2 * ARRAY_SIZE) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            typedef void (*DeleteFn_t)(TestObj*&);
            unique_ptr<TestObj[], DeleteFn_t> ptrObj5 (0, testObjArrDeleteFn);
            if (0 != ptrObj5.get () ||
                (count + 2 * ARRAY_SIZE) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
            TestObj* const pObj6 = new TestObj[ARRAY_SIZE];
            unique_ptr<TestObj[], DeleteFn_t> ptrObj6 (pObj6, testObjArrDeleteFn);
            if (pObj6 != ptrObj6.get () ||
                (count + 3 * ARRAY_SIZE) != TestObj::s_count)
            {
                result = CTOR_FAILED;
            }
        }
        if ((4 + totalDeleterCount) != TestObjArrDeleter::s_execution_count ||
            (2 + totalDeleteFnCount) != s_testObjArrDel_execution_count ||
            count != TestObj::s_count)
        {
            result = CTOR_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestBool()
    {
        result_type result = SUCCESS;
        {
            unique_ptr<int> pInt1;
            if (pInt1)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
            unique_ptr<int> pInt2 (new int);
            if (!pInt2)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
            unique_ptr<TestObj> pObj1;
            if (pObj1)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
            unique_ptr<TestObj> pObj2 (new TestObj);
            if (!pObj2)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestBoolArray()
    {
        result_type result = SUCCESS;
        {
            unique_ptr<int[]> pInt1;
            if (pInt1)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
            unique_ptr<int[]> pInt2 (new int[ARRAY_SIZE]);
            if (!pInt2)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
            unique_ptr<TestObj[]> pObj1;
            if (pObj1)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
            unique_ptr<TestObj[]> pObj2 (new TestObj[ARRAY_SIZE]);
            if (!pObj2)
            {
                result |= OPERATOR_BOOL_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestRelease()
    {
        result_type result = SUCCESS;
        unique_ptr<int> ptrInt1;
        if (0 != ptrInt1.release () ||
            0 != ptrInt1.get ())
        {
            result |= RELEASE_FAILED;
        }
        int* pIntMaster = new int;
        try
        {
            unique_ptr<int> ptrInt2 (pIntMaster);
            if (pIntMaster != ptrInt2.release () ||
                0 != ptrInt2.get ())
            {
                result |= RELEASE_FAILED;
            }
        }
        catch (...)
        {
            // ignore
        }
        delete pIntMaster;
        unique_ptr<TestObj> ptrObj1;
        if (0 != ptrObj1.release () ||
            0 != ptrObj1.get ())
        {
            result |= RELEASE_FAILED;
        }
        TestObj* pObjMaster = new TestObj;
        try
        {
            unique_ptr<TestObj> ptrObj2 (pObjMaster);
            if (pObjMaster != ptrObj2.release () ||
                0 != ptrObj2.get ())
            {
                result |= RELEASE_FAILED;
            }
        }
        catch (...)
        {
            // ignore
        }
        delete pObjMaster;

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestReleaseArray()
    {
        result_type result = SUCCESS;
        unique_ptr<int[]> ptrInt1;
        if (0 != ptrInt1.release () ||
            0 != ptrInt1.get ())
        {
            result |= RELEASE_FAILED;
        }
        int* pIntMaster = new int[ARRAY_SIZE];
        try
        {
            unique_ptr<int[]> ptrInt2 (pIntMaster);
            if (pIntMaster != ptrInt2.release () ||
                0 != ptrInt2.get ())
            {
                result |= RELEASE_FAILED;
            }
        }
        catch (...)
        {
            // ignore
        }
        delete[] pIntMaster;
        unique_ptr<TestObj[]> ptrObj1;
        if (0 != ptrObj1.release () ||
            0 != ptrObj1.get ())
        {
            result |= RELEASE_FAILED;
        }
        TestObj* pObjMaster = new TestObj[ARRAY_SIZE];
        try
        {
            unique_ptr<TestObj[]> ptrObj2 (pObjMaster);
            if (pObjMaster != ptrObj2.release () ||
                0 != ptrObj2.get ())
            {
                result |= RELEASE_FAILED;
            }
        }
        catch (...)
        {
            // ignore
        }
        delete[] pObjMaster;

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestReset()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        // reset nothing to nothing
        unique_ptr<TestObj> ptrObj1;
        ptrObj1.reset ();
        if (0 != ptrObj1.get () ||
            count != TestObj::s_count)
        {
            result |= RELEASE_FAILED;
        }
        TestObj* pObj1 = new TestObj;
        TestObj* pObj2 = new TestObj;
        try
        {
            // reset nothing to something
            unique_ptr<TestObj> ptrObj2;
            ptrObj2.reset (pObj1);
            if (pObj1 != ptrObj2.get () ||
                (2 + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            ptrObj2.release ();
            // reset something to nothing
            unique_ptr<TestObj> ptrObj3 (pObj1);
            ptrObj3.reset (0);
            if (0 != ptrObj3.get () ||
                (1 + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            pObj1 = new TestObj;
            // reset something to something else
            unique_ptr<TestObj> ptrObj4 (pObj1);
            ptrObj4.reset (pObj2);
            if (pObj2 != ptrObj4.get () ||
                (1 + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            pObj1 = 0;
            ptrObj4.release ();
            // reset something to the same thing
            unique_ptr<TestObj> ptrObj5 (pObj2);
            ptrObj5.reset (pObj2);
            if (pObj2 != ptrObj5.get () ||
                (1 + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            ptrObj5.release ();
            // reset nothing to nothing
            unique_ptr<TestObj> ptrObj6;
            ptrObj6.reset (0);
        }
        catch (...)
        {
            // ignore
        }
        delete pObj2;
        delete pObj1;

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestResetArray()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        // reset nothing to nothing
        unique_ptr<TestObj[]> ptrObj1;
        ptrObj1.reset ();
        if (0 != ptrObj1.get () ||
            count != TestObj::s_count)
        {
            result |= RELEASE_FAILED;
        }
        TestObj* pObj1 = new TestObj[ARRAY_SIZE];
        TestObj* pObj2 = new TestObj[ARRAY_SIZE];
        try
        {
            // reset nothing to something
            unique_ptr<TestObj[]> ptrObj2;
            ptrObj2.reset (pObj1);
            if (pObj1 != ptrObj2.get () ||
                (2 * ARRAY_SIZE + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            ptrObj2.release ();
            // reset something to nothing
            unique_ptr<TestObj[]> ptrObj3 (pObj1);
            ptrObj3.reset (0);
            if (0 != ptrObj3.get () ||
                (1 * ARRAY_SIZE + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            pObj1 = new TestObj[ARRAY_SIZE];
            // reset something to something else
            unique_ptr<TestObj[]> ptrObj4 (pObj1);
            ptrObj4.reset (pObj2);
            if (pObj2 != ptrObj4.get () ||
                (1 * ARRAY_SIZE + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            pObj1 = 0;
            ptrObj4.release ();
            // reset something to the same thing
            unique_ptr<TestObj[]> ptrObj5 (pObj2);
            ptrObj5.reset (pObj2);
            if (pObj2 != ptrObj5.get () ||
                (1 * ARRAY_SIZE + count) != TestObj::s_count)
            {
                result |= RELEASE_FAILED;
            }
            ptrObj5.release ();
            // reset nothing to nothing
            unique_ptr<TestObj[]> ptrObj6;
            ptrObj6.reset (0);
        }
        catch (...)
        {
            // ignore
        }
        delete[] pObj2;
        delete[] pObj1;

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestSwap()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        TestObj* pObj1 = 0;
        TestObj* pObj2 = 0;
        try
        {
            // swap nothing for nothing
            unique_ptr<TestObj> ptrObj1;
            unique_ptr<TestObj> ptrObj2;
            ptrObj1.swap (ptrObj2);
            if (0 != ptrObj1.get () ||
                0 != ptrObj2.get ())
            {
                result |= SWAP_FAILED;
            }
            // swap nothing for something
            pObj1 = new TestObj;
            ptrObj1.reset (pObj1);
            ptrObj2.swap (ptrObj1);
            if (0 != ptrObj1.get () ||
                pObj1 != ptrObj2.get () ||
                (1 + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            // swap something for nothing
            ptrObj2.swap (ptrObj1);
            if (pObj1 != ptrObj1.get () ||
                0 != ptrObj2.get () ||
                (1 + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            // swap something for something else
            pObj2 = new TestObj;
            ptrObj2.reset (pObj2);
            ptrObj1.swap (ptrObj2);
            if (pObj2 != ptrObj1.get () ||
                pObj1 != ptrObj2.get () ||
                (2 + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            // swap something for the same thing
            std::swap (ptrObj1, ptrObj1);
            if (pObj2 != ptrObj1.get () ||
                (2 + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            ptrObj2.release ();
            ptrObj1.release ();
        }
        catch (...)
        {
            // ignore
        }
        delete pObj2;
        delete pObj1;

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestSwapArray()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        TestObj* pObj1 = 0;
        TestObj* pObj2 = 0;
        try
        {
            // swap nothing for nothing
            unique_ptr<TestObj[]> ptrObj1;
            unique_ptr<TestObj[]> ptrObj2;
            ptrObj1.swap (ptrObj2);
            if (0 != ptrObj1.get () ||
                0 != ptrObj2.get ())
            {
                result |= SWAP_FAILED;
            }
            // swap nothing for something
            pObj1 = new TestObj[ARRAY_SIZE];
            ptrObj1.reset (pObj1);
            ptrObj2.swap (ptrObj1);
            if (0 != ptrObj1.get () ||
                pObj1 != ptrObj2.get () ||
                (1 * (ARRAY_SIZE) + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            // swap something for nothing
            ptrObj2.swap (ptrObj1);
            if (pObj1 != ptrObj1.get () ||
                0 != ptrObj2.get () ||
                (1 * (ARRAY_SIZE) + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            // swap something for something else
            pObj2 = new TestObj[ARRAY_SIZE];
            ptrObj2.reset (pObj2);
            ptrObj1.swap (ptrObj2);
            if (pObj2 != ptrObj1.get () ||
                pObj1 != ptrObj2.get () ||
                (2 * (ARRAY_SIZE) + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            // swap something for the same thing
            std::swap (ptrObj1, ptrObj1);
            if (pObj2 != ptrObj1.get () ||
                (2 * (ARRAY_SIZE) + count) != TestObj::s_count)
            {
                result |= SWAP_FAILED;
            }
            ptrObj2.release ();
            ptrObj1.release ();
        }
        catch (...)
        {
            // ignore
        }
        delete[] pObj2;
        delete[] pObj1;

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestMove()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        {
            typedef unique_ptr<TestObj> TestObjPtr_t;
            TestObjPtr_t ptrObj1;
            TestObjPtr_t::move_type moveObj1 (ptrObj1.move ());
            TestObjPtr_t ptrObj2 (moveObj1);
            if (0 != ptrObj1.get () ||
                count != TestObj::s_count ||
                0 != ptrObj2.get ())
            {
                result |= MOVE_FAILED;
            }
            TestObjPtr_t ptrObj3 (new TestObj);
            TestObj const* const pObj3 = ptrObj3.get ();
            TestObjPtr_t::move_type moveObj3 (ptrObj3.move ());
            TestObjPtr_t ptrObj4 (moveObj3);
            TestObjPtr_t ptrObj5 (moveObj3);
            if (0 != ptrObj3.get () ||
                (1 + count) != TestObj::s_count ||
                pObj3 != ptrObj4.get () ||
                0 != ptrObj5.get ())
            {
                result |= MOVE_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestMoveArray()
    {
        result_type result = SUCCESS;
        size_t const count = TestObj::s_count;
        {
            typedef unique_ptr<TestObj[]> TestObjPtr_t;
            TestObjPtr_t ptrObj1;
            TestObjPtr_t::move_type moveObj1 (ptrObj1.move ());
            TestObjPtr_t ptrObj2 (moveObj1);
            if (0 != ptrObj1.get () ||
                count != TestObj::s_count ||
                0 != ptrObj2.get ())
            {
                result |= MOVE_FAILED;
            }
            TestObjPtr_t ptrObj3 (new TestObj[ARRAY_SIZE]);
            TestObj const* const pObj3 = ptrObj3.get ();
            TestObjPtr_t::move_type moveObj3 (ptrObj3.move ());
            TestObjPtr_t ptrObj4 (moveObj3);
            TestObjPtr_t ptrObj5 (moveObj3);
            if (0 != ptrObj3.get () ||
                (1 * ARRAY_SIZE + count) != TestObj::s_count ||
                pObj3 != ptrObj4.get () ||
                0 != ptrObj5.get ())
            {
                result |= MOVE_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestUniquePtrMove()
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj> TestObjPtr_t;
        size_t const count = TestObj::s_count;
        {
            // test empty move object
            {
                TestObjPtr_t ptrObj;
                TestObjPtr_t::move_type moveObj (ptrObj.move ());
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test move object
            {
                TestObjPtr_t ptrObj1 (new TestObj);
                TestObjPtr_t::pointer const pObj1 = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj1 (ptrObj1.move ());
                TestObjPtr_t ptrObj2 (moveObj1);
                if (pObj1 != ptrObj2.get () ||
                    (1 + count) != TestObj::s_count)
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test move object copy constructor
            {
                TestObjPtr_t ptrObj1 (new TestObj);
                TestObj* pObj1 = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj1 (ptrObj1.move ());
                TestObjPtr_t::move_type moveObj2 (moveObj1);
                TestObjPtr_t ptrObj2 (moveObj1);
                TestObjPtr_t ptrObj3 (moveObj2);
                TestObjPtr_t ptrObj4 (moveObj2);
                if ((1 + count) != TestObj::s_count ||
                    0 != ptrObj1.get () ||
                    0 != ptrObj2.get () ||
                    pObj1 != ptrObj3.get () ||
                    0 != ptrObj4.get ())
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test move object returned but not used
            {
                returnMoveType<TestObjPtr_t::element_type,
                TestObjPtr_t::deleter_type> ();
                if (count != TestObj::s_count)
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            // test move object returned and used
            {
                TestObjPtr_t::move_type moveObj (
                    returnMoveType<TestObjPtr_t::element_type,
                    TestObjPtr_t::deleter_type> ());
                if ((1 + count) != TestObj::s_count)
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test pass empty move object to function
            {
                TestObjPtr_t ptrObj;
                result |= passMoveType<
                    TestObjPtr_t::element_type,
                    TestObjPtr_t::deleter_type>(ptrObj.move (), count, 0);
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test pass move object to function
            {
                TestObjPtr_t ptrObj (new TestObj);
                TestObjPtr_t::pointer pObj = ptrObj.get ();
                result |= passMoveType<TestObjPtr_t::element_type,
                TestObjPtr_t::deleter_type>(ptrObj.move (),
                                            1 + count, pObj);
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestUniquePtrMoveArray()
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj[]> TestObjPtr_t;
        size_t const count = TestObj::s_count;
        {
            // test empty move object
            {
                TestObjPtr_t ptrObj;
                TestObjPtr_t::move_type moveObj (ptrObj.move ());
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test move object
            {
                TestObjPtr_t ptrObj1 (new TestObj[ARRAY_SIZE]);
                TestObjPtr_t::pointer const pObj1 = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj1 (ptrObj1.move ());
                TestObjPtr_t ptrObj2 (moveObj1);
                TestObjPtr_t ptrObj3 (moveObj1);
                if (0 != ptrObj1.get () ||
                    pObj1 != ptrObj2.get () ||
                    0 != ptrObj3.get () ||
                    (1 * ARRAY_SIZE + count) != TestObj::s_count)
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test move object copy constructor
            {
                TestObjPtr_t ptrObj1 (new TestObj[ARRAY_SIZE]);
                TestObj const* const pObj1 = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj1 (ptrObj1.move ());
                TestObjPtr_t::move_type moveObj2 (moveObj1);
                TestObjPtr_t ptrObj2 (moveObj1);
                TestObjPtr_t ptrObj3 (moveObj2);
                TestObjPtr_t ptrObj4 (moveObj2);
                if (0 != ptrObj1.get () ||
                    0 != ptrObj2.get () ||
                    pObj1 != ptrObj3.get () ||
                    0 != ptrObj4.get () ||
                    (1 * ARRAY_SIZE + count) != TestObj::s_count)
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test move object returned but not used
            {
                returnMoveTypeArray<TestObjPtr_t::element_type,
                TestObjPtr_t::deleter_type> ();
                if (count != TestObj::s_count)
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            // test move object returned and used
            {
                TestObjPtr_t::move_type moveObj (
                    returnMoveTypeArray<TestObjPtr_t::element_type,
                    TestObjPtr_t::deleter_type> ());
                if ((1 * ARRAY_SIZE + count) != TestObj::s_count)
                {
                    result |= MOVE_OBJ_FAILED;
                }
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test pass empty move object to function
            {
                TestObjPtr_t ptrObj;
                result |= passMoveType<TestObjPtr_t::element_type,
                TestObjPtr_t::deleter_type>(ptrObj.move (),
                                            count, 0);
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
            // test pass move object to function
            {
                TestObjPtr_t ptrObj (new TestObj[ARRAY_SIZE]);
                TestObjPtr_t::pointer pObj = ptrObj.get ();
                result |=
                passMoveType<TestObjPtr_t::element_type,
                TestObjPtr_t::deleter_type>(ptrObj.move (),
                                            1 * ARRAY_SIZE + count,
                                            pObj);
            }
            if (count != TestObj::s_count)
            {
                result |= MOVE_OBJ_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestMoveConstructor()
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj> TestObjPtr_t;
        size_t const count = TestObj::s_count;
        {
            // test empty move object
            {
                TestObjPtr_t ptrObj1;
                TestObjPtr_t::pointer const pObj = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj = ptrObj1.move ();
                {
                    TestObjPtr_t ptrObj2 (moveObj);
                    TestObjPtr_t ptrObj3 (moveObj);
                    if (0 != ptrObj1.get () ||
                        pObj != ptrObj2.get () ||
                        0 != ptrObj3.get () ||                        
                        count != TestObj::s_count)
                    {
                        result |= MOVE_CTOR_FAILED;
                    }
                }
                if (0 != ptrObj1.get () ||
                    count != TestObj::s_count)
                {
                    result |= MOVE_CTOR_FAILED;
                }
            }
            // test move object
            {
                TestObjPtr_t ptrObj1 (new TestObj);
                TestObjPtr_t::pointer const pObj = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj = ptrObj1.move ();
                {
                    TestObjPtr_t ptrObj2 (moveObj);
                    TestObjPtr_t ptrObj3 (moveObj);
                    if (0 != ptrObj1.get () ||
                        pObj != ptrObj2.get () ||
                        0 != ptrObj3.get () ||
                        (1 + count) != TestObj::s_count)
                    {
                        result |= MOVE_CTOR_FAILED;
                    }
                }
                if (0 != ptrObj1.get () ||
                    count != TestObj::s_count)
                {
                    result |= MOVE_CTOR_FAILED;
                }
            }
            // test move ctor return from function
            {
                {
                    TestObjPtr_t ptrObj (
                        returnMoveType<TestObjPtr_t::element_type,
                        TestObjPtr_t::deleter_type> ());
                    if (0 == ptrObj.get () ||
                        (1 + count) != TestObj::s_count)
                    {
                        result |= MOVE_CTOR_FAILED;
                    }
                }
                if (count != TestObj::s_count)
                {
                    result |= MOVE_CTOR_FAILED;
                }
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestMoveConstructorArray()
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj[]> TestObjPtr_t;
        size_t const count = TestObj::s_count;
        {
            // test empty move object
            {
                TestObjPtr_t ptrObj1;
                TestObjPtr_t::pointer const pObj = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj = ptrObj1.move ();
                {
                    TestObjPtr_t ptrObj2 (moveObj);
                    TestObjPtr_t ptrObj3 (moveObj);
                    if (0 != ptrObj1.get () ||
                        pObj != ptrObj2.get () ||
                        0 != ptrObj3.get () ||
                        count != TestObj::s_count)
                    {
                        result |= MOVE_CTOR_FAILED;
                    }
                }
                if (0 != ptrObj1.get () ||
                    count != TestObj::s_count)
                {
                    result |= MOVE_CTOR_FAILED;
                }
            }
            // test move object
            {
                TestObjPtr_t ptrObj1 (new TestObj[ARRAY_SIZE]);
                TestObjPtr_t::pointer const pObj = ptrObj1.get ();
                TestObjPtr_t::move_type moveObj = ptrObj1.move ();
                {
                    TestObjPtr_t ptrObj2 (moveObj);
                    TestObjPtr_t ptrObj3 (moveObj);
                    if (0 != ptrObj1.get () ||
                        pObj != ptrObj2.get () ||
                        0 != ptrObj3.get () ||
                        (1 * ARRAY_SIZE + count) != TestObj::s_count)
                    {
                        result |= MOVE_CTOR_FAILED;
                    }
                }
                if (0 != ptrObj1.get () ||
                    count != TestObj::s_count)
                {
                    result |= MOVE_CTOR_FAILED;
                }
            }
            // test move ctor return from function
            {
                {
                    TestObjPtr_t ptrObj (
                        returnMoveTypeArray<TestObjPtr_t::element_type,
                        TestObjPtr_t::deleter_type> ());
                    if (0 == ptrObj.get () ||
                        ((1 * ARRAY_SIZE) + count) != TestObj::s_count)
                    {
                        result |= MOVE_CTOR_FAILED;
                    }
                }
                if (count != TestObj::s_count)
                {
                    result |= MOVE_CTOR_FAILED;
                }
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_Pointer() // test operator *
    {
        result_type result = SUCCESS;
        unique_ptr<int> ptrInt;
        if (0 != &(*ptrInt))
        {
            result |= SPLAT_OP_FAILED;
        }
        ptrInt.reset (new int (1));
        *ptrInt = 2;
        if (0 == &(*ptrInt) ||
            ptrInt.get () != &(*ptrInt) ||
            2 != *ptrInt)
        {
            result |= SPLAT_OP_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_Deref() // test operator ->
    {
        result_type result = SUCCESS;
        unique_ptr<ValueObj> ptrInt (new ValueObj (1));
        ptrInt->m_Val = 2;
        if (0 == ptrInt.operator -> () ||
            2 != ptrInt->m_Val ||
            2 != (ptrInt.get ())->m_Val)
        {
            result |= ARROW_OP_FAILED;
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_Array() // test operator []
    {
        result_type result = SUCCESS;
        unique_ptr<size_t[]> ptrInt (new size_t[ARRAY_SIZE]);
        for (size_t i = 0; i < ARRAY_SIZE; ++i)
        {
            ptrInt[i] = i;
        }
        for (size_t i = 0; i < ARRAY_SIZE; ++i)
        {
            if (i != ptrInt[i] ||
                i != *(ptrInt.get () + i))
            {
                result |= INDEX_OP_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_Equals() // test operator ==
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj, NoOpDeleter> TestObjPtr_t;
        typedef unique_ptr<DerivedTestObj, NoOpDeleter> DerivedObjPtr_t;
        unique_ptr<TestObj[]> sourceArray (new TestObj[ARRAY_SIZE]);
        unique_ptr<DerivedTestObj> derivedObj (new DerivedTestObj);
        // bool less = derivedObj.get () < sourceArray.get ();

        // compare nothing to nothing
        {
            TestObjPtr_t ptrObj1;
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived;
            if (!(0 == ptrObj1) ||
                !(ptrObj1 == 0) ||
                !(ptrObj1 == ptrObj2) ||
                !(ptrDerived == ptrObj1) ||
                !(ptrObj1 == ptrDerived))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to nothing
        {
            TestObjPtr_t ptrObj1(sourceArray.get ());
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived1 (derivedObj.get ());
            DerivedObjPtr_t ptrDerived2;
            if (sourceArray.get () == ptrObj2 ||
                ptrObj2 == sourceArray.get () ||
                ptrObj1 == ptrObj2 ||
                ptrObj2 == ptrObj1 ||
                ptrDerived2 == ptrObj1 ||
                ptrObj1 == ptrDerived2 ||
                ptrDerived1 == ptrObj2 ||
                ptrObj2 == ptrDerived1 ||
                ptrDerived1.get () == ptrObj2 ||
                ptrObj2 == ptrDerived1.get ())
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to the same thing
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get ());
            TestObjPtr_t ptrObj3 (derivedObj.get ());
            DerivedObjPtr_t ptrDerived (derivedObj.get ());
            if (!(sourceArray.get () == ptrObj2) ||
                !(ptrObj2 == sourceArray.get ()) ||
                !(ptrObj1 == ptrObj2) ||
                !(ptrObj2 == ptrObj1) ||
                !(ptrDerived == ptrObj3) ||
                !(ptrObj3 == ptrDerived) ||
                !(ptrDerived.get () == ptrObj3) ||
                !(ptrObj3 == ptrDerived.get ()))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to something else
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get () + 1);
            TestObjPtr_t ptrObj3 (derivedObj.get ());
            unique_ptr<DerivedTestObj> ptrDerived (new DerivedTestObj ());
            if (sourceArray.get () == ptrObj2 ||
                ptrObj2 == sourceArray.get () ||
                ptrObj1 == ptrObj2 ||
                ptrObj2 == ptrObj1 ||
                ptrDerived == ptrObj3 ||
                ptrObj3 == ptrDerived ||
                ptrDerived.get () == ptrObj3 ||
                ptrObj3 == ptrDerived.get () ||
                ptrDerived == ptrObj2 ||
                ptrObj2 == ptrDerived ||
                ptrDerived.get () == ptrObj2 ||
                ptrObj2 == ptrDerived.get ())
            {
                result |= REL_OP_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_NotEqual() // test operator !=
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj, NoOpDeleter> TestObjPtr_t;
        typedef unique_ptr<DerivedTestObj, NoOpDeleter> DerivedObjPtr_t;
        unique_ptr<TestObj[]> sourceArray (new TestObj[ARRAY_SIZE]);
        unique_ptr<DerivedTestObj> derivedObj (new DerivedTestObj);

        // compare nothing to nothing
        {
            TestObjPtr_t ptrObj1;
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived;
            if (0 != ptrObj1 ||
                ptrObj1 != 0 ||
                ptrObj1 != ptrObj2 ||
                ptrDerived != ptrObj1 ||
                ptrObj1 != ptrDerived)
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to nothing
        {
            TestObjPtr_t ptrObj1(sourceArray.get ());
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived1 (derivedObj.get ());
            DerivedObjPtr_t ptrDerived2;
            if (!(sourceArray.get () != ptrObj2) ||
                !(ptrObj2 != sourceArray.get ()) ||
                !(ptrObj1 != ptrObj2) ||
                !(ptrObj2 != ptrObj1) ||
                !(ptrDerived2 != ptrObj1) ||
                !(ptrObj1 != ptrDerived2) ||
                !(ptrDerived1 != ptrObj2) ||
                !(ptrObj2 != ptrDerived1) ||
                !(ptrDerived1.get () != ptrObj2) ||
                !(ptrObj2 != ptrDerived1.get ()))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to the same thing
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get ());
            TestObjPtr_t ptrObj3 (derivedObj.get ());
            DerivedObjPtr_t ptrDerived (derivedObj.get ());
            if (sourceArray.get () != ptrObj2 ||
                ptrObj2 != sourceArray.get () ||
                ptrObj1 != ptrObj2 ||
                ptrObj2 != ptrObj1 ||
                ptrDerived != ptrObj3 ||
                ptrObj3 != ptrDerived ||
                ptrDerived.get () != ptrObj3 ||
                ptrObj3 != ptrDerived.get ())
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to something else
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get () + 1);
            TestObjPtr_t ptrObj3 (derivedObj.get ());
            unique_ptr<DerivedTestObj> ptrDerived (new DerivedTestObj ());
            if (!(sourceArray.get () != ptrObj2) ||
                !(ptrObj2 != sourceArray.get ()) ||
                !(ptrObj1 != ptrObj2) ||
                !(ptrObj2 != ptrObj1) ||
                !(ptrDerived != ptrObj3) ||
                !(ptrObj3 != ptrDerived) ||
                !(ptrDerived.get () != ptrObj3) ||
                !(ptrObj3 != ptrDerived.get ()) ||
                !(ptrDerived != ptrObj2) ||
                !(ptrObj2 != ptrDerived) ||
                !(ptrDerived.get () != ptrObj2) ||
                !(ptrObj2 != ptrDerived.get ()))
            {
                result |= REL_OP_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_LessThan() // test operator <
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj, NoOpDeleter> TestObjPtr_t;
        typedef unique_ptr<DerivedTestObj, NoOpDeleter> DerivedObjPtr_t;
        unique_ptr<TestObj[]> sourceArray (new TestObj[ARRAY_SIZE]);
        unique_ptr<DerivedTestObj[]> derivedArray (new DerivedTestObj[ARRAY_SIZE]);
        bool less = derivedArray.get () < sourceArray.get ();

        // compare nothing to nothing
        {
            TestObjPtr_t ptrObj1;
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived;
            if (nullptr_t < ptrObj1 ||
                ptrObj1 < nullptr_t ||
                ptrObj1 < ptrObj2 ||
                ptrDerived < ptrObj1 ||
                ptrObj1 < ptrDerived)
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to nothing
        {
            TestObjPtr_t ptrObj1(sourceArray.get ());
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived1 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived2;
            if (sourceArray.get () < ptrObj2 ||
                !(ptrObj2 < sourceArray.get ()) ||
                ptrObj1 < ptrObj2 ||
                !(ptrObj2 < ptrObj1) ||
                !(ptrDerived2 < ptrObj1) ||
                ptrObj1 < ptrDerived2 ||
                ptrDerived1 < ptrObj2 ||
                !(ptrObj2 < ptrDerived1) ||
                ptrDerived1.get () < ptrObj2 ||
                !(ptrObj2 < ptrDerived1.get ()))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to the same thing
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get ());
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get ());
            if (sourceArray.get () < ptrObj2 ||
                ptrObj2 < sourceArray.get () ||
                ptrObj1 < ptrObj2 ||
                ptrObj2 < ptrObj1 ||
                ptrDerived < ptrObj3 ||
                ptrObj3 < ptrDerived ||
                ptrDerived.get () < ptrObj3 ||
                ptrObj3 < ptrDerived.get ())
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to something else
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get () + 1);
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get () + 1);
            if (!(sourceArray.get () < ptrObj2) ||
                ptrObj2 < sourceArray.get () ||
                !(ptrObj1 < ptrObj2) ||
                ptrObj2 < ptrObj1 ||
                ptrDerived < ptrObj3 ||
                !(ptrObj3 < ptrDerived) ||
                ptrDerived.get () < ptrObj3 ||
                !(ptrObj3 < ptrDerived.get ()) ||
                (less != (ptrDerived < ptrObj1)) ||
                (less == (ptrObj1 < ptrDerived)) ||
                (less != (ptrDerived.get () < ptrObj2)) ||
                (less == (ptrObj2 < ptrDerived.get ())))
            {
                result |= REL_OP_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_GreaterThan() // test operator >
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj, NoOpDeleter> TestObjPtr_t;
        typedef unique_ptr<DerivedTestObj, NoOpDeleter> DerivedObjPtr_t;
        unique_ptr<TestObj[]> sourceArray (new TestObj[ARRAY_SIZE]);
        unique_ptr<DerivedTestObj[]> derivedArray (new DerivedTestObj[ARRAY_SIZE]);
        bool less = derivedArray.get () < sourceArray.get ();

        // compare nothing to nothing
        {
            TestObjPtr_t ptrObj1;
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived;
            if (nullptr_t > ptrObj1 ||
                ptrObj1 > nullptr_t ||
                ptrObj1 > ptrObj2 ||
                ptrDerived > ptrObj1 ||
                ptrObj1 > ptrDerived)
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to nothing
        {
            TestObjPtr_t ptrObj1(sourceArray.get ());
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived1 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived2;
            if (!(sourceArray.get () > ptrObj2) ||
                ptrObj2 > sourceArray.get () ||
                !(ptrObj1 > ptrObj2) ||
                ptrObj2 > ptrObj1 ||
                ptrDerived2 > ptrObj1 ||
                !(ptrObj1 > ptrDerived2) ||
                !(ptrDerived1 > ptrObj2) ||
                ptrObj2 > ptrDerived1 ||
                !(ptrDerived1.get () > ptrObj2) ||
                ptrObj2 > ptrDerived1.get ())
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to the same thing
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get ());
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get ());
            if (sourceArray.get () > ptrObj2 ||
                ptrObj2 > sourceArray.get () ||
                ptrObj1 > ptrObj2 ||
                ptrObj2 > ptrObj1 ||
                ptrDerived > ptrObj3 ||
                ptrObj3 > ptrDerived ||
                ptrDerived.get () > ptrObj3 ||
                ptrObj3 > ptrDerived.get ())
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to something else
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get () + 1);
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get () + 1);
            if (sourceArray.get () > ptrObj2 ||
                !(ptrObj2 > sourceArray.get ()) ||
                ptrObj1 > ptrObj2 ||
                !(ptrObj2 > ptrObj1) ||
                !(ptrDerived > ptrObj3) ||
                ptrObj3 > ptrDerived ||
                !(ptrDerived.get () > ptrObj3) ||
                ptrObj3 > ptrDerived.get () ||
                (less == (ptrDerived > ptrObj1)) ||
                (less != (ptrObj1 > ptrDerived)) ||
                (less == (ptrDerived.get () > ptrObj2)) ||
                (less != (ptrObj2 > ptrDerived.get ())))
            {
                result |= REL_OP_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_LessThanOrEqual() // test operator <=
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj, NoOpDeleter> TestObjPtr_t;
        typedef unique_ptr<DerivedTestObj, NoOpDeleter> DerivedObjPtr_t;
        unique_ptr<TestObj[]> sourceArray (new TestObj[ARRAY_SIZE]);
        unique_ptr<DerivedTestObj[]> derivedArray (new DerivedTestObj[ARRAY_SIZE]);
        bool less = derivedArray.get () < sourceArray.get ();

        // compare nothing to nothing
        {
            TestObjPtr_t ptrObj1;
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived;
            if (!(nullptr_t <= ptrObj1) ||
                !(ptrObj1 <= nullptr_t) ||
                !(ptrObj1 <= ptrObj2) ||
                !(ptrDerived <= ptrObj1) ||
                !(ptrObj1 <= ptrDerived))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to nothing
        {
            TestObjPtr_t ptrObj1(sourceArray.get ());
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived1 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived2;
            if (sourceArray.get () <= ptrObj2 ||
                !(ptrObj2 <= sourceArray.get ()) ||
                ptrObj1 <= ptrObj2 ||
                !(ptrObj2 <= ptrObj1) ||
                !(ptrDerived2 <= ptrObj1) ||
                ptrObj1 <= ptrDerived2 ||
                ptrDerived1 <= ptrObj2 ||
                !(ptrObj2 <= ptrDerived1) ||
                ptrDerived1.get () <= ptrObj2 ||
                !(ptrObj2 <= ptrDerived1.get ()))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to the same thing
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get ());
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get ());
            if (!(sourceArray.get () <= ptrObj2) ||
                !(ptrObj2 <= sourceArray.get ()) ||
                !(ptrObj1 <= ptrObj2) ||
                !(ptrObj2 <= ptrObj1) ||
                !(ptrDerived <= ptrObj3) ||
                !(ptrObj3 <= ptrDerived) ||
                !(ptrDerived.get () <= ptrObj3) ||
                !(ptrObj3 <= ptrDerived.get ()))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to something else
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get () + 1);
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get () + 1);
            if (!(sourceArray.get () <= ptrObj2) ||
                ptrObj2 <= sourceArray.get () ||
                !(ptrObj1 <= ptrObj2) ||
                ptrObj2 <= ptrObj1 ||
                ptrDerived <= ptrObj3 ||
                !(ptrObj3 <= ptrDerived) ||
                ptrDerived.get () <= ptrObj3 ||
                !(ptrObj3 <= ptrDerived.get ()) ||
                (less != (ptrDerived <= ptrObj1)) ||
                (less == (ptrObj1 <= ptrDerived)) ||
                (less != (ptrDerived.get () <= ptrObj2)) ||
                (less == (ptrObj2 <= ptrDerived.get ())))
            {
                result |= REL_OP_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }

    void TestOperator_GreaterThanOrEqual() // test operator >=
    {
        result_type result = SUCCESS;
        typedef unique_ptr<TestObj, NoOpDeleter> TestObjPtr_t;
        typedef unique_ptr<DerivedTestObj, NoOpDeleter> DerivedObjPtr_t;
        unique_ptr<TestObj[]> sourceArray (new TestObj[ARRAY_SIZE]);
        unique_ptr<DerivedTestObj[]> derivedArray (new DerivedTestObj[ARRAY_SIZE]);
        bool less = derivedArray.get () < sourceArray.get ();

        // compare nothing to nothing
        {
            TestObjPtr_t ptrObj1;
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived;
            if (!(nullptr_t >= ptrObj1) ||
                !(ptrObj1 >= nullptr_t) ||
                !(ptrObj1 >= ptrObj2) ||
                !(ptrDerived >= ptrObj1) ||
                !(ptrObj1 >= ptrDerived))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to nothing
        {
            TestObjPtr_t ptrObj1(sourceArray.get ());
            TestObjPtr_t ptrObj2;
            DerivedObjPtr_t ptrDerived1 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived2;
            if (!(sourceArray.get () >= ptrObj2) ||
                ptrObj2 >= sourceArray.get () ||
                !(ptrObj1 >= ptrObj2) ||
                ptrObj2 >= ptrObj1 ||
                ptrDerived2 >= ptrObj1 ||
                !(ptrObj1 >= ptrDerived2) ||
                !(ptrDerived1 >= ptrObj2) ||
                ptrObj2 >= ptrDerived1 ||
                !(ptrDerived1.get () >= ptrObj2) ||
                ptrObj2 >= ptrDerived1.get ())
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to the same thing
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get ());
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get ());
            if (!(sourceArray.get () >= ptrObj2) ||
                !(ptrObj2 >= sourceArray.get ()) ||
                !(ptrObj1 >= ptrObj2) ||
                !(ptrObj2 >= ptrObj1) ||
                !(ptrDerived >= ptrObj3) ||
                !(ptrObj3 >= ptrDerived) ||
                !(ptrDerived.get () >= ptrObj3) ||
                !(ptrObj3 >= ptrDerived.get ()))
            {
                result |= REL_OP_FAILED;
            }
        }

        // compare something to something else
        {
            TestObjPtr_t ptrObj1 (sourceArray.get ());
            TestObjPtr_t ptrObj2 (sourceArray.get () + 1);
            TestObjPtr_t ptrObj3 (derivedArray.get ());
            DerivedObjPtr_t ptrDerived (derivedArray.get () + 1);
            if (sourceArray.get () >= ptrObj2 ||
                !(ptrObj2 >= sourceArray.get ()) ||
                ptrObj1 >= ptrObj2 ||
                !(ptrObj2 >= ptrObj1) ||
                !(ptrDerived >= ptrObj3) ||
                ptrObj3 >= ptrDerived ||
                !(ptrDerived.get () >= ptrObj3) ||
                ptrObj3 >= ptrDerived.get () ||
                (less == (ptrDerived >= ptrObj1)) ||
                (less != (ptrObj1 >= ptrDerived)) ||
                (less == (ptrDerived.get () >= ptrObj2)) ||
                (less != (ptrObj2 >= ptrDerived.get ())))
            {
                result |= REL_OP_FAILED;
            }
        }

        CPPUNIT_ASSERT_EQUAL(static_cast<result_type>(SUCCESS), result);
    }
};


CPPUNIT_TEST_SUITE_REGISTRATION( Unique_Ptr_Test );
