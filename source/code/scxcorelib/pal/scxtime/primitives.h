/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Time primitives 
    
    \date        07-10-29 11:23:02

    < Optional detailed description of file purpose >
    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXTIME_PRIMITIVES_H
#define SCXTIME_PRIMITIVES_H

#include <scxcorelib/scxtime.h>

namespace SCXCoreLib {
    bool IsLeapYear(SCXCoreLib::scxyear year);
    unsigned DaysInMonth(SCXCoreLib::scxyear year, SCXCoreLib::scxmonth month);
    void CalculatePriorMonth(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month);
    void CalculateNextMonth(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month);
    void CalculatePriorDay(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month, SCXCoreLib::scxday &day);
    void CalculateNextDay(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month, SCXCoreLib::scxday &day);
    void CalculatePriorHour(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month, SCXCoreLib::scxday &day, SCXCoreLib::scxhour &hour);
    void CalculateNextHour(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month, SCXCoreLib::scxday &day, SCXCoreLib::scxhour &hour);
    void CalculatePriorMinute(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month, SCXCoreLib::scxday &day, SCXCoreLib::scxhour &hour, SCXCoreLib::scxminute &minute);
    void CalculateNextMinute(SCXCoreLib::scxyear &year, SCXCoreLib::scxmonth &month, SCXCoreLib::scxday &day, SCXCoreLib::scxhour &hour, SCXCoreLib::scxminute &minute);
    unsigned DaysInYear(SCXCoreLib::scxyear year);
    unsigned HoursInDay(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday); 
    unsigned MinutesInDay(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday);
    unsigned MinutesInPriorDay(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday);
    unsigned DaysInPriorMonth(SCXCoreLib::scxyear, SCXCoreLib::scxmonth);
    unsigned HoursInMonth(SCXCoreLib::scxyear, SCXCoreLib::scxmonth);
    unsigned HoursInPriorMonth(SCXCoreLib::scxyear, SCXCoreLib::scxmonth);
    unsigned HoursInYear(SCXCoreLib::scxyear);
    unsigned HoursInPriorDay(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday);
    unsigned MinutesInMonth(SCXCoreLib::scxyear, SCXCoreLib::scxmonth);
    unsigned MinutesInPriorMonth(SCXCoreLib::scxyear year, SCXCoreLib::scxmonth month);
    unsigned MinutesInYear(SCXCoreLib::scxyear);
    unsigned MinutesInHour(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday, SCXCoreLib::scxhour);
    unsigned MinutesInPriorHour(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday, SCXCoreLib::scxhour);
    unsigned MicrosecondsInMinute(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday, SCXCoreLib::scxhour, SCXCoreLib::scxminute);
    scxulong MicrosecondsInHour(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday, SCXCoreLib::scxhour);
    scxulong MicrosecondsInDay(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday);
    scxulong MicrosecondsInMonth(SCXCoreLib::scxyear year, SCXCoreLib::scxmonth month);
    scxulong MicrosecondsInYear(SCXCoreLib::scxyear year);
    unsigned MicrosecondsInPriorMinute(SCXCoreLib::scxyear, SCXCoreLib::scxmonth, SCXCoreLib::scxday, SCXCoreLib::scxhour, SCXCoreLib::scxminute);
    scxulong MicrosecondsInPriorHour(SCXCoreLib::scxyear year, SCXCoreLib::scxmonth month, SCXCoreLib::scxday day, SCXCoreLib::scxhour hour);
    scxulong MicrosecondsInPriorDay(SCXCoreLib::scxyear year, SCXCoreLib::scxmonth month, SCXCoreLib::scxday day);
    scxulong MicrosecondsInPriorMonth(SCXCoreLib::scxyear year, SCXCoreLib::scxmonth month);   
}

#endif /* SCXTIME_PRIMITIVES_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
