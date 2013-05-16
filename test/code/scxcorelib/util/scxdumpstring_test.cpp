/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Test DumpString utilities

    \date        2007-12-04 14:51:56

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxdumpstring.h> 
#include <scxcorelib/scxexception.h> 
#include <iostream>
#include <vector> 

using namespace SCXCoreLib;

class SCXCalendarTimeForDump
{
public:
    SCXCalendarTimeForDump(int year, unsigned short month, unsigned short day, unsigned short hour, 
        unsigned short minute, float second, short minutesFromUTC)
            : m_year(year), m_month(month), m_day(day), 
              m_hour(hour), m_minute(minute), m_second(second),
              m_decimalCount(6), m_minutesFromUTC(minutesFromUTC), m_initialized(true) {
    }

    std::wstring DumpString() const {
           return SCXDumpStringBuilder("SCXCalendarTime")
            .Scalar("year", m_year)
            .Scalar("month", m_month)
            .Scalar("day", m_day)
            .Scalar("hour", m_hour)
            .Scalar("minute", m_minute)
            .Scalar("second", m_second)
            .Scalar("minutesFromUTC", m_minutesFromUTC)
            .Scalar("initialized", m_initialized);            
    }

private:
    int m_year;                 
    unsigned short m_month;     
    unsigned short m_day;       
    unsigned short m_hour;      
    unsigned short m_minute;    
    float m_second;
    unsigned short m_decimalCount;
    short m_minutesFromUTC;       
    bool m_initialized;    
};

class SCXDumpStringTest : public CPPUNIT_NS::TestFixture 
{
    CPPUNIT_TEST_SUITE( SCXDumpStringTest );
    CPPUNIT_TEST( TestBuildingSingleValues );
    CPPUNIT_TEST( TestBuildingInstances );
    CPPUNIT_TEST( TestBuildingValues );
    /* CUSTOMIZE: Add more tests here */
    CPPUNIT_TEST_SUITE_END();
  
private:
        
public:
    void setUp(void)
    {
        /* CUSTOMIZE: This method will be called once before each test function. Use it to set up commonly used objects */
    }
  
    void tearDown(void)
    {
        /* CUSTOMIZE: This method will be called once after each test function with no regard of success or failure. */
    }
 
    void TestBuildingSingleValues(void)
    {     
        SCXCalendarTimeForDump lastModified(2007,11,12,15,30, 0, 0);
        std::wstring str1 = SCXDumpStringBuilder("SCXFile") 
            .Scalar("size", 123456)
            .Scalar("writable", true)            
            .Instance("lastModified", lastModified)
            .Text("path", L"/usr/local/kalle.txt");
        CPPUNIT_ASSERT(str1 == L"SCXFile: size=123456 writable=1 lastModified=[SCXCalendarTime: year=2007 month=11 "
                               L"day=12 hour=15 minute=30 second=0 minutesFromUTC=0 initialized=1] path='/usr/local/kalle.txt'"); 
    }
    
    void TestBuildingInstances() {
        std::vector<SCXCalendarTimeForDump> items;
        items.push_back(SCXCalendarTimeForDump(2007,11,12,15,30, 0, 0));
        items.push_back(SCXCalendarTimeForDump(2005,10,30,18,20, 0, 120));
        std::wstring str1 = SCXDumpStringBuilder("Testclass").Instances("times", items);
        CPPUNIT_ASSERT(str1 == L"Testclass: times={[SCXCalendarTime: year=2007 month=11 day=12 hour=15 minute=30 "
                               L"second=0 minutesFromUTC=0 initialized=1] [SCXCalendarTime: year=2005 " 
                               L"month=10 day=30 hour=18 minute=20 second=0 minutesFromUTC=120 initialized=1]}");
    }
    
    void TestBuildingValues() {
        std::vector<int> items;
        items.push_back(3);
        items.push_back(5);
        std::wstring str1 = SCXDumpStringBuilder("Testclass").Scalars("numbers", items);    
        CPPUNIT_ASSERT(str1 == L"Testclass: numbers={3 5}");
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXDumpStringTest ); /* CUSTOMIZE: Name must be same as classname */
