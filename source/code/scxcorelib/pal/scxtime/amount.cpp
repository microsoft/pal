/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Representation and calculation concerning amount of times
    
    \date        07-10-29 14:00:00
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxtime.h>
#include <scxcorelib/scxmath.h>
#include <algorithm>


namespace SCXCoreLib {
/*----------------------------------------------------------------------------*/
//! Amount of seconds
//! \returns Amount of seconds
//! \note Parts of seconds are represented as the decimal part
scxseconds SCXAmountOfTime::GetSeconds() const {
    return static_cast<scxseconds> (m_microseconds) / 1000000;
}
  
/*----------------------------------------------------------------------------*/
//! Change amount of seconds
//! \param[in]    seconds        Amount of seconds
//! \returns      *this
//! \note Parts of seconds are represented as the decimal part
SCXAmountOfTime &SCXAmountOfTime::SetSeconds(scxseconds seconds) {
    m_microseconds = RoundToScxLong(seconds * 1000000);
    return *this;
}

/*----------------------------------------------------------------------------*/
//! Number of significant decimals
//! \returns Number of significant decimals
scxdecimalnr SCXAmountOfTime::GetDecimalCount() const {
    return m_decimalCount;
}

/*----------------------------------------------------------------------------*/
//! Change information about number of significant decimals
//! \param[in]   decimalCount                    New number of significant decimals
//! \throws         SCXIllegalArgumentException     Invalid number of decimals 
//! Determines for example the default number of decimals when converting to
//! a decimal textual format. 
//! Needed because the binary representation of, for example, 3 and 3.00 is the same.
//! \note Does not change the amount of time, just information _about_ the time. Lowering
//! the number of significant digits does not remove any digits of time, in the same way
//! as increasing the number of significant digits does not add any new digits of time.
//! \note The current implementation support up to and including 6 decimals
SCXAmountOfTime &SCXAmountOfTime::SetDecimalCount(scxdecimalnr decimalCount) {
    if (!IsAscending(0, decimalCount, 6)) {
        throw SCXIllegalIndexException<scxdecimalnr>(L"decimalCount", decimalCount, 0, true, 6, true, SCXSRCLOCATION);
    }
    m_decimalCount = decimalCount;
    return *this;
}

/*----------------------------------------------------------------------------*/
//! Add an amount to the current
//! \param[in]    amount        To be added   
//! \returns      *this
//! \note Adding an amount with fewer significant digits lowers
//!       the number of significant digits of the current amount as well.
SCXAmountOfTime& SCXAmountOfTime::operator+=(SCXAmountOfTime amount) {
    m_microseconds += amount.m_microseconds;
    m_decimalCount = std::min(m_decimalCount, amount.m_decimalCount);
    return *this;
}

/*----------------------------------------------------------------------------*/
//! Subtract an amount from the current
//! \param[in]    amount        To be subtracted   
//! \returns      *this
//! \note Subtracting an amount with fewer significant digits lowers
//!       the number of significant digits of the current amount as well.
SCXAmountOfTime& SCXAmountOfTime::operator-=(SCXAmountOfTime amount) {
    m_microseconds -= amount.m_microseconds;
    m_decimalCount = std::min(m_decimalCount, amount.m_decimalCount);
    return *this;        
}

/*----------------------------------------------------------------------------*/
//! Calculates the negation of the current amount.
//! \param[in]    amount        To be negated
//! \returns    Positive amount if current is negative, and vice versa
SCXAmountOfTime operator-(const SCXAmountOfTime &amount)  {
    return SCXAmountOfTime() - amount;
}

/*----------------------------------------------------------------------------*/
//! Retrieve the absolute value of an amount
//! \param[in]    amount    Positive, or non positive amount
//! \returns    Mathematical absolute amount
SCXAmountOfTime Abs(SCXAmountOfTime amount) {
    return SCXAmountOfTime(Abs(amount.m_microseconds), amount.m_decimalCount);
}

/*----------------------------------------------------------------------------*/
//! Check if two amounts are equal
//! \param[in]    amount1        Amount to be compared
//! \param[in]    amount2        Amount to be compared
//! \returns    true iff the amounts are equal
bool operator==(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1.m_microseconds == amount2.m_microseconds;
}

/*----------------------------------------------------------------------------*/
//! Check if two amounts are not equals
//! \param[in]    amount1        Amount to be compared
//! \param[in]    amount2        Amount to be compared
//! \returns    true iff the amounts are not equal
bool operator!=(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1.m_microseconds != amount2.m_microseconds;
}

/*----------------------------------------------------------------------------*/
//! Check if one amount is less than another
//! \param[in]    amount1        Potentially smaller amount
//! \param[in]    amount2        Potentially larger amount
//! \returns    true iff amount1 is less than amount2
//! \note false if the amounts are equal
bool operator<(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1.m_microseconds < amount2.m_microseconds;
}

/*----------------------------------------------------------------------------*/
//! Check if one amount is greater than another
//! \param[in]    amount1        Potentially larger amount
//! \param[in]    amount2        Potentially smaller amount
//! \returns    true iff amount1 is greater than amount2
//! \note false if the amounts are equal
bool operator>(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1.m_microseconds > amount2.m_microseconds;        
}

/*----------------------------------------------------------------------------*/
//! Check if one amount is less than or equal to another
//! \param[in]    amount1        Potentially smaller amount
//! \param[in]    amount2        Potentially larger amount
//! \returns    true iff amount1 is less than or equal to amount2
//! \note true if the amounts are equal
bool operator<=(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1.m_microseconds <= amount2.m_microseconds;        
}

/*----------------------------------------------------------------------------*/
//! Check if one amount is greater than or equal to another
//! \param[in]    amount1        Potentially larger amount
//! \param[in]    amount2        Potentially smaller amount
//! \returns    true iff amount1 is greater than or equal to amount2
//! \note true if the amounts are equal
bool operator>=(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1.m_microseconds >= amount2.m_microseconds;
    
}

/*----------------------------------------------------------------------------*/
//! Calculates the sum of two amounts
//! \param[in]        amount1        To be added
//! \param[in]        amount2        To be added
//! \returns         
//! (amount1 + amount2).GetSeconds() == amount1.GetSeconds() + amount2.GetSeconds()
//! \note Since seconds are floating point numbers, adding them directly would cause
//!       rounding errors. Always use this method instead
//! \note The number of significant digits of the sum cannot be greater
//!       than the lowest number of significant digits of the individual amounts
SCXAmountOfTime operator+(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1 += amount2;
}

/*----------------------------------------------------------------------------*/
//! Calculates the difference of two amounts
//! \param[in]        amount1        Amount to be subtracted from
//! \param[in]        amount2        To be subtracted
//! \returns         
//! (amount1 - amount2).GetSeconds() == amount1.GetSeconds() - amount2.GetSeconds()
//! \note Since seconds are floating point numbers, subtracting them directly would cause
//!       rounding errors. Always use this method instead
//! \note The number of significant digits of the difference cannot be greater
//!       than the lowest number of significant digits of the individual amounts
SCXAmountOfTime operator-(SCXAmountOfTime amount1, SCXAmountOfTime amount2) {
    return amount1 -= amount2;        
}

/*----------------------------------------------------------------------------*/
//! Checks if two amounts can be considered equivalent taking a certain tolerance into account
//! \param[in]    amount1        To be compared
//! \param[in]    amount2        To be compared
//! \param[in]    tolerance    Max difference allowed to consider the amounts equivalent
//! \throws        SCXInvalidArgumentException        Negative tolerance specified
bool IsEquivalent(SCXAmountOfTime amount1, SCXAmountOfTime amount2, SCXAmountOfTime tolerance) {
    if (tolerance < SCXAmountOfTime()) {
        throw SCXInvalidArgumentException(L"tolerance", L"Tolerance must not be negative", SCXSRCLOCATION);
    }
    return Abs(amount1 - amount2) <= tolerance;
}


}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
