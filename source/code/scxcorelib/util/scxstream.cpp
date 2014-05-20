/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Implementation of a collection of functions for working with streams

    \date        07-09-11 17:08:00


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxstream.h>
#include "scxfacets.h"
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxassert.h>
#include <scxcorelib/scxlocale.h>
#include <vector>
#include <algorithm>
#include <locale.h>


#if defined(sun)

#include <iconv.h>
#include <stdlib.h>
#include <errno.h>

#endif

namespace {
    const unsigned char cCR = 0xD;              //!< Carriage Return
    const unsigned char cLF = 0xA;              //!< Line Feed
    const unsigned char cVT = 0x0B;             //!< Vertical Tab
    const unsigned char cFF = 0x0C;             //!< Form Feed
    const unsigned char cNEL = 0x85;            //!< NExt Line (EBCDIC)

#if !defined(sun)

    const wchar_t cLS = 0x2028;                 //!< Line Separator (Unicode)
    const wchar_t cPS = 0x2029;                 //!< Paragraph Separator (Unicode)

#endif

    const unsigned char c10 = 0x2;              //!< Bit sequence 10
    const unsigned char c1111 = 0xF;            //!< Bit sequence 1111
    const unsigned char c11111 = 0x1F;          //!< Bit sequence 11111
    const unsigned char c111111 = 0x3F;         //!< Bit sequence 111111
    const unsigned char c10000000 = 0x80;       //!< Bit sequence 10000000
    const unsigned char c11111111 = 0xFF;       //!< Bit sequence 11111111

    //! Number of unicode bits that a number of extra bytes may represent
    //! \param[in]      extraByteCount  Number of UTF8 extra bytes
    //! 8 - (1 + 1 + extraByteCount) + 6 * extraBytesAllowed
    //! Only valid when extraByteCount > 0
    inline int unicodeBitCount(const int extraByteCount) {
        SCXASSERT(extraByteCount > 0);
        return 5 * extraByteCount + 6;
    }

    //! Number of extra bytes UTF8 encoding may use, limited by the fact that
    //! wchar_t must be able to accomodate the decoded character.
    //! Calculated by solving the following equation:
    //! 8 - (1 + 1 + extraBytesAllowed) + 6 * extraBytesAllowed == sizeof(wchar_t) * 8
    const int cExtraBytesAllowed = static_cast<int> ((sizeof(wchar_t) * 8 - 6) / 5);


    /*----------------------------------------------------------------------------*/
    //! Create an UTF8 byte sequence representing a partially constructed unicode character.
    //! \param[in]  firstByte           First byte of the sequence
    //! \param[in]  partialCodepoint    Partially constructed unicode character
    //! \param[in]  extraBytes          Number of extra bytes represented in partialCodepoint
    std::vector<unsigned char> CreateByteSequence(unsigned char firstByte, const wchar_t partialCodepoint, const int extraBytes) {
        std::vector<unsigned char> byteSequence;
        wchar_t shiftedCodepoint = partialCodepoint;
        for (int i = 0; i < extraBytes; i++) {
            byteSequence.push_back(static_cast<unsigned char>((c10 << 6) | (shiftedCodepoint & c111111)));
            shiftedCodepoint >>= 6;
        }
        byteSequence.push_back(firstByte);
        std::reverse(byteSequence.begin(), byteSequence.end());
        return byteSequence;
    }

#if defined (sun)

    /*----------------------------------------------------------------------------*/
    //! Check is a given byte seuqence represents a valid UTF-8 character
    //! \param[in]  bytes      Bytye sequence conatining UTF-8 character
    //! \param[in]  byteCount  Number of bytes in byte sequence
    //! \returns     true if bytes contains a vaild UTF-8 character
    bool IsValidUTF8(unsigned char* bytes, int byteCount)
    {
        int nrOfLeadingDigitOne = 0;
        unsigned char mask = c11111111;
        for (unsigned char bits = bytes[0]; (bits & c10000000) != 0; bits <<= 1) {
            mask >>= 1;
            ++nrOfLeadingDigitOne;
        }

        if (nrOfLeadingDigitOne == 1) {
            return false;
        }

        if (nrOfLeadingDigitOne > 1) {
            if (nrOfLeadingDigitOne != byteCount) {
                return false;
            }
            for (int i = 0; i < nrOfLeadingDigitOne - 1; i++)  {
                if ((bytes[i+1] >> 6) != c10) {
                    return false;
                } 
            }
        }
        
        return true;
    }

#endif

