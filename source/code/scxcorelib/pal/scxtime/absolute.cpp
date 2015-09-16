/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation of representation and calculation
                 concerning absolute moments in time.

    \date        07-09-25 16:21:00
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/stringaid.h>
#include <math.h>
#include <iostream>
#include <algorithm>
#include <errno.h>
#include <time.h>
#include <sstream>
#include <iomanip>
#include <locale.h>

#if defined(WIN32)
#include <windows.h>

#elif defined(SCX_UNIX)
#include <sys/time.h>
#endif

#include "primitives.h"

using namespace SCXCoreLib;
using namespace std;

namespace {
#if defined(WIN32)

    /*----------------------------------------------------------------------------*/
    //! Copy time information from FILETIME structure into individual variables
    //! \param[in]      filetime            Information to copy
    //! \param[out]     microseconds        Complete microseconds
    //! \param[out]     extraNanoseconds    Part of last noncomplete microsecond
    inline void CopyFromStruct(const _FILETIME filetime, scxulong &microseconds, scxulong &extraNanoseconds) {
        _LARGE_INTEGER nano100s;
        nano100s.LowPart = filetime.dwLowDateTime;
        nano100s.HighPart = filetime.dwHighDateTime;
        microseconds = nano100s.QuadPart / 10;
        extraNanoseconds = (nano100s.QuadPart % 10) * 100;
    }


    /*----------------------------------------------------------------------------*/
    //! Copy the information in the SYSTEMTIME structure into individual variables
    //! \param[in]  time                Structure to copy information from
    //! \param[in]  extraMicroseconds   Extra microsecond to add
    //! \param[out] year                Year copied from structure
    //! \param[out] month               Month copied from structure
    //! \param[out] day                 Day copied from structure
    //! \param[out] hour                Hour copied from structure
    //! \param[out] hour                Hour copied from structure
    //! \param[out] minute              Minute copied from structure
    //! \param[out] microsecond Microsecond calculated from structure
    //! \note Microseconds includes seconds as well as microseconds
    inline void CopyFromStruct(const _SYSTEMTIME &time, scxlong extraMicroseconds,
            scxyear &year, scxmonth &month,
            scxday &day, scxhour &hour, scxminute &minute,
            unsigned int &microsecond) {
        year = time.wYear;
        month = time.wMonth;
        day = time.wDay;
        hour = time.wHour;
        minute = time.wMinute;
        microsecond = static_cast<unsigned>(min(60*1000*1000-1,(time.wSecond * 1000 + time.wMilliseconds) * 1000 + extraMicroseconds));
    }

#endif

#if defined(SCX_UNIX)
    /*----------------------------------------------------------------------------*/
    //! Copy the information in the tm structure into variables
    //! \param[in]  time              Structure to copy information from
    //! \param[in]  timeMicrosecond   Complements missing microseconds in structure
    //! \param[out] year              Year copied from structure
    //! \param[out] month             Month copied from structure
    //! \param[out] day               Day copied from structure
    //! \param[out] hour              Hour copied from structure
    //! \param[out] hour              Hour copied from structure
    //! \param[out] minute            Minute copied from structure
    //! \param[out] microsecond       Microsecond calculated from structure
    //! \note Microseconds includes seconds as well as microseconds
    inline void CopyFromStruct(const tm &time, const int timeMicrosecond, scxyear &year,
            scxmonth &month, scxday &day, scxhour &hour, scxminute &minute,
            unsigned int &microsecond) {
        year = time.tm_year + 1900;
        month = time.tm_mon + 1;
        day = time.tm_mday;
        hour = time.tm_hour;
        minute = time.tm_min;
        microsecond = time.tm_sec * 1000000 + timeMicrosecond;
    }

#endif

}

namespace SCXCoreLib {

    /*----------------------------------------------------------------------------*/
    //! Convert the time to a representation useful for comparison. The pseudo
    //! microsecond is only valid to compare with other pseudo microseconds created
    //! with this method. The purpose of the method is to make implementation of
    //! comparison straightforward and simple.
    //! To convert a number of parameters to a single number
    //! "round" numbers are used for efficiency, like "16" for month, 32 for days/hours etc.
    //! "1048576" (or 2^20) is used for microseconds and extra "64" is used to account "seconds"
    //! within microseconds, since valid range for m_microsecond is (0..59 999 999) not (0..999 999).
    //! \returns    A number of microseconds that might be individually compared to
    //!             order momements in time.
    //! \note The difference between pseudo microseconds does not hold any information
    //!       other than that the sign telles the order of the corresponding moments.
    scxulong SCXCalendarTime::ToComparablePseudoMicrosecond(SCXCalendarTimePrecision precision) const {
        scxulong pseudoMS = 0;
        switch (precision) {
        case eCalendarTimePrecisionUnknown: // If unknown - use everything else
        case eCalendarTimePrecisionSecond:
            pseudoMS += (m_microsecond/Pow(10,6-m_decimalCount))*Pow(10,6-m_decimalCount);
        case eCalendarTimePrecisionMinute:
            pseudoMS += (static_cast<scxulong>(m_minute) << 26);// * 1048576 * 64;
        case eCalendarTimePrecisionHour:
            pseudoMS += (static_cast<scxulong>(m_hour) << 32);// * 64 * 1048576 * 64;
        case eCalendarTimePrecisionDay:
            pseudoMS += (static_cast<scxulong>(m_day) << 37); // * 32 * 64 * 1048576 * 64;
        case eCalendarTimePrecisionMonth:
            pseudoMS += (static_cast<scxulong>(m_month) << 42); // * 32 * 32 * 64 * 1048576 * 64;
        case eCalendarTimePrecisionYear:
            pseudoMS += (static_cast<scxulong>(m_year) << 46);// * 16 * 32 * 32 * 64 * 1048576 * 64;
        };
        return pseudoMS;
    }

    /*----------------------------------------------------------------------------*/
    //! Calculate number of microseconds one must add to reach "time"
    //! \param[in]      time    Later in time
    //! \returns    The number of microseconds one would have to add to reach "time"
    //! The value of each unit of time in "time" must be at least as large as the corresponding
    //! unit of "this" time.
    scxulong SCXCalendarTime::MicrosecondsUntil(const SCXCalendarTime &time) const {
        SCXASSERT(m_year <= time.m_year);
        SCXASSERT(m_month <= time.m_month);
        SCXASSERT(m_day <= time.m_day);
        SCXASSERT(m_hour <= time.m_hour);
        SCXASSERT(m_minute <= time.m_minute);
        SCXASSERT(m_microsecond <= time.m_microsecond);
        SCXASSERT(m_minutesFromUTC <= time.m_minutesFromUTC);
        scxulong microseconds = 0;
        for (scxyear year = m_year; year < time.m_year; year++) {
            microseconds += MicrosecondsInYear(year);
        }
        for (scxmonth month = m_month; month < time.m_month; month++) {
            microseconds += MicrosecondsInMonth(time.m_year, month);
        }
        for (scxday day = m_day; day < time.m_day; day++) {
            microseconds += MicrosecondsInDay(time.m_year, time.m_month, day);
        }
        for (scxhour hour = m_hour; hour < time.m_hour; hour++) {
            microseconds += MicrosecondsInHour(time.m_year, time.m_month, time.m_day, hour);
        }
        for (scxminute minute = m_minute; minute < time.m_minute; minute++) {
            microseconds += MicrosecondsInMinute(time.m_year, time.m_month, time.m_day, time.m_hour, minute);
        }
        microseconds += time.m_microsecond - m_microsecond;
        microseconds -= static_cast<scxulong>(60)*1000*1000*(time.m_minutesFromUTC - m_minutesFromUTC);
        return microseconds;
    }


    /*----------------------------------------------------------------------------*/
    //! Converts a timestamp from a CIM-formatted string to
    //! an instance of SCXCalendarTime.
    //! \param[in]    str        Timestamp, for example, 20041203162010.123456+120
    //! \returns    Timestamp as an instance of SCXCalendarTime
    SCXCalendarTime SCXCalendarTime::FromCIM(std::wstring str) {
        if (str.length() != 25) {
            throw SCXInvalidTimeFormatException(L"Not formatted according to CIM DATETIME",
                    str, SCXSRCLOCATION);
        }
        unsigned int year = StrToUInt(str.substr(0, 4));
        unsigned int month = StrToUInt(str.substr(4, 2));
        unsigned int day = StrToUInt(str.substr(6, 2));
        unsigned int hour = StrToUInt(str.substr(8, 2));
        unsigned int minute = StrToUInt(str.substr(10, 2));
        unsigned int second = StrToUInt(str.substr(12, 2));
        if (str.at(14) != '.') {
            throw SCXInvalidTimeFormatException(L"Not formatted according to CIM DATETIME",
                    str, SCXSRCLOCATION);
        }
        unsigned int microsecond = StrToUInt(str.substr(15, 6));
        if (str.at(21) != '+' && str.at(21) != '-') {
            throw SCXInvalidTimeFormatException(L"Not formatted according to CIM DATETIME",
                    str, SCXSRCLOCATION);
        }
        int sign = str.at(21) == '-' ? -1 : +1;
        int minutesFromUTC = sign * StrToUInt(str.substr(22, 3));
        unsigned totalMicrosecond = second * 1000*1000 + microsecond;
        return SCXCalendarTime(year, month, day, hour, minute, totalMicrosecond, 6, minutesFromUTC);
    }

    /*----------------------------------------------------------------------------*/
    //! Convert a timestamp in Posix format to an instance of SCXCalendarTime
    //! \param[in]  seconds     Since posix epoch
    //! \returns    Timestamp as SCXCalendarTime
    //! Maps negative values of seconds to a time before posix epoch
    //! \throws     SCXNotSupportedException    Time before U**x epoch
    SCXCalendarTime SCXCalendarTime::FromPosixTime(scxlong seconds) {
        scxulong absSeconds = Abs(seconds);
        SCXRelativeTime absEpochTime;
        absEpochTime.SetDays(ToInt(absSeconds / cPosixSecondsInDay));
        absEpochTime.SetSeconds(ToInt(absSeconds % cPosixSecondsInDay));
        return seconds >= 0? cUnixEpoch + absEpochTime : cUnixEpoch - absEpochTime;
    }

    /*----------------------------------------------------------------------------*/
    //! Convert a timestamp from ISO 8601 textual format to an instance of SCXCalendarTime
    //! \param[in]    str        Timestamp in textual format
    //! \returns    Timestamp as an instance of SCXCalendarTime
    //! \throws        SCXInvalidTimeFormatException    When the timestamp is not formatted according to ISO 8601
    //! \throws        SCXNotSupportedException        When the timestamp is on a "truncated" (not complete) form
    //! \throws        SCXNotSupportedException    When the number of decimals cannot be represented
    //! \note The current implementation supports microseconds, that is, up to and including 6 decimals of second
    SCXCalendarTime SCXCalendarTime::FromISO8601(std::wstring str) {
        std::wstring::size_type timeSeparatorPos = str.find(L"T", 0);
        if (timeSeparatorPos == std::wstring::npos) {
            throw SCXInvalidTimeFormatException(L"Missing separator T", str, SCXSRCLOCATION);
        }
        SCXCalendarTime time(DateFromIso8601(str.substr(0, timeSeparatorPos)));
        SCXRelativeTime absOffsetFromUTC;
        std::wstring::size_type afterTimePos = 0;
        bool westOfUTC = false;
        if (str.at(str.length() - 1) != 'Z') {
            if (str.at(str.length() - 3) == '+') {
                afterTimePos = str.length() - 3;
            } else if (str.at(str.length() - 3) == '-') {
                westOfUTC = true;
                afterTimePos = str.length() - 3;
            } else if (str.at(str.length() - 6) == '+') {
                afterTimePos = str.length() - 6;
            } else if (str.at(str.length() - 6) == '-') {
                westOfUTC = true;
                afterTimePos = str.length() - 6;
            }
            absOffsetFromUTC = OffsetFromUTCFromIso8601(str.substr(afterTimePos + 1,
                    str.length() - afterTimePos - 1));
        } else {
            afterTimePos = str.length() - 1;
        }

        time.SetTimeOfDay(TimeFromIso8601(str.substr(timeSeparatorPos + 1,
                afterTimePos - timeSeparatorPos - 1)));
        time.SetOffsetFromUTC(westOfUTC? -absOffsetFromUTC : absOffsetFromUTC);
        return time;
    }

    /*----------------------------------------------------------------------------*/
    //! Convert a date in ISO-8601 textual form to an instance initialized accordingly
    //! \param[in]  str     Timestamp in ISO 8601 date format
    //! \returns    Timestamp initialized according to "str"
    SCXCalendarTime SCXCalendarTime::DateFromIso8601(const std::wstring &str) {
        if (str.find(L"W") != std::wstring::npos) {
            throw SCXNotSupportedException(L"Week dates", SCXSRCLOCATION);
        }
        scxyear year = 0;
        scxmonth month = 0;
        scxday day = 0;
        if (str.length() == 8) {
            year = StrToUInt(str.substr(0, 4));
            month = StrToUInt(str.substr(4, 2));
            day = StrToUInt(str.substr(6, 2));
        } else if (str.length() == 10) {
            if (str.at(4) != '-') {
                throw SCXInvalidTimeFormatException(L"Missing separator -", str, SCXSRCLOCATION);
            }
            if (str.at(7) != '-') {
                throw SCXInvalidTimeFormatException(L"Missing separator -", str, SCXSRCLOCATION);
            }
            year = StrToUInt(str.substr(0, 4));
            month = StrToUInt(str.substr(5, 2));
            day = StrToUInt(str.substr(8, 2));
        } else if (str.length() == 7) {
            if (str.at(4) != '-') {
                throw SCXInvalidTimeFormatException(L"Missing separator -", str, SCXSRCLOCATION);
            }
            StrToUInt(str.substr(0, 4));
            StrToUInt(str.substr(5, 2));
            throw SCXNotSupportedException(L"Date with lower precision (YYYY-MM only)", SCXSRCLOCATION);
        } else if (str.length() == 4) {
            StrToUInt(str.substr(0, 4));
            throw SCXNotSupportedException(L"Date with lower precision (YYYY only)", SCXSRCLOCATION);
        } else {
            throw SCXInvalidTimeFormatException(L"Non ISO date", str, SCXSRCLOCATION);
        }
        return SCXCalendarTime(year, month, day, 0, 0, 0, 0, 0);
    }

    /*----------------------------------------------------------------------------*/
    //! Convert an "offset from UTC" in ISO-8601 format to an relative time instance
    //! \param[in]  str     Offset according to ISO-8601
    //! \returns    Instance of relative time initialized according to "str"
    SCXRelativeTime SCXCalendarTime::OffsetFromUTCFromIso8601(const std::wstring &str) {
        scxhour hour = 0;
        scxminute minute = 0;
        if (str.length() == 2) {
            hour = StrToUInt(str.substr(0, 2));
        } else if (str.length() == 5) {
            hour = StrToUInt(str.substr(0, 2));
            minute = StrToUInt(str.substr(3, 2));
        } else {
            throw SCXInvalidTimeFormatException(L"Timezone not according to ISO-8601", str, SCXSRCLOCATION);
        }
        return SCXRelativeTime(0, 0, 0, hour, minute, 0, 0);
    }

    /*----------------------------------------------------------------------------*/
    //! Convert a ISO 8601 time to an instance of relative time
    //! \param[in]      str     Time according to ISO-8601
    //! \returns        An instance initialized according to "str"
    SCXRelativeTime SCXCalendarTime::TimeFromIso8601(const std::wstring &str) {
        std::wstring::size_type dotpos = str.find(L".", 0);
        std::wstring::size_type commapos = str.find(L",", 0);
        std::wstring::size_type decimalpos = dotpos != std::wstring::npos? dotpos
                : commapos != std::wstring::npos? commapos
                : str.length();
        scxhour hour = 0;
        scxminute minute = 0;
        unsigned second = 0;
        if (decimalpos == 8) {
            if (str.at(2) != ':') {
                throw SCXInvalidTimeFormatException(L"Missing separator -", str, SCXSRCLOCATION);
            }
            if (str.at(5) != ':') {
                throw SCXInvalidTimeFormatException(L"Missing separator -", str, SCXSRCLOCATION);
            }
            hour = StrToUInt(str.substr(0, 2));
            minute = StrToUInt(str.substr(3, 2));
            second = StrToUInt(str.substr(6, 2));
        } else if (decimalpos == 6) {
            hour = StrToUInt(str.substr(0, 2));
            minute = StrToUInt(str.substr(2, 2));
            second = StrToUInt(str.substr(4, 2));
        } else if (decimalpos == 2) {
            StrToUInt(str.substr(0, 2));
            throw SCXNotSupportedException(L"Time with lower precision (hh only)", SCXSRCLOCATION);
        } else if (decimalpos == 4) {
            StrToUInt(str.substr(0, 2));
            StrToUInt(str.substr(2, 2));
            throw SCXNotSupportedException(L"Time with lower precision (hhmm only)", SCXSRCLOCATION);
        } else if (decimalpos == 5) {
            if (str.at(2) != ':') {
                throw SCXInvalidTimeFormatException(L"Missing separator -", str, SCXSRCLOCATION);
            }
            StrToUInt(str.substr(0, 2));
            StrToUInt(str.substr(3, 2));
            throw SCXNotSupportedException(L"Time with lower precision (hh:mm only)", SCXSRCLOCATION);
        } else {
            throw SCXInvalidTimeFormatException(L"str", L"Not ISO-8601", SCXSRCLOCATION);
        }
        scxulong microsecondOnly = 0;
        std::wstring::size_type decimalCount = str.length() - decimalpos - 1;
        if (decimalCount > 6) {
            throw SCXNotSupportedException(L"More decimals than 6", SCXSRCLOCATION);
        }
        if (decimalCount > 0) {
            std::wstring decimalStr(str.substr(decimalpos + 1, decimalCount));
            decimalCount = decimalStr.length();
            microsecondOnly = StrToUInt(decimalStr) * Pow(10, 6 - decimalCount);
        }
        SCXRelativeTime amount(0, 0, 0, hour, minute, 0, static_cast<unsigned int>(decimalCount));
        amount.m_microseconds = second * 1000 * 1000 + microsecondOnly;
        return amount;
    }

