/*---------------------------------------------------------------------------------------
rep
   Copyright (c) Microsoft Corporation.  All right reserved.

   Created date    2015-06-10  09:54:00

   Net Route test class.

   This class tests the linus implementation

   It checks the results of Route information.
*/

/*---------------------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/nxnetrouteenumeration.h>
#include <scxsystemlib/nxnetrouteinstance.h>
#include <scxsystemlib/nxnetroutedependencies.h>
#include <fstream>

#include <testutils/scxunit.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxip.h>

using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;

static const wstring s_LogModuleName = L"scx.core.common.pal.system.netroute.nxnetrouteenumeration";
static const wstring testFilePath = L"/tmp/route";

class NxNetRouteTestDependencies : public NxNetRouteDependencies
{

public:
    NxNetRouteTestDependencies()
    { 
        SetPathToFile(testFilePath); 
    }

    void Init()
    {
        SCX_LOGTRACE(m_log, L"NxNetRouteTestDependencies Init()");
 
        vDependencyInjection.push_back(wstring(L"eth3\t1273AB31\tA8EAFFFF\t0003\t0\t0\t2\t05178000\tx\ty\tz"));
        vDependencyInjection.push_back(wstring(L"eth0\t00803B98\t00000000\t0001\t0\t0\t1\t00FCFFFF\t0\t0\t0"));
    }
   
};

class NxNetRoute_Test: public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(NxNetRoute_Test);

    CPPUNIT_TEST(testDependenciesNotNull);
    CPPUNIT_TEST(testEnumerationNotNull);
    CPPUNIT_TEST(testInstanceNotNull);
    CPPUNIT_TEST(testNxNetRouteEnumerationGetSize);  
    CPPUNIT_TEST(testReadingFromFile);
    CPPUNIT_TEST(testIfaceIsLoopBack);
    CPPUNIT_TEST(testIfaceValidEth);
    CPPUNIT_TEST(testIfaceInvalidEth);
    CPPUNIT_TEST(testNxNetRouteAddOneNetRouteInstance);
    CPPUNIT_TEST(testParseLines);
    CPPUNIT_TEST(testNonRequiredParameters);

    CPPUNIT_TEST_SUITE_END();

private:
    SCXHandle<NxNetRouteEnumeration> m_netrouteenum;
    void deleteTestFile();
        
public:
    void setUp(void);
    void tearDown(void);

    void testNxNetRouteEnumerationGetSize();
    void testDependenciesNotNull();
    void testEnumerationNotNull();
    void testInstanceNotNull();   
    void testReadingFromFile();
    void testIfaceIsLoopBack();
    void testIfaceValidEth();
    void testIfaceInvalidEth();
    void testNxNetRouteAddOneNetRouteInstance();
    void testParseLines();
    void testNonRequiredParameters();
};

void NxNetRoute_Test::setUp(void)
{
    m_netrouteenum = NULL;
    deleteTestFile();
}

void NxNetRoute_Test::tearDown(void)
{
    if (NULL != m_netrouteenum)
    {
        m_netrouteenum->CleanUp();
        m_netrouteenum = 0;
    }
    deleteTestFile();
}

void NxNetRoute_Test::deleteTestFile()
{
    SCXFileInfo testFile(testFilePath);
    if (testFile.PathExists()) {
        SCXFilePath path(testFilePath);
        SCXFile::Delete(path);
    }
}

void NxNetRoute_Test::testNxNetRouteEnumerationGetSize()
{
   SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteTestDependencies());
   deps->Init();
   m_netrouteenum = new NxNetRouteEnumeration(deps);
   m_netrouteenum->Update(true);  
   
   CPPUNIT_ASSERT(m_netrouteenum->Size() == 2);
}

void NxNetRoute_Test::testNxNetRouteAddOneNetRouteInstance()
{
    m_netrouteenum = new NxNetRouteEnumeration();
    SCXHandle<NxNetRouteInstance> instance(new NxNetRouteInstance());
    m_netrouteenum->AddNetRouteInstance(instance);
   
    CPPUNIT_ASSERT(1 == m_netrouteenum->Size());
}


void NxNetRoute_Test::testParseLines()
{
    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteTestDependencies());
    deps->Init();

    m_netrouteenum = new NxNetRouteEnumeration(deps);

    // dependency injection lines will be read when true is passed in
    m_netrouteenum->Update(true);    

    SCXHandle<NxNetRouteInstance>instance0(new NxNetRouteInstance());
    instance0 = m_netrouteenum->GetInstance(0);

    CPPUNIT_ASSERT_EQUAL(instance0->GetInterface() , L"eth3");
#if defined(linux)
    CPPUNIT_ASSERT_EQUAL(instance0->GetDestination() , L"18.115.171.49");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGateway() , L"168.234.255.255");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGenMask() , L"5.23.128.0");
#elif defined(aix) || defined(sun) || defined(hpux)
    CPPUNIT_ASSERT_EQUAL(instance0->GetDestination() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGateway() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGenMask() , L"REQUEST_NOT_AVAILABLE");
#endif
    CPPUNIT_ASSERT_EQUAL(instance0->GetFlags() , L"0003");
    CPPUNIT_ASSERT_EQUAL(instance0->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance0->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMetric() , L"2");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMtu(), L"x");
    CPPUNIT_ASSERT_EQUAL(instance0->GetWindow() ,L"y");
    CPPUNIT_ASSERT_EQUAL(instance0->GetIrtt() , L"z");

    SCXHandle<NxNetRouteInstance>instance1(new NxNetRouteInstance());
    instance1 = m_netrouteenum->GetInstance(1);

    CPPUNIT_ASSERT_EQUAL(instance1->GetInterface() , L"eth0");
#if defined(linux)
    CPPUNIT_ASSERT_EQUAL(instance1->GetDestination() , L"0.128.59.152");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGenMask() , L"0.252.255.255");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGateway() , L"0.0.0.0");
#elif defined(aix) || defined(sun) || defined(hpux)
    CPPUNIT_ASSERT_EQUAL(instance0->GetDestination() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGateway() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGenMask() , L"REQUEST_NOT_AVAILABLE");
#endif
    CPPUNIT_ASSERT_EQUAL(instance1->GetFlags() , L"0001");
    CPPUNIT_ASSERT_EQUAL(instance1->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetMetric() , L"1");
    CPPUNIT_ASSERT_EQUAL(instance1->GetWindow() ,L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetIrtt() , L"0");
}

void NxNetRoute_Test::testReadingFromFile()
{
    vector<wstring> lines;
    lines.push_back(L"Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask\t\tMTU\tWindow\tIRTT                                                       ");
    lines.push_back(L"eth9\t1273AB31\tA8EAFFFF\t0003\t0\t0\t2\t05178000\tx\ty\tz                                                                               ");
    lines.push_back(L"eth8\t00803B98\t00000000\t0001\t0\t0\t1\t00FCFFFF\t0\t0\t0                                                                               ");

    SCXFile::WriteAllLines(testFilePath, lines, std::ios_base::out);

    SCXFileInfo testFile(testFilePath);
    CPPUNIT_ASSERT( testFile.PathExists() ); 

    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteTestDependencies());
    m_netrouteenum = new NxNetRouteEnumeration(deps);
    m_netrouteenum->Update();  
    CPPUNIT_ASSERT(m_netrouteenum->Size() == 2);

    SCXHandle<NxNetRouteInstance>instance0(new NxNetRouteInstance());
    instance0 = m_netrouteenum->GetInstance(0);

    CPPUNIT_ASSERT_EQUAL(instance0->GetInterface() , L"eth9");
#if defined(linux)
    CPPUNIT_ASSERT_EQUAL(instance0->GetDestination() , L"18.115.171.49");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGateway() , L"168.234.255.255");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGenMask() , L"5.23.128.0");
#elif defined(sun) || defined(aix) || defined(hpux)
    CPPUNIT_ASSERT_EQUAL(instance0->GetDestination() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGateway() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGenMask() , L"REQUEST_NOT_AVAILABLE");
#endif
    CPPUNIT_ASSERT_EQUAL(instance0->GetFlags() , L"0003");
    CPPUNIT_ASSERT_EQUAL(instance0->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance0->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMetric() , L"2");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMtu(), L"x");
    CPPUNIT_ASSERT_EQUAL(instance0->GetWindow() ,L"y");
    CPPUNIT_ASSERT_EQUAL(instance0->GetIrtt() , L"z");


    SCXHandle<NxNetRouteInstance>instance1(new NxNetRouteInstance());
    instance1 = m_netrouteenum->GetInstance(1);

    CPPUNIT_ASSERT_EQUAL(instance1->GetInterface() , L"eth8");
#if defined(linux)
    CPPUNIT_ASSERT_EQUAL(instance1->GetDestination() , L"0.128.59.152");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGenMask() , L"0.252.255.255");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGateway() , L"0.0.0.0");
#elif defined(aix) || defined(sun) || defined(hpux)
    CPPUNIT_ASSERT_EQUAL(instance0->GetDestination() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGateway() , L"REQUEST_NOT_AVAILABLE");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGenMask() , L"REQUEST_NOT_AVAILABLE");
#endif
    CPPUNIT_ASSERT_EQUAL(instance1->GetFlags() , L"0001");
    CPPUNIT_ASSERT_EQUAL(instance1->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetMetric() , L"1");
    CPPUNIT_ASSERT_EQUAL(instance1->GetWindow() ,L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetIrtt() , L"0");
}

void NxNetRoute_Test::testDependenciesNotNull()
{
    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteDependencies());
    CPPUNIT_ASSERT(deps != NULL);
}

void NxNetRoute_Test::testEnumerationNotNull()
{
    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteDependencies());
    m_netrouteenum = new NxNetRouteEnumeration(deps);
    CPPUNIT_ASSERT(m_netrouteenum != NULL);
}

void NxNetRoute_Test::testInstanceNotNull()
{
    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteDependencies());
    SCXCoreLib::SCXHandle<NxNetRouteInstance> inst(new NxNetRouteInstance(deps));
    CPPUNIT_ASSERT(inst != NULL);
}

void NxNetRoute_Test::testIfaceIsLoopBack()
{
    SCXHandle<NxNetRouteTestDependencies>deps(new NxNetRouteTestDependencies());
    m_netrouteenum = new NxNetRouteEnumeration(deps);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"lo"), true);
}


void NxNetRoute_Test::testIfaceValidEth()
{
    SCXHandle<NxNetRouteTestDependencies>deps(new NxNetRouteTestDependencies());
    m_netrouteenum = new NxNetRouteEnumeration(deps);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"eth0"), true);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"eth10"), true);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"eth99"), true);
}


void NxNetRoute_Test::testIfaceInvalidEth()
{
    SCXHandle<NxNetRouteTestDependencies>deps(new NxNetRouteTestDependencies());
    m_netrouteenum = new NxNetRouteEnumeration(deps);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"ethABC"), false);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"eth100"), false);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"et"), false);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"eth"), false);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L""), false);
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateIface(L"abc"), false);
}

void NxNetRoute_Test:: testNonRequiredParameters()
{
    SCXHandle<NxNetRouteTestDependencies>deps(new NxNetRouteTestDependencies());
    m_netrouteenum = new NxNetRouteEnumeration(deps);
    std::wstring testWstring = L"";
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateNonRequiredParameters(testWstring), true);
    CPPUNIT_ASSERT_EQUAL(testWstring, L"0");
    testWstring = L"15";
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateNonRequiredParameters(testWstring), true);
    testWstring = L"et";
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateNonRequiredParameters(testWstring), false);
    testWstring = L"abc";
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateNonRequiredParameters(testWstring), false);
    testWstring = L"134";
    CPPUNIT_ASSERT_EQUAL(m_netrouteenum->ValidateNonRequiredParameters(testWstring), true);
}
CPPUNIT_TEST_SUITE_REGISTRATION( NxNetRoute_Test );