    /*----------------------------------------------------------------------------*/
    //! Read a character of an UTF8 encoded stream assuming wchar_t is in UTF-32 encoding.
    //! \param[in]  source                          UTF8 encoded stream
    //! \throws     SCXLineStreamContentException   Invalid byte sequence according to UTF8
    //! The stream must contain at least one byte to be read.
    wchar_t ReadCharUTF8Basic(std::istream& source) {
        SCXASSERT(SCXCoreLib::SCXStream::IsGood(source));
        wchar_t unicodeChar = WEOF;
        unsigned char firstByte = static_cast<unsigned char>(source.get());
        int nrOfLeadingDigitOne = 0;
        unsigned char mask = c11111111;
        for (unsigned char bits = firstByte; (bits & c10000000) != 0; bits = static_cast<unsigned char>(bits << 1)) 
        {
            mask = static_cast<unsigned char>(mask >> 1);
            ++nrOfLeadingDigitOne;
        }
        if (nrOfLeadingDigitOne == 0) {
            unicodeChar = firstByte;
        } else if (nrOfLeadingDigitOne == 1) {
            // The bit sequence 10 is reserved as prefix for any extra bytes
            std::vector<unsigned char> byteSequence;
            byteSequence.push_back(firstByte);
            throw SCXCoreLib::SCXLineStreamContentException(CreateByteSequence(firstByte, 0, 0),
                    SCXSRCLOCATION);
        } else {
            // Read as many UTF8 extra bytes as first byte states and the platform accomodates
            unicodeChar = firstByte & mask;
            const int extraBytesSpecified = nrOfLeadingDigitOne - 1;
            const int legalExtraBytes = std::min(extraBytesSpecified, cExtraBytesAllowed);
            for (int i = 0; i < legalExtraBytes; i++)  {
                if ((source.peek() >> 6) == c10) {
                    unsigned char extraByte = static_cast<unsigned char>(source.get());
                    unicodeChar <<= 6;
                    unicodeChar |= (extraByte & c111111);
                } else {
                    throw SCXCoreLib::SCXLineStreamContentException(CreateByteSequence(firstByte, unicodeChar, i),
                            SCXSRCLOCATION);
                }
            }
            // Read any extra bytes not legal on the platform due to limited wchar_t
            if (legalExtraBytes < extraBytesSpecified) {
                std::vector<unsigned char> byteSequence(CreateByteSequence(firstByte, unicodeChar, legalExtraBytes));
                for (int i = legalExtraBytes; i < extraBytesSpecified; i++)  {
                    if ((source.peek() >> 6) == c10) {
                        byteSequence.push_back(static_cast<unsigned char>(source.get()));
                    } else {
                        throw SCXCoreLib::SCXLineStreamContentException(byteSequence, SCXSRCLOCATION);
                    }
                }
                throw SCXCoreLib::SCXLineStreamContentException(byteSequence, SCXSRCLOCATION);
            }
        }
        return unicodeChar;
    }

#if defined (sun)

    /*----------------------------------------------------------------------------*/
    //! Read a character of an UTF8 encoded stream.
    //! Solaris version that try to use iconv if available
    //! \param[in]  source                          UTF8 encoded stream
    //! \throws     SCXLineStreamContentException   Invalid byte sequence according to UTF8
    //! The stream must contain at least one byte to be read.
    wchar_t ReadCharUTF8(std::istream& source) {
        if (!SCXCoreLib::SCXLocaleContext::UseIconv())
        {
            return ReadCharUTF8Basic(source);
        }
        SCXASSERT(SCXCoreLib::SCXStream::IsGood(source));
        wchar_t unicodeChar = WEOF;
        unsigned char bytes[10];
        bytes[0] = static_cast<unsigned char>(source.get());
        int nrOfLeadingDigitOne = 0;
        unsigned char mask = c11111111;
        for (unsigned char bits = bytes[0]; (bits & c10000000) != 0; bits <<= 1) {
            mask >>= 1;
            ++nrOfLeadingDigitOne;
        }
        int byteCount = 1;
 
        if (nrOfLeadingDigitOne == 1) {
            // The bit sequence 10 is reserved as prefix for any extra bytes
            throw SCXCoreLib::SCXLineStreamContentException(CreateByteSequence(bytes[0], 0, 0),
                    SCXSRCLOCATION);
        }

        if (nrOfLeadingDigitOne > 1) {
            // Read as many UTF8 extra bytes as first byte states
            const int extraBytesSpecified = nrOfLeadingDigitOne - 1;
            for (int i = 0; i < extraBytesSpecified; i++)  {
                if ((source.peek() >> 6) == c10) {
                    bytes[1+i] = static_cast<unsigned char>(source.get());
                    byteCount++;
                } else {
                    throw SCXCoreLib::SCXLineStreamContentException(CreateByteSequence(bytes[0], 0, 0),
                            SCXSRCLOCATION);
                }
            }
        }

        iconv_t ic = SCXCoreLib::SCXLocaleContext::GetFromUTF8iconv();

        if (ic == NULL)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Failed to get iconv for from UTF-8 conversion", SCXSRCLOCATION);
        }

