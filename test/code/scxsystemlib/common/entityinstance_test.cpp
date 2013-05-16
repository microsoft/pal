/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
    
  Created date    2007-08-22 11:55:09

  Test class for SCXKstat
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>
#include <iostream>

#include <string>
#include <vector>

#include <math.h>

#include <scxcorelib/scxmath.h>
#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/entityenumeration.h>

using namespace std;
using namespace SCXSystemLib;
using namespace SCXCoreLib; 

class TestInst : public EntityInstance {
public:
    int  m_nonce;
    
    TestInst( const wstring& id ) : EntityInstance( id, false ), m_nonce(-1){}

    virtual void            Update() {
        // instance with id '2' throws
        if ( GetId() == L"2" ){
            throw SCXNotSupportedException( L"instance 2", SCXSRCLOCATION );
        }

        m_nonce = c_Nonce; // get nonce in synch
    }

    // used to check 'update-id' - marker that update was invoked
    static int c_Nonce;
};

class TestEnum : public EntityEnumeration<TestInst> {
public:
    virtual void Init(){
        // create 3 instances
        SCXCoreLib::SCXHandle<TestInst> inst( new TestInst(L"1" ) );
        AddInstance( inst );
        AddInstance( SCXCoreLib::SCXHandle<TestInst>(new TestInst(L"2" )) );
        AddInstance( SCXCoreLib::SCXHandle<TestInst>(new TestInst(L"3" )) );
    }

};

int TestInst::c_Nonce = 0;

class SCXEntityInstanceTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXEntityInstanceTest );
    CPPUNIT_TEST( testUpdateInstanceThrows );
    CPPUNIT_TEST_SUITE_END();

private:

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void testUpdateInstanceThrows()
    {
        SCXCoreLib::SCXLogHandle log = SCXCoreLib::SCXLogHandleFactory::Instance().GetLogHandle(L"scx.core.common.pal.system.common");
        SCX_LOGINFO(log, L"This test raises exceptions; the following two log messages are normal");

        // if one instance from many throws exeption, update should not be interrupted
        TestEnum tstEnum;

        tstEnum.Init();

        // UpdateInstance has to deal with exceptions
        CPPUNIT_ASSERT_NO_THROW( tstEnum.UpdateInstances() );

        TestInst::c_Nonce++;
        
        // UpdateInstance has to deal with exceptions
        CPPUNIT_ASSERT_NO_THROW( tstEnum.UpdateInstances() );

        for ( TestEnum::EntityIterator it = tstEnum.Begin(); it != tstEnum.End(); it++ ){
            SCXCoreLib::SCXHandle<TestInst> inst = *it;

            // note: instance with id '2' throws exception and is not updated;
            // others must be updated
            if ( inst->GetId() == L"2" ){
                CPPUNIT_ASSERT ( inst->m_nonce != TestInst::c_Nonce);
                
                // verify that exception-caught flag is set
                CPPUNIT_ASSERT ( inst->IsUnexpectedExceptionSet() );
                //wcout << inst->GetUnexpectedExceptionText() << endl;
                
            } else {
                CPPUNIT_ASSERT ( inst->m_nonce == TestInst::c_Nonce);

                // verify that exception-caught flag is not set
                CPPUNIT_ASSERT ( !inst->IsUnexpectedExceptionSet() );
            }
        }

    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXEntityInstanceTest );



