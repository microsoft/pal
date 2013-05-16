/*--------------------------------------------------------------------------------
 Copyright (c) Microsoft Corporation.  All rights reserved.

 */
/**
 \file

 \brief       Implementation of facets needed to make STL internalization behave the
 same on scx supported platforms

 \date        08-08-05 10:18:00


 */
/*----------------------------------------------------------------------------*/

#include "scxfacets.h"
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <vector>
#include <iostream>
#include <stdlib.h>

namespace {

/*----------------------------------------------------------------------------*/
//! Encode a wide charater into a multibyte state
//! \param[in]  wc     Wide character to encode
//!\param[in,out]   state  Multi byte state to be updated
//! \returns true iff the encoding succeded
bool Encode(wchar_t wc, mbstate_t &state) {

#if defined(linux) || defined(hpux)

#if defined(linux)
    char *stateBuf = state.__value.__wchb;
#elif defined(hpux)
    char *stateBuf = static_cast<char *>(state.__parse_buf);
#endif

    bool encodedOk = false;
    memset(&state, 0, sizeof(state));
    size_t count = wcrtomb(stateBuf, wc,  &state);
    if (count != static_cast<size_t>(-1)) {
#if defined(linux)
        state.__count = static_cast<unsigned char>(count);
#elif defined(hpux)
        state.__parse_size = static_cast<unsigned char>(count);
#endif
        encodedOk = true;
    }
    return encodedOk;
#else
    // Prevent compiler warnings
    wc = wc;
    state = state;

    throw SCXCoreLib::SCXNotSupportedException(std::wstring(L"Not yet implemented"), SCXSRCLOCATION);

    // Never reached, but will pacify compiler to return a value
    return false;
#endif

}

/*----------------------------------------------------------------------------*/
//! Write a multibyte state to a buffer
//! \param[in,out]  toNext   Start of buffer
//! \param[in]  toEnd    End of buffer (exclusive)
//!\param[in,out]   state  Multi byte state to be updated
//! \returns    true iff all characters were written
//! Written characters are removed from the state.
bool WriteTo(char *&toNext, const char *toEnd, mbstate_t &state) {

#if defined(linux) || defined(hpux)

#if defined(linux)
    char *stateBuf = state.__value.__wchb;
    size_t stateCount = state.__count;
#elif defined(hpux)
    char *stateBuf = static_cast<char *>(state.__parse_buf);
    size_t stateCount = state.__parse_size;
#endif

    size_t newCount = 0;
    bool allWritten = true;
    for (size_t pos = 0; pos < stateCount; pos++) {
        if (toNext < toEnd) {
            *toNext++ = stateBuf[pos];
        } else {
            size_t newSize = stateCount - pos;
            memmove(&stateBuf[0], &stateBuf[pos], newSize);
            newCount = newSize;
            allWritten = false;
            break;
        }
    }
#if defined(linux)
    state.__count = static_cast<unsigned char>(newCount);
#elif defined(hpux)
    state.__parse_size = static_cast<unsigned char>(newCount);
#endif
    return allWritten;
#else
    // Prevent compiler warnings
    toNext = toNext;
    toEnd = toEnd;
    state = state;

    throw SCXCoreLib::SCXNotSupportedException(L"Not yet implemented", SCXSRCLOCATION);

    // Never reached, but will pacify compiler to return a value
    return false;
#endif

}

}

