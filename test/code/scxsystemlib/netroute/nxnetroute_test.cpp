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
#include <testutils/scxunit.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <cassert>

using namespace SCXCoreLib;
using namespace SCXSystemLib;

static const wstring s_LogModuleName = L"scx.core.common.pal.system.netroute.nxnetrouteenumeration";

class NxNetRouteTestDependencies : public NxNetRouteDependencies
{
    friend class NxNetRouteEnumeration;
    friend class NxNetRouteDependencies;

public:
    NxNetRouteTestDependencies()
    {
        // this path does not exist so much for testing as it is to keep me
        // from accidentally overwriting the real route file while
        // writing tests.
        SetPathToFile(L"/tmp/route");
    }
};

class NxNetRouteTestDependenciesWithLines : public NxNetRouteDependencies
{

public:
    NxNetRouteTestDependenciesWithLines()
    {
        SetPathToFile(L"/tmp/route");
        // The line below has values like x, y, and z that aren't found in a real file.
        // Because I want to make sure I am reading and then writing the correct columns, I added
        // these characters to tell them apart.  Normally they would all be zeroes with their default values, and
        // I wouldn't be able to tell if I accidentally read the fields out of order since the values
        // would all be zero and look correct.
        m_lines.push_back(L"eth3\t1273AB31\tA8EAFFFF\t0003\ta\tb\t2\t05178000\tx\ty\tz");
        m_lines.push_back(L"eth0\t00803B98\t00000000\t0001\t0\t0\t1\t00FCFFFF\t0\t0\t0");
    }
};

class NxNetRoute_Test: public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE(NxNetRoute_Test);

#if defined(linux)
    CPPUNIT_TEST(testDependencyObjectFileReading);
    CPPUNIT_TEST(testDependenciesNotNull);
    CPPUNIT_TEST(testEnumerationNotNull);
    CPPUNIT_TEST(testNxNetRouteEnumerationGetSize);
    CPPUNIT_TEST(testReadingFromFile);
    CPPUNIT_TEST(testIfaceIsLoopBack);
    CPPUNIT_TEST(testIfaceValidEth);
    CPPUNIT_TEST(testIfaceInvalidEth);
    CPPUNIT_TEST(testNxNetRouteAddOneNetRouteInstance);
    CPPUNIT_TEST(testParseLines);
    CPPUNIT_TEST(testNonRequiredParameters);
    CPPUNIT_TEST(testWrite);
    CPPUNIT_TEST(testReadAndWriteRealRouteFile);
    CPPUNIT_TEST(testNoFileWrittenWhenNothingToWrite);
#endif

    CPPUNIT_TEST_SUITE_END();

private:
    SCXHandle<NxNetRouteEnumeration> m_netrouteenum;
    void deleteTestFile();

public:
    void setUp(void);
    void tearDown(void);

    void testDependencyObjectFileReading();
    void testNxNetRouteEnumerationGetSize();
    void testDependenciesNotNull();
    void testEnumerationNotNull();
    void testReadingFromFile();
    void testIfaceIsLoopBack();
    void testIfaceValidEth();
    void testIfaceInvalidEth();
    void testNxNetRouteAddOneNetRouteInstance();
    void testParseLines();
    void testNonRequiredParameters();
    void testWrite();
    void testReadAndWriteRealRouteFile();
    void testNoFileWrittenWhenNothingToWrite();

};

void NxNetRoute_Test::setUp(void)
{
    m_netrouteenum = NULL;
}

void NxNetRoute_Test::tearDown(void)
{
    if (NULL != m_netrouteenum)
    {
        m_netrouteenum->CleanUp();
        m_netrouteenum = 0;
    }
}

