/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2007-09-28 12:34:56

    Test of time

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxprocess.h>

#include <iomanip>
#include <iostream>
#include <locale.h>
#include <sstream>
#include <string.h>

#if defined(SCX_UNIX)
#include <sys/time.h>
#endif

#if defined(TRAVIS)
const bool s_fTravis = true;
#else
const bool s_fTravis = false;
#endif

using namespace SCXCoreLib;
using namespace std;

wstring DumpString(const SCXException &e) {
    return e.What() + L" occured at " + e.Where();
}

class SCXTimeTest : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE( SCXTimeTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( createInvalidTimeExceptionForCoverage );
    CPPUNIT_TEST( TestAmountOfTime );
    CPPUNIT_TEST( TestRelativeTimeConstruction );
    CPPUNIT_TEST( TestCalendarTimeConstruction );
    CPPUNIT_TEST( TestCalendarTimeModification );
    CPPUNIT_TEST( TestCurrentLocal );
    CPPUNIT_TEST( TestCurrentUTC );
    CPPUNIT_TEST( TestCalendarTimeComparisons );
    CPPUNIT_TEST( TestToCIM );
    CPPUNIT_TEST( TestFromCIM );
    CPPUNIT_TEST( TestGetTimeOfDay );
    CPPUNIT_TEST( TestSetTimeOfDay );
    CPPUNIT_TEST( TestCalendarTimeToBasicISO8601 );
    CPPUNIT_TEST( TestCalendarTimeToExtendedISO8601 );
    CPPUNIT_TEST( TestCalendarTimeToLocalizedTime );
    CPPUNIT_TEST( TestCalendarTimeDecimalCount );
    CPPUNIT_TEST( TestRelativeTimeAllowsAucklandNZ );
    CPPUNIT_TEST( TestRelativeTimeToBasicISO8601Time );
    CPPUNIT_TEST( TestRelativeTimeToExtendedISO8601Time );
    CPPUNIT_TEST( TestAddYears );
    CPPUNIT_TEST( TestSubtractYears );
    CPPUNIT_TEST( TestAddMonths );
    CPPUNIT_TEST( TestSubtractMonths );
    CPPUNIT_TEST( TestAddDays );
    CPPUNIT_TEST( TestSubtractDays );
    CPPUNIT_TEST( TestAddHours );
    CPPUNIT_TEST( TestSubtractHours );
    CPPUNIT_TEST( TestAddMinutes );
    CPPUNIT_TEST( TestSubtractMinutes );
    CPPUNIT_TEST( TestAddSeconds );
    CPPUNIT_TEST( TestSubtractSeconds );
    CPPUNIT_TEST( TestSubtractCalendarTimes );
    CPPUNIT_TEST( TestFromISO8601 );
    CPPUNIT_TEST( TestPosixTime );
    CPPUNIT_TEST( TestMakeLocal );
    CPPUNIT_TEST( TestMakeLocalNoParam );
    CPPUNIT_TEST( TestMakeLocalNoParamDST );
    CPPUNIT_TEST( TestWI3245 );
    CPPUNIT_TEST( TestWI7268 );
    CPPUNIT_TEST( TestWI7350 );
    CPPUNIT_TEST( TestPrecisionSetAndGet );
    CPPUNIT_TEST( TestCompareWithYearOnly );
    CPPUNIT_TEST( TestCompareWithYearAndMonthOnly );
    CPPUNIT_TEST( TestCompareWithDateOnly );
    CPPUNIT_TEST( TestCompareWithDateAndHourOnly );
    CPPUNIT_TEST( TestCompareWithDateHourAndMinuteOnly );
    CPPUNIT_TEST_SUITE_END();

private:

public:
    void setUp() {
    }

    void tearDown() {
    }

    void callDumpStringForCoverage()
    {
        CPPUNIT_ASSERT(SCXCalendarTime::CurrentLocal().DumpString().find(L"SCXCalendarTime") != wstring::npos);
        CPPUNIT_ASSERT(SCXRelativeTime(1, 02, 03, 16, 50, 10.5, 1).DumpString().find(L"SCXRelativeTime") != wstring::npos);
    }

    void createInvalidTimeExceptionForCoverage()
    {
        try {
            SCXCalendarTime::FromISO8601(L"200102T040506,123456+07:30");
            CPPUNIT_FAIL("SCXInvalidTimeFormatException not throws!");
        } catch (SCXInvalidTimeFormatException& e) {
            CPPUNIT_ASSERT(L"200102" == e.GetInvalidText());
            CPPUNIT_ASSERT(e.What().find(e.GetInvalidText()) != wstring::npos);
        }
    }

    void TestAmountOfTime() {
        SCXAmountOfTime amount1;
        amount1.SetSeconds(8);
        SCXAmountOfTime amount3;
        amount3.SetSeconds(2);
        amount3 += amount3;
        amount3 += amount1;
        amount3 -= amount1;
        CPPUNIT_ASSERT(amount3 + amount3 == amount1);
        CPPUNIT_ASSERT(amount3 - amount3 == SCXAmountOfTime());
        CPPUNIT_ASSERT(amount3 <= amount1);
        CPPUNIT_ASSERT(amount3 < amount1);
        CPPUNIT_ASSERT(amount1 >= amount3);
        CPPUNIT_ASSERT(amount1 > amount3);
        CPPUNIT_ASSERT(Abs(SCXAmountOfTime().SetSeconds(-5)) == SCXAmountOfTime().SetSeconds(5));
        CPPUNIT_ASSERT(IsEquivalent(amount3, amount3, SCXAmountOfTime()));
        CPPUNIT_ASSERT(!IsEquivalent(amount1, amount3, SCXAmountOfTime()));
        CPPUNIT_ASSERT(IsEquivalent(amount1, amount3, amount1 - amount3));
        CPPUNIT_ASSERT_THROW(IsEquivalent(amount1, amount3, amount3 - amount1), SCXInvalidArgumentException);
        CPPUNIT_ASSERT(SCXAmountOfTime() + SCXAmountOfTime() == SCXAmountOfTime());
        CPPUNIT_ASSERT(SCXAmountOfTime() - SCXAmountOfTime() == SCXAmountOfTime());
        CPPUNIT_ASSERT(-amount3 == SCXAmountOfTime() - amount3);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestRelativeTimeConstruction() {
        SCXRelativeTime time1(1, 02, 03, 16, 50, 10.5, 1);
        CPPUNIT_ASSERT(time1.GetYears() == 1);
        CPPUNIT_ASSERT(time1.GetMonths() == 02);
        CPPUNIT_ASSERT(time1.GetDays() == 03);
        CPPUNIT_ASSERT(time1.GetHours() == 16);
        CPPUNIT_ASSERT(time1.GetMinutes() == 50);
        CPPUNIT_ASSERT(Equal(time1.GetSeconds(), 10.5, 0));
        CPPUNIT_ASSERT(IsIdentical(SCXRelativeTime() + SCXRelativeTime(), SCXRelativeTime()));
        CPPUNIT_ASSERT(IsIdentical(SCXRelativeTime() - SCXRelativeTime(), SCXRelativeTime()));
    }

    void TestRelativeTimeModification() {
        SCXRelativeTime time1;
        time1.SetYears(2);
        time1.SetMonths(3);
        time1.SetDays(5);
        time1.SetHours(6);
        time1.SetMinutes(7);
        time1.SetSeconds(8);
        CPPUNIT_ASSERT(time1.GetYears() == 2 && time1.GetMonths() == 3 && time1.GetDays() == 5);
        CPPUNIT_ASSERT(time1.GetHours() == 6 && time1.GetMinutes() == 7 && Equal(time1.GetSeconds(), 8, 0));
        SCXRelativeTime time2;
        time2.SetYears(20);
        time2.SetMonths(30);
        time2.SetDays(50);
        time2.SetHours(60);
        time2.SetMinutes(70);
        time2.SetSeconds(80);
        SCXRelativeTime time3;
        time3.SetYears(22);
        time3.SetMonths(33);
        time3.SetDays(44);
        time3.SetDays(54);
        time3.SetHours(65);
        time3.SetMinutes(77);
        time3.SetSeconds(88);
        CPPUNIT_ASSERT(IsIdentical(time1 + time2, time3));
        CPPUNIT_ASSERT(IsIdentical(time3 - time2, time1));
        CPPUNIT_ASSERT(IsIdentical(time1 + time2, time3));
        CPPUNIT_ASSERT(IsIdentical(time3 - time2, time1));
        CPPUNIT_ASSERT(-time3 == SCXRelativeTime() - time3);

    }

    void TestCalendarTimeConstruction() {
        SCXCalendarTime time(2002, 12, 31, 23, 59, 59.5, 1, SCXRelativeTime().SetMinutes(30));
        CPPUNIT_ASSERT(time.GetYear() == 2002);
        CPPUNIT_ASSERT(time.GetMonth() == 12);
        CPPUNIT_ASSERT(time.GetDay() == 31);
        CPPUNIT_ASSERT(time.GetHour() == 23);
        CPPUNIT_ASSERT(time.GetMinute() == 59);
        CPPUNIT_ASSERT(Equal(time.GetSecond(), 59.5, 0));
        CPPUNIT_ASSERT(IsIdentical(time.GetOffsetFromUTC(), SCXRelativeTime().SetMinutes(30)));
        CPPUNIT_ASSERT_THROW(SCXCalendarTime(1969, 12, 31, 23, 59, 59.5, 1, SCXRelativeTime()), SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime(1980, 0, 31, 23, 59, 59.5, 1, SCXRelativeTime()), SCXIllegalIndexException<scxmonth>);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime(1980, 13, 31, 23, 59, 59.5, 1, SCXRelativeTime()), SCXIllegalIndexException<scxmonth>);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime(1980, 5, 0, 23, 59, 59.5, 1, SCXRelativeTime()), SCXIllegalIndexException<scxday>);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime(1980, 5, 32, 23, 59, 59.5, 1, SCXRelativeTime()), SCXIllegalIndexException<scxday>);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime(1980, 5, 20, 24, 59, 59.5, 1, SCXRelativeTime()), SCXIllegalIndexException<scxhour>);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime(1980, 5, 20, 20, 60, 59.5, 1, SCXRelativeTime()), SCXIllegalIndexException<scxminute>);
        SCXCalendarTime time2(2000, 01, 02, 03, 04, 05, 1, SCXRelativeTime().SetHours(2).SetMinutes(30));

        SCXCalendarTime time3;
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT(time3.GetYear() == 0);
        CPPUNIT_ASSERT(time3.GetMonth() == 0);
        CPPUNIT_ASSERT(time3.GetDay() == 0);
        CPPUNIT_ASSERT(time3.GetHour() == 0);
        CPPUNIT_ASSERT(time3.GetMinute() == 0);
        CPPUNIT_ASSERT(Equal(time3.GetSecond(), 0, 0));
        CPPUNIT_ASSERT(time3.GetOffsetFromUTC().GetHours() == 0);
        CPPUNIT_ASSERT(time3.GetDecimalCount() == 0);
        SCXUNIT_ASSERTIONS_FAILED(8);

        time3 = time2;
        CPPUNIT_ASSERT(IsIdentical(time3, time2));
        CPPUNIT_ASSERT(IsIdentical(SCXCalendarTime(time2), time2));

        SCXCalendarTime time4(2002, 12, 31);
        CPPUNIT_ASSERT_EQUAL(time.GetYear(), time4.GetYear());
        CPPUNIT_ASSERT_EQUAL(time.GetMonth(), time4.GetMonth());
        CPPUNIT_ASSERT_EQUAL(time.GetDay(), time4.GetDay());
        CPPUNIT_ASSERT(0 == time4.GetHour());
        CPPUNIT_ASSERT(0 == time4.GetMinute());
        CPPUNIT_ASSERT(Equal(time4.GetSecond(), 0, 0));
        CPPUNIT_ASSERT(IsIdentical(time4.GetOffsetFromUTC(), SCXRelativeTime().SetMinutes(0)));
    }


    void TestCalendarTimeModification() {
        SCXCalendarTime time(2000, 1, 3, 2, 2, 10, 0, SCXRelativeTime());
        time.SetYear(2002);
        CPPUNIT_ASSERT(time.GetYear() == 2002);
        time.SetMonth(12);
        CPPUNIT_ASSERT(time.GetMonth() == 12);
        time.SetDay(31);
        CPPUNIT_ASSERT(time.GetDay() == 31);
        time.SetHour(23);
        CPPUNIT_ASSERT(time.GetHour() == 23);
        time.SetMinute(59);
        CPPUNIT_ASSERT(time.GetMinute() == 59);
        time.SetSecond(59.5);
        CPPUNIT_ASSERT(Equal(time.GetSecond(), 59.5, 0));
        time.SetOffsetFromUTC(SCXRelativeTime().SetMinutes(30));
        CPPUNIT_ASSERT(IsIdentical(time.GetOffsetFromUTC(), SCXRelativeTime().SetMinutes(30)));
        CPPUNIT_ASSERT_THROW(time.SetYear(1969), SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(time.SetMonth(0), SCXIllegalIndexException<scxmonth>);
        CPPUNIT_ASSERT_THROW(time.SetMonth(13), SCXIllegalIndexException<scxmonth>);
        CPPUNIT_ASSERT_THROW(time.SetDay(0), SCXIllegalIndexException<scxday>);
        CPPUNIT_ASSERT_THROW(time.SetDay(32), SCXIllegalIndexException<scxday>);
        CPPUNIT_ASSERT_THROW(time.SetHour(24), SCXIllegalIndexException<scxhour>);
        CPPUNIT_ASSERT_THROW(time.SetMinute(60), SCXIllegalIndexException<scxminute>);

        SCXCalendarTime time2(2000, 1, 3, 2, 2, 10, 0, SCXRelativeTime().SetHours(2));
        time2.MakeUTC();
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 1, 3, 0, 2, 10, 0, SCXRelativeTime()));
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestAddYears() {
        SCXCalendarTime time1(2001, 01, 02, 2, 3, 4, 1, SCXRelativeTime());
        time1 += SCXRelativeTime().SetYears(1);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2002, 01, 02, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime());
        time3 += SCXRelativeTime().SetYears(1);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2001, 03, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 02, 28, 2, 3, 4, 1, SCXRelativeTime());
        time4 += SCXRelativeTime().SetYears(1);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2001, 02, 28, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime());
        time5 += SCXRelativeTime().SetYears(4);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2004, 02, 29, 2, 3, 4, 1, SCXRelativeTime()));
    }

    void TestSubtractYears() {
        SCXCalendarTime time2(2001, 01, 02, 2, 3, 4, 1, SCXRelativeTime());
        time2 -= SCXRelativeTime().SetYears(1);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 01, 02, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime());
        time4 -= SCXRelativeTime().SetYears(1);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(1999, 03, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 02, 28, 2, 3, 4, 1, SCXRelativeTime());
        time5 -= SCXRelativeTime().SetYears(1);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(1999, 02, 28, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time6(2004, 02, 29, 2, 3, 4, 1, SCXRelativeTime());
        time6 -= SCXRelativeTime().SetYears(4);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime()));
    }

    void TestAddMonths() {
        SCXCalendarTime time1(2000, 01, 02, 2, 3, 4, 1, SCXRelativeTime());
        time1 += SCXRelativeTime().SetMonths(2);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 03, 02, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 01, 31, 2, 3, 4, 1, SCXRelativeTime());
        time2 += SCXRelativeTime().SetMonths(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 03, 31, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 01, 31, 2, 3, 4, 1, SCXRelativeTime());
        time3 += SCXRelativeTime().SetMonths(1);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 03, 02, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 01, 31, 2, 3, 4, 1, SCXRelativeTime());
        time4 += SCXRelativeTime().SetMonths(14);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2001, 03, 31, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 11, 30, 2, 3, 4, 1, SCXRelativeTime());
        time5 += SCXRelativeTime().SetMonths(1);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2000, 12, 30, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time6(2000, 11, 30, 2, 3, 4, 1, SCXRelativeTime());
        time6 += SCXRelativeTime().SetMonths(2);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2001, 01, 30, 2, 3, 4, 1, SCXRelativeTime()));

    }

    void TestSubtractMonths() {

        SCXCalendarTime time1(2000, 03, 02, 2, 3, 4, 1, SCXRelativeTime());
        time1 -= SCXRelativeTime().SetMonths(2);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 01, 02, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 03, 31, 2, 3, 4, 1, SCXRelativeTime());
        time2 -= SCXRelativeTime().SetMonths(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 01, 31, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 03, 31, 2, 3, 4, 1, SCXRelativeTime());
        time3 -= SCXRelativeTime().SetMonths(1);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 03, 02, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 01, 31, 2, 3, 4, 1, SCXRelativeTime());
        time4 -= SCXRelativeTime().SetMonths(14);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(1998, 12, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 03, 29, 2, 3, 4, 1, SCXRelativeTime());
        time5 -= SCXRelativeTime().SetMonths(13);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(1999, 03, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time6(2000, 03, 31, 2, 3, 4, 1, SCXRelativeTime());
        time6 -= SCXRelativeTime().SetMonths(2);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2000, 01, 31, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time7(2000, 03, 31, 2, 3, 4, 1, SCXRelativeTime());
        time7 -= SCXRelativeTime().SetMonths(3);
        CPPUNIT_ASSERT(time7 == SCXCalendarTime(1999, 12, 31, 2, 3, 4, 1, SCXRelativeTime()));

    }

    void TestAddDays() {
        SCXCalendarTime time1(2000, 12, 30, 2, 3, 4, 1, SCXRelativeTime());
        time1 += SCXRelativeTime().SetDays(1);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 12, 31, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 12, 30, 2, 3, 4, 1, SCXRelativeTime());
        time2 += SCXRelativeTime().SetDays(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2001, 01, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 11, 30, 2, 3, 4, 1, SCXRelativeTime());
        time3 += SCXRelativeTime().SetDays(1);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 12, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 02, 28, 2, 3, 4, 1, SCXRelativeTime());
        time4 += SCXRelativeTime().SetDays(1);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime());
        time5 += SCXRelativeTime().SetDays(1);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2000, 03, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time6(2000, 04, 10, 2, 3, 4, 1, SCXRelativeTime());
        time6 += SCXRelativeTime().SetDays(40);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2000, 05, 20, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time7(2000, 04, 10, 2, 3, 4, 1, SCXRelativeTime());
        time7 += SCXRelativeTime().SetDays(366 + 40);
        CPPUNIT_ASSERT(time7 == SCXCalendarTime(2001, 05, 20, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time8(2001, 04, 10, 2, 3, 4, 1, SCXRelativeTime());
        time8 += SCXRelativeTime().SetDays(365 + 40);
        CPPUNIT_ASSERT(time8 == SCXCalendarTime(2002, 05, 20, 2, 3, 4, 1, SCXRelativeTime()));

    }

    void TestSubtractDays() {
        SCXCalendarTime time1(2000, 01, 02, 2, 3, 4, 1, SCXRelativeTime());
        time1 -= SCXRelativeTime().SetDays(1);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 01, 01, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 01, 02, 2, 3, 4, 1, SCXRelativeTime());
        time2 -= SCXRelativeTime().SetDays(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(1999, 12, 31, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 02, 02, 2, 3, 4, 1, SCXRelativeTime());
        time3 -= SCXRelativeTime().SetDays(2);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 01, 31, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 03, 01, 2, 3, 4, 1, SCXRelativeTime());
        time4 -= SCXRelativeTime().SetDays(1);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 02, 29, 2, 3, 4, 1, SCXRelativeTime());
        time5 -= SCXRelativeTime().SetDays(1);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2000, 02, 28, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time6(2000, 05, 05, 2, 3, 4, 1, SCXRelativeTime());
        time6 -= SCXRelativeTime().SetDays(40);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2000, 03, 26, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time7(2000, 05, 05, 2, 3, 4, 1, SCXRelativeTime());
        time7 -= SCXRelativeTime().SetDays(365 + 40);
        CPPUNIT_ASSERT(time7 == SCXCalendarTime(1999, 03, 26, 2, 3, 4, 1, SCXRelativeTime()));

        SCXCalendarTime time8(2001, 05, 05, 2, 3, 4, 1, SCXRelativeTime());
        time8 -= SCXRelativeTime().SetDays(366 + 40);
        CPPUNIT_ASSERT(time8 == SCXCalendarTime(2000, 03, 26, 2, 3, 4, 1, SCXRelativeTime()));
    }

    void TestAddHours() {
        SCXCalendarTime time1(2000, 10, 01, 22, 00, 00, 0, SCXRelativeTime());
        time1 += SCXRelativeTime().SetHours(1);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 10, 01, 23, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 10, 01, 22, 00, 00, 0, SCXRelativeTime());
        time2 += SCXRelativeTime().SetHours(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 10, 02, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 10, 31, 22, 00, 00, 0, SCXRelativeTime());
        time3 += SCXRelativeTime().SetHours(2);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 11, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 12, 31, 22, 00, 00, 0, SCXRelativeTime());
        time4 += SCXRelativeTime().SetHours(2);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2001, 01, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time5(2001, 04, 05, 10, 00, 00, 0, SCXRelativeTime());
        time5 += SCXRelativeTime().SetHours(26);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2001, 04, 06, 12, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time6(2001, 04, 05, 10, 00, 00, 0, SCXRelativeTime());
        time6 += SCXRelativeTime().SetHours(30*24 + 2);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2001, 05, 05, 12, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time7(2001, 04, 05, 10, 00, 00, 0, SCXRelativeTime());
        time7 += SCXRelativeTime().SetHours(365*24 + 2);
        CPPUNIT_ASSERT(time7 == SCXCalendarTime(2002, 04, 05, 12, 00, 00, 0, SCXRelativeTime()));

    }

    void TestSubtractHours() {
        SCXCalendarTime time1(2000, 10, 01, 1, 00, 00, 0, SCXRelativeTime());
        time1 -= SCXRelativeTime().SetHours(1);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 10, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 10, 02, 01, 00, 00, 0, SCXRelativeTime());
        time2 -= SCXRelativeTime().SetHours(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 10, 01, 23, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 10, 01, 01, 00, 00, 0, SCXRelativeTime());
        time3 -= SCXRelativeTime().SetHours(2);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 9, 30, 23, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 01, 01, 01, 00, 00, 0, SCXRelativeTime());
        time4 -= SCXRelativeTime().SetHours(2);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(1999, 12, 31, 23, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time5(2001, 04, 05, 10, 00, 00, 0, SCXRelativeTime());
        time5 -= SCXRelativeTime().SetHours(26);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2001, 04, 04, 8, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time6(2001, 04, 05, 10, 00, 00, 0, SCXRelativeTime());
        time6 -= SCXRelativeTime().SetHours(31*24 + 2);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2001, 03, 05, 8, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time7(2001, 04, 05, 10, 00, 00, 0, SCXRelativeTime());
        time7 -= SCXRelativeTime().SetHours(365*24 + 2);
        CPPUNIT_ASSERT(time7 == SCXCalendarTime(2000, 04, 05, 8, 00, 00, 0, SCXRelativeTime()));
    }

    void TestAddMinutes() {
        SCXCalendarTime time1(2000, 10, 01, 23, 58, 00, 0, SCXRelativeTime());
        time1 += SCXRelativeTime().SetMinutes(1);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 10, 01, 23, 59, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 10, 01, 23, 58, 00, 0, SCXRelativeTime());
        time2 += SCXRelativeTime().SetMinutes(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 10, 02, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 12, 31, 23, 58, 00, 0, SCXRelativeTime());
        time3 += SCXRelativeTime().SetMinutes(2);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2001, 01, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 10, 01, 19, 58, 00, 0, SCXRelativeTime());
        time4 += SCXRelativeTime().SetMinutes(61);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2000, 10, 01, 20, 59, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 10, 01, 19, 58, 00, 0, SCXRelativeTime());
        time5 += SCXRelativeTime().SetMinutes(24 * 60 + 1);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2000, 10, 02, 19, 59, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time6(2000, 10, 01, 19, 58, 00, 0, SCXRelativeTime());
        time6 += SCXRelativeTime().SetMinutes(31 * 24 * 60 + 1);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2000, 11, 01, 19, 59, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time7(2001, 10, 01, 19, 58, 00, 0, SCXRelativeTime());
        time7 += SCXRelativeTime().SetMinutes(365 * 24 * 60 + 1);
        CPPUNIT_ASSERT(time7 == SCXCalendarTime(2002, 10, 01, 19, 59, 00, 0, SCXRelativeTime()));

    }

    void TestSubtractMinutes() {
        SCXCalendarTime time1(2000, 10, 01, 00, 01, 00, 0, SCXRelativeTime());
        time1 -= SCXRelativeTime().SetMinutes(1);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 10, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 10, 01, 00, 01, 00, 0, SCXRelativeTime());
        time2 -= SCXRelativeTime().SetMinutes(2);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 9, 30, 23, 59, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 01, 01, 00, 01, 00, 0, SCXRelativeTime());
        time3 -= SCXRelativeTime().SetMinutes(2);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(1999, 12, 31, 23, 59, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 10, 01, 19, 58, 00, 0, SCXRelativeTime());
        time4 -= SCXRelativeTime().SetMinutes(60);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2000, 10, 01, 18, 58, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time5(2000, 10, 02, 19, 58, 00, 0, SCXRelativeTime());
        time5 -= SCXRelativeTime().SetMinutes(24 * 60);
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2000, 10, 01, 19, 58, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time6(2000, 10, 01, 19, 58, 00, 0, SCXRelativeTime());
        time6 -= SCXRelativeTime().SetMinutes(30 * 24 * 60 + 1);
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2000, 9, 01, 19, 57, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time7(2002, 10, 01, 19, 58, 00, 0, SCXRelativeTime());
        time7 -= SCXRelativeTime().SetMinutes(365 * 24 * 60 + 1);
        CPPUNIT_ASSERT(time7 == SCXCalendarTime(2001, 10, 01, 19, 57, 00, 0, SCXRelativeTime()));
    }

    void TestAddSeconds() {
        SCXCalendarTime time1(2000, 10, 01, 00, 00, 00, 0, SCXRelativeTime());
        time1 += SCXRelativeTime().SetSeconds(16);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 10, 01, 00, 00, 16, 0, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 10, 01, 00, 00, 00, 0, SCXRelativeTime());
        time2 += SCXRelativeTime().SetSeconds(3600);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 10, 01, 01, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 02, 01, 00, 00, 00, 0, SCXRelativeTime());
        time3 += SCXRelativeTime().SetSeconds(29 * 24 * 60 * 60);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 03, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time4(2000, 02, 01, 00, 00, 00, 0, SCXRelativeTime());
        time4 += SCXRelativeTime().SetSeconds((366+365+365) * 24 * 60 * 60);
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2003, 02, 01, 00, 00, 00, 0, SCXRelativeTime()));

    }

    void TestSubtractSeconds() {
        SCXCalendarTime time1(2000, 10, 01, 00, 00, 16, 0, SCXRelativeTime());
        time1 -= SCXRelativeTime().SetSeconds(16);
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2000, 10, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time2(2000, 10, 01, 01, 00, 00, 0, SCXRelativeTime());
        time2 -= SCXRelativeTime().SetSeconds(3600);
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2000, 10, 01, 00, 00, 00, 0, SCXRelativeTime()));

        SCXCalendarTime time3(2000, 03, 01, 00, 00, 00, 0, SCXRelativeTime());
        time3 -= SCXRelativeTime().SetSeconds(29 * 24 * 60 * 60);
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2000, 02, 01, 00, 00, 00, 0, SCXRelativeTime()));
    }

    void TestSubtractCalendarTimes() {
        SCXCalendarTime time1a(2001, 1, 1, 00, 00, 00, 0, SCXRelativeTime());
        SCXCalendarTime time1b(2002, 2, 2, 02, 02, 02, 0, SCXRelativeTime());
        SCXAmountOfTime diff1b(time1b - time1a);
        SCXAmountOfTime diff1a(time1a - time1b);
        CPPUNIT_ASSERT(diff1a == -diff1b);
        CPPUNIT_ASSERT(time1a + diff1b == time1b);
        CPPUNIT_ASSERT(time1b + diff1a == time1a);

        SCXCalendarTime time2a(2002, 12, 02, 00, 10, 2, 0, SCXRelativeTime());
        SCXCalendarTime time2b(2004, 01, 30, 02, 02, 8, 0, SCXRelativeTime());
        SCXAmountOfTime diff2b(time2b - time2a);
        SCXAmountOfTime diff2a(time2a - time2b);
        CPPUNIT_ASSERT(diff2a == -diff2b);
        CPPUNIT_ASSERT(time2a + diff2b == time2b);
        CPPUNIT_ASSERT(time2b + diff2a == time2a);

        SCXCalendarTime time3a(2002, 12, 02, 10, 10, 2, 3, SCXRelativeTime().SetHours(2));
        SCXCalendarTime time3b(2002, 12, 02, 11, 10, 2, 5, SCXRelativeTime().SetHours(3));
        CPPUNIT_ASSERT(time3a - time3b == SCXAmountOfTime().SetDecimalCount(3));
    }