    /*----------------------------------------------------------------------------*/
    //! Retrieves the local offset from UTC of the passed in time
    //! \param[in]   posixTime           Unix epoch
    //! \returns How much "east of" UTC in minutes
    int SCXCalendarTime::GetMinutesFromUTC(scxlong posixTime)
    {
        int minutesFromUTC = 0;
#if defined(SCX_UNIX)
        tm localparts;
        timeval utc;

        utc.tv_sec = (time_t) posixTime;
        utc.tv_usec = 0;
        if (localtime_r(&utc.tv_sec, &localparts) != &localparts) {
            throw SCXInternalErrorException(UnexpectedErrno(L"Call to localtime_r failed", errno), SCXSRCLOCATION);
        }

#if defined(linux) || defined(macos)
        minutesFromUTC = static_cast<unsigned int>(localparts.tm_gmtoff / 60);
#elif defined(sun) || defined(hpux) || defined(aix)
        minutesFromUTC = -timezone / 60 + ( (localparts.tm_isdst > 0 && daylight) ? 60 : 0);
#else
        struct timezone tz;
        if (gettimeofday(&utc, &tz) < 0) {
            throw SCXInternalErrorException(UnexpectedErrno(L"Call to gettimeofday failed", errno), SCXSRCLOCATION);
        }
        minutesFromUTC = -timezone / 60 + ( (tz.tz_dsttime > 0) ? 60 : 0);
#endif
        return minutesFromUTC;
#else
#error Unexpected platform
#endif
    }

    /*----------------------------------------------------------------------------*/
    //! Retrieve the current time as UTC
    //! \returns Current time expressed as UTC
    SCXCalendarTime SCXCalendarTime::CurrentUTC() {
#if defined(WIN32)
        _SYSTEMTIME utc;
        GetSystemTime(&utc);
        return SCXCalendarTime(utc.wYear, utc.wMonth, utc.wDay,
            utc.wHour, utc.wMinute, utc.wSecond, 3, 0);
#elif defined(SCX_UNIX)
        timeval utc;
        if (gettimeofday(&utc, NULL) < 0) {
            throw SCXInternalErrorException(UnexpectedErrno(L"Call to gettimeofday failed", errno), SCXSRCLOCATION);
        }
        tm utcparts;
        if (gmtime_r(&utc.tv_sec, &utcparts) != &utcparts) {
            throw SCXInternalErrorException(UnexpectedErrno(L"Call to gmtime_r failed", errno), SCXSRCLOCATION);
        }
        scxyear year = 0;
        scxmonth month = 0;
        scxday day = 0;
        scxhour hour = 0;
        scxminute minute = 0;
        unsigned int microsecond;
        CopyFromStruct(utcparts, static_cast<int>(utc.tv_usec), year, month, day, hour, minute, microsecond);
        return SCXCalendarTime(year, month, day, hour, minute, microsecond, 3, 0);
#endif
    }

    /*----------------------------------------------------------------------------*/
    //! Retrieve the current time in the local, current, timezone
    //! \returns Local time
    SCXCalendarTime SCXCalendarTime::CurrentLocal() {
#if defined(WIN32)
        _SYSTEMTIME local;
        GetLocalTime(&local);
        TIME_ZONE_INFORMATION zoneInfo;
        if (GetTimeZoneInformation(&zoneInfo) == TIME_ZONE_ID_INVALID) {
            throw SCXInternalErrorException(L"GetTimeZoneInformation failed", SCXSRCLOCATION);
        }
        int minutesFromUTC = -(zoneInfo.Bias + zoneInfo.DaylightBias);
        return SCXCalendarTime(local.wYear, local.wMonth, local.wDay,
                local.wHour, local.wMinute, local.wMilliseconds * 1000, 3, minutesFromUTC);
#elif defined(SCX_UNIX)
        timeval utc;
        if (gettimeofday(&utc, NULL) < 0) {
            throw SCXInternalErrorException(UnexpectedErrno(L"Call to gettimeofday failed", errno), SCXSRCLOCATION);
        }
        tm localparts;
        if (localtime_r(&utc.tv_sec, &localparts) != &localparts) {
            throw SCXInternalErrorException(UnexpectedErrno(L"Call to localtime_r failed", errno), SCXSRCLOCATION);
        }
        scxyear year = 0;
        scxmonth month = 0;
        scxday day = 0;
        scxhour hour = 0;
        scxminute minute = 0;
        unsigned int microsecond;
        CopyFromStruct(localparts, static_cast<int>(utc.tv_usec), year, month, day, hour, minute, microsecond);

        int minutesFromUTC = GetMinutesFromUTC(utc.tv_sec);
        return SCXCalendarTime(year, month, day, hour, minute, microsecond, 6, minutesFromUTC);
#else
#error Unexpected platform
#endif
    }

    /*----------------------------------------------------------------------------*/
    //! Retrieves the current local offset from UTC
    //! \returns How much "east of" UTC
    SCXRelativeTime SCXCalendarTime::CurrentOffsetFromUTC() {
        return CurrentLocal().GetOffsetFromUTC();
    }

    /*----------------------------------------------------------------------------*/
    //! Constructs an uninitalized instance.
    //! Useful for out parameters and arrays/vectors.
    //! \note Currently the only allowed operation on an uninitialized instance is assigment to it.
    SCXCalendarTime::SCXCalendarTime()
            : m_year(0), m_month(0), m_day(0), m_hour(0), m_minute(0),
              m_microsecond(0), m_decimalCount(0), m_minutesFromUTC(0),
              m_initialized(false), m_precision(eCalendarTimePrecisionUnknown) {
    }

    /*----------------------------------------------------------------------------*/
    //! Constructs an SCXCalendarTime
    //! \param[in]   year           Year, the "real" one, 2000 is four digits
    //! \param[in]   month          Month of year, 1-12
    //! \param[in]   day            Day in month, 1-31, depending on month
    //! \param[in]   hour           Hour of day, 0-23
    //! \param[in]   minute         Minute of hour, 0-59
    //! \param[in]   second         Second of minute, 0 <= floating point value < 60
    //! \param[in]    decimalCount    Number of significant decimals
    //! \param[in]   offsetFromUTC  Relative time "east of UTC"
    //! \throws     SCXInvalidArgumentException         Time unit outside of value domain
    //! \throws     SCXNotSupportedException            Year before U**x epoch
    SCXCalendarTime::SCXCalendarTime(scxyear year, scxmonth month, scxday day, scxhour hour, scxminute minute, scxsecond second,
            scxdecimalnr decimalCount, const SCXRelativeTime &offsetFromUTC)
                : m_year(year), m_month(month), m_day(day),
                  m_hour(hour), m_minute(minute), m_microsecond(RoundToUnsignedInt(second * 1000000)),
                  m_decimalCount(decimalCount), m_minutesFromUTC(0), m_initialized(true),
                  m_precision(eCalendarTimePrecisionSecond) {
        if (year < 1970) {
            throw SCXNotSupportedException(L"Year before U**x epoch", SCXSRCLOCATION);
        }
        if (!IsAscending(1, month, 12)) {
            throw SCXIllegalIndexException<scxmonth>(L"month", month, 1, true, 12, true, SCXSRCLOCATION);
        }
        if (!IsAscending(1, day, 31)) {
            throw SCXIllegalIndexException<scxday>(L"day", day, 1, true, 31, true, SCXSRCLOCATION);
        }
        if (!IsAscending(0, hour, 23)) {
            throw SCXIllegalIndexException<scxhour>(L"hour", hour, 0, true, 23, true, SCXSRCLOCATION);
        }
        if (!IsAscending(0, minute, 59)) {
            throw SCXIllegalIndexException<scxminute>(L"minute", minute, 0, true, 59, true, SCXSRCLOCATION);
        }
        if (second >= 60) {
            throw SCXInvalidArgumentException(L"second", L"not 0 <= second < 60", SCXSRCLOCATION);
        }
        if (decimalCount > 6) {
            throw SCXInvalidArgumentException(L"decimalCount", L"not 0 <= decimalCount <= 6", SCXSRCLOCATION);
        }
        if (!offsetFromUTC.IsValidAsOffsetFromUTC()) {
            throw SCXInvalidArgumentException(L"offsetFromUTC", L"Offset from UTC not valid", SCXSRCLOCATION);
        }
        m_minutesFromUTC = offsetFromUTC.GetHours() * 60 + offsetFromUTC.GetMinutes();
    }