namespace SCXCoreLib {

/*----------------------------------------------------------------------------*/
//! Tells whether the facet might perform conversions or not
//! \returns true iff conversions are never performed
bool SCXDefaultEncodingFacet::do_always_noconv() const throw () {
    return false;
}

/*----------------------------------------------------------------------------*/
//! Retrieve information about the the encoding of the external representation
//! \returns The number of of external characters needed to represent an internal (0 means varying)
int SCXDefaultEncodingFacet::do_encoding() const throw () {
    // Number of chars that correspond to one wchar_t may vary.
    // "Unknown" environment encoding at compile time
    return 0;
}

/*----------------------------------------------------------------------------*/
//! Calculate how many external characters that must be decoded to form a specified number of internal characters
//! \param[in]  state    Current state of conversion
//! \param[in]  begin    Start of buffer
//! \param[in]  end      End of buffer (exclusive)
//! \param[in]  maxWideCharCount   Upper limit of number of internal character of interest
//! \returns    Number of available external characters that's needed to represent a number of internal characters
int SCXDefaultEncodingFacet::do_length(do_length_mbstateRef state, const char *begin, const char *end, size_t maxWideCharCount) const {
    mbstate_t stateCopy = state;
    const char *next = begin;
    size_t wideCharCount = 0;
    wchar_t wc;
    while (next < end && wideCharCount < maxWideCharCount) {
        size_t bytesRead = mbrtowc(&wc, next, end - next, &stateCopy);
        if (bytesRead == static_cast<size_t>(-1) || bytesRead == static_cast<size_t>(-2)) {
            // Found illegal or incomplete sequence, not possible to decode any more characters
            break;
        }
        next += bytesRead;
        ++wideCharCount;
    }
    return static_cast<int>(next - begin);
}

/*----------------------------------------------------------------------------*/
//! Decode external represntation of characters
//! \param[in,out]  state    Current state of conversion
//! \param[in]   fromBegin   Start of buffer to decode
//! \param[in]   fromEnd     End of buffer to decode (exclusive)
//! \param[out]  fromNext    Start of next character sequence to decode
//! \param[in]   toBegin     Start of buffer to be populated with the internal representation
//! \param[in]   toEnd       End of buffer (exclusive) to be populated with the internal representation
//! \param[out]  toNext      End of written representation (exclusive)
//! \returns     If all characters were decoded or there were some problems
std:: codecvt_base::result SCXDefaultEncodingFacet::do_in(mbstate_t &state,
           const char *fromBegin, const char *fromEnd, const char *&fromNext,
           wchar_t *toBegin, wchar_t *toEnd, wchar_t *&toNext) const {
    fromNext = fromBegin;
    toNext = toBegin;
    while (fromNext < fromEnd && toNext < toEnd) {
        size_t bytesRead = mbrtowc(toNext, fromNext, fromEnd - fromNext, &state);
        if (bytesRead == static_cast<size_t>(-1)) {
            // Unable to decode a sequence not adhering to the encoding in question
            return error;
        }
        if (bytesRead == static_cast<size_t>(-2)) {
            // All characters of an encoded sequence was not found in the buffer.
            // The already read characters are encoded in the state and the next
            // invocation of "do_in" will hopefully give the rest
            return partial;
        }
        fromNext += bytesRead;
        ++toNext;
    }
    return fromNext < fromEnd ? partial : ok;
}

/*----------------------------------------------------------------------------*/
//! Encode internal represntation of characters
//! \param[in,out]  state    Current state of conversion
//! \param[in]   fromBegin   Start of buffer to encode
//! \param[in]   fromEnd     End of buffer to encode (exclusive)
//! \param[out]  fromNext    Start of next character to encode
//! \param[in]   toBegin     Start of buffer to be populated with the external representation
//! \param[in]   toEnd       End of buffer (exclusive) to be populated with the external representation
//! \param[out]  toNext      End of written representation (exclusive)
//! \returns     If all characters were encoded or there were some problems
std::codecvt_base::result SCXDefaultEncodingFacet::do_out(
        mbstate_t &state, const wchar_t *fromBegin,
        const wchar_t *fromEnd, const wchar_t *&fromNext, char *toBegin,
        char *toEnd, char *&toNext) const {
    fromNext = fromBegin;
    toNext = toBegin;
    // The state may contain prior characters encoded but not written, must always write them first
    if (!WriteTo(toNext, toEnd, state)) {
        return partial;
    }
    if (fromNext >= fromEnd) {
        return ok;
    }
    if (!Encode(*fromNext++, state)) {
        return error;
    }
    while (WriteTo(toNext, toEnd, state) && fromNext < fromEnd) {
        if (!Encode(*fromNext++, state)) {
            return error;
        }
    }
    return fromNext < fromEnd ? partial : ok;
}

/*----------------------------------------------------------------------------*/
//! Write remaining characters of a multibyte state
//! \param[in,out]  state    Current state of conversion
//! \param[in]   toBegin     Start of buffer to be populated with the external representation
//! \param[in]   toEnd       End of buffer (exclusive) to be populated with the external representation
//! \param[out]  toNext      End of written representation (exclusive)
//! \returns     If all characters were written or not
std::codecvt_base::result SCXDefaultEncodingFacet::do_unshift(mbstate_t &state, char *toBegin, char *toEnd, char *&toNext) const {
    toNext = toBegin;
    if (!WriteTo(toNext, toEnd, state)) {
        // State still contain characters not written
        return partial;
    }
    return ok;
}



}

/*----------------------------------------------------------------------------*/

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
