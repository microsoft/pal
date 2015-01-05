/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Representation and caclulation concerning relative amount of time
    
    \date        07-10-29 14:00:00
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxdumpstring.h>

#include <iostream>
#include <iomanip>

namespace SCXCoreLib {

/*----------------------------------------------------------------------------*/
//! Constructs a relative time
//! \param[in]        years         Unrestricted number of years, positive as well as negative
//! \param[in]        months        Unrestricted number of months, may be > 12 as well as negative
//! \param[in]        days          Unrestricted number of days, may be > 31 as well as negative
//! \param[in]        hours         Unrestricted number of hours, may be > 24 as well as negative
//! \param[in]        minutes       Unrestricted number of minutes, may be > 60 as well as negative
//! \param[in]        seconds       Unrestricted number of seconds, may be > 60 as well as negative
//! \param[in]        decimalCount  Number of significant digits 
//! \note Parts of seconds are represented as the fraction part of seconds
SCXRelativeTime::SCXRelativeTime(int years, int months, int days, int hours, int minutes, double seconds, unsigned decimalCount) 
        : m_years(years), m_months(months), m_days(days), 
          m_hours(hours), m_minutes(minutes), m_microseconds(RoundToInt(seconds * 1000000)), 
          m_decimalCount(decimalCount) {
    
}

/*----------------------------------------------------------------------------*/
//! Constructs a relative time
//! \param[in]        years         Unrestricted number of years, positive as well as negative
//! \param[in]        months        Unrestricted number of months, may be > 12 as well as negative
//! \param[in]        days          Unrestricted number of days, may be > 31 as well as negative
//! \param[in]        hours         Unrestricted number of hours, may be > 24 as well as negative
//! \param[in]        minutes       Unrestricted number of minutes, may be > 60 as well as negative
//! \param[in]        seconds       Unrestricted number of seconds, may be > 60 as well as negative
//! \note Parts of seconds are represented as the fraction part of seconds,
//!       the precision is expected to be the maximally supported
SCXRelativeTime::SCXRelativeTime(int years, int months, int days, int hours, int minutes, double seconds) 
        : m_years(years), m_months(months), m_days(days), 
          m_hours(hours), m_minutes(minutes), m_microseconds(RoundToInt(seconds * 1000000)), 
          m_decimalCount(6) {
    
}

/*----------------------------------------------------------------------------*/
//! Checks if two relative times conveys identical information
//! \param[in]  time1   To be compared
//! \param[in]  time2   To be compared
//! \returns true iff the times are unitwise identical
//! \note The information is not canoninized, 1 hour is not identical to 60 minutes
bool IsIdentical(const SCXRelativeTime &time1, const SCXRelativeTime &time2) {
    return time1.m_years == time2.m_years 
        && time1.m_months == time2.m_months
        && time1.m_days == time2.m_days 
        && time1.m_hours == time2.m_hours
        && time1.m_minutes == time2.m_minutes 
        && time1.m_microseconds == time2.m_microseconds
        && time1.m_decimalCount == time2.m_decimalCount;
}

/*----------------------------------------------------------------------------*/
//! Check if two times are equal
//! \param[in] time1  To be compared   
//! \param[in] time2  To be compared
//! \returns true iff the times are equal
bool operator==(const SCXRelativeTime &time1, const SCXRelativeTime &time2) {
    return IsIdentical(time1, time2);
}

/*----------------------------------------------------------------------------*/
//! Check if two times are not equal
//! \param[in] time1  To be compared   
//! \param[in] time2  To be compared
//! \returns true iff the times are not equal
bool operator!=(const SCXRelativeTime &time1, const SCXRelativeTime &time2) {
    return !(time1 == time2);
}

/*----------------------------------------------------------------------------*/
//! Adds a relative amount of time
//! \param[in]    amount    To be added
//! \returns    *this;
//! \note Adding an amount with fewer significant digits lowers
//!       the number of significant digits of the current amount as well.
SCXRelativeTime& SCXRelativeTime::operator+=(const SCXRelativeTime &amount) {
    m_years += amount.m_years;
    m_months += amount.m_months;
    m_days += amount.m_days;
    m_hours += amount.m_hours;
    m_minutes += amount.m_minutes;
    m_microseconds += amount.m_microseconds;
    m_decimalCount = std::min(m_decimalCount, amount.m_decimalCount);
    return *this;
}

/*----------------------------------------------------------------------------*/
//! Subtract a relative amount of time
//! \param[in]    amount    To be subtracted
//! \returns    *this;
//! \note Subtracting an amount with fewer significant digits lowers
//!       the number of significant digits of the current amount as well.
SCXRelativeTime& SCXRelativeTime::operator-=(const SCXRelativeTime &amount) {
    m_years -= amount.m_years;
    m_months -= amount.m_months;
    m_days -= amount.m_days;
    m_hours -= amount.m_hours;
    m_minutes -= amount.m_minutes;
    m_microseconds -= amount.m_microseconds;        
    m_decimalCount = std::min(m_decimalCount, amount.m_decimalCount);
    return *this;
}

/*----------------------------------------------------------------------------*/  
//! Constructs a string representing the content, suitable for debugging
//! \returns Texual representation of content
std::wstring SCXRelativeTime::DumpString() const {
        return SCXDumpStringBuilder("SCXRelativeTime")
            .Scalar("years", m_years) 
            .Scalar("months", m_months)
            .Scalar("days", m_days)
            .Scalar("hours", m_hours)
            .Scalar("minutes", m_minutes)
            .Scalar("microseconds", m_microseconds);             

}

/*----------------------------------------------------------------------------*/
//! Seconds and parts of the seconds
//! \returns Seconds of time
//! \note Parts of seconds are represented as the decimal part of seconds.
//!       Example: 3 seconds and 5 miliseconds is represented as 3.005
scxseconds SCXRelativeTime::GetSeconds() const {
    return static_cast<scxseconds>(m_microseconds) / 1000000;
}

/*----------------------------------------------------------------------------*/
//! Number of significant decimals
//! \returns Number of significant decimals
scxdecimalnr SCXRelativeTime::GetDecimalCount() const {
    return m_decimalCount;
}

/*----------------------------------------------------------------------------*/
//! Change number of seconds
//! \param[in]    seconds        Amount of seconds
//! \note Parts of seconds are represented as the decimal part of seconds.
//!       Example: 3 seconds and 5 miliseconds is represented as 3.005
SCXRelativeTime &SCXRelativeTime::SetSeconds(scxseconds seconds) {
    m_microseconds = RoundToScxLong(seconds * 1000000);
    return *this;
}

/*----------------------------------------------------------------------------*/
//! Change information about number of significant decimals
//! \param[in]   decimalCount                    New number of significant decimals
//! \throws         SCXIllegalArgumentException     Invalid number of decimals 
//! Determines for example the default number of decimals when converting to
//! a decimal textual format.
//! Needed because the binary representation of, for example, 3 and 3.00 is the same.
//! \note Does not change the relative time, just information _about_ the time. Lowering
//! the number of significant digits does not remove any digits of time, in the same way
//! as increasing the number of significant digits does not add any new digits of time.
//! \note The current implementation support up to and including 6 decimals
SCXRelativeTime &SCXRelativeTime::SetDecimalCount(scxdecimalnr decimalCount) {
    if (!IsAscending(0, decimalCount, 6)) {
        throw SCXIllegalIndexException<scxminute>(L"decimalCount", decimalCount, 0, true, 6, true, SCXSRCLOCATION);
    }
    m_decimalCount = decimalCount;
    return *this;
}

/*----------------------------------------------------------------------------*/
//! Check if the amount of time may be used as offset from UTC
//! \returns If or not the amount may be used as offset from UTC
bool SCXRelativeTime::IsValidAsOffsetFromUTC() const {
    return m_years == 0 && m_months == 0 && m_days == 0 && m_microseconds == 0
        && IsAscending(-13 * 60, m_hours * 60 + m_minutes, 13 * 60);
}

/*----------------------------------------------------------------------------*/
//! Formats relative time according to basic ISO8601
//! \returns Textual format according to basic IS8601 time
//! \throws     SCXInternalErrorException    Years, month or days are not zero    
//! \throws     SCXInternalErrorException    Hours, minutes or seconds are negative    
//! 
//! http://en.wikipedia.org/wiki/ISO_8601
std::wstring SCXRelativeTime::ToBasicISO8601Time() const {
    return ToISO8601Time(L"");
}

/*----------------------------------------------------------------------------*/    
//! Formats relative time according to extended ISO8601
//! \returns Textual format according to extended IS8601 time
//! \throws     SCXInternalErrorException    Years, month or days are not zero    
//! \throws     SCXInternalErrorException    Hours, minutes or seconds are negative    
//! 
//! http://en.wikipedia.org/wiki/ISO_8601
std::wstring SCXRelativeTime::ToExtendedISO8601Time() const {
    return ToISO8601Time(L":");
}

/*----------------------------------------------------------------------------*/
//! Internal helper function that formats relative time according to ISO8601 time
//! \param[in]    timeSeparator    L":" or empty string
//! \throws     SCXInternalErrorException    Years, month or days are not zero    
//! \throws     SCXInternalErrorException    Hours, minutes or seconds are negative    
//! \returns Textual format according to IS8601 time
//! 
//! http://en.wikipedia.org/wiki/ISO_8601
std::wstring SCXRelativeTime::ToISO8601Time(std::wstring timeSeparator) const {
    if (m_years != 0 || m_months != 0 || m_days != 0) {
        throw SCXInternalErrorException(L"Years, months or days cannot be part of a time", SCXSRCLOCATION);
    }
    if (m_hours < 0 || m_minutes < 0 || m_microseconds < 0) {
        throw SCXInternalErrorException(L"Negative hours, minutes or seconds cannot be part of time", SCXSRCLOCATION);            
    }
    SCXASSERT(timeSeparator == L"" || timeSeparator == L":");    
    std::wostringstream buf;
    buf << std::setw(2) << std::setfill(L'0') << m_hours;
    buf << timeSeparator << std::setw(2) << std::setfill(L'0') << m_minutes;
    buf << timeSeparator << std::setw(2) << std::setfill(L'0') << (m_microseconds / 1000000);
    int decimalsToRemove = std::max(0, 6 - static_cast<int>(m_decimalCount));
    scxlong microsecond = (m_microseconds % 1000000) / Pow(10, decimalsToRemove);
    if (m_decimalCount > 0) {
        buf << "," << std::setw(m_decimalCount) << std::setfill(L'0') << microsecond;
    }
    return buf.str();        
} 

/*----------------------------------------------------------------------------*/
//! Calculates the negation of the current relative amount of time
//! \param[in]        amount        To be negated
//! \returns    Positive amount if current is negative, and vice versa
//! \note         Negates each unit of time separately
SCXRelativeTime operator-(const SCXRelativeTime &amount)  {
    return SCXRelativeTime() - amount;
}

/*----------------------------------------------------------------------------*/
//! Calculates the sum of two relative amount of times
//! \param[in]    amount1        To be added
//! \param[in]    amount2        To be added
//! \returns    The unitwise sum of the amounts
//! \note The number of significant digits of the sum cannot be greater
//!       than the lowest number of significant digits of the individual amounts
SCXRelativeTime operator+(const SCXRelativeTime &amount1, const SCXRelativeTime &amount2) {
    return SCXRelativeTime(amount1) += amount2;
}

/*----------------------------------------------------------------------------*/    
//! Calculates the difference of two relative amount of times
//! \param[in]    amount1        To be subtracted from
//! \param[in]    amount2        To be subtracted
//! \returns    The unitwise difference of the amounts
//! \note The number of significant digits of the difference cannot be greater
//!       than the lowest number of significant digits of the individual amounts
SCXRelativeTime operator-(const SCXRelativeTime &amount1, const SCXRelativeTime &amount2) {
    return SCXRelativeTime(amount1) -= amount2;        
}


}


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