    /*----------------------------------------------------------------------------*/
    //! Constructs an SCXCalendarTime
    //! \param[in]   year           Year, the "real" one, 2000 is four digits
    //! \param[in]   month          Month of year, 1-12
    //! \param[in]   day            Day in month, 1-31, depending on month
    //! \param[in]   hour           Hour of day, 0-23
    //! \param[in]   minute         Minute of hour, 0-59
    //! \param[in]   second         Second of minute, 0 <= floating point value < 60
    //! \param[in]   offsetFromUTC  Relative time "east of UTC"
    //! \note        DecimalCount is assumed to be the maximally supported value
    //! \throws     SCXInvalidArgumentException         Time unit outside of value domain
    //! \throws     SCXNotSupportedException            Year before U**x epoch
    SCXCalendarTime::SCXCalendarTime(scxyear year, scxmonth month, scxday day, scxhour hour, scxminute minute, scxsecond second,
            const SCXRelativeTime &offsetFromUTC)
                : m_year(year), m_month(month), m_day(day),
                  m_hour(hour), m_minute(minute), m_microsecond(RoundToUnsignedInt(second * 1000000)),
                  m_decimalCount(6), m_minutesFromUTC(0), m_initialized(true),
                  m_precision(eCalendarTimePrecisionSecond) {
        if (year < 1970) {
            throw SCXNotSupportedException(L"Year before U**x epoch", SCXSRCLOCATION);
        }
        if (!IsAscending(1, month, 12)) {
            throw SCXIllegalIndexException<scxmonth>(L"month", month, 1, true, 12, true, SCXSRCLOCATION);
        }
        if (!IsAscending(1, day, 31)) {
            throw SCXIllegalIndexException<scxday>(L"day", day, 1, true, 31, true, SCXSRCLOCATION);
        }
        if (!IsAscending(0, hour, 23)) {
            throw SCXIllegalIndexException<scxhour>(L"hour", hour, 0, true, 23, true, SCXSRCLOCATION);
        }
        if (!IsAscending(0, minute, 59)) {
            throw SCXIllegalIndexException<scxminute>(L"minute", minute, 0, true, 59, true, SCXSRCLOCATION);
        }
        if (second >= 60) {
            throw SCXInvalidArgumentException(L"second", L"not 0 <= second < 60", SCXSRCLOCATION);
        }
        if (!offsetFromUTC.IsValidAsOffsetFromUTC()) {
            throw SCXInvalidArgumentException(L"offsetFromUTC", L"Offset from UTC not valid", SCXSRCLOCATION);
        }
        m_minutesFromUTC = offsetFromUTC.GetHours() * 60 + offsetFromUTC.GetMinutes();
    }


