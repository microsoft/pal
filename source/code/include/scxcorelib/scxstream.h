/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief        A collection of stream functions assisting the stream classes

   \date        07-09-11 16:56:00

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXSTREAM_H
#define SCXSTREAM_H

#include <scxcorelib/scxexception.h>
#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <set>
#include <locale>
#include <streambuf>

namespace SCXCoreLib {
    /*----------------------------------------------------------------------------*/
    //!
    //! Placeholder not intended to be instantiated. Instead of
    //! defining a new stream abstraction the purpose is to integrate as well
    //! as possible with the standard template library definition of streams.
    //!
    class SCXStream {
        public:
        //! "New Line Function" as used in the Unicode documentation.
        enum NLF {
            eUnknown,
            eCR,        //!< Carriage Return
            eLF,        //!< Line Feed
            eCRLF,      //!< Carriage Return followed by Line Feed
            eNEL,       //!< NExt Line (EBCDIC)
            eVT,        //!< Vertical Tab (Microsoft Word)
            eFF         //!< Form Feed
#if !defined(sun)
            ,eLS,        //!< Line Separator (Unicode)
            ePS         //!< Paragraph Separator (Unicode)
#endif
        };
        //! Set of properties of a filesystem item
        typedef std::set<NLF> NLFs;

        static void ReadPartialLine(std::wistream& source, const std::wstring::size_type maxLineLength,
                                    std::wstring& line, NLF&);
        static void ReadLine(std::wistream& source, std::wstring& line, NLF&);
        static void ReadAllLines(std::wistream& source, std::vector<std::wstring>& lines, NLFs&);
        static wchar_t ReadCharAsUTF8(std::istream& source);
        static void ReadPartialLineAsUTF8(std::istream& source, const std::wstring::size_type maxLineLength,
                                          std::wstring& line, NLF&);
        static void ReadLineAsUTF8(std::istream& source, std::wstring& line, NLF&);
        static void ReadAllLinesAsUTF8(std::istream& source, std::vector<std::wstring>& lines, NLFs&);
        static void Write(std::wostream& target, wchar_t content);
        static void Write(std::wostream& target, const std::wstring& content);
        static void WriteNewLine(std::wostream& target, const NLF nlf);
        static void WriteAsUTF8(std::ostream& target, const wchar_t content);
        static void WriteAsUTF8(std::ostream& target, const std::wstring& content);
        static void WriteAsUTF8(std::ostream& target, const std::string& content);
        static void WriteNewLineAsUTF8(std::ostream& target, const NLF nlf);
        template <typename charT> static bool IsGood(std::basic_istream<charT>& source); /* Defined below */

        static std::locale MakeEnvironmentLocaleDefault();
        static std::locale GetLocaleWithSCXDefaultEncodingFacet();

        private:
        SCXStream() {}  //!< Prevent class to be inherited as well as instantiated
        static void WriteAsUTF8Basic(std::ostream& target, const wchar_t content);
    };

    /*----------------------------------------------------------------------------*/
    //! Adapter class that makes it possible to write to a std::wstreambuf like it was a
    //! streambuf, performing narrow-to-wide conversion according to the global locale.
    //! The function "SrrFromMultibyte" is convenient when the complete string is available,
    //! but it also _requires_ that the complete string is available.
    //! Example:
    //! std::wostringstream target;
    //! SCXWideAsNarrowStreamBuf adapterBuffer(target.rdbuf());
    //! std::ostream adapter(&adapterBuffer);
    class SCXWideAsNarrowStreamBuf : public std::streambuf {
    public:
        SCXWideAsNarrowStreamBuf(std::wstreambuf *target);
        ~SCXWideAsNarrowStreamBuf();
    protected:
        static const size_t s_bufferSize = 64; //!< Size of internal buffers.
        virtual int_type overflow(int_type c);
        virtual int sync();
        bool FlushBuffer();
    private:
        std::wstreambuf *m_target; //!< Buffer that the converted data will be sent to
        mbstate_t m_mbstate;       //!< Multibyte conversion state
        std::codecvt<wchar_t, char, mbstate_t>  *m_policy;     //!< How the conversion is to be done. Not SCXHandle by design, see constructor impl
        char m_sourceBuffer[s_bufferSize];        //!< Data awaiting conversion
        wchar_t m_targetBuffer[s_bufferSize];    //!< Intermediate data, converted but not sent
        wchar_t *m_targetReadPos;                //!< Where to read the next converted character to be sent
        wchar_t *m_targetWritePos;               //!< Where to write the next converted character to be sent

    };