void NxNetRoute_Test::testDependencyObjectFileReading()
{
    // Create a route file.
    // please note that the trailing white space is necessary for this to look exactly like the real route file.
    // Maybe the trailing whitespace helps with formatting, but its the standard so I don't want to change it.
    vector<wstring> lines;
    lines.push_back(L"Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask\t\tMTU\tWindow\tIRTT                                                       ");
    lines.push_back(L"eth9\t1273AB31\tA8EAFFFF\t0003\ta\tc\t2\t05178000\tx\ty\tz                                                                               ");
    lines.push_back(L"eth8\t00803B98\t00000000\t0001\t0\t0\t1\t00FCFFFF\t0\t0\t0                                                                               ");


    SCXFilePath tempFilePath(SCXFile::CreateTempFile(L"route"));
    CPPUNIT_ASSERT(SCXFile::Exists(tempFilePath));

    SCXFileInfo testFile(tempFilePath.Get());
    CPPUNIT_ASSERT( testFile.PathExists() );

    // write out lines to the test file to a real file
    SCXFile::WriteAllLines(tempFilePath.Get(), lines, std::ios_base::out);

    // pass in the path to our new file
    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteDependencies(tempFilePath.Get()));

    // read the file
    deps->Init();
    vector<wstring>lines2 = deps->GetLines();

    CPPUNIT_ASSERT( deps->GetLines().size() == 2 );
    CPPUNIT_ASSERT_EQUAL(deps->GetLines().at(0), lines.at(1));
    CPPUNIT_ASSERT_EQUAL(deps->GetLines().at(1), lines.at(2));

    // clean up our temp file
    SCXFile::Delete(tempFilePath);
    CPPUNIT_ASSERT(!SCXFile::Exists(tempFilePath));
}

void NxNetRoute_Test::testNxNetRouteEnumerationGetSize()
{
   SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteTestDependenciesWithLines());

   m_netrouteenum = new NxNetRouteEnumeration(deps);
   m_netrouteenum->Update(false);

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
    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteTestDependenciesWithLines());

    m_netrouteenum = new NxNetRouteEnumeration(deps);

    m_netrouteenum->Update(false);

    SCXHandle<NxNetRouteInstance>instance0(new NxNetRouteInstance());
    instance0 = m_netrouteenum->GetInstance(0);

    CPPUNIT_ASSERT_EQUAL(instance0->GetInterface() , L"eth3");
    CPPUNIT_ASSERT_EQUAL(instance0->GetDestination() , L"18.115.171.49");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGateway() , L"168.234.255.255");
    CPPUNIT_ASSERT_EQUAL(instance0->GetGenMask() , L"5.23.128.0");
    CPPUNIT_ASSERT_EQUAL(instance0->GetFlags() , L"0003");
    CPPUNIT_ASSERT_EQUAL(instance0->GetRefCount() , L"a");
    CPPUNIT_ASSERT_EQUAL(instance0->GetUse() , L"b");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMetric() , L"2");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMtu(), L"x");
    CPPUNIT_ASSERT_EQUAL(instance0->GetWindow() ,L"y");
    CPPUNIT_ASSERT_EQUAL(instance0->GetIrtt() , L"z");

    SCXHandle<NxNetRouteInstance>instance1(new NxNetRouteInstance());
    instance1 = m_netrouteenum->GetInstance(1);

    CPPUNIT_ASSERT_EQUAL(instance1->GetInterface() , L"eth0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetDestination() , L"0.128.59.152");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGenMask() , L"0.252.255.255");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGateway() , L"0.0.0.0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetFlags() , L"0001");
    CPPUNIT_ASSERT_EQUAL(instance1->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetMetric() , L"1");
    CPPUNIT_ASSERT_EQUAL(instance1->GetWindow() ,L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetIrtt() , L"0");
}