    /*----------------------------------------------------------------------------*/
    //! Constructs an SCXCalendarTime
    //! \param[in]   year           Year, the "real" one, 2000 is four digits
    //! \param[in]   month          Month of year, 1-12
    //! \param[in]   day            Day in month, 1-31, depending on month
    //! \throws     SCXInvalidArgumentException         Time unit outside of value domain
    //! \throws     SCXNotSupportedException            Year before U**x epoch
    SCXCalendarTime::SCXCalendarTime(scxyear year, scxmonth month, scxday day)
        : m_year(year), m_month(month), m_day(day),
          m_hour(0), m_minute(0), m_microsecond(0), m_decimalCount(0), m_minutesFromUTC(0),
          m_initialized(true), m_precision(eCalendarTimePrecisionDay) {
        if (year < 1970) {
            throw SCXNotSupportedException(L"Year before U**x epoch", SCXSRCLOCATION);
        }
        if (!IsAscending(1, month, 12)) {
            throw SCXIllegalIndexException<scxmonth>(L"month", month, 1, true, 12, true, SCXSRCLOCATION);
        }
        if (!IsAscending(1, day, 31)) {
            throw SCXIllegalIndexException<scxday>(L"day", day, 1, true, 31, true, SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Assign values of another timestamp to "this"
    //! \param[in]      other       Timestamp to copy
    //! \returns *this
    SCXCalendarTime &SCXCalendarTime::operator=(const SCXCalendarTime &other) {
        m_year = other.m_year;
        m_month = other.m_month;
        m_day = other.m_day;
        m_hour = other.m_hour;
        m_minute = other.m_minute;
        m_microsecond = other.m_microsecond;
        m_decimalCount = other.m_decimalCount;
        m_minutesFromUTC = other.m_minutesFromUTC;
        m_initialized = other.m_initialized;
        m_precision = other.m_precision;
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    //! Checks if the timezone corresponds to UTC
    //! \returns true iff The calendartime is UTC
    bool SCXCalendarTime::IsUTC() const {
        SCXASSERT(m_initialized);
        return m_minutesFromUTC == 0;
    }

    /*----------------------------------------------------------------------------*/
    //! The "real" year, 2000 is represented as 2000, not 00 or something else
    //! \returns year >= 1970
    scxyear SCXCalendarTime::GetYear() const {
        SCXASSERT(m_initialized);
        return m_year;
    }

    /*----------------------------------------------------------------------------*/

    //! The month.
    //! \returns 1 <= month <= 12
    scxmonth SCXCalendarTime::GetMonth() const {
        SCXASSERT(m_initialized);
        return m_month;
    }

    /*----------------------------------------------------------------------------*/
    //! The day.
    //! \returns 1 <= day <= 31
    scxday SCXCalendarTime::GetDay() const {
        SCXASSERT(m_initialized);
        return m_day;
    }

    /*----------------------------------------------------------------------------*/
    //! The hour
    //! \returns 0 <= hour <= 23
    scxhour SCXCalendarTime::GetHour() const {
        SCXASSERT(m_initialized);
        return m_hour;
    }

    /*----------------------------------------------------------------------------*/
    //! The minute
    //! \returns 0 <= minute <= 59
    scxminute SCXCalendarTime::GetMinute() const {
        SCXASSERT(m_initialized);
        return m_minute;
    }

    /*----------------------------------------------------------------------------*/
    //! The second
    //! \returns 0 <= second < 60
    scxsecond SCXCalendarTime::GetSecond() const {
        SCXASSERT(m_initialized);
        return static_cast<scxsecond>(m_microsecond) / 1000000;
    }

    /*----------------------------------------------------------------------------*/
    //! The offset from UTC
    //! \returns Hour and minutes "east of UTC=0"
    SCXRelativeTime SCXCalendarTime::GetOffsetFromUTC() const {
        SCXASSERT(m_initialized);
        return SCXRelativeTime().SetMinutes(m_minutesFromUTC);
    }

    /*----------------------------------------------------------------------------*/
    //! Number of significant decimals
    //! \returns Number of significant decimals
    scxdecimalnr SCXCalendarTime::GetDecimalCount() const {
        SCXASSERT(m_initialized);
        return m_decimalCount;
    }

    /*----------------------------------------------------------------------------*/
    //! Calendar time precision.
    //! \returns Precision.
    SCXCalendarTime::SCXCalendarTimePrecision SCXCalendarTime::GetPrecision() const {
        SCXASSERT(m_initialized);
        return m_precision;
    }

    /*----------------------------------------------------------------------------*/
    //! Change to another year
    //! \param[in]        year     New year
    //! \throws            SCXNotSupportedException    year < 1970
    void SCXCalendarTime::SetYear(scxyear year) {
        SCXASSERT(m_initialized);
        if (year < 1970) {
            throw SCXNotSupportedException(L"Year before U**x epoch", SCXSRCLOCATION);
        }
        m_year = year;
    }

    /*----------------------------------------------------------------------------*/
    //! Change to another month
    //! \param[in]        month         New month
    //! \throws            SCXIllegalIndexException<scxmonth>    month < 1 or month > 12
    void SCXCalendarTime::SetMonth(scxmonth month) {
        SCXASSERT(m_initialized);
        if (!IsAscending(1, month, 12)) {
            throw SCXIllegalIndexException<scxmonth>(L"month", month, 1, true, 12, true, SCXSRCLOCATION);
        }
        m_month = month;
    }

    /*----------------------------------------------------------------------------*/
    //! Change to another day
    //! \param[in]        day         1 <= day <= 31
    //! \throws            SCXIllegalIndexException<scxday>    day < 1 or day > 31
    void SCXCalendarTime::SetDay(scxday day) {
        SCXASSERT(m_initialized);
        if (!IsAscending(1, day, 31)) {
            throw SCXIllegalIndexException<scxday>(L"day", day, 1, true, 31, true, SCXSRCLOCATION);
        }
        m_day = day;
    }

    /*----------------------------------------------------------------------------*/
    //! Change to another hour
    //! \param[in]        hour         New hour
    //! \throws            SCXIllegalIndexException<scxhour>     hour >= 24
    void SCXCalendarTime::SetHour(scxhour hour) {
        SCXASSERT(m_initialized);
        if (!IsAscending(0, hour, 23)) {
            throw SCXIllegalIndexException<scxhour>(L"hour", hour, 0, true, 23, true, SCXSRCLOCATION);
        }
        m_hour = hour;
    }

    /*----------------------------------------------------------------------------*/
    //! Change to another minute
    //! \param[in]        minute         New minute
    //! \throws            SCXIllegalIndexException<scxminute>     minute >= 60
    void SCXCalendarTime::SetMinute(scxminute minute) {
        SCXASSERT(m_initialized);
        if (!IsAscending(0, minute, 59)) {
            throw SCXIllegalIndexException<scxminute>(L"minute", minute, 0, true, 59, true, SCXSRCLOCATION);
        }
        m_minute = minute;
    }

    /*----------------------------------------------------------------------------*/
    //! Change to another second
    //! \param[in]        second         New second
    //! \throws            SCXIllegalIndexException<scxsecond>     second >= 60
    void SCXCalendarTime::SetSecond(scxsecond second) {
        SCXASSERT(m_initialized);
        if (second >= 60) {
            throw SCXInvalidArgumentException(L"second", L"not 0 <= second < 60", SCXSRCLOCATION);
        }
        m_microsecond = RoundToUnsignedInt(second * 1000000);
    }

    /*----------------------------------------------------------------------------*/
    //! Change to another offset from UTC
    //! \param[in]        offset         New offset from UTC
    //! \throws            SCXInvalidArgumentException     offset not valid as offset from UTC
    void SCXCalendarTime::SetOffsetFromUTC(const SCXRelativeTime &offset) {
        SCXASSERT(m_initialized);
        if (!offset.IsValidAsOffsetFromUTC()) {
            throw SCXInvalidArgumentException(L"offset", L"not IsValidAsOffestFromUTC", SCXSRCLOCATION);
        }
        m_minutesFromUTC = offset.GetHours() * 60 + offset.GetMinutes();
    }

    /*----------------------------------------------------------------------------*/
    //! Change information about number of significant decimals
    //! \param[in]   decimalCount                    New number of significant decimals
    //! \throws         SCXIllegalArgumentException     Invalid number of decimals
    //! Determines for example the default number of decimals when converting to
    //! a decimal textual format.
    //! Needed because the binary representation of, for example, 3 and 3.00 is the same.
    //! \note Does not change the timestamp, just information _about_ the timestamp. Lowering
    //! the number of significant digits does not remove any digits of time, in the same way
    //! as increasing the number of significant digits does not add any new digits of time.
    //! \note The current implementation support up to and including 6 decimals
    void SCXCalendarTime::SetDecimalCount(scxdecimalnr decimalCount) {
        SCXASSERT(m_initialized);
        if (!IsAscending(0, decimalCount, 6)) {
            throw SCXIllegalIndexException<scxminute>(L"decimalCount", decimalCount, 0, true, 6, true, SCXSRCLOCATION);
        }
        m_decimalCount = decimalCount;
    }

    /*----------------------------------------------------------------------------*/
    //! Change information about precision.
    //! \param[in]   precision New precision.
    //! This is a complement to decimalCount and is used to set precision on for example
    //! date only.
    void SCXCalendarTime::SetPrecision(SCXCalendarTime::SCXCalendarTimePrecision precision) {
        SCXASSERT(m_initialized);
        m_precision = precision;
    }

    /*----------------------------------------------------------------------------*/
    //! Extracts the time relative to the start of day.
    //! \returns Hour, minutes and seconds (including fractions of seconds)
    SCXRelativeTime SCXCalendarTime::GetTimeOfDay() const {
        SCXASSERT(m_initialized);
        SCXRelativeTime timeOfDay(0, 0, 0, m_hour, m_minute, 0, m_decimalCount);
        timeOfDay.m_microseconds = m_microsecond;
        return timeOfDay;
    }

    /*----------------------------------------------------------------------------*/
    //! Changes the time relative to the start of day.
    //! \param[in]    timeOfDay    Time relative start of day
    void SCXCalendarTime::SetTimeOfDay(const SCXRelativeTime &timeOfDay) {
        SCXASSERT(m_initialized);
        SCXCalendarTime copy(m_year, m_month, m_day, 0, 0, 0, timeOfDay.GetDecimalCount(),
                m_minutesFromUTC);
        copy +=timeOfDay;
        if (copy.GetYear() != m_year || copy.GetMonth() != m_month || copy.GetDay() != m_day) {
            throw SCXInvalidArgumentException(L"timeOfDay", timeOfDay.DumpString(), SCXSRCLOCATION);
        }
        *this = copy;
    }

    /*----------------------------------------------------------------------------*/
    //! Adjust the timestamp to refer to the same moment in time in UTC
    //! \returns    *this
    SCXCalendarTime& SCXCalendarTime::MakeUTC() {
        SCXASSERT(m_initialized);
        if (m_minutesFromUTC != 0) {
            SCXCalendarTime copy(*this);
            scxlong microsecondsToSubtract = static_cast<scxlong>(m_minutesFromUTC) * (60*1000*1000);
            if (microsecondsToSubtract > 0) {
                copy.SubtractMicroseconds(microsecondsToSubtract);
            } else {
                copy.AddMicroseconds(-microsecondsToSubtract);
            }
            copy.m_minutesFromUTC = 0;
            *this = copy;
        }
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    //! Adjust the timestamp to refer to the same moment in time in another timezone
    //! \param[in]  offsetFromUTC   New offset from UTC
    //! \returns    *this
    SCXCalendarTime& SCXCalendarTime::MakeLocal(SCXRelativeTime offsetFromUTC) {
        SCXASSERT(m_initialized);
        if (!offsetFromUTC.IsValidAsOffsetFromUTC()) {
            throw SCXInvalidArgumentException(L"offsetFromUTC",
                    offsetFromUTC.DumpString(), SCXSRCLOCATION);
        }
        SCXCalendarTime copy(*this);
        copy.MakeUTC();
        copy.m_minutesFromUTC = offsetFromUTC.GetHours() * 60 + offsetFromUTC.GetMinutes();
        scxlong microsecondsToAdd = static_cast<scxlong>(copy.m_minutesFromUTC) * (60*1000*1000);
        if (microsecondsToAdd > 0) {
            copy.AddMicroseconds(microsecondsToAdd);
        } else {
            copy.SubtractMicroseconds(-microsecondsToAdd);
        }
        *this = copy;
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    //! Adjust the timestamp to local timezone
    //! \returns    *this
    SCXCalendarTime& SCXCalendarTime::MakeLocal() {
        SCXASSERT(m_initialized);

        SCXCalendarTime copy(*this);
        copy.MakeUTC();
        copy.m_minutesFromUTC = GetMinutesFromUTC(ToPosixTime());

        scxlong microsecondsToAdd = static_cast<scxlong>(copy.m_minutesFromUTC) * (60*1000*1000);
        if (microsecondsToAdd > 0) {
            copy.AddMicroseconds(microsecondsToAdd);
        } else {
            copy.SubtractMicroseconds(-microsecondsToAdd);
        }
        *this = copy;
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    //! Converts the timestamp to Posix time
    //! \returns Number of seconds since posix epoch
    scxlong SCXCalendarTime::ToPosixTime() const {
        return RoundToScxLong((*this - cUnixEpoch).GetSeconds());
    }

    /*----------------------------------------------------------------------------*/
    //! Formats the timestamp to a string according to the CIM DATETIME format.
    //! \returns    Timestamp, for example, 20041203162010.123456+120
    std::wstring SCXCalendarTime::ToCIM() const {
        SCXASSERT(m_initialized);
        std::wostringstream buf;
        buf << std::setw(4) << std::setfill(L'0') << m_year;
        buf << std::setw(2) << std::setfill(L'0') << m_month;
        buf << std::setw(2) << std::setfill(L'0') << m_day;
        buf << std::setw(2) << std::setfill(L'0') << m_hour;
        buf << std::setw(2) << std::setfill(L'0') << m_minute;
        buf << std::setw(2) << std::setfill(L'0') << (m_microsecond / 1000000);
        buf << "." << std::setw(6) << std::setfill(L'0') << (m_microsecond % 1000000);
        buf << (m_minutesFromUTC >= 0? L'+' : L'-');
        buf << std::setw(3) << std::setfill(L'0') << abs(m_minutesFromUTC);
        return buf.str();
    }

    /*----------------------------------------------------------------------------*/
    //! Formats the timestamp to a string according to the current locale (LC_TIME)
    //! \returns Textual format localized to LC_TIME
    //!          for example on SLES10,
    //!              2009-01-29 16.30.05 (LC_TIME=sv_SE)
    //!              01/29/09 16:30:05   (LC_TIME=POSIX)
    std::wstring SCXCalendarTime::ToLocalizedTime() const {
        SCXASSERT(m_initialized);
        const time_t posixTime = static_cast<time_t>(ToPosixTime());
        const char timeFormat[] = "%x %X"; ///> format string for strftime (date time)
        char timeBuffer[128];

        struct tm timeInfo;
        if (localtime_r(&posixTime, &timeInfo) != &timeInfo) {
            throw SCXInternalErrorException(UnexpectedErrno(L"Call to localtime_r failed", errno), SCXSRCLOCATION);
        }

        // Someone may be relying on the 'default' LC_TIME locale;
        // after formatting our string we should restore the default
        char* currentLocale = setlocale(LC_TIME, NULL);

        // Switch to user defined locale to format time string
        setlocale(LC_TIME, "");
        if (strftime(timeBuffer, 128, timeFormat, &timeInfo) == 0) {
            // error timeBuffer length not enough
            timeBuffer[0] = '\0';
        }

        // Restore locale context
        setlocale(LC_TIME, currentLocale);

        // Return localized time string
        std::wostringstream buf;
        buf << timeBuffer;
        return buf.str();
    }

    /*----------------------------------------------------------------------------*/
    //! Formats timestamp according to basic ISO8601
    //! \returns Textual format according to basic IS8601 combined date/time
    //!          for example, 20041203T162010,123456+02
    //!
    //! http://en.wikipedia.org/wiki/ISO_8601
    std::wstring SCXCalendarTime::ToBasicISO8601() const {
        SCXASSERT(m_initialized);
        return ToISO8601(L"", L"");
    }

    /*----------------------------------------------------------------------------*/
    //! Formats timestamp according to extended ISO8601
    //! \returns Textual format according to extended IS8601 combined date/time
    //!          for example, 2004-12-03T16:20:10,123456+02:30
    //!
    //! http://en.wikipedia.org/wiki/ISO_8601
    std::wstring SCXCalendarTime::ToExtendedISO8601() const {
        SCXASSERT(m_initialized);
        return ToISO8601(L"-", L":");
    }

    /*----------------------------------------------------------------------------*/
    //! Internal helper function that formats timestamp according to ISO8601
    //! \param[in]    dateSeparator    L"-" or empty string
    //! \param[in]    timeSeparator    L":" or empty string
    //! \returns Textual format according IS8601 combined date/time,
    //!          for example, 20041203T162010,123456+02 or
    //!          2004-12-03T16:20:10,123456+02:30
    //!
    //! Both or none of dateSeparator and timeSeparator must be present
    //! http://en.wikipedia.org/wiki/ISO_8601
    std::wstring SCXCalendarTime::ToISO8601(std::wstring dateSeparator, std::wstring timeSeparator) const {
        SCXASSERT((dateSeparator == L"" && timeSeparator == L"") ||
                  (dateSeparator == L"-" && timeSeparator == L":"));
        std::wostringstream buf;
        buf << std::setw(4) << std::setfill(L'0') << m_year;
        buf << dateSeparator << std::setw(2) << std::setfill(L'0') << m_month;
        buf << dateSeparator << std::setw(2) << std::setfill(L'0') << m_day;
        if (timeSeparator.length() > 0) {
            buf << L"T";
        }
        buf << std::setw(2) << std::setfill(L'0') << m_hour;
        buf << timeSeparator << std::setw(2) << std::setfill(L'0') << m_minute;
        buf << timeSeparator << std::setw(2) << std::setfill(L'0') << (m_microsecond / 1000000);
        int decimalsToRemove = max(0, 6 - static_cast<int>(m_decimalCount));
        unsigned microsecond = (m_microsecond % 1000000) / RoundToInt(pow(static_cast<double> (10), decimalsToRemove));
        if (m_decimalCount > 0) {
            buf << "," << std::setw(m_decimalCount) << std::setfill(L'0') << microsecond;
        }
        if (m_minutesFromUTC != 0) {
            unsigned absMinutesFromUTC = abs(m_minutesFromUTC);
            buf << (m_minutesFromUTC >= 0? L'+' : L'-');
            buf << std::setw(2) << std::setfill(L'0') << (absMinutesFromUTC / 60);
            if (absMinutesFromUTC % 60 != 0) {
                buf << timeSeparator << std::setw(2) << std::setfill(L'0') << (absMinutesFromUTC % 60);
            }
        } else {
            buf << "Z";
        }
        return buf.str();
    }

    /*----------------------------------------------------------------------------*/
    //! Adda a relative amount of time
    //! \param[in]    amount    Amount to add
    //! \returns        *this
    //! \throws  SCXNotSupportedException   Time before U**x epoch
    SCXCalendarTime& SCXCalendarTime::operator+=(const SCXRelativeTime &amount) {
        SCXASSERT(m_initialized);
        SCXCalendarTime copy(*this);
        copy.Add(amount);
        if (copy < cUnixEpoch) {
            throw SCXNotSupportedException(L"Time before U**x epoch", SCXSRCLOCATION);
        }
        *this = copy;
        return *this;
    }

    /*----------------------------------------------------------------------------*/
    //! Subtract a relative amount of time
    //! \param[in]    amount    Amount to subtract
    //! \returns        *this
    //! \throws  SCXNotSupportedException   Time before U**x epoch
    SCXCalendarTime& SCXCalendarTime::operator-=(const SCXRelativeTime &amount) {
        SCXASSERT(m_initialized);
        SCXCalendarTime copy(*this);
        copy.Add(-amount);
        if (copy < cUnixEpoch) {
            throw SCXNotSupportedException(L"Time before U**x epoch", SCXSRCLOCATION);
        }
        *this = copy;
        return *this;
    }


    /*----------------------------------------------------------------------------*/
    //! Convert a relative amount of time to an absolute amount of time
    //! \param[in]    amount    Amount to convert
    //! \returns     Absolute amount of time
    SCXAmountOfTime SCXCalendarTime::AmountOfTime(const SCXRelativeTime &amount) const {
        SCXASSERT(m_initialized);
        return (SCXCalendarTime(*this) += amount) - *this;
    }

    /*----------------------------------------------------------------------------*/
    //! Check if two times convey identical information
    //! \param[in]        time1         Time to compare
    //! \param[in]        time2         Time to compare
    //! \returns          true iff every unit of information is the same
    //! \note Calendar times in different timezones are by definition not identical,
    //!       but may still be equal
    bool IsIdentical(const SCXCalendarTime &time1, const SCXCalendarTime &time2) {
        SCXASSERT(time1.m_initialized);
        SCXASSERT(time2.m_initialized);
        SCXCalendarTime::SCXCalendarTimePrecision precision = MIN(time1.m_precision, time2.m_precision);
        return time1.ToComparablePseudoMicrosecond(precision) == time2.ToComparablePseudoMicrosecond(precision)
            && time1.m_minutesFromUTC == time2.m_minutesFromUTC
            && time1.m_decimalCount == time2.m_decimalCount
            && time1.m_precision == time2.m_precision;
    }


    /*----------------------------------------------------------------------------*/
    //! Check if two times are equal
    //! \param[in]     time1            Time to compare
    //! \param[in]     time2            Time to compare
    //! \returns       true iff the times refer to the same moment in time
    //! \note Times in different timezones may be equal
    bool operator==(const SCXCalendarTime &time1, const SCXCalendarTime &time2) {
        SCXCalendarTime::SCXCalendarTimePrecision precision = MIN(time1.m_precision, time2.m_precision);
        return SCXCalendarTime(time1).MakeUTC().ToComparablePseudoMicrosecond(precision)
            == SCXCalendarTime(time2).MakeUTC().ToComparablePseudoMicrosecond(precision);
    }

    /*----------------------------------------------------------------------------*/
    //! Check if two times are not equal
    //! \param[in]     time1            Time to compare
    //! \param[in]     time2            Time to compare
    //! \returns       true iff the times do not refer to the same moment in time
    //! \note Times in different timezones may be equal
    bool operator!=(const SCXCalendarTime &time1, const SCXCalendarTime &time2) {
        return ! (time1 == time2);
    }


    /*----------------------------------------------------------------------------*/
    //! Check if one time is less than or equal to another time
    //! \param[in]     time1            Time to compare
    //! \param[in]     time2            Time to compare
    //! \returns       false iff time1 refer to a later moment in time than time1
    //! \note Times in different timezones may be equal
    bool operator<=(const SCXCalendarTime &time1, const SCXCalendarTime &time2) {
        SCXCalendarTime::SCXCalendarTimePrecision precision = MIN(time1.m_precision, time2.m_precision);
        return SCXCalendarTime(time1).MakeUTC().ToComparablePseudoMicrosecond(precision)
            <= SCXCalendarTime(time2).MakeUTC().ToComparablePseudoMicrosecond(precision);
    }

    /*----------------------------------------------------------------------------*/
    //! Check if one time is greater than or equal to another time
    //! \param[in]     time1            Time to compare
    //! \param[in]     time2            Time to compare
    //! \returns       false iff time1 refer to an earlier moment in time than time2
    //! \note Times in different timezones may be equal
    bool operator>=(const SCXCalendarTime &time1, const SCXCalendarTime &time2) {
        SCXCalendarTime::SCXCalendarTimePrecision precision = MIN(time1.m_precision, time2.m_precision);
        return SCXCalendarTime(time1).MakeUTC().ToComparablePseudoMicrosecond(precision)
            >= SCXCalendarTime(time2).MakeUTC().ToComparablePseudoMicrosecond(precision);
    }



    /*----------------------------------------------------------------------------*/
    //! Check if one time is less than another time
    //! \param[in]     time1            Time to compare
    //! \param[in]     time2            Time to compare
    //! \returns       true iff time1 refer to an earlier moment in time than time2
    //! \note Times in different timezones may be equal
    bool operator<(const SCXCalendarTime &time1, const SCXCalendarTime &time2) {
        SCXCalendarTime::SCXCalendarTimePrecision precision = MIN(time1.m_precision, time2.m_precision);
        return SCXCalendarTime(time1).MakeUTC().ToComparablePseudoMicrosecond(precision)
            < SCXCalendarTime(time2).MakeUTC().ToComparablePseudoMicrosecond(precision);
    }

    /*---------------------------------------------------------------------l-------*/
    //! Check if one time is greater than another time
    //! \param[in]     time1            Time to compare
    //! \param[in]     time2            Time to compare
    //! \returns       true iff time1 refer to a later moment in time than time2
    //! \note Times in different timezones may be equal
    bool operator>(const SCXCalendarTime &time1, const SCXCalendarTime &time2) {
        SCXCalendarTime::SCXCalendarTimePrecision precision = MIN(time1.m_precision, time2.m_precision);
        return SCXCalendarTime(time1).MakeUTC().ToComparablePseudoMicrosecond(precision)
            > SCXCalendarTime(time2).MakeUTC().ToComparablePseudoMicrosecond(precision);
    }

    /*----------------------------------------------------------------------------*/
    //! Subtract one calendar time from another
    //! \param[in]    sum        To be subtracted from
    //! \param[in]    term    To be subtracted
    //! \returns        sum - term
    SCXAmountOfTime operator-(const SCXCalendarTime &sum, const SCXCalendarTime &term) {
        SCXASSERT(sum.m_initialized);
        SCXASSERT(term.m_initialized);
        SCXCalendarTime term1(sum);
        SCXCalendarTime term2(term);
        term1.MakeUTC();
        term2.MakeUTC();
        scxyear minYear = min(term1.m_year, term2.m_year);
        scxmonth minMonth = min(term1.m_month, term2.m_month);
        scxday minDay = min(term1.m_day, term2.m_day);
        scxhour minHour = min(term1.m_hour, term2.m_hour);
        scxminute minMinute = min(term1.m_minute, term2.m_minute);
        unsigned minMicrosecond = min(term1.m_microsecond, term2.m_microsecond);
        unsigned minDecimalCount = min(term1.m_decimalCount, term2.m_decimalCount);
        SCXCalendarTime minTime(minYear, minMonth, minDay, minHour, minMinute,
                minMicrosecond, minDecimalCount, 0);
        return SCXAmountOfTime(minTime.MicrosecondsUntil(term1) - minTime.MicrosecondsUntil(term2),
                minDecimalCount);
    }

    /*----------------------------------------------------------------------------*/
    //! Constructs a calendar time
    //! \param[in]        year              Initial year
    //! \param[in]        month             Initial month
    //! \param[in]        day               Initial day
    //! \param[in]        hour              initial hour
    //! \param[in]        minute            Initial minute
    //! \param[in]        microsecond       Initial minute
    //! \param[in]        decimalCount      Number of significant decimals
    //! \param[in]        minutesFromUTC    Number of minutes to add to UTC
    SCXCalendarTime::SCXCalendarTime(scxyear year, scxmonth month, scxday day,
                scxhour hour, scxminute minute, unsigned microsecond, unsigned decimalCount, int minutesFromUTC)
            : m_year(year), m_month(month), m_day(day),
                m_hour(hour), m_minute(minute), m_microsecond(microsecond), m_decimalCount(decimalCount),
                m_minutesFromUTC(minutesFromUTC), m_initialized(true), m_precision(eCalendarTimePrecisionSecond) {
        SCXASSERT(year >= 1970);
        SCXASSERT(IsAscending(1, month, 12));
        SCXASSERT(IsAscending(1, day, 31));
        SCXASSERT(IsAscending(0, hour, 23));
        SCXASSERT(IsAscending(0, minute, 59));
    }

    /*----------------------------------------------------------------------------*/
    //! Make a textual representation of the content
    //!\ returns Textual representation
    wstring SCXCalendarTime::DumpString() const {
        return SCXDumpStringBuilder("SCXCalendarTime")
            .Scalar("year", m_year)
            .Scalar("month", m_month)
            .Scalar("day", m_day)
            .Scalar("hour", m_hour)
            .Scalar("minute", m_minute)
            .Scalar("microsecond", m_microsecond)
            .Scalar("minutesFromUTC", m_minutesFromUTC)
            .Scalar("initialized", m_initialized)
            .Scalar("precision", m_precision);
    }

    /*----------------------------------------------------------------------------*/
    //! Add a relative amount of time to "this"
    //! \param[in]      amount      To be added
    void SCXCalendarTime::Add(const SCXRelativeTime &amount) {
        m_year += amount.m_years;
        AdjustDayOfMonth();
        if (amount.m_months > 0) {
            AddMonths(amount.m_months);
        } else if (amount.m_months < 0){
            SubtractMonths(-amount.m_months);
        }
        if (amount.m_days > 0) {
            AddDays(amount.m_days);
        } else if (amount.m_days < 0) {
            SubtractDays(-amount.m_days);
        }
        if (amount.m_hours > 0) {
            AddHours(amount.m_hours);
        } else if (amount.m_hours < 0) {
            SubtractHours(-amount.m_hours);
        }
        if (amount.m_minutes > 0) {
            AddMinutes(amount.m_minutes);
        } else if (amount.m_minutes < 0){
            SubtractMinutes(-amount.m_minutes);
        }
        if (amount.m_microseconds >= 0) {
            AddMicroseconds(amount.m_microseconds);
        } else if (amount.m_microseconds < 0) {
            SubtractMicroseconds(-amount.m_microseconds);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Add a number of months
    //! \param[in]  months      To be added
    void SCXCalendarTime::AddMonths(unsigned months) {
        m_year += months / 12;
        months %= 12;
        if (m_month + months > 12) {
            m_month += months;
            ++m_year;
            m_month -= 12;
        } else {
            m_month += months;
        }
        AdjustDayOfMonth();
    }

    /*----------------------------------------------------------------------------*/
    //! Subtract a number of months
    //! \param[in]  months      To be subtracted
    void SCXCalendarTime::SubtractMonths(unsigned months) {
        m_year -= months / 12;
        months %= 12;
        if (months >= m_month) {
            --m_year;
            m_month += 12;
            m_month -= months;
        } else {
            m_month -= months;
        }
        AdjustDayOfMonth();
    }

    /*----------------------------------------------------------------------------*/
    //! Add a number of days
    //! \param[in]  days      To be added
    void SCXCalendarTime::AddDays(unsigned days) {
        unsigned daysInYear = DaysInYear(m_year);
        while (days >= daysInYear) {
            days -= daysInYear;
            ++m_year;
            daysInYear = DaysInYear(m_year);
        }
        unsigned daysInMonth = DaysInMonth(m_year, m_month);
        while (days >= daysInMonth) {
            days -= daysInMonth;
            CalculateNextMonth(m_year, m_month);
            daysInMonth = DaysInMonth(m_year, m_month);
        }
        if (m_day + days > daysInMonth) {
            CalculateNextMonth(m_year, m_month);
            m_day += days;
            m_day -= daysInMonth;
        } else {
            m_day += days;
        }
        AdjustDayOfMonth();
    }

    /*----------------------------------------------------------------------------*/
    //! Subtract a number of days
    //! \param[in]  days      To be subtracted
    void SCXCalendarTime::SubtractDays(unsigned days) {
        unsigned daysInYear = DaysInYear(m_year - 1);
        while (days >= daysInYear) {
            days -= daysInYear;
            --m_year;
            daysInYear = DaysInYear(m_year - 1);
        }
        unsigned daysInMonth = DaysInPriorMonth(m_year, m_month);
        while (days >= daysInMonth) {
            days -= daysInMonth;
            CalculatePriorMonth(m_year, m_month);
            daysInMonth = DaysInPriorMonth(m_year, m_month);
        }
        if (days >= m_day) {
            CalculatePriorMonth(m_year, m_month);
            m_day = DaysInMonth(m_year, m_month) - (days - m_day);
        } else {
            m_day -= days;
        }
        AdjustDayOfMonth();
    }

    /*----------------------------------------------------------------------------*/
    //! Add a number of hours
    //! \param[in]  hours      To be added
    void SCXCalendarTime::AddHours(unsigned hours) {
        unsigned hoursInYear = HoursInYear(m_year);
        while (hours >= hoursInYear) {
            hours -= hoursInYear;
            ++m_year;
            hoursInYear = HoursInYear(m_year);
        }
        unsigned hoursInMonth = HoursInMonth(m_year, m_month);
        while (hours >= hoursInMonth) {
            hours -= hoursInMonth;
            CalculateNextMonth(m_year, m_month);
            hoursInMonth = HoursInMonth(m_year, m_month);
        }
        AdjustDayOfMonth();
        unsigned hoursInDay = HoursInDay(m_year, m_month, m_day);
        while (hours >= hoursInDay) {
            hours -= hoursInDay;
            CalculateNextDay(m_year, m_month, m_day);
            hoursInDay = HoursInDay(m_year, m_month, m_day);
        }
        if (m_hour + hours > 23) {
            m_hour += hours;
            m_hour -= 24;
            CalculateNextDay(m_year, m_month, m_day);
        } else {
            m_hour += hours;
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Subtract a number of hours
    //! \param[in]  hours      To be subtracted
    void SCXCalendarTime::SubtractHours(unsigned hours) {
        unsigned hoursInYear = HoursInYear(m_year - 1);
        while (hours >= hoursInYear) {
            hours -= hoursInYear;
            --m_year;
            hoursInYear = HoursInYear(m_year - 1);
        }
        unsigned hoursInMonth = HoursInPriorMonth(m_year, m_month);
        while (hours >= hoursInMonth) {
            hours -= hoursInMonth;
            CalculatePriorMonth(m_year, m_month);
            hoursInMonth = HoursInPriorMonth(m_year, m_month);
        }
        AdjustDayOfMonth();
        unsigned hoursInDay = HoursInPriorDay(m_year, m_month, m_day);
        while (hours >= hoursInDay) {
            hours -= hoursInDay;
            CalculatePriorDay(m_year, m_month, m_day);
            hoursInDay = HoursInPriorDay(m_year, m_month, m_day);
        }
        if (hours > m_hour) {
            m_hour += 24;
            m_hour -= hours;
            CalculatePriorDay(m_year, m_month, m_day);
        } else {
            m_hour -= hours;
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Add a number of minutes
    //! \param[in]  minutes      To be added
    void SCXCalendarTime::AddMinutes(unsigned minutes) {
        unsigned minutesInYear = MinutesInYear(m_year);
        while (minutes >= minutesInYear) {
            minutes -= minutesInYear;
            ++m_year;
            minutesInYear = MinutesInYear(m_year);
        }
        unsigned minutesInMonth = MinutesInMonth(m_year, m_month);
        while (minutes >= minutesInMonth) {
            minutes -= minutesInMonth;
            CalculateNextMonth(m_year, m_month);
            minutesInMonth = MinutesInMonth(m_year, m_month);
        }
        AdjustDayOfMonth();
        unsigned minutesInDay = MinutesInDay(m_year, m_month, m_day);
        while (minutes >= minutesInDay) {
            minutes -= minutesInDay;
            CalculateNextDay(m_year, m_month, m_day);
            minutesInDay = MinutesInDay(m_year, m_month, m_day);
        }
        unsigned minutesInHour = MinutesInHour(m_year, m_month, m_day, m_hour);
        while (minutes >= minutesInHour) {
            minutes -= minutesInHour;
            CalculateNextHour(m_year, m_month, m_day, m_hour);
            minutesInHour = MinutesInHour(m_year, m_month, m_day, m_hour);
        }
        if (m_minute + minutes >= 60) {
            CalculateNextHour(m_year, m_month, m_day, m_hour);
            m_minute += minutes;
            m_minute -= 60;
        } else {
            m_minute += minutes;
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Subtract a number of minutes
    //! \param[in]  minutes      To be subtracted
    void SCXCalendarTime::SubtractMinutes(unsigned minutes) {
        unsigned minutesInYear = MinutesInYear(m_year - 1);
        while (minutes >= minutesInYear) {
            minutes -= minutesInYear;
            --m_year;
            minutesInYear = MinutesInYear(m_year - 1);
        }
        unsigned minutesInMonth = MinutesInPriorMonth(m_year, m_month);
        while (minutes >= minutesInMonth) {
            minutes -= minutesInMonth;
            CalculatePriorMonth(m_year, m_month);
            minutesInMonth = MinutesInPriorMonth(m_year, m_month);
        }
        AdjustDayOfMonth();
        unsigned minutesInDay = MinutesInPriorDay(m_year, m_month, m_day);
        while (minutes >= minutesInDay) {
            minutes -= minutesInDay;
            CalculatePriorDay(m_year, m_month, m_day);
            minutesInDay = MinutesInPriorDay(m_year, m_month, m_day);
        }
        unsigned minutesInHour = MinutesInPriorHour(m_year, m_month, m_day, m_hour);
        while (minutes >= minutesInHour) {
            minutes -= minutesInHour;
            CalculatePriorHour(m_year, m_month, m_day, m_hour);
            minutesInHour = MinutesInPriorHour(m_year, m_month, m_day, m_hour);
        }
        if (minutes > m_minute) {
            CalculatePriorHour(m_year, m_month, m_day, m_hour);
            m_minute += 60;
        }
        m_minute -= minutes;
    }

    /*----------------------------------------------------------------------------*/
    //! Add a number of microseconds
    //! \param[in]  microseconds      To be added
    void SCXCalendarTime::AddMicroseconds(scxulong microseconds) {
        scxulong microsecondsInYear = MicrosecondsInYear(m_year);
        while (microseconds >= microsecondsInYear) {
            microseconds -= microsecondsInYear;
            ++m_year;
            microsecondsInYear = MicrosecondsInYear(m_year);
        }
        scxulong microsecondsInMonth = MicrosecondsInMonth(m_year, m_month);
        while (microseconds >= microsecondsInMonth) {
            microseconds -= microsecondsInMonth;
            CalculateNextMonth(m_year, m_month);
            microsecondsInMonth = MicrosecondsInMonth(m_year, m_month);
        }
        AdjustDayOfMonth();
        scxulong microsecondsInDay = MicrosecondsInDay(m_year, m_month, m_day);
        while (microseconds >= microsecondsInDay) {
            microseconds -= microsecondsInDay;
            CalculateNextDay(m_year, m_month, m_day);
            microsecondsInDay = MicrosecondsInDay(m_year, m_month, m_day);
        }
        scxulong microsecondsInHour = MicrosecondsInHour(m_year, m_month, m_day, m_hour);
        while (microseconds >= microsecondsInHour) {
            microseconds -= microsecondsInHour;
            CalculateNextHour(m_year, m_month, m_day, m_hour);
            microsecondsInHour = MicrosecondsInHour(m_year, m_month, m_day, m_hour);
        }
        scxulong microsecondsInMinute = MicrosecondsInMinute(m_year, m_month, m_day, m_hour, m_minute);
        while (microseconds >= microsecondsInMinute) {
            microseconds -= microsecondsInMinute;
            CalculateNextMinute(m_year, m_month, m_day, m_hour, m_minute);
            microsecondsInMinute = MicrosecondsInMinute(m_year, m_month, m_day, m_hour, m_minute);
        }
        if (m_microsecond + microseconds >= 60 * 1000 * 1000) {
            CalculateNextMinute(m_year, m_month, m_day, m_hour, m_minute);
            m_microsecond += static_cast<unsigned>(microseconds);
            m_microsecond -= 60 * 1000 * 1000;
        } else {
            m_microsecond += static_cast<unsigned>(microseconds);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Subtract a number of microseconds
    //! \param[in]  microseconds      To be subtracted
    void SCXCalendarTime::SubtractMicroseconds(scxulong microseconds) {
        scxulong microsecondsInYear = MicrosecondsInYear(m_year - 1);
        while (microseconds >= microsecondsInYear) {
            microseconds -= microsecondsInYear;
            --m_year;
            microsecondsInYear = MicrosecondsInYear(m_year - 1);
        }
        scxulong microsecondsInMonth = MicrosecondsInPriorMonth(m_year, m_month);
        while (microseconds >= microsecondsInMonth) {
            microseconds -= microsecondsInMonth;
            CalculatePriorMonth(m_year, m_month);
            microsecondsInMonth = MicrosecondsInPriorMonth(m_year, m_month);
        }
        AdjustDayOfMonth();
        scxulong microsecondsInDay = MicrosecondsInPriorDay(m_year, m_month, m_day);
        while (microseconds >= microsecondsInDay) {
            microseconds -= microsecondsInDay;
            CalculatePriorDay(m_year, m_month, m_day);
            microsecondsInDay = MicrosecondsInPriorDay(m_year, m_month, m_day);
        }
        scxulong microsecondsInHour = MicrosecondsInPriorHour(m_year, m_month, m_day, m_hour);
        while (microseconds >= microsecondsInHour) {
            microseconds -= microsecondsInHour;
            CalculatePriorHour(m_year, m_month, m_day, m_hour);
            microsecondsInHour = MicrosecondsInPriorHour(m_year, m_month, m_day, m_hour);
        }
        scxulong microsecondsInMinute = MicrosecondsInPriorMinute(m_year, m_month, m_day, m_hour, m_minute);
        while (microseconds > microsecondsInMinute) {
            microseconds -= microsecondsInMinute;
            CalculatePriorMinute(m_year, m_month, m_day, m_hour, m_minute);
            microsecondsInMinute = MicrosecondsInPriorMinute(m_year, m_month, m_day, m_hour, m_minute);
        }
        if (microseconds > m_microsecond) {
            CalculatePriorMinute(m_year, m_month, m_day, m_hour, m_minute);
            m_microsecond += 60 * 1000 * 1000;
            m_microsecond -= static_cast<unsigned>(microseconds);;
        } else {
            m_microsecond -= static_cast<unsigned>(microseconds);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Adjust day of month to comply with number of days in month.
    //! Handle overflow by moving into next month
    void SCXCalendarTime::AdjustDayOfMonth() {
        // Since no month has more days  than december (31) we will never have to adjust year
        unsigned int daysInMonth = DaysInMonth(m_year, m_month);
        if (m_day > daysInMonth) {
            m_day -= daysInMonth;
            ++m_month;
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Calculate a new timestamp by adding a relative amount of time to an existing timestamp
    //! \param[in]  time    Existing timestamp
    //! \param[in]  amount  To be added
    //! \returns    New calculated timestamp
    SCXCalendarTime operator+(const SCXCalendarTime &time, const SCXRelativeTime &amount) {
        return SCXCalendarTime(time) += amount;
    }

    /*----------------------------------------------------------------------------*/
    //! Calculate a new timestamp by adding a relative amount of time to an existing timestamp
    //! \param[in]  time    Existing timestamp
    //! \param[in]  amount  To be added
    //! \returns    New calculated timestamp
    SCXCalendarTime operator+(const SCXRelativeTime &amount, const SCXCalendarTime &time) {
        return SCXCalendarTime(time) += amount;
    }

    /*----------------------------------------------------------------------------*/
    //! Subtracts a relative amount of time from an existing timestamp
    //! \param[in]  time    To be subtracted from
    //! \param[in]  amount  To be subtracted
    //! \returns    New calculated timestamp
    SCXCalendarTime operator-(const SCXCalendarTime &time, const SCXRelativeTime &amount) {
        return SCXCalendarTime(time) -= amount;
    }

    /******************************************************************************************/
    //! Checks if two moments in time does not differ more than that they may be considered equivalent
    //! \param[in]      time1       One moment in time
    //! \param[in]      time2       Another momement in time
    //! \param[in]      tolerance   Accepted difference
    //! \returns        true iff "time1" and "time2" does not differ more than "tolerance"
    bool Equivalent(const SCXCalendarTime &time1, const SCXCalendarTime &time2, SCXAmountOfTime tolerance) {
        return Abs(time1 - time2) <= tolerance;
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
