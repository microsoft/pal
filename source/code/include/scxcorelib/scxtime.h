/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
*/ 
/**
    \file        

    \brief       Specification of time related functionality 
     
    \date        07-09-25 16:14:00

    \note        The specification will be implemented incrementally  
       
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXTIME_H
#define SCXTIME_H

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <string>

namespace SCXCoreLib {

    typedef unsigned int scxyear;       //!< Represents a year
    typedef unsigned int scxmonth;    //!< Represents a month of a year
    typedef unsigned int scxday;      //!< Represents a day of a month
    typedef unsigned int scxhour;     //!< Represents an hour of a day
    typedef unsigned int scxminute;   //!< Represents a minute of an hour
    typedef unsigned int scxdecimalnr;  //!< Number of decimals (3.001 has 3 decimals)
    typedef double scxsecond;             //!< A single value second (0 <= second <= 60)
    typedef double scxseconds;            //!< An arbitrary amount of seconds 

    /** 
         Some text was not formatted as expected.
     */
    class SCXInvalidTimeFormatException : public SCXException {
    public:
        //! Constructor
        SCXInvalidTimeFormatException(const std::wstring &problem, 
                const std::wstring &invalidText, const SCXCodeLocation &location)
            : SCXException(location), m_problem(problem), m_invalidText(invalidText)
        {}

        //! Retrieve the text that caused the problem
        //! \returns The text that caused the problem
        std::wstring GetInvalidText() const {return m_invalidText;}
        
        std::wstring What() const;
    private:
        std::wstring m_problem;      //!< Description of the problem
        std::wstring m_invalidText;  //!< Text that caused the problem
    };

    class SCXCalendarTime;
    class SCXRelativeTime;
    
    /**
     * \brief Represents Amount of time in seconds. 
     * 
     * Seconds is the largest unit of time that may be used to represent an amount of time 
     * in a unique way. Using minutes is not an option since a minute relative some moment 
     * in time may be ambigous due to leap seconds.
     * 
     * Amounts of time may be added, subtracted as well as compared. 
     * 
     * \note Seconds are represented by a floating point number to make it independant
     * of the actual implementation, as well as scalable concerning precision. However, 
     * the internal implementation assures that there is no rounding errors or comparison 
     * problems when working with SCXAmountOfTime  
     */
    class SCXAmountOfTime {
    public:        
        //! Constructs a zero SCXAmountOfTime
        //! \note Takes no integer or float parameter to avoid ambiguity concerning the unit.
        //!       Writing "SCXAmountOfTime().SetSeconds(10)" clearly states what unit you mean.
        //!
        //! \note We have m_filler on certain platforms to quiet down Purify due to alignment issues
#if defined(sun)
        SCXAmountOfTime() : m_microseconds(0), m_decimalCount(6), m_filler(0) { };
#else
        SCXAmountOfTime() : m_microseconds(0), m_decimalCount(6) { };
#endif

        scxseconds GetSeconds() const;
        SCXAmountOfTime &SetSeconds(scxseconds seconds);

        scxdecimalnr GetDecimalCount() const;
        SCXAmountOfTime &SetDecimalCount(scxdecimalnr decimalCount);

        SCXAmountOfTime& operator+=(SCXAmountOfTime);
        SCXAmountOfTime& operator-=(SCXAmountOfTime);

        friend SCXAmountOfTime Abs(SCXAmountOfTime amount);
        
        friend bool operator==(SCXAmountOfTime, SCXAmountOfTime);
        friend bool operator!=(SCXAmountOfTime, SCXAmountOfTime);
        friend bool operator<(SCXAmountOfTime, SCXAmountOfTime);
        friend bool operator>(SCXAmountOfTime, SCXAmountOfTime);
        friend bool operator<=(SCXAmountOfTime, SCXAmountOfTime);
        friend bool operator>=(SCXAmountOfTime, SCXAmountOfTime);
        friend SCXAmountOfTime operator-(const SCXCalendarTime &, const SCXCalendarTime &);    

    private:
        friend class SCXRelativeTime;
   
        //! Constructs an SCXAmountOfTime given an amount of microseconds
        //! \param[in]    microseconds    Amount of microseconds
        //! \param[in]    decimalCount    Number of significant decimals
        //! \note Is private to encapsulate the implementation unit "microseconds"
        SCXAmountOfTime(scxlong microseconds, unsigned decimalCount) 
#if defined(sun)
                : m_microseconds(microseconds), m_decimalCount(decimalCount), m_filler(0) { }
#else
                : m_microseconds(microseconds), m_decimalCount(decimalCount) { }
#endif
        
        scxlong m_microseconds; //!< Non floating point to avoid rounding errors
        scxdecimalnr m_decimalCount;    //!< Precision, number of significant digits of seconds
#if defined(sun)
        scxdecimalnr m_filler;  //!< Filler for alignment purposes (used only to pacify Purify)
#endif
    };

    SCXAmountOfTime operator-(const SCXAmountOfTime &);
    SCXAmountOfTime operator+(SCXAmountOfTime, SCXAmountOfTime);    
    SCXAmountOfTime operator-(SCXAmountOfTime, SCXAmountOfTime);        
    bool IsEquivalent(SCXAmountOfTime amount1, SCXAmountOfTime amount2, SCXAmountOfTime tolerance);
     
    /**
     * \brief Represents an amount of time relative some moment in time.
     * 
     * In contrast to SCXAmountOfTime that always represents the same amount
     * of time, the same SCXRelativeTime may represent different amount of time dependent 
     * on what moment it is taken relative. For example, a month in february is
     * not the same amount of time as a month in mars. Relative times may therefore 
     * only be compared for memberwise equality, the class do not offer any means
      * to determine if two relative times represent the same amount of time or if one 
      * relative time represents a larger mount of time than another.
      * \note The relative time do not have to be normalized, that is, time may be
      * expressed using the units of choice.
     */
    class SCXRelativeTime {        
    public:
        //! Constructor for no time        
        SCXRelativeTime() : m_years(0), m_months(0), m_days(0), 
                m_hours(0), m_minutes(0), m_microseconds(0), m_decimalCount(6) { }
        
        SCXRelativeTime(int years, int months, int days, int hours, int minutes, 
                double seconds, unsigned decimalCount);

        SCXRelativeTime(int years, int months, int days, int hours, int minutes, 
                double seconds);

        //! Constructor
        //! \param[in]   amount     Amount of time
        SCXRelativeTime(const SCXAmountOfTime &amount): m_years(0), m_months(0), m_days(0), 
                m_hours(0), m_minutes(0), m_microseconds(amount.m_microseconds), 
                m_decimalCount(amount.m_decimalCount) { }

        
        bool IsValidAsOffsetFromUTC() const;
        
        //! Number of years, positive as well as negative
        int GetYears() const { return m_years; }

        //! Number of months, may be greater than 12 as well as negative
        int GetMonths() const { return m_months; }
        
        //! Number of days, may be greater than 31 as well as negative
        int GetDays() const { return m_days; }          
        
        //! Number of hours, may be greater than 24 as well as negative
        int GetHours() const { return m_hours; }
        
        //! Number of minutes, may be greater han 60 as well as negative
        int GetMinutes() const { return m_minutes; }    
        
        scxseconds GetSeconds() const;  
        
        scxdecimalnr GetDecimalCount() const;
                
        //! Change number of years
        //! \param[in]    years      Number of years, positive as well as negative
        //! \returns The updated relative time
        SCXRelativeTime &SetYears(int years) {m_years = years; return *this;}
        
        //! Change number of months
        //! \param[in]    months     Number of months, may be greater than 12 as well as negative
        //! \returns The updated relative time
        SCXRelativeTime &SetMonths(int months) {m_months = months; return *this;}
        
        //! Change number of days
        //! \param[in]   days        Number of days, may be greater than 31 as well as negative
        //! \returns The updated relative time
        SCXRelativeTime &SetDays(int days){m_days = days; return *this;}
        
        //! Change number of hours
        //! \param[in]    hours      Number of hours, may be greater than 24 as well as negative        
        //! \returns The updated relative time
        SCXRelativeTime &SetHours(int hours) {m_hours = hours; return *this;}
        
        //! Number of minutes
        //! \param[in]    minutes    Number of minutes, may be greater han 60 as well as negative  
        //! \returns The updated relative time
        SCXRelativeTime &SetMinutes(int minutes) {m_minutes = minutes; return *this; }
        
        SCXRelativeTime &SetSeconds(scxseconds seconds);
        
        SCXRelativeTime &SetDecimalCount(scxdecimalnr decimalCount);

        friend bool IsIdentical(const SCXRelativeTime &, const SCXRelativeTime &);

        SCXRelativeTime& operator+=(const SCXRelativeTime&);
        SCXRelativeTime& operator-=(const SCXRelativeTime&);
      
        friend bool operator==(const SCXRelativeTime &, const SCXRelativeTime &);
        friend bool operator!=(const SCXRelativeTime &, const SCXRelativeTime &);    

        //void Normalize(const SCXCalendarTime &base);

        std::wstring ToBasicISO8601Time() const;
        std::wstring ToExtendedISO8601Time() const;
        
        std::wstring DumpString() const;
 
    private:
        friend class SCXCalendarTime;
        std::wstring ToISO8601Time(std::wstring timeSeparator) const;
        
        int m_years;                //!< Number of years, positive as well as negative
        int m_months;               //!< Number of months, may be greater than 12 as well as negative
        int m_days;                 //!< Number of days, may be greater than 31 as well as negative
        int m_hours;                //!< Number of hours, may be greater than 24 as well as negative
        int m_minutes;              //!< Number of minutes, may be greater han 60 as well as negative
        scxlong m_microseconds;         //!< Number of microseconds, may be greater than 1000000 as well as negative
        scxdecimalnr m_decimalCount;    //!< Precision, number of significant digits of seconds
    };

    SCXRelativeTime operator-(const SCXRelativeTime &);
    SCXRelativeTime operator+(const SCXRelativeTime&, const SCXRelativeTime&);        
    SCXRelativeTime operator-(const SCXRelativeTime&, const SCXRelativeTime&);    

    
    /*----------------------------------------------------------------------------*/
    /**
        \brief A moment in time including both date and time of day as well as the
        timezone.
        
           A calendar time always has to include the timezone, otherwise it would be ambigous.
           Calendar times may be subtracted, yielding the amount of time between the individual
           times. An amount of time may be added to a calendar time yielding a new calendar time.
           Calendar times may also be compared to be determine their cronological order.
           
           The first month of a year as well as the first day of a month has the value "1". The
           first hour and the last hour of a day has the values "0" and "23", respectively.
           
           This class is responsible for convertng to and from other simple representations
           of moments in time as well. One example of such a format is the 
           CIM DATETIME textual format. A Cim timestamp is formatted according as:
           YYYYMMDDhhmmss.uuuuuuSzzz where "uuuuuu" represents microsecconds and "zzz"
           numbers of minutes from UTC. The "S" represents east/west of UTC,  "+" means
           east and "-" means west.
                       
           \note Though the implementation on current platforms might not place any restrictions 
           on "year", calendar times earlier than 1970 is not allowed. It is easier to lift that
           restriction later when it is discovered that it is not needed on any platform, than it
           is to introduce it if needed.  
    */
    class SCXCalendarTime {
    public:
        /**
            Represents the precision of an SCXCalendarTime.
         */
        enum SCXCalendarTimePrecision {
            eCalendarTimePrecisionUnknown = 0,
            eCalendarTimePrecisionYear = 1,
            eCalendarTimePrecisionMonth = 2,
            eCalendarTimePrecisionDay = 3,
            eCalendarTimePrecisionHour = 4,
            eCalendarTimePrecisionMinute = 5,
            eCalendarTimePrecisionSecond = 6
        };
        
        static SCXCalendarTime FromPosixTime(scxlong seconds);
        static SCXCalendarTime FromISO8601(std::wstring);
        static SCXCalendarTime FromCIM(std::wstring);
        
        static SCXCalendarTime CurrentUTC();        
        static SCXCalendarTime CurrentLocal();
        static SCXRelativeTime CurrentOffsetFromUTC();
        
        SCXCalendarTime();
        SCXCalendarTime(scxyear, scxmonth, scxday, scxhour, scxminute, scxsecond, scxdecimalnr decimalCount,
                const SCXRelativeTime &offsetFromUTC);
        SCXCalendarTime(scxyear, scxmonth, scxday, scxhour, scxminute, scxsecond, const SCXRelativeTime &offsetFromUTC);
        SCXCalendarTime(scxyear, scxmonth, scxday);
         
        SCXCalendarTime &operator=(const SCXCalendarTime &);
        
        bool IsUTC() const;
        scxyear GetYear() const;
        scxmonth GetMonth() const;      
        scxday GetDay() const;          
        scxhour GetHour() const;        
        scxminute GetMinute() const;    
        scxsecond GetSecond() const;    
        SCXRelativeTime GetOffsetFromUTC() const;  
        scxdecimalnr GetDecimalCount() const;
        SCXCalendarTimePrecision GetPrecision() const;
        
        void SetYear(scxyear);
        void SetMonth(scxmonth);
        void SetDay(scxday);
        void SetHour(scxhour);
        void SetMinute(scxminute);
        void SetSecond(scxsecond);
        void SetOffsetFromUTC(const SCXRelativeTime&);
        void SetDecimalCount(scxdecimalnr decimalCount);
        void SetPrecision(SCXCalendarTimePrecision precision);

        SCXRelativeTime GetTimeOfDay() const;
        void SetTimeOfDay(const SCXRelativeTime &);
        
        SCXCalendarTime& MakeLocal(SCXRelativeTime offsetFromUTC);
        SCXCalendarTime& MakeUTC();
                
        std::wstring ToCIM() const;
        std::wstring ToBasicISO8601() const;
        std::wstring ToExtendedISO8601() const;
        std::wstring ToLocalizedTime() const;
        scxlong ToPosixTime() const;
        
        SCXCalendarTime& operator+=(const SCXRelativeTime&);
        SCXCalendarTime& operator-=(const SCXRelativeTime&);
        
        SCXAmountOfTime AmountOfTime(const SCXRelativeTime&) const;
        //SCXRelativeTime RelativeTime(SCXAmountOfTime) const;
        
        std::wstring DumpString() const;

        /** Tests if instance was initialized with a proper value. */
        bool IsInitialized() const { return m_initialized; }

        friend bool IsIdentical(const SCXCalendarTime &, const SCXCalendarTime &);

        friend bool operator==(const SCXCalendarTime &, const SCXCalendarTime &);
        friend bool operator!=(const SCXCalendarTime &, const SCXCalendarTime &);    
        friend bool operator<(const SCXCalendarTime &, const SCXCalendarTime &);
        friend bool operator>(const SCXCalendarTime &, const SCXCalendarTime &);
        friend bool operator<=(const SCXCalendarTime &, const SCXCalendarTime &);
        friend bool operator>=(const SCXCalendarTime &, const SCXCalendarTime &);
        friend SCXAmountOfTime operator-(const SCXCalendarTime &, const SCXCalendarTime &);    

    private: 
        static const SCXCalendarTime cUnixEpoch;
        static const unsigned cPosixSecondsInDay = 86400; //!< Seconds of day according to Posix
        
        static SCXCalendarTime DateFromIso8601(const std::wstring &str);
        static SCXRelativeTime OffsetFromUTCFromIso8601(const std::wstring &str);
        static SCXRelativeTime TimeFromIso8601(const std::wstring &str);
        SCXCalendarTime(scxyear, scxmonth, scxday, scxhour, scxminute, unsigned microsecond, unsigned decimalCount, 
                int minutesFromUTC);
        scxulong ToComparablePseudoMicrosecond(SCXCalendarTimePrecision precision) const;
        scxulong MicrosecondsUntil(const SCXCalendarTime &) const;
        std::wstring ToISO8601(std::wstring dateSeparator, std::wstring timeSeparator) const;
        void Add(const SCXRelativeTime &amount);
        void AddMonths(unsigned months);
        void AddDays(unsigned days);
        void AddHours(unsigned hours);
        void AddMinutes(unsigned minutes);
        void AddMicroseconds(scxulong microseconds);
        void SubtractMonths(unsigned months);
        void SubtractDays(unsigned days);
        void SubtractHours(unsigned hours);
        void SubtractMinutes(unsigned minutes);
        void SubtractMicroseconds(scxulong microseconds);
        void AdjustDayOfMonth();
        
        scxyear m_year;                 //!< m_year >= 1970
        scxmonth m_month;               //!< 1 <= m_month <= 12
        scxday m_day;                   //!< 1 <= m_day <= 31
        scxhour m_hour;                 //!< 0 <= m_hour <= 23
        scxminute m_minute;             //!< 0 <= m_minute <= 59;
        unsigned int m_microsecond;     //!< 0 <= m_microsecond <= 59 999 999
        scxdecimalnr m_decimalCount;    //!< Second precision, number of significant digits of seconds
        int m_minutesFromUTC;           //!< -13*60 <= m_minutesFromUTC <= 12 * 60
        bool m_initialized;             //!< May the instance be used. Needed to support default constructor
        SCXCalendarTimePrecision m_precision; //!< Precision, what parts of calendar time is significant
    };
         
    SCXCalendarTime operator+(const SCXCalendarTime &, const SCXRelativeTime &);
    SCXCalendarTime operator+(const SCXRelativeTime &, const SCXCalendarTime &);
    
    SCXCalendarTime operator-(const SCXCalendarTime &, const SCXRelativeTime &);
                    
    bool Equivalent(const SCXCalendarTime &time1, const SCXCalendarTime &time2, SCXAmountOfTime tolerance);
    

}

#endif /* SCXTIME_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