        static const unsigned int BUFSIZE = 10;

#if CPP_ICONV_CONST_CHARPTR
        const char* inp = (char*)&bytes[0];
#else
        char* inp = (char*)&bytes[0];
#endif

        size_t inl = (size_t)byteCount;
        char buf[BUFSIZE];
        char *outp = &buf[0];
        size_t outl = BUFSIZE;
    
        size_t res = iconv(ic, &inp, &inl, &outp, &outl); 

        if (res == (size_t)-1)
        {
            throw SCXCoreLib::SCXErrnoException(L"iconv call to convert from UTF-8 failed", errno, SCXSRCLOCATION);
        }

        mbstate_t ps = {0};

        res = mbrtowc(&unicodeChar, &buf[0], BUFSIZE-outl, &ps);

        if (res == (size_t)-1)
        {
            throw SCXCoreLib::SCXErrnoException(L"mbrtowc call to convert to wchar_t failed", errno, SCXSRCLOCATION);
        }

        return unicodeChar;
    }

#else

    /*----------------------------------------------------------------------------*/
    //! Read a character of an UTF8 encoded stream.
    //! Non-Solaris version, that assumes wchar_t is encoded in UTF-32
    //! \param[in]  source                          UTF8 encoded stream
    //! \throws     SCXLineStreamContentException   Invalid byte sequence according to UTF8
    //! The stream must contain at least one byte to be read.
    wchar_t ReadCharUTF8(std::istream& source) {
        return ReadCharUTF8Basic(source);
    }

#endif

    /*----------------------------------------------------------------------------*/
    //! Reads as much of a line of an UTF8 encoded stream as possible.
    //! \param[in]  source                                  UTF8 encoded stream
    //! \param[in]  maxLineLength                           Stop reading when line is this long, must be > 0
    //! \param[out] line                                    Line read, part or whole.
    //! \param[out] nlf                                     Actual newline symbol following the line
    //! \throws     SCXLineStreamContentException           Invalid byte sequence according to UTF8
    //! The stream must contain at least one byte to be read. If the line is to be written back to
    //! the originating system the same nlf should in general be used, if we do not have other information.
    //! Stops reading when either end of line is encountered or the line is maxLineLength.
    //! \note If nlf is eUnknown and the source still is good, the line is only partially read
    void ReadLineAsUTF8Partially(std::istream& source, const std::wstring::size_type maxLineLength, std::wstring& line,
            SCXCoreLib::SCXStream::NLF& nlf) {
        SCXASSERT(SCXCoreLib::SCXStream::IsGood(source) && maxLineLength > 0);
        line.clear();
        nlf = SCXCoreLib::SCXStream::eUnknown;
        do {
            const wchar_t charRead = ReadCharUTF8(source);
            if (charRead == cCR) {
                nlf = SCXCoreLib::SCXStream::eCR;
                if (source.peek() == cLF) {
                    nlf = SCXCoreLib::SCXStream::eCRLF;
                    source.get();
                }
                break;
            } else if (charRead == cLF) {
                nlf = SCXCoreLib::SCXStream::eLF;
                break;

#if !defined(sun)

            } else if (charRead == cLS) {
                nlf = SCXCoreLib::SCXStream::eLS;
                break;
            } else if (charRead == cPS) {
                nlf = SCXCoreLib::SCXStream::ePS;
                break;

#endif

            } else if (charRead == cNEL) {
                nlf = SCXCoreLib::SCXStream::eNEL;
                break;
            } else if (charRead == cVT) {
                nlf = SCXCoreLib::SCXStream::eVT;
                break;
            } else if (charRead == cFF) {
                nlf = SCXCoreLib::SCXStream::eFF;
                break;
            } else  {
                SCXASSERT(line.length() < maxLineLength);
                line.push_back(charRead);
            }
        } while (source.peek() != EOF && source.good() && line.length() < maxLineLength);
    }

}