    /*----------------------------------------------------------------------------*/
    //! Reading of a line oriented stream ended prematurely.
    class SCXLineStreamReadException : public SCXException {
        protected:
        //!  Protected constructor states that the class is not intended to be instantiated
        //!  \param[in]         location                        Where the exception occured
        SCXLineStreamReadException(const SCXCodeLocation& location)
            : SCXException(location)
            { };

    };

    /*----------------------------------------------------------------------------*/
    //! Reading of a line oriented stream was aborted due to problem with the content.
    //! Depending on circumstances the problem may be recoverable and the content
    //! appearing after the invalid content might be read in another operation.
    class SCXLineStreamContentException : public SCXLineStreamReadException {
        public:
        //!  Constructor
        //!  \param[in]         byteSequence            Content invalid in its context
        //!  \param[in]         location                        Where the exception occured
        SCXLineStreamContentException(const std::vector<unsigned char>& byteSequence,
                                      const SCXCodeLocation& location)
            : SCXLineStreamReadException(location),
            m_byteSequence(byteSequence)
            { }

        virtual std::wstring What() const;

        //! Content invalid in its context
        const std::vector<unsigned char>& GetByteSequence() const {return m_byteSequence; }
        private:
        std::vector<unsigned char> m_byteSequence; //!< Content invalid in its context

    };

    /*----------------------------------------------------------------------------*/
    //! Reading of a line oriented stream was aborted due to non stream related technical
    //! limitations such as no buffer space. The problem has nothing to do with the stream
    //! and in general the rest of the stream may be read in another operation.
    class SCXLineStreamPartialReadException : public SCXLineStreamReadException {
        public:
        //!  Constructor
        //!  \param[in]         location                        Where the exception occured
        SCXLineStreamPartialReadException( const SCXCodeLocation& location)
            : SCXLineStreamReadException(location)
            { }

        virtual std::wstring What() const {
            return L"Last line not completely read";
        };
    };

    /*----------------------------------------------------------------------------*/
    //! Writing of a line oriented stream ended prematurely.
    class SCXLineStreamWriteException : public SCXException {
        public:
        protected:
        //!  Protected constructor states that the class is not intended to be instantiated
        //!  \param[in]         location                                        Where the exception occured
        SCXLineStreamWriteException(const SCXCodeLocation& location)
            : SCXException(location)
            { };
    };

    /*----------------------------------------------------------------------------*/
    //! Writing data to a line oriented stream failed
    class SCXLineStreamContentWriteException : public SCXLineStreamWriteException {
        public:
        //!  Constructor
        //!  \param[in]         location                                        Where the exception occured
        SCXLineStreamContentWriteException(const SCXCodeLocation& location)
            : SCXLineStreamWriteException(location)
            { }

        virtual std::wstring What() const {
            return L"Writing of data did not complete sucessfully";
        };
    };

    /*----------------------------------------------------------------------------*/
    //! Writing new line to a line oriented stream failed
    class SCXLineStreamNewLineWriteException : public SCXLineStreamWriteException {
        public:
        //!  Constructor
        //!  \param[in]         location                                        Where the exception occured
        SCXLineStreamNewLineWriteException(const SCXCodeLocation& location)
            : SCXLineStreamWriteException(location)
            { }

        virtual std::wstring What() const {
            return L"Writing newline did not complete sucessfully";
        };
    };

    /**
       Tests that a stream is in a good state after read-ahead was attempted.

       If the previously called method on the istream argument was a peek then
       this function tests if next call to get will succeed or not.
       The whole point of this function is to get around the fact that HPUX/aCC
       does not update the stream state when peek() is executed.

       \param source Input stream to test for goodness
       \return true if next call to get will succeed
    */
    template <typename charT>
    inline bool SCXStream::IsGood(std::basic_istream<charT>& source) {
        return (source.peek() != std::char_traits<charT>::eof()) && source.good();
    }
}


#endif /* SCXSTREAM_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
