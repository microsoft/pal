/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Base functionality concerning time
    
    \date        07-10-29 14:00:00
    
    Month and day numbers are 1 based, that is, the first month == 1 and
    the first day == 1. Month == 0 and day == 0 is interpreted as the
    last month of the previous year, and the last day of the previous month, 
    respectively.     
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>


#include "primitives.h"
#include <scxcorelib/stringaid.h>

namespace SCXCoreLib {

/*----------------------------------------------------------------------------*/
//! Timestamp representing the start of U**x epoch, Posix time == 0
const SCXCalendarTime SCXCalendarTime::cUnixEpoch(1970, 1, 1, 0, 0, 0, SCXRelativeTime());

/*----------------------------------------------------------------------------*/
std::wstring SCXInvalidTimeFormatException::What() const {
    return m_problem + L"(" + m_invalidText + L")";
}

/*----------------------------------------------------------------------------*/
//! Checks whether a year is a leap year or not
//! \param[in]  year    To be checked
//! \returns    true iff leap year
bool IsLeapYear(scxyear year) {
    return year % 400 == 0 || (year % 100 != 0 && year % 4 == 0);
}    

/*----------------------------------------------------------------------------*/
//! Number of days in month
//! \param[in]  year    Year of month
//! \param[in]  month   Month of interest
//! \returns    Number of days in month of year
//! \throws     SCXInvalidArgumentException     When month is ouside range
unsigned DaysInMonth(scxyear year, scxmonth month) {
    switch (month) {
    case 1:
        return 31;
    case 2:
        return IsLeapYear(year)? 29: 28;
    case 3:
        return 31;
    case 4:
        return 30;
    case 5:
        return 31;
    case 6:
        return 30;
    case 7:
        return 31;
    case 8:
        return 31;
    case 9:
        return 30;
    case 10:
        return 31;
    case 11:
        return 30;
    case 12:
        return 31;
    default:
        throw SCXInvalidArgumentException(L"month", StrFrom(month), SCXSRCLOCATION);
    }
}
 
/*----------------------------------------------------------------------------*/
//! Calculate month prior to specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of interest, first month == 1
void CalculatePriorMonth(scxyear &year, scxmonth &month) {
    if (month <= 1) {
        month += 12 - 1;
        --year;
    } else {
        --month;
    }
}

/*----------------------------------------------------------------------------*/
//! Calculate month after specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of interest, first month == 1
void CalculateNextMonth(scxyear &year, scxmonth &month) {
    if (month >= 12) {
        month -= 12 - 1;
        ++year;
    } else {
        ++month;
    }
}

/*----------------------------------------------------------------------------*/
//! Calculate day prior to specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of day
//! \param[in,out]  day     Day of interest
void CalculatePriorDay(scxyear &year, scxmonth &month, scxday &day) {
    if (day <= 1) {
        CalculatePriorMonth(year, month);
        day += DaysInMonth(year, month) - 1;
    } else {
        --day;
    }
}

/*----------------------------------------------------------------------------*/
//! Calculate day after specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of day
//! \param[in,out]  day     Day of interest
void CalculateNextDay(scxyear &year, scxmonth &month, scxday &day) {        
    unsigned int daysInMonth = DaysInMonth(year, month);
    if (day >= daysInMonth) {
        day -= daysInMonth - 1;
        CalculateNextMonth(year, month);
    } else {
        ++day;
    }
}

/*----------------------------------------------------------------------------*/
//! Calculate hour prior to specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of day
//! \param[in,out]  day     Day of hour
//! \param[in,out]  hour    Hour of interest
void CalculatePriorHour(scxyear &year, scxmonth &month, scxday &day, scxhour &hour) {
    if (hour == 0) {
        hour += 24 - 1;
        CalculatePriorDay(year, month, day);
    } else {
        --hour;
    }
}

/*----------------------------------------------------------------------------*/
//! Calculate hour after specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of day
//! \param[in,out]  day     Day of hour
//! \param[in,out]  hour    Hour of interest
void CalculateNextHour(scxyear &year, scxmonth &month, scxday &day, scxhour &hour) {
    if (hour >= 23) {
        hour -= 24 - 1;
        CalculateNextDay(year, month, day);
    } else {
        ++hour;
    }
}

/*----------------------------------------------------------------------------*/
//! Calculate minute prior to specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of day
//! \param[in,out]  day     Day of hour
//! \param[in,out]  hour    Hour of minute
//! \param[in,out]  minute  minute of interest
void CalculatePriorMinute(scxyear &year, scxmonth &month, scxday &day, scxhour &hour, scxminute &minute) {
    if (minute == 0) {
        minute += 60 - 1;
        CalculatePriorHour(year, month, day, hour);
    } else {
        --minute;
    }
}

/*----------------------------------------------------------------------------*/
//! Calculate minute after specified
//! \param[in,out]  year    Year of month
//! \param[in,out]  month   Month of day
//! \param[in,out]  day     Day of hour
//! \param[in,out]  hour    Hour of minute
//! \param[in,out]  minute  minute of interest
void CalculateNextMinute(scxyear &year, scxmonth &month, scxday &day, scxhour &hour, scxminute &minute) {
    if (minute >= 59) {
        minute -= 60 - 1;
        CalculateNextHour(year, month, day, hour);
    } else {
        ++minute;
    }
}

/*----------------------------------------------------------------------------*/
//! Number of days in year
//! \param[in]  year   Year of interest
//! \returns Number of days in "year"     
unsigned DaysInYear(scxyear year) {
    return IsLeapYear(year) ? 366 : 365;
}

/*----------------------------------------------------------------------------*/
//! Number of hours in day
//! \returns Number of hours in "day"
//! \note Currenly it does not take daylight saving into account, may be changed.
unsigned HoursInDay(scxyear, scxmonth, scxday) {
    return 24;
}

/*----------------------------------------------------------------------------*/
//! Number of minutes in day
//! \param[in]  year   Year of month
//! \param[in]  month  Month of day
//! \param[in]  day     Day of interest
//! \returns Number of minutes in "day"     
unsigned MinutesInDay(scxyear year, scxmonth month, scxday day) {
    return HoursInDay(year, month, day) * 60;
}

/*----------------------------------------------------------------------------*/
//! Number of minutes in prior day
//! \param[in]  year   Year of month
//! \param[in]  month  Month of day
//! \param[in]  day     Day of interest
//! \returns Number of minutes in day prior to "day"     
unsigned MinutesInPriorDay(scxyear year, scxmonth month, scxday day) {
    CalculatePriorDay(year, month, day);
    return MinutesInDay(year, month, day);
}


/*----------------------------------------------------------------------------*/
//! Number of days in prior month
//! \param[in]  year   Year of month
//! \param[in]  month  Month of interest
//! \returns Number of days in month prior to "month"     
unsigned DaysInPriorMonth(scxyear year, scxmonth month) {
    CalculatePriorMonth(year, month);
    return DaysInMonth(year, month);
}

/*----------------------------------------------------------------------------*/
//! Number of hours in month
//! \param[in]  year   Year of month
//! \param[in]  month  Month of interest
//! \returns    Number of hours in month     
//! \note Currenly it does not take daylight saving into account, may be changed.
unsigned HoursInMonth(scxyear year, scxmonth month) {
    return 24 * DaysInMonth(year, month);
}

/*----------------------------------------------------------------------------*/
//! Number of hours in prior month
//! \param[in]  year   Year of month
//! \param[in]  month  Month of interest
//! \returns Number of hours in month prior to "month"  
//! \note Currenly it does not take daylight saving into account, may be changed.
unsigned HoursInPriorMonth(scxyear year, scxmonth month) {
    CalculatePriorMonth(year, month);
    return HoursInMonth(year, month);
}

/*----------------------------------------------------------------------------*/
//! Number of hours in year
//! \param[in]  year   Year of interest
//! \returns    Number of hours in "year"     
unsigned HoursInYear(scxyear year) {
    return 24 * DaysInYear(year);
}


/*----------------------------------------------------------------------------*/
//! Number of hours in prior day
//! \param[in]  year   Year of month
//! \param[in]  month  Month of day
//! \param[in]  day    Day of interest
//! \returns Number of hours in day prior to "day"     
unsigned HoursInPriorDay(scxyear year, scxmonth month, scxday day) {
    CalculatePriorDay(year, month, day);
    return HoursInDay(year, month, day);
}

/*----------------------------------------------------------------------------*/
//! Number of minutes in month
//! \param[in]  year   Year of month
//! \param[in]  month  Month of interest
//! \returns    Number of minutes in month     
//! \note Currenly it does not take daylight saving into account, may be changed.
unsigned MinutesInMonth(scxyear year, scxmonth month) {
    return DaysInMonth(year, month) * 24 * 60;
}

/*----------------------------------------------------------------------------*/
//! Number of minutes in prior month
//! \param[in]  year   Year of month
//! \param[in]  month  Month of interest
//! \returns    Number of minutes in month prior to "month"     
unsigned MinutesInPriorMonth(scxyear year, scxmonth month) {
    CalculatePriorMonth(year, month);
    return MinutesInMonth(year, month);
}

/*----------------------------------------------------------------------------*/
//! Number of minutes in year
//! \param[in]  year   Year of interest
//! \returns    Number of minutes in year     
unsigned MinutesInYear(scxyear year) {
    return DaysInYear(year) * 24 * 60;
}

/*----------------------------------------------------------------------------*/
//! Number of minutes in hour
//! \returns    Number of minutes in hour     
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
unsigned MinutesInHour(scxyear, scxmonth, scxday, scxhour) {
    return 60;
}

/*----------------------------------------------------------------------------*/
//! Number of minutes in prior hour
//! \returns    Number of minutes in prior hour     
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
unsigned MinutesInPriorHour(scxyear, scxmonth, scxday, scxhour) {
    return 60;
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in minute
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
unsigned MicrosecondsInMinute(scxyear, scxmonth, scxday, scxhour, scxminute) {
    return 60 * 1000 * 1000;
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in hour
//! \returns    Number of microseconds in hour     
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
scxulong MicrosecondsInHour(scxyear, scxmonth, scxday, scxhour) {
    return static_cast<scxulong>(60) * 60 * 1000 * 1000;
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in day
//! \returns    Number of microseconds in day     
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
scxulong MicrosecondsInDay(scxyear, scxmonth, scxday) {
    return static_cast<scxulong>(24) * 60 * 60 * 1000 * 1000;
}
 
/*----------------------------------------------------------------------------*/
//! Number of microseconds in month
//! \param[in]  year    Year of month
//! \param[in]  month   Month of interest
//! \returns    Number of microseconds in month     
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
scxulong MicrosecondsInMonth(scxyear year, scxmonth month) {
    return DaysInMonth(year, month) * (static_cast<scxulong>(24) * 60 * 60 * 1000 * 1000);
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in year
//! \param[in]  year    Year of interest
//! \returns    Number of microseconds in year     
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
scxulong MicrosecondsInYear(scxyear year) {
    return DaysInYear(year) * (static_cast<scxulong>(24) * 60 * 60 * 1000 * 1000);
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in prior minute
//! \returns    Number of microseconds in prior minute     
//! \note Currenly it does not take daylight saving into account, 
//!       all parameters are ignoreed, may be changed.
unsigned MicrosecondsInPriorMinute(scxyear, scxmonth, scxday, scxhour, scxminute) {
    return 60 * 1000 * 1000;
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in prior hour
//! \param[in]  year    Year of month
//! \param[in]  month   Month of day
//! \param[in]  day     Day of hour
//! \param[in]  hour    Hour of minute
//! \returns    Number of microseconds in hour prior to "hour"     
scxulong MicrosecondsInPriorHour(scxyear year, scxmonth month, scxday day, scxhour hour) {
    CalculatePriorHour(year, month, day, hour);
    return MicrosecondsInHour(year, month, day, hour);
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in prior day
//! \param[in]  year    Year of month
//! \param[in]  month   Month of day
//! \param[in]  day     Day of hour
//! \returns    Number of microseconds in day prior to "day"     
scxulong MicrosecondsInPriorDay(scxyear year, scxmonth month, scxday day) {
    CalculatePriorDay(year, month, day);
    return MicrosecondsInDay(year, month, day);
}

/*----------------------------------------------------------------------------*/
//! Number of microseconds in prior month
//! \param[in]  year    Year of month
//! \param[in]  month   Month of interest
//! \returns    Number of microseconds in month prior to "month"     
scxulong MicrosecondsInPriorMonth(scxyear year, scxmonth month) {
    CalculatePriorMonth(year, month);
    return MicrosecondsInMonth(year, month);
}


}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