namespace SCXCoreLib {
/*----------------------------------------------------------------------------*/
    std::wstring SCXLineStreamContentException::What() const {
        std::wostringstream msg;
        msg << L"Byte sequence ";
        msg << static_cast<int> (m_byteSequence.at(0));
        for (size_t i = 1; i < m_byteSequence.size(); i++) {
            msg << ", " << static_cast<int> (m_byteSequence.at(i));
        }
        msg << L" not part of UTF-8";
        return msg.str();
    }

    /*----------------------------------------------------------------------------*/
    //! Read as much of a line of a stream as possible.
    //! \param[in]  source                                  Stream to read
    //! \param[in]  maxLineLength                           Stop reading when line is this long
    //! \param[out] line                                    Line that was read, part or whole.
    //! \param[out] nlf                                     Actual newline symbol following the line
    //! \throws     SCXInvalidArgumentException             No line available
    //! If the line is to be written back to the originating system the same nlf should in general
    //! be used, if we do not have other information.
    //! Stops reading when either end of line is encountered or the line is maxLineLength.
    //! \note If nlf is eUnknown and the source still is good, the line is only partially read
    //! \note To check that at least one char is available in the stream, you may call
    //! stream.peek() followed by a test of stream.good()
    void SCXStream::ReadPartialLine(std::wistream& source, const std::wstring::size_type maxLineLength,
            std::wstring& line, NLF& nlf) {
        if (source.peek() == WEOF || !source.good()) {
            throw SCXInvalidArgumentException(L"source", L"source stream is in a bad state",
                                              SCXSRCLOCATION);
        }
        line.clear();
        nlf = eUnknown;
        std::wistream::char_type nextChar;
        while (source.get(nextChar)) {
            if (nextChar == cCR) {
                nlf = eCR;
                if (source.peek() == cLF) {
                    source.get();
                    nlf = eCRLF;
                }
                break;
            } else if (nextChar == cLF) {
                nlf = eLF;
                break;

#if !defined(sun)

            } else if (nextChar == cLS) {
                nlf = eLS;
                break;
            } else if (nextChar == cPS) {
                nlf = ePS;
                break;

#endif

            } else if (nextChar == cNEL) {
                nlf = eNEL;
                break;
            } else if (nextChar == cVT) {
                nlf = eVT;
                break;
            } else if (nextChar == cFF) {
                nlf = eFF;
                break;
            } else if (line.length() < maxLineLength) {
                line.push_back(nextChar);
            } else {
                source.unget(); // Put back nextChar into source stream
                break;
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Read a line of a stream.
    //! \param[in]      source      Stream to be read
    //! \param[out]     line        Line that was read
    //! \param[out]     nlf         Newline symbol following the line
    //! \throws         SCXInvalidArgumentException             No line available
    //! \throws         SCXLineStreamPartialReadException       Line to long to be stored in a wstring
    //! At least one line must be available. Handles newline symbols in a platform independent way,
    //! that is, may be used to read a stream originating on one platform on another platform.
    //! If the line is to be written back to the originating system
    //! the same nlf should in general be used, if we do not have other information.
    //! \note To check that at least one line is available in the stream, you may call
    //! stream.peek() followed by a test of stream.good()
    void SCXStream::ReadLine(std::wistream& source, std::wstring& line, NLF& nlf) {
        ReadPartialLine(source, line.max_size(), line, nlf);
        if (nlf == eUnknown && SCXStream::IsGood(source)) {
            throw SCXLineStreamPartialReadException(SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Read as many lines of a stream as possible.
    //! \param[in]      source      Stream to be read
    //! \param[out]     lines       Line that was read
    //! \param[out]     nlfs        Newline symbols used in the stream
    //! \throws     SCXLineStreamPartialReadException       Line to long to be stored in a wstring
    //! Handles newline symbols in a platform independent way, that is, may
    //! be used to read a stream originating on one platform on another platform.
    //! If different kinds of newline symbols is found in the stream, or the stream is empty,
    //! nlf will be set to eUnknown. If the lines is to be written back to the originating system
    //! the same nlf should in general be used, if we do not have other information.
    void SCXStream::ReadAllLines(std::wistream& source, std::vector<std::wstring>& lines, NLFs& nlfs) {
        source.peek();
        nlfs.clear();
        lines.clear();
        std::wstring line;
        NLF nlf;
        while (SCXStream::IsGood(source)) {
            ReadPartialLine(source, line.max_size(), line, nlf);
            if (nlf != eUnknown) {
                nlfs.insert(nlf);
                lines.push_back(line);
            } else if (!SCXStream::IsGood(source)) {
                lines.push_back(line);
            } else {
                throw SCXCoreLib::SCXLineStreamPartialReadException(SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Read a character of an UTF8 encoded stream.
    //! \param[in]  source                          UTF8 encoded stream
    //! \throws     SCXLineStreamContentException   Invalid byte sequence according to UTF8
    //! \throws     SCXInvalidArgumentException     No character available
    //! \note To check that at least one character is available in the stream, you may call
    //! stream.peek() followed by a test of stream.good()
    wchar_t SCXStream::ReadCharAsUTF8(std::istream& source) {
        if (source.peek() != EOF && source.good()) {
            return ReadCharUTF8(source);
        } else {
            throw SCXInvalidArgumentException(L"source", L"source stream is in a bad state",
                                              SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Read as much of a line of a UTF8 encoded stream as possible.
    //! \param[in]  source                                  Stream to read
    //! \param[in]  maxLineLength                           Stop reading when line is this long
    //! \param[out] line                                    Line that was read, part or whole.
    //! \param[out] nlf                                     Actual newline symbol following the line
    //! \throws     SCXInvalidArgumentException             No line available
    //! \throws     SCXLineStreamContentException           Invalid byte sequence according to UTF8
    //! If the line is to be written back to the originating system the same nlf should in general
    //! be used, if we do not have other information.
    //! Stops reading when either end of line is encountered or the line is maxLineLength.
    //! \note If nlf is eUnknown and the source still is good, the line is only partially read
    //! \note To check that at least one line is available in the stream, you may call
    //! stream.peek() followed by a test of stream.good()
    void SCXStream::ReadPartialLineAsUTF8(std::istream& source, const std::wstring::size_type maxLineLength,
            std::wstring& line, NLF& nlf) {
        if (maxLineLength > 0) {
            if (source.peek() != EOF && source.good()) {
                ReadLineAsUTF8Partially(source, maxLineLength, line, nlf);
            } else {
                throw SCXInvalidArgumentException(L"source",
                                                  L"source stream is in a bad state",
                                                  SCXSRCLOCATION);
            }
        } else {
            throw SCXInvalidArgumentException(L"maxLineLength", L"maxLineLength must be > 0",
                                              SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Read a line of an UTF8 encoded stream.
    //! \param[in]  source                                  UTF8 encoded stream
    //! \param[out] line                                    Line that was read
    //! \param[out] nlf                                     Actual newline symbol following the line
    //! \throws     SCXInvalidArgumentException             No line available
    //! \throws     SCXLineStreamContentException           Invalid byte sequence according to UTF8
    //! \throws     SCXLineStreamPartialReadException       Line to long to be stored in a wstring
    //! Handles newline symbols in a platform independent way, that is, may
    //! be used to read a stream originating on one platform on another platform.
    //! If the line is to be written back to the originating system
    //! the same nlf should in general be used, if we do not have other information.
    //! \note To check that at least one line is available in the stream, you may call
    //! stream.peek() followed by a test of stream.good()
    void SCXStream::ReadLineAsUTF8(std::istream& source, std::wstring& line, NLF& nlf) {
        if (source.peek() != EOF && source.good()) {
            ReadLineAsUTF8Partially(source, line.max_size(), line, nlf);
            if (nlf == eUnknown && SCXStream::IsGood(source)) {
                throw SCXCoreLib::SCXLineStreamPartialReadException(SCXSRCLOCATION);
            }
        } else {
            throw SCXInvalidArgumentException(L"source", L"source stream is in a bad state",
                                              SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Reads as many lines as possible of an UTF8 encoded stream.
    //! \param[in]  source                                  UTF8 encoded stream
    //! \param[out] lines                                   Lines that was read
    //! \param[out] nlfs                                    Actual newline symbols used in the stream
    //! \throws     SCXLineStreamContentException           Invalid byte sequence according to UTF8
    //! \throws     SCXLineStreamPartialReadException       Line to long to be stored in a wstring
    //! Handles newline symbols in a platform independent way, that is, may
    //! be used to read a stream originating on one platform on another platform.
    //! If the lines is to be written back to the originating system
    //! the same nlf should in general be used, if we do not have other information.
    void SCXStream::ReadAllLinesAsUTF8(std::istream& source, std::vector<std::wstring>& lines, NLFs& nlfs) {
        lines.clear();
        nlfs.clear();
        std::wstring line;
        source.peek();
        NLF nlf = eUnknown;
        while (SCXStream::IsGood(source)) {
            ReadLineAsUTF8Partially(source, line.max_size(), line, nlf);
            if (nlf != eUnknown) {
                nlfs.insert(nlf);
                lines.push_back(line);
            } else if (!SCXStream::IsGood(source)) {
                lines.push_back(line);
            } else {
                throw SCXCoreLib::SCXLineStreamPartialReadException(SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Writes content to a stream
    //! \param[in]  target      Stream to be written to
    //! \param[in]  content     Content to be written
    //! \throws     SCXLineStreamContentWriteException      Content was not written completely
    void SCXStream::Write(std::wostream& target, const wchar_t content) {
        target.put(content);
        if (!target.good()) {
            throw SCXLineStreamContentWriteException(SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Writes content to a stream
    //! \param[in]  target      Stream to be written to
    //! \param[in]  content     Data to be written
    //! \throws     SCXLineStreamContentWriteException      Content was not written completely
    void SCXStream::Write(std::wostream& target, const std::wstring& content) {
        std::wstring::size_type contentLength = content.length();
        for (std::wstring::size_type i = 0; i < contentLength; i++) {
            target.put(content.at(i));
            if (!target.good()) {
                throw SCXLineStreamContentWriteException(SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Writes new line to a stream
    //! \param[in]  target      Stream to be written to
    //! \param[in]  nlf         Kind of new line to write
    //! \throws     SCXLineStreamNewLineWriteException      New line was not written ok
    //! \throws     SCXInvalidArgumentException             nlf == eUnknown
    void SCXStream::WriteNewLine(std::wostream& target, const NLF nlf) {
        switch (nlf) {
        case eUnknown:
            throw SCXInvalidArgumentException(L"nlf", L"eUnknown", SCXSRCLOCATION);
        case eCR:
            target.put(cCR);
            break;
        case eLF:
            target.put(cLF);
            break;
        case eCRLF:
            target.put(cCR);
            target.put(cLF);
            break;
        case eVT:
            target.put(cVT);
            break;
        case eFF:
            target.put(cFF);
            break;
        case eNEL:
            target.put(cNEL);
            break;

#if !defined(sun)

        case eLS:
            target.put(cLS);
            break;
        case ePS:
            target.put(cPS);
            break;

#endif

        default:
            throw SCXInternalErrorException(L"All NLFs not handled", SCXSRCLOCATION);
        }
        if (!target.good()) {
            throw SCXLineStreamNewLineWriteException(SCXSRCLOCATION);
        }
    }

#if defined(sun)

    /*----------------------------------------------------------------------------*/
    //! Writes content to a UTF8 encoded stream
    //! Solaris version that use iconv if available
    //! \param[in]  target      Stream to be written to
    //! \param[in]  content     Content to be written
    //! \throws     SCXLineStreamContentWriteException      Content was not written completely
    void SCXStream::WriteAsUTF8(std::ostream& target, const wchar_t content) {

        if (!SCXCoreLib::SCXLocaleContext::UseIconv())
        {
            WriteAsUTF8Basic(target, content);
            return;
        }

        static const unsigned int BUFSIZE = 10;

        iconv_t ic = SCXCoreLib::SCXLocaleContext::GetToUTF8iconv();

        if (ic == NULL)
        {
            throw SCXInternalErrorException(L"Failed to get iconv for to UTF-8 conversion", SCXSRCLOCATION);
        }

        char buf2[BUFSIZE];

        mbstate_t ps = {0};

        size_t res = wcrtomb(buf2, content, &ps);

        if (res == (size_t)-1)
        {
            throw SCXErrnoException(L"wcrtomb call to convert from wchar_t failed", errno, SCXSRCLOCATION);
        }

#if CPP_ICONV_CONST_CHARPTR
        const char* inp = &buf2[0];
#else
        char* inp = &buf2[0];
#endif

        size_t inl = res;
        char buf[BUFSIZE];
        char *outp = &buf[0];
        size_t outl = BUFSIZE;
    
        res = iconv(ic, &inp, &inl, &outp, &outl); 

        if (res == (size_t)-1)
        {
            throw SCXErrnoException(L"iconv call to convert to UTF-8 failed", errno, SCXSRCLOCATION);
        }

        for (size_t i=0; i<BUFSIZE-outl; i++)
        {
            target.put(buf[i]);
            if (!target.good()) {
                throw SCXLineStreamContentWriteException(SCXSRCLOCATION);
            }
        }
    }

#else

    /*----------------------------------------------------------------------------*/
    //! Writes content to a UTF8 encoded stream
    //! Non-Solaris version that assumes that wchar_t is encoed as UTF-32
    //! \param[in]  target      Stream to be written to
    //! \param[in]  content     Content to be written
    //! \throws     SCXLineStreamContentWriteException      Content was not written completely
    void SCXStream::WriteAsUTF8(std::ostream& target, const wchar_t content) {
        WriteAsUTF8Basic(target, content);
    }

#endif


    /*----------------------------------------------------------------------------*/
    //! Writes content to a UTF8 encoded stream assuming wchar_t is encoded as UTF-32
    //! \param[in]  target      Stream to be written to
    //! \param[in]  content     Content to be written
    //! \throws     SCXLineStreamContentWriteException      Content was not written completely
    void SCXStream::WriteAsUTF8Basic(std::ostream& target, const wchar_t content) {
        SCXASSERT(static_cast<long>(content) >= 0); // Cast needed to solve bug 1376
        unsigned char bytes[10];
        int byteCount = 0;
        if ((content >> 7) == 0) {
            bytes[byteCount++] = static_cast<unsigned char>(content & c11111111);
        } else {
            int extraByteCount = 1;
            while ((content >> unicodeBitCount(extraByteCount)) != 0) {
                ++extraByteCount;
            }
            SCXASSERT(extraByteCount <= 7);
            const int nrOfExtraBitsInContent = extraByteCount * 6;
            bytes[byteCount++] = static_cast<unsigned char>(c11111111 << (7 - extraByteCount) | (content >> nrOfExtraBitsInContent));
            for (int extraBitNr = nrOfExtraBitsInContent - 6; extraBitNr >= 0; extraBitNr -= 6) {
                bytes[byteCount++] = static_cast<unsigned char>(((content >> extraBitNr) & c111111) | (c10 << 6));
            }
        }

#if defined(sun)

        if (SCXCoreLib::SCXLocaleContext::WantToUseIconv() && !IsValidUTF8(bytes, byteCount))
        {
            bytes[0] = '?';
            byteCount = 1;
        }

#endif

        for (int i=0; i<byteCount; i++) {
            target.put(bytes[i]);
            if (!target.good()) {
                throw SCXLineStreamContentWriteException(SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Writes content to a UTF8 encoded stream
    //! \param[in]  target      Stream to be written to
    //! \param[in]  content     Content to be written
    //! \throws     SCXLineStreamContentWriteException      Content was not written completely
    void SCXStream::WriteAsUTF8(std::ostream& target, const std::wstring& content) {
        std::wstring::size_type contentLength = content.length();
        for (std::wstring::size_type charNr = 0; charNr < contentLength; charNr++) {
            WriteAsUTF8(target, content.at(charNr));
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Writes new line to a UTF8 encoded stream
    //! \param[in]  target      Stream to be written to
    //! \param[in]  nlf         Kind of new line to write
    //! \throws     SCXLineStreamNewLineWriteException      New line was not written ok
    //! \throws     SCXInvalidArgumentException             nlf == eUnknown
    void SCXStream::WriteNewLineAsUTF8(std::ostream& target, const NLF nlf) {
        switch (nlf) {
        case eUnknown:
            throw SCXInvalidArgumentException(L"nlf", L"eUnknown", SCXSRCLOCATION);
        case eCR:
            target.put(cCR);
            break;
        case eLF:
            target.put(cLF);
            break;
        case eCRLF:
            target.put(cCR);
            target.put(cLF);
            break;
        case eVT:
            target.put(cVT);
            break;
        case eFF:
            target.put(cFF);
            break;
        case eNEL:
            WriteAsUTF8(target, cNEL);
            break;

#if !defined(sun)

        case eLS:
            WriteAsUTF8(target, cLS);
            break;
        case ePS:
            WriteAsUTF8(target, cPS);
            break;

#endif

        default:
            throw SCXInternalErrorException(L"All NLFs not handled", SCXSRCLOCATION);
        }
        if (!target.good()) {
            throw SCXLineStreamNewLineWriteException(SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    //! Make the locale specificed in the environment default in application
    //! \returns The environment locale
    //! \note There are two main reasons there's no method to just retrieve the environment
    //!        locale, without making it default:
    //!       (1) The ISO 99 multibyte functions  underlying our fix require the conversion locale to be set,
    //!           that is, the returned locale wouldn't be an environment locale if the locale was not set.
    //!       (2) On those platforms that locale("") work we should use it instead of our fix, but
    //!           that would mean that on those platforms that use our fix, the locale has to be set outside the function,
    //!           but not on the others. Hence the behaviour of the API wouldn't be platform independent.
    //!
    std::locale SCXStream::MakeEnvironmentLocaleDefault() {

#if defined(hpux)
        std::locale myLocale(std::locale::classic(), new SCXDefaultEncodingFacet());
        std::locale environmentLocale(std::locale(""), myLocale, std::locale::ctype);
#else
        std::locale environmentLocale(std::locale(""));
#endif
        std::locale::global(environmentLocale);
        return environmentLocale;
    }

    /**
       Gets a better codecvt facet, wrapped in a locale object.
       \returns locale object with default encoding facet.
    */
    std::locale SCXStream::GetLocaleWithSCXDefaultEncodingFacet() {
#if defined(hpux)
        std::locale myLocale(std::locale::classic(),
                             new SCXCoreLib::SCXDefaultEncodingFacet());
#else
        std::locale myLocale(std::locale::classic());
#endif
        return myLocale;
    }

    /*----------------------------------------------------------------------------*/
    //! Constructs an buffer adapter that will write to a specified wstreambuf
    //! \param[in]  target  To be written to
    SCXWideAsNarrowStreamBuf::SCXWideAsNarrowStreamBuf(std::wstreambuf *target)
        : m_target(target), m_policy(0), m_targetReadPos(m_targetBuffer), m_targetWritePos(m_targetBuffer)
    {
        // Using ordinary pointer for holding a referencee for a good reason.
        // Doesn't want to expose our facet class since its existence is a STL update that shouldn't be needed.
        // Therefore SCXHandle would have been parameterized by the STL base class, but that class
        // defines the destructor as beeing protected, which means that it isn't compatible with SCXHandle
#ifdef new
#pragma push_macro("new")
#define NEW_AS_MACRO
#undef new
#endif
        m_policy = new SCXDefaultEncodingFacet();
#ifdef NEW_AS_MACRO
#pragma pop_macro("new")
#endif
       
        setp(m_sourceBuffer, m_sourceBuffer + (s_bufferSize - 1));
        memset(&m_mbstate, '\0', sizeof(m_mbstate));
    }

    /*----------------------------------------------------------------------------*/
    //! Destructs the instance
    //! \note The adapted target wstreambuf is _not_ destructed
    SCXWideAsNarrowStreamBuf::~SCXWideAsNarrowStreamBuf() {
        try {
            FlushBuffer();
        } catch (...) {

        }
        delete static_cast<SCXDefaultEncodingFacet *> (m_policy);
    }

    /*----------------------------------------------------------------------------*/
    //! Called when there is time to flush the buffer
    //! \param[in]  c   Next character to be written
    //! \returns     Last character written, or EOF if it couldn't be written
    int SCXWideAsNarrowStreamBuf::overflow(int_type c) {
        if (c != EOF) {
            *pptr() = static_cast<char>(c);
            pbump(1);
        }
        return FlushBuffer() ? c : EOF;
    }

    /*----------------------------------------------------------------------------*/
    //! Write all intermediate content
    //! \returns    0 if everything could be writtem, -1 otherwise
    int SCXWideAsNarrowStreamBuf::sync() {
        return FlushBuffer() ? 0 : -1;
    }

    /*----------------------------------------------------------------------------*/
    //! Write all intermediate content
    //! \returns    true  iff everything could be written
    bool SCXWideAsNarrowStreamBuf::FlushBuffer() {
        SCXDefaultEncodingFacet *policy = static_cast<SCXDefaultEncodingFacet *> (m_policy);
        SCXASSERT( NULL != policy );
        const char *fromNext = pbase();
        wchar_t *targetEnd = m_targetBuffer + s_bufferSize;
        int charsWritten = 0;
        do {
            // Convert as much as possible and write it to the target buffer
            policy->do_in(m_mbstate, fromNext, pptr(), fromNext,
                    m_targetWritePos, targetEnd, m_targetWritePos);

            // Write as much of the target buffer as possible to the target
            charsWritten = static_cast<int>(m_target->sputn(m_targetReadPos, m_targetWritePos - m_targetReadPos));
            m_targetReadPos += std::max(charsWritten, 0);

            // Reset the buffer to its initial state if there isn't anymore to write
            if (m_targetReadPos >= m_targetWritePos) {
                m_targetReadPos = m_targetBuffer;
                m_targetWritePos = m_targetBuffer;
            }
        } while (charsWritten > 0);

        // Move characters left to be converted to the beginning of the buffer
        memmove(pbase(), fromNext, pptr() - fromNext);
        pbump(static_cast<int>(pbase() - fromNext));

        return pptr() == pbase() && m_targetReadPos >= m_targetWritePos;
    }
}


/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
