/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file
 
    \brief     Math helper functions
 
    \date      07-05-28 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlimit.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxexception.h>

#include <math.h>
#include <limits>

using namespace std;

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Calculate percentage
        
        \param[in]  old_tic - Old value of counter
        \param[in]  new_tic - New value of counter
        \param[in]  old_tot - Old value of total counter
        \param[in]  new_tot - New value of total counter
        \param[in]  inverse - Inverse the calculation
        \returns              Number of percent tic has taken of tot

        \throws               SCXInvalidArgumentException if new values are less than old values.
    */
    scxulong GetPercentage(scxulong old_tic, scxulong new_tic, scxulong old_tot, scxulong new_tot, bool inverse)
    {
        if (new_tot < old_tot)
        {
            throw SCXInvalidArgumentException(L"new_tot",L"smaller than old_tot", SCXSRCLOCATION);
        }
        if (new_tic < old_tic)
        {
            throw SCXInvalidArgumentException(L"new_tic",L"smaller than old_tic", SCXSRCLOCATION);
        }

        scxulong ans = 0;

        scxulong tic_diff = new_tic - old_tic;
        scxulong tot_diff = new_tot - old_tot;

        if (inverse)
        {
            tic_diff = tot_diff - tic_diff;
            ans = 100;
        }

        // when there is no data collected, set the return as 0
        if (inverse && old_tic == 0 && new_tic == 0 && old_tot == 0 && new_tot == 0)
        {
                ans = 0;
        }

        if (tot_diff > 0)
        {
            ans = MIN(100, static_cast<unsigned int>( (static_cast<double>(tic_diff) / static_cast<double>(tot_diff)) * 100  + 0.5));
        }

        return ans;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Convert bytes to megabytes.
        
        \param[in]  bytes - Value in bytes
        \returns            Value converted to megabytes.

    */
    scxulong BytesToMegaBytes(scxulong bytes)
    {
        return bytes / 1024 / 1024;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Convert bytes to megabytes.
        
        \param[in]  bytes - Value in bytes
        \returns            Value converted to megabytes.

    */
    double BytesToMegaBytes(double bytes)
    {
        return bytes / 1024.0 / 1024.0;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Convert kilobytes to megabytes.
        
        \param[in]  kiloBytes - Value in kilobytes
        \returns                Value converted to megabytes.

    */
    scxulong KiloBytesToMegaBytes(scxulong kiloBytes)
    {
        return kiloBytes / 1024;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Convert kilobytes to megabytes.
        
        \param[in]  kiloBytes - Value in kilobytes
        \returns                Value converted to megabytes.

    */
    double KiloBytesToMegaBytes(double kiloBytes)
    {
        return kiloBytes / 1024.0;
    }
    
    /*----------------------------------------------------------------------------*/
    /**
         Rounds floating point to nearest double value with no fraction
         \returns Nearest double value with no fraction
     */
    double Round(double value) 
    {
        return floor(value + 0.5); 
    }
    
    /*----------------------------------------------------------------------------*/
    /**
         Rounds floating point to nearest scxlong value
         \returns     Nearest scxlong value
         \throws     SCXInvalidArgumentException        Value cannot be represented by scxlong     
     */
    scxlong RoundToScxLong(double value) 
    {
        double roundedValue = Round(value);
        if (cMinScxLong <= roundedValue && roundedValue <= cMaxScxLong) {
            return static_cast<scxlong> (roundedValue);
        } else {
            throw SCXInvalidArgumentException(L"value", L"Value of double outside the range of long", 
                    SCXSRCLOCATION);
            
        }
    }
    
    /*----------------------------------------------------------------------------*/
    /**
         Rounds floating point to nearest signed int value
         \returns     Nearest signed int value
         \throws     SCXInvalidArgumentException        Value cannot be represented by signed int     
     */
    int RoundToInt(double value) 
    {
        double roundedValue = Round(value);
        if (numeric_limits<int>::min() <= roundedValue && roundedValue <= numeric_limits<int>::max()) {
            return static_cast<int> (roundedValue);
        } else {
            throw SCXInvalidArgumentException(L"value", L"Value of double outside the range of int", 
                    SCXSRCLOCATION);
            
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
         Rounds floating point to nearest unsigned int value
         \returns     Nearest unsigned int value
         \throws     SCXInvalidArgumentException        Value cannot be represented by unsigned int     
     */
    unsigned int RoundToUnsignedInt(double value) 
    {
        double roundedValue = Round(value);
        if (numeric_limits<unsigned int>::min() <= roundedValue && roundedValue <= numeric_limits<unsigned int>::max()) {
            return static_cast<unsigned int> (roundedValue);
        } else {
            throw SCXInvalidArgumentException(L"value", L"Value of double outside the range of unsigned int", 
                    SCXSRCLOCATION);
            
        }
    }
    
    /*----------------------------------------------------------------------------*/
    /**
         Calculates the absolute value
         \param[in]    value
         \returns    The absolute value
         \note Using STL abs on scxlong may cause function signature ambiguity problems
    */
    scxulong Abs(scxlong value) 
    {
        return value >= 0? value : -value;
    }
    
    /*----------------------------------------------------------------------------*/
    /**
       Check if values are in ascending order
       \param[in]     value1         Potentially smallest value
       \param[in]    value2        Potentially middle value
       \param[in]    value3        Potentially largest value
       \returns     true iff value1 <= value2 <= value3    
       Useful when checking if a value is within bounds.
       Write IsAscending(1, x + y, 10) instead of (1 <= x + y) && (x + y <= 10)
       \note   The sequence (1, 1, 1) is ascending, but not _strictly_ ascending
     */
    bool IsAscending(int value1, int value2, int value3) 
    {
        return value1 <= value2 && value2 <= value3;
    }
 
    /*----------------------------------------------------------------------------*/
    /**
       Check if values are in strictly ascending order
       \param[in]     value1         Potentially smallest value
       \param[in]    value2        Potentially middle value
       \param[in]    value3        Potentially largest value
       \returns     true iff value1 < value2 < value3    
       Useful when checking if a value is within bounds
       Write IsStrictlyAscending(1, x + y, 10) instead of (1 < x + y) && (x + y < 10)
       \note   The sequence (1, 1, 1) is ascending, but not _strictly_ ascending
    */
    bool IsStrictlyAscending(int value1, int value2, int value3)
    {
        return value1 < value2 && value2 < value3;        
    }
    
        /*----------------------------------------------------------------------------*/
    /**
       Takes the "exponent" power of "base"
       \param[in]       base            Base
       \param[in]       exponent        Exponent
       \returns         base * base * ... * base
       Uses integer arithmetic to prevent loss of precision.
    */ 
        scxlong Pow(scxlong base, scxulong exponent) {
            // Relies on exponent >= 0 since it is unsigned
                if (exponent == 0) {
                        return 1;
                } else {
                        scxlong result = 1;
                        while (exponent > 0) {
                                if (exponent % 2 == 0) {
                                        // pow(base, 2*exp) == pow(base*base, exp)
                                        base *= base;        
                                        exponent /= 2;
                                } else {
                                        result *= base;
                                        --exponent;
                                }
                        }
                        return result;
                }
        }

    /*----------------------------------------------------------------------------*/
        /**
           Changes the type of a value to int
           \param[in]  value   Value in int value domain
           \returns    Same value, though type int
           \throws     SCXInvalidArgumentException     Value outside int value domain  
         */
        signed int ToInt(scxlong value) 
        {
            if (value < numeric_limits<signed int>::min() ||
                    value > numeric_limits<signed int>::max()) 
            {
                throw SCXInvalidArgumentException(L"value", L"Outside int value domain", SCXSRCLOCATION);
            }
            return static_cast<signed int>(value);
        }

        
    /*----------------------------------------------------------------------------*/
        /**
         * Compare if two values can be considered equal, according to a certain precision
         * \param[in] value1     Value to compare
         * \param[in] value2     Value to ompare
         * \param[in] precision  Max allowed difference to consider the values equal
         * \\returns  true iff the values can be considered equal
         * 
         */
        bool Equal(double value1, double value2, double precision)
        {
           return fabs(value1 - value2) <= precision; 
        }
}
 
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