void NxNetRoute_Test::testReadingFromFile()
{
    // create the lines of a route file
    // please note that the trailing white space is necessary for this to look exactly like the real route file.
    // Maybe the trailing whitespace helps with formatting, but its the standard so I don't want to change it.
    vector<wstring> lines;
    lines.push_back(L"Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask\t\tMTU\tWindow\tIRTT                                                       ");
    lines.push_back(L"eth9\t1273AB31\tA8EAFFFF\t0003\t0\t0\t2\t05178000\tx\ty\tz                                                                               ");
    lines.push_back(L"eth8\t00803B98\t00000000\t0001\t0\t0\t1\t00FCFFFF\t0\t0\t0                                                                               ");

    SCXFilePath tempFilePath(SCXFile::CreateTempFile(L"route"));
    CPPUNIT_ASSERT(SCXFile::Exists(tempFilePath));

    // write out lines to the test file to be read in again
    SCXFile::WriteAllLines(tempFilePath.Get(), lines, std::ios_base::out);

    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteDependencies(tempFilePath.Get()));
    m_netrouteenum = new NxNetRouteEnumeration(deps);
    m_netrouteenum->Update();
    CPPUNIT_ASSERT(m_netrouteenum->Size() == 2);

    // make sure first line of route file was read correctly
    SCXHandle<NxNetRouteInstance>instance0(new NxNetRouteInstance());
    instance0 = m_netrouteenum->GetInstance(0);

    CPPUNIT_ASSERT_EQUAL(instance0->GetInterface() , L"eth9");
    CPPUNIT_ASSERT_EQUAL(instance0->GetFlags() , L"0003");
    CPPUNIT_ASSERT_EQUAL(instance0->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance0->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMetric() , L"2");
    CPPUNIT_ASSERT_EQUAL(instance0->GetMtu(), L"x");
    CPPUNIT_ASSERT_EQUAL(instance0->GetWindow() ,L"y");
    CPPUNIT_ASSERT_EQUAL(instance0->GetIrtt() , L"z");

    // make sure second line of route file was read correctly
    SCXHandle<NxNetRouteInstance>instance1(new NxNetRouteInstance());
    instance1 = m_netrouteenum->GetInstance(1);

    CPPUNIT_ASSERT_EQUAL(instance1->GetInterface() , L"eth8");
    CPPUNIT_ASSERT_EQUAL(instance1->GetDestination() , L"0.128.59.152");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGenMask() , L"0.252.255.255");
    CPPUNIT_ASSERT_EQUAL(instance1->GetGateway() , L"0.0.0.0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetFlags() , L"0001");
    CPPUNIT_ASSERT_EQUAL(instance1->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetMetric() , L"1");
    CPPUNIT_ASSERT_EQUAL(instance1->GetWindow() ,L"0");
    CPPUNIT_ASSERT_EQUAL(instance1->GetIrtt() , L"0");

    // clean up our temp file
    SCXFile::Delete(tempFilePath);
    CPPUNIT_ASSERT(!SCXFile::Exists(tempFilePath));
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

void NxNetRoute_Test::testNonRequiredParameters()
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

void NxNetRoute_Test::testWrite()
{
    SCXHandle<NxNetRouteTestDependenciesWithLines>deps(new NxNetRouteTestDependenciesWithLines());
    m_netrouteenum = new NxNetRouteEnumeration(deps);

    CPPUNIT_ASSERT_EQUAL(static_cast<long unsigned int>(0), m_netrouteenum->Size());
    m_netrouteenum->Update(false);// false means use the lines pre-loaded in TestDependencies class
    CPPUNIT_ASSERT_EQUAL(static_cast<long unsigned int>(2), m_netrouteenum->Size());

    SCXFilePath tempFilePath(SCXFile::CreateTempFile(L"route"));
    CPPUNIT_ASSERT(SCXFile::Exists(tempFilePath));

    // set our newly create temp file path
    deps->SetPathToFile(tempFilePath);

    // write to our unique temp file
    m_netrouteenum->Write();

    CPPUNIT_ASSERT_EQUAL(static_cast<long unsigned int>(2), m_netrouteenum->Size());
    // read our temp file
    m_netrouteenum->Update();
    CPPUNIT_ASSERT_EQUAL(static_cast<long unsigned int>(2), m_netrouteenum->Size());


    SCXHandle<NxNetRouteInstance>instance = m_netrouteenum->GetInstance(0);

    CPPUNIT_ASSERT_EQUAL(instance->GetInterface() , L"eth3");
    CPPUNIT_ASSERT_EQUAL(instance->GetDestination() , L"18.115.171.49");
    CPPUNIT_ASSERT_EQUAL(instance->GetGateway() , L"168.234.255.255");
    CPPUNIT_ASSERT_EQUAL(instance->GetGenMask() , L"5.23.128.0");
    CPPUNIT_ASSERT_EQUAL(instance->GetFlags() , L"0003");
    CPPUNIT_ASSERT_EQUAL(instance->GetRefCount() , L"a");
    CPPUNIT_ASSERT_EQUAL(instance->GetUse() , L"b");
    CPPUNIT_ASSERT_EQUAL(instance->GetMetric() , L"2");
    CPPUNIT_ASSERT_EQUAL(instance->GetMtu(), L"x");
    CPPUNIT_ASSERT_EQUAL(instance->GetWindow() ,L"y");
    CPPUNIT_ASSERT_EQUAL(instance->GetIrtt() , L"z");

    instance = m_netrouteenum->GetInstance(1);

    CPPUNIT_ASSERT_EQUAL(instance->GetInterface() , L"eth0");
    CPPUNIT_ASSERT_EQUAL(instance->GetDestination() , L"0.128.59.152");
    CPPUNIT_ASSERT_EQUAL(instance->GetGenMask() , L"0.252.255.255");
    CPPUNIT_ASSERT_EQUAL(instance->GetGateway() , L"0.0.0.0");
    CPPUNIT_ASSERT_EQUAL(instance->GetFlags() , L"0001");
    CPPUNIT_ASSERT_EQUAL(instance->GetRefCount() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance->GetUse() , L"0");
    CPPUNIT_ASSERT_EQUAL(instance->GetMetric() , L"1");
    CPPUNIT_ASSERT_EQUAL(instance->GetWindow() ,L"0");
    CPPUNIT_ASSERT_EQUAL(instance->GetIrtt() , L"0");

    // clean up our temp file
    SCXFile::Delete(tempFilePath);
    CPPUNIT_ASSERT(!SCXFile::Exists(tempFilePath));
}

void NxNetRoute_Test::testReadAndWriteRealRouteFile()
{
    SCXHandle<NxNetRouteDependencies>deps(new NxNetRouteDependencies());
    m_netrouteenum = new NxNetRouteEnumeration(deps);

    // make sure we have the real route file
    assert(deps->GetPathToFile().compare(L"/proc/net/route") == 0);

    // read in the real file
    m_netrouteenum->Update();

    SCXFilePath tempFilePath(SCXFile::CreateTempFile(L"route"));
    CPPUNIT_ASSERT(SCXFile::Exists(tempFilePath));

    // set the path to the  newly created temp file path
    deps->SetPathToFile(tempFilePath);

    // make sure we are NOT writing to the real route file
    assert(deps->GetPathToFile().compare(L"/proc/net/route") != 0);

    // write to our unique temp file
    m_netrouteenum->Write();


    // compare the whole lines of the original route file to the one we just wrote
    SCXStream::NLFs nlfs;
    vector<wstring> realRouteLines;
    vector<wstring> tempRouteLines;

    SCXFile::ReadAllLines(SCXFilePath(L"/proc/net/route"), realRouteLines, nlfs);
    SCXFile::ReadAllLines(SCXFilePath(tempFilePath), tempRouteLines, nlfs);

    CPPUNIT_ASSERT_EQUAL(realRouteLines.size(), tempRouteLines.size());

    for(unsigned int i=0; i < realRouteLines.size(); i++)
    {
        wstring originalLine = realRouteLines.at(i);
        wstring providerWrittenLine = tempRouteLines.at(i);

        CPPUNIT_ASSERT_EQUAL(originalLine.length(), providerWrittenLine.length() );
        CPPUNIT_ASSERT(originalLine.compare(providerWrittenLine) == 0);
    }

    // clean up our temp file
    SCXFile::Delete(tempFilePath);
    CPPUNIT_ASSERT(!SCXFile::Exists(tempFilePath));
}

void NxNetRoute_Test::testNoFileWrittenWhenNothingToWrite()
{
    SCXFilePath tempFilePath(SCXFile::CreateTempFile(L"route"));

    // delete the file so I have a path with no existing file
    SCXFile::Delete(tempFilePath);

    SCXHandle<NxNetRouteTestDependenciesWithLines>deps(new NxNetRouteTestDependenciesWithLines());
    m_netrouteenum = new NxNetRouteEnumeration(deps);

    // set our temp path as the file output path
    deps->SetPathToFile(tempFilePath);

    // verify there is nothing to write
    CPPUNIT_ASSERT_EQUAL(static_cast<long unsigned int>(0), m_netrouteenum->Size());
    m_netrouteenum->Write();

    // a file should not exist if there was nothing to write
    CPPUNIT_ASSERT(!SCXFile::Exists(tempFilePath));

}

CPPUNIT_TEST_SUITE_REGISTRATION( NxNetRoute_Test );