#if !defined(WIN32)
    SCXCalendarTime FetchCurrentTime() {
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        SCXCoreLib::SCXProcess::Run(L"/bin/date '+%Y-%m-%dT%H:%M:%S%z'", input, output, error);
        scxyear year = 0;
        scxmonth month = 1;
        scxday day = 1;
        scxhour hour = 0;
        scxminute minute = 0;
        scxsecond second = 0;
        int hourMinutesFromUTC = 0;
        char c;
        istringstream time(output.str());
        time >> year;
        time >> c;
        time >> month;
        time >> c;
        time >> day;
        time >> c;
        time >> hour;
        time >> c;
        time >> minute;
        time >> c;
        time >> second;
        time >> hourMinutesFromUTC;
        return SCXCalendarTime(year, month, day, hour, minute, second, SCXRelativeTime().SetHours(hourMinutesFromUTC / 100).SetMinutes(hourMinutesFromUTC % 100));
    }
#endif

    void TestCurrentLocal() {
#if defined(WIN32)
        SCXUNIT_WARNING(L"No sanity check of current local time since SCXProcess::Run isn't supported on windows");
#else
        SCXCalendarTime currentLocal(SCXCalendarTime::CurrentLocal());
        SCXCalendarTime envLocal(FetchCurrentTime());
#if defined(hpux) && (PF_MAJOR==11) && (PF_MINOR<=23) ||  defined(aix) || defined(sun) && (PF_MAJOR==5) && (PF_MINOR<10)
        SCXUNIT_WARNING(L"No sanity check of current timezone since date command current platform doesn't print numerical timezone");
        currentLocal.SetOffsetFromUTC(SCXRelativeTime());
        envLocal.SetOffsetFromUTC(SCXRelativeTime());
#endif
        ostringstream errStream;

        errStream << "currentLocal: " << currentLocal.ToPosixTime()
                  << ", currentLocal (UTC): " << StrToUTF8(currentLocal.GetOffsetFromUTC().DumpString())
                  << ", envLocal: " << envLocal.ToPosixTime()
                  << ", envLocal (UTC): " << StrToUTF8(envLocal.GetOffsetFromUTC().DumpString());

        CPPUNIT_ASSERT_MESSAGE(
            errStream.str().c_str(),
            currentLocal.GetOffsetFromUTC() == envLocal.GetOffsetFromUTC());
        CPPUNIT_ASSERT_MESSAGE(
            errStream.str().c_str(),
            Equivalent(currentLocal, envLocal, SCXAmountOfTime().SetSeconds(2)));
#endif
    }

    void TestCurrentUTC() {
#if defined(WIN32)
        SCXUNIT_WARNING(L"No sanity check of current UTC time since SCXProcess::Run isn't supported on windows");
#else
        SCXCalendarTime currentLocal(SCXCalendarTime::CurrentUTC());
        SCXRelativeTime offsetFromUTC(SCXCalendarTime::CurrentLocal().GetOffsetFromUTC());
        currentLocal.MakeLocal(SCXCalendarTime::CurrentOffsetFromUTC());
        SCXCalendarTime envLocal(FetchCurrentTime());
#if defined(hpux) && (PF_MAJOR==11) && (PF_MINOR<=23) ||  defined(aix)  || defined(sun) && (PF_MAJOR==5) && (PF_MINOR<10)
        SCXUNIT_WARNING(L"No sanity check of current timezone since date command current platform doesn't print numerical timezone");
        currentLocal.SetOffsetFromUTC(SCXRelativeTime());
        envLocal.SetOffsetFromUTC(SCXRelativeTime());
#endif
        ostringstream errStream;

        errStream << "currentLocal: " << currentLocal.ToPosixTime()
                  << ", currentLocal (UTC): " << StrToUTF8(currentLocal.GetOffsetFromUTC().DumpString())
                  << ", envLocal: " << envLocal.ToPosixTime()
                  << ", envLocal (UTC): " << StrToUTF8(envLocal.GetOffsetFromUTC().DumpString());
        CPPUNIT_ASSERT_MESSAGE(
            errStream.str().c_str(),
            currentLocal.GetOffsetFromUTC() == envLocal.GetOffsetFromUTC());
        CPPUNIT_ASSERT_MESSAGE(
            errStream.str().c_str(),
            Equivalent(currentLocal, envLocal, SCXAmountOfTime().SetSeconds(2)));
#endif
    }

    void TestCalendarTimeComparisons() {
        SCXCalendarTime date1(2000, 10, 01, 14, 50, 10, 0, SCXRelativeTime());
        SCXCalendarTime date2(2002, 10, 01, 14, 50, 10, 0, SCXRelativeTime());

        CPPUNIT_ASSERT(date1 == date1);
        CPPUNIT_ASSERT(!(date1 != date1));
        CPPUNIT_ASSERT(!(date1 < date1));
        CPPUNIT_ASSERT(!(date1 > date1));
        CPPUNIT_ASSERT(date1 <= date1);
        CPPUNIT_ASSERT(date1 >= date1);

        CPPUNIT_ASSERT(!(date1 == date2));
        CPPUNIT_ASSERT(date1 != date2);
        CPPUNIT_ASSERT(date1 < date2);
        CPPUNIT_ASSERT(date2 > date1);
        CPPUNIT_ASSERT(date1 <= date2);
        CPPUNIT_ASSERT(date2 >= date1);

        SCXCalendarTime date3(2002, 10, 01, 14, 50, 10, 0, SCXRelativeTime());
        SCXCalendarTime date4(2002, 10, 01, 16, 50, 10, 0, SCXRelativeTime().SetHours(2));
        CPPUNIT_ASSERT(date3 == date4);

        SCXCalendarTime date5(2002, 10, 01, 14, 50, 10, 0, SCXRelativeTime());
        SCXCalendarTime date6(2002, 10, 01, 14, 50, 20, 0, SCXRelativeTime());
        CPPUNIT_ASSERT(Equivalent(date5, date6, SCXAmountOfTime().SetSeconds(10)));
        CPPUNIT_ASSERT(!Equivalent(date5, date6, SCXAmountOfTime().SetSeconds(9)));
    }

    void TestToCIM() {
        SCXCalendarTime time1(1994, 12, 10, 16, 14, 15.001, 3, SCXRelativeTime().SetHours(2));
        wstring time1Str(time1.ToCIM());
        CPPUNIT_ASSERT(time1Str == L"19941210161415.001000+120");
        SCXCalendarTime time2(1994, 02, 03, 04, 05, 06.001, 3, SCXRelativeTime().SetMinutes(-30));
        wstring time2Str(time2.ToCIM());
        CPPUNIT_ASSERT(time2Str == L"19940203040506.001000-030");
    }

    void TestFromCIM() {
        SCXCalendarTime time1(SCXCalendarTime::FromCIM(L"19941210161415.001000+120"));
        SCXCalendarTime time2(1994, 12, 10, 16, 14, 15.001, 6, SCXRelativeTime().SetHours(2));
        CPPUNIT_ASSERT(IsIdentical(time1, time2));
        SCXCalendarTime time3(SCXCalendarTime::FromCIM(L"19940102030405.001000+120"));
        SCXCalendarTime time4(1994, 01, 02, 03, 04, 05.001, 6, SCXRelativeTime().SetHours(2));
        CPPUNIT_ASSERT(IsIdentical(time3, time4));
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromCIM(L"19941210161415.001000+1200"),
                SCXInvalidTimeFormatException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromCIM(L"19941210161415.001000+12"),
                SCXInvalidTimeFormatException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromCIM(L"19941210161415.001000#120"),
                SCXInvalidTimeFormatException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromCIM(L"19941210161415,001000+120"),
                SCXInvalidTimeFormatException);
    }

    void TestGetTimeOfDay() {
        SCXCalendarTime time1(SCXCalendarTime::FromCIM(L"19941210161415.001000+120"));
        SCXRelativeTime time2(time1.GetTimeOfDay());
        SCXRelativeTime time3(0, 0, 0, time1.GetHour(), time1.GetMinute(), time1.GetSecond(),
                time1.GetDecimalCount());
        CPPUNIT_ASSERT(IsIdentical(time2, time3));
    }

    void TestSetTimeOfDay() {
        SCXCalendarTime time1(1994, 12, 10, 16, 14, 15, 0, SCXRelativeTime());
        time1.SetTimeOfDay(SCXRelativeTime(0, 0, 0, 12, 13, 14.001, 3));
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(1994, 12, 10, 12, 13, 14.001, 3, SCXRelativeTime()));
    }

    void TestCalendarTimeToBasicISO8601() {
        SCXCalendarTime time3(1994, 12, 10, 16, 14, 15.001, 3, SCXRelativeTime().SetHours(2));
        wstring time3Str(time3.ToBasicISO8601());
        CPPUNIT_ASSERT(time3Str == L"19941210161415,001+02");
        SCXCalendarTime time0(1994, 02, 01, 06, 04, 05, 0, SCXRelativeTime().SetHours(2));
        wstring time0Str(time0.ToBasicISO8601());
        CPPUNIT_ASSERT(time0Str == L"19940201060405+02");
        SCXCalendarTime timeUTC(1994, 12, 10, 16, 14, 15, 0, SCXRelativeTime());
        wstring timeUTCStr(timeUTC.ToBasicISO8601());
        CPPUNIT_ASSERT(timeUTCStr == L"19941210161415Z");
        SCXCalendarTime time2(1994, 02, 01, 06, 04, 05, 2, SCXRelativeTime().SetHours(-2).SetMinutes(-40));
        wstring time2Str(time2.ToBasicISO8601());
        CPPUNIT_ASSERT(time2Str == L"19940201060405,00-0240");
    }

    void TestCalendarTimeToExtendedISO8601() {
        SCXCalendarTime time3(1994, 12, 10, 16, 14, 15.001, 3, SCXRelativeTime().SetHours(2));
        wstring time3Str(time3.ToExtendedISO8601());
        CPPUNIT_ASSERT(time3Str == L"1994-12-10T16:14:15,001+02");
        SCXCalendarTime time0(1994, 02, 01, 06, 04, 05, 0, SCXRelativeTime().SetHours(2));
        wstring time0Str(time0.ToExtendedISO8601());
        CPPUNIT_ASSERT(time0Str == L"1994-02-01T06:04:05+02");
        SCXCalendarTime timeUTC(1994, 12, 10, 16, 14, 15, 0, SCXRelativeTime());
        wstring timeUTCStr(timeUTC.ToExtendedISO8601());
        CPPUNIT_ASSERT(timeUTCStr == L"1994-12-10T16:14:15Z");
        SCXCalendarTime time2(1994, 02, 01, 06, 04, 05, 2, SCXRelativeTime().SetHours(-2).SetMinutes(-40));
        wstring time2Str(time2.ToExtendedISO8601());
        CPPUNIT_ASSERT(time2Str == L"1994-02-01T06:04:05,00-02:40");
    }

    void TestCalendarTimeToLocalizedTime() {
#if defined(WIN32)
        SCXUNIT_WARNING(L"No sanity check of localized time on windows (LC_TIME env var unsupported)");
#else
        if ( s_fTravis )
        {
            SCXUNIT_WARNING(L"Skipping test SCXTimeTest::TestCalendarTimeToLocalizedTime on TRAVIS");
            return;
        }

        SCXCalendarTime now(SCXCalendarTime::CurrentLocal());
        SCXCalendarTime time1(now.GetYear(), now.GetMonth(), now.GetDay(), 12, 01, 0, now.GetOffsetFromUTC());
        char *currentLocale = setlocale(LC_TIME, NULL);

        putenv(const_cast<char *>("LC_TIME=POSIX"));
        wstring time1Str(time1.ToLocalizedTime());

        wstringstream ss;
        ss << setfill(L'0') << setw(2) << now.GetMonth() << L"/" << setw(2) << now.GetDay() << L"/";
        ss << setw(2) << (now.GetYear() - 2000) << L" 12:01:00";

        CPPUNIT_ASSERT(time1Str == ss.str());

        // Restore the LC_TIME environment variable
        putenv(const_cast<char *>("LC_TIME"));

        // Restore locale
        setlocale(LC_TIME, currentLocale);
#endif
    }

    void TestFromISO8601() {
        SCXCalendarTime time1(SCXCalendarTime::FromISO8601(L"2001-02-03T04:05:06,123456+07:30"));
        CPPUNIT_ASSERT(time1 == SCXCalendarTime(2001, 02, 03, 04, 05, 6.123456, 6,
                SCXRelativeTime().SetHours(7).SetMinutes(30)));
        SCXCalendarTime time2(SCXCalendarTime::FromISO8601(L"20010203T040506,123456+07:30"));
        CPPUNIT_ASSERT(time2 == SCXCalendarTime(2001, 02, 03, 04, 05, 6.123456, 6,
                SCXRelativeTime().SetHours(7).SetMinutes(30)));
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromISO8601(L"2001-02T040506,123456+07:30"), SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromISO8601(L"200102T040506,123456+07:30"), SCXInvalidTimeFormatException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromISO8601(L"2001:02:03T040506,123456+07:30"), SCXInvalidTimeFormatException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromISO8601(L"2001T040506,123456+07:30"), SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromISO8601(L"2001-02-03T04-05-06,123456+07:30"), SCXInvalidTimeFormatException);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromISO8601(L"2001-02-03T04:05:06,1234567+07:30"), SCXNotSupportedException);
        SCXCalendarTime time3(SCXCalendarTime::FromISO8601(L"2001-02-03T04:05:06.125-07"));
        CPPUNIT_ASSERT(time3 == SCXCalendarTime(2001, 02, 03, 04, 05, 6.125, 3,
                SCXRelativeTime().SetHours(-7)));
        SCXCalendarTime time4(SCXCalendarTime::FromISO8601(L"2001-02-03T04:05:06.125-07:30"));
        CPPUNIT_ASSERT(time4 == SCXCalendarTime(2001, 02, 03, 04, 05, 6.125, 3,
                SCXRelativeTime().SetHours(-7).SetMinutes(-30)));
        SCXCalendarTime time5(SCXCalendarTime::FromISO8601(L"2001-02-03T04:05:06.125Z"));
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2001, 02, 03, 04, 05, 6.125, 3,
                SCXRelativeTime()));

    }

    void TestCalendarTimeDecimalCount() {
        SCXCalendarTime time(1994, 12, 10, 16, 14, 15, 0, SCXRelativeTime());
        CPPUNIT_ASSERT(time.GetDecimalCount() == 0);
        time.SetDecimalCount(2);
        CPPUNIT_ASSERT(time.GetDecimalCount() == 2);
        wstring timeStr(time.ToBasicISO8601());
        CPPUNIT_ASSERT(timeStr == L"19941210161415,00Z");
    }

    void TestRelativeTimeAllowsAucklandNZ() {
        // Auckland, NZ is an oddity: During DST, it is 13 hours ahead of UTC
        // (most everything else is <= UTC+12).  Verify this is actually allowed.

        SCXRelativeTime time1(0, 0, 0, 13, 0, 0.0);
        CPPUNIT_ASSERT( time1.IsValidAsOffsetFromUTC() );
    }

    void TestRelativeTimeToBasicISO8601Time() {
        SCXRelativeTime time1(0, 0, 0, 3, 4, 5.2, 2);
        wstring time1Str(time1.ToBasicISO8601Time());
        CPPUNIT_ASSERT(time1Str == L"030405,20");
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(1, 0, 0, 3, 4, 5.2, 2).ToBasicISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 1, 0, 3, 4, 5.2, 2).ToBasicISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 1, 3, 4, 5.2, 2).ToBasicISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 0, -1, 4, 5.2, 2).ToBasicISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 0, 1, -1, 5.2, 2).ToBasicISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 0, 1, 2, -1, 2).ToBasicISO8601Time(), SCXInternalErrorException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestRelativeTimeToExtendedISO8601Time() {
        SCXRelativeTime time1(0, 0, 0, 3, 4, 5.2, 2);
        wstring time1Str(time1.ToExtendedISO8601Time());
        CPPUNIT_ASSERT(time1Str == L"03:04:05,20");
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(1, 0, 0, 3, 4, 5.2, 2).ToExtendedISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 1, 0, 3, 4, 5.2, 2).ToExtendedISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 1, 3, 4, 5.2, 2).ToExtendedISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 0, -1, 4, 5.2, 2).ToExtendedISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 0, 1, -1, 5.2, 2).ToExtendedISO8601Time(), SCXInternalErrorException);
        CPPUNIT_ASSERT_THROW(SCXRelativeTime(0, 0, 0, 1, 2, -1, 2).ToExtendedISO8601Time(), SCXInternalErrorException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestPosixTime() {
        SCXCalendarTime time1(1971, 1, 1, 0, 0, 0, SCXRelativeTime());
        CPPUNIT_ASSERT(time1.ToPosixTime() == 86400 * 365);
        SCXCalendarTime time2(SCXCalendarTime::FromPosixTime(86400 * 365));
        CPPUNIT_ASSERT(time1 == time2);
        CPPUNIT_ASSERT_THROW(SCXCalendarTime::FromPosixTime(-86400 * 365), SCXNotSupportedException);
    }

    void TestMakeLocal() {
        SCXCalendarTime time1(2007,11,28,23,39,0, SCXRelativeTime().SetMinutes(-300));
        SCXCalendarTime time2(time1);
        time2.MakeLocal(SCXRelativeTime().SetMinutes(-300));

        CPPUNIT_ASSERT(time1 == time2);

        SCXCalendarTime time3(2007,11,28,23,39,0, SCXRelativeTime().SetMinutes(300));
        SCXCalendarTime time4(time3);
        time4.MakeLocal(SCXRelativeTime().SetMinutes(300));
        CPPUNIT_ASSERT(time3 == time4);

        SCXCalendarTime time5(2006,03,20,22,0,0, SCXRelativeTime().SetHours(-2));
        time5.MakeLocal(SCXRelativeTime().SetHours(2));
        CPPUNIT_ASSERT(time5 == SCXCalendarTime(2006,03,21, 2, 0, 0, SCXRelativeTime().SetHours(2)));

        SCXCalendarTime time6(2006,03,20,4,0,0, SCXRelativeTime().SetHours(4));
        time6.MakeLocal(SCXRelativeTime().SetHours(-3));
        CPPUNIT_ASSERT(time6 == SCXCalendarTime(2006,03,19, 21, 0, 0, SCXRelativeTime().SetHours(-3)));
    }

    void TestMakeLocalNoParam() {
        // GMT: Mon, 19 Jan 2015 03:55:06 GMT
        // Your time zone: 1/18/2015, 7:55:06 PM GMT-8:00
        SCXCalendarTime time1 = SCXCalendarTime::FromPosixTime(1421639706);
        time1.MakeLocal();
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The offset from utc was not set correctly. Make sure the machine is in the PST timezone",
            -480, time1.GetOffsetFromUTC().GetMinutes());
        CPPUNIT_ASSERT_EQUAL((scxyear) 2015, time1.GetYear());
        CPPUNIT_ASSERT_EQUAL((scxmonth) 1, time1.GetMonth());
        CPPUNIT_ASSERT_EQUAL((scxday) 18, time1.GetDay());
        CPPUNIT_ASSERT_EQUAL((scxhour) 19, time1.GetHour());
        CPPUNIT_ASSERT_EQUAL((scxminute) 55, time1.GetMinute());
    }

    void TestMakeLocalNoParamDST() {
        // GMT: Thu, 14 May 2015 18:40:33 GMT
        // Your time zone: 5/14/2015, 11:40:33 AM GMT-7:00 DST
        SCXCalendarTime time1 = SCXCalendarTime::FromPosixTime(1431628833);
        time1.MakeLocal();
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The offset from utc was not set correctly. Make sure the machine is in the PST timezone",
            -420, time1.GetOffsetFromUTC().GetMinutes());
        CPPUNIT_ASSERT_EQUAL((scxyear) 2015, time1.GetYear());
        CPPUNIT_ASSERT_EQUAL((scxmonth) 5, time1.GetMonth());
        CPPUNIT_ASSERT_EQUAL((scxday) 14, time1.GetDay());
        CPPUNIT_ASSERT_EQUAL((scxhour) 11, time1.GetHour());
        CPPUNIT_ASSERT_EQUAL((scxminute) 40, time1.GetMinute());
    }

    std::string PosixToISO8601(time_t ut) {
        SCXCoreLib::SCXCalendarTime c2(SCXCoreLib::SCXCalendarTime::FromPosixTime(ut));
        std::string iso8601(SCXCoreLib::StrToUTF8(c2.ToExtendedISO8601()));
        return(iso8601);
    }

    scxlong ISO8601ToPosix(std::string iso8601)
    {
        SCXCoreLib::SCXCalendarTime c3(SCXCoreLib::SCXCalendarTime::FromISO8601(SCXCoreLib::StrFromUTF8(iso8601)));
        return(c3.ToPosixTime());
    }

    void TestWI3245() {
        time_t ut;
        time(&ut);
        std::string isoTime(PosixToISO8601(ut));
        CPPUNIT_ASSERT(ISO8601ToPosix(isoTime) == ut);
}

    void TestWI7268(){
#if defined(SCX_UNIX)
        struct timeval realtime;

        gettimeofday(&realtime, 0);

        SCXCoreLib::SCXCalendarTime testdate = SCXCalendarTime::FromPosixTime(realtime.tv_sec);

        SCXAmountOfTime usec;

        usec.SetSeconds(static_cast<SCXCoreLib::scxseconds>(realtime.tv_usec / 1000000));

        testdate += usec;

        SCXRelativeTime offset = SCXCalendarTime::CurrentOffsetFromUTC();


        testdate.MakeLocal(offset);

        SCXCoreLib::SCXCalendarTime pre_fork = SCXCoreLib::SCXCalendarTime::CurrentLocal();


        if ( pre_fork.ToBasicISO8601().substr(0,14) != testdate.ToBasicISO8601().substr(0,14) ){
            wcout << endl << L"CurrentOffset from UTC = " << offset.DumpString() << endl;
            wcout << endl << L"testZombie() - pre_fork_time1: " << pre_fork.ToBasicISO8601() << endl;
            wcout << L"testZombie() - pre_fork_time2: " << testdate.ToBasicISO8601() << endl;
        }

        CPPUNIT_ASSERT( pre_fork.ToBasicISO8601().substr(0,14) == testdate.ToBasicISO8601().substr(0,14) );
#endif
    }

    void TestWI7350(){
        // This problem was discovered by chance; turned out that any date (2008-06-23,05:09:50) is not less than
        // date (2008-06-23,05:11:30) as expected;
        //
        // real problem is inside function "ToComparablePseudoMicrosecond", since it converts date/time to sxculong incorrectly

        SCXCoreLib::SCXCalendarTime t1( 2008, 06, 23, 5, 9, 50, SCXRelativeTime( 0,0,0,-7,0,0,0) );
        SCXCoreLib::SCXCalendarTime t2( 2008, 06, 23, 5, 11, 30, SCXRelativeTime( 0,0,0,-7,0,0,0) );

        CPPUNIT_ASSERT( t1 < t2 );
    }

    void TestPrecisionSetAndGet()
    {
        SCXCoreLib::SCXCalendarTime t(2007, 11, 8, 14, 39, 42, SCXRelativeTime( 0,0,0,0,0,0,0) );
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionSecond, t.GetPrecision());
        t.SetPrecision(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionHour);
        CPPUNIT_ASSERT_EQUAL(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionHour, t.GetPrecision());
    }

    void TestCompareWithYearOnly()
    {
        SCXCoreLib::SCXCalendarTime t_sec( 2007, 11, 8, 14, 39, 42, SCXRelativeTime( 0,0,0,0,0,0,0) );
        SCXCoreLib::SCXCalendarTime t_year( 2007, 4, 2 );
        t_year.SetPrecision(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionYear);

        CPPUNIT_ASSERT(t_sec == t_year);
    }

    void TestCompareWithYearAndMonthOnly()
    {
        SCXCoreLib::SCXCalendarTime t_sec( 2007, 11, 8, 14, 39, 42, SCXRelativeTime( 0,0,0,0,0,0,0) );
        SCXCoreLib::SCXCalendarTime t_yearMonth( 2007, 11, 2 );
        t_yearMonth.SetPrecision(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionMonth);

        CPPUNIT_ASSERT(t_sec == t_yearMonth);
    }

    void TestCompareWithDateOnly()
    {
        SCXCoreLib::SCXCalendarTime t_sec( 2007, 11, 8, 14, 39, 42, SCXRelativeTime( 0,0,0,0,0,0,0) );
        SCXCoreLib::SCXCalendarTime t_date( 2007, 11, 8 );
        t_date.SetPrecision(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionDay);

        CPPUNIT_ASSERT(t_sec == t_date);
    }

    void TestCompareWithDateAndHourOnly()
    {
        SCXCoreLib::SCXCalendarTime t_sec( 2007, 11, 8, 14, 39, 42, SCXRelativeTime( 0,0,0,0,0,0,0) );
        SCXCoreLib::SCXCalendarTime t_hour( 2007, 11, 8, 14, 00, 00, SCXRelativeTime( 0,0,0,0,0,0,0) );
        t_hour.SetPrecision(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionHour);

        CPPUNIT_ASSERT(t_sec == t_hour);
    }

    void TestCompareWithDateHourAndMinuteOnly()
    {
        SCXCoreLib::SCXCalendarTime t_sec( 2007, 11, 8, 14, 39, 42, SCXRelativeTime( 0,0,0,0,0,0,0) );
        SCXCoreLib::SCXCalendarTime t_hm( 2007, 11, 8, 14, 39, 0, SCXRelativeTime( 0,0,0,0,0,0,0) );
        t_hm.SetPrecision(SCXCoreLib::SCXCalendarTime::eCalendarTimePrecisionYear);

        CPPUNIT_ASSERT(t_sec == t_hm);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXTimeTest );
