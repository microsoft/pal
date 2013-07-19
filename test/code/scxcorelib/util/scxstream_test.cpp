/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2007-09-17 10:31:00


*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxlocale.h>
#include <scxcorelib/scxfile.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/plugin/TestPlugIn.h>

#include <scxcorelib/scxexception.h>
#include <testutils/scxtestutils.h>

#include <sstream>
#include <locale.h>
#include <locale>

using namespace std;
using namespace SCXCoreLib;

class SCXStreamTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStreamTest ); 

    CPPUNIT_TEST( TestReadLineNewlines );
    CPPUNIT_TEST( TestReadLineNoLine);
    CPPUNIT_TEST( TestReadLineEmpty );
    CPPUNIT_TEST( TestReadAllLinesMoreThanOne );
    CPPUNIT_TEST( TestReadAllLinesNoLine );
    CPPUNIT_TEST( TestReadPartialLine );
    CPPUNIT_TEST( TestReadCharAsUTF8NoChar );

    CPPUNIT_TEST( TestReadCharAsUTF8ManyChars );

    CPPUNIT_TEST( TestReadCharAsUTF8Errors );
    CPPUNIT_TEST( TestReadLineAsUTF8Newlines );
    CPPUNIT_TEST( TestReadLineAsUTF8NoLine );
    CPPUNIT_TEST( TestReadLineAsUTF8Empty );
    CPPUNIT_TEST( TestReadAllLinesAsUTF8MoreThanOne );
    CPPUNIT_TEST( TestReadAllLinesAsUTF8NoLine );
    CPPUNIT_TEST( TestReadPartialLineAsUTF8 );
    CPPUNIT_TEST( TestWriteCharAsUTF8 );
    CPPUNIT_TEST( TestWriteNewLineAsUTF8 );
    CPPUNIT_TEST( TestWriteAsUTF8 );
    CPPUNIT_TEST( TestWriteNewLine );
    CPPUNIT_TEST( TestWrite );
    CPPUNIT_TEST( TestEnvironmentLocale );
    SCXUNIT_TEST( TestConversionFacet, 0 ); // Disable threaded test. Won't work on Linux.
    CPPUNIT_TEST( TestNarrowToWideOutStream );

    SCXUNIT_TEST_ATTRIBUTE(TestNarrowToWideOutStream, SLOW);
    CPPUNIT_TEST_SUITE_END();

    private:
    /* CUSTOMIZE: Add any data commonly used in several tests as members here. */
    unsigned char m_CR;
    unsigned char m_LF;
    unsigned char m_VT;
    unsigned char m_FF;
#if !defined(sun)
    unsigned char m_LSPS1;
    unsigned char m_LSPS2;
    unsigned char m_LS3;
    unsigned char m_PS3;
    wchar_t m_LS;
    wchar_t m_PS;
#endif
    unsigned char m_NEL1;
    unsigned char m_NEL2;
    wchar_t m_NEL;
    bool m_SolarisAndClocale;

    public:
    void setUp(void)
    {
        m_CR = 0x0D;
        m_LF = 0x0A;
        m_VT = 0x0B;
        m_FF = 0x0C;
#if !defined(sun)
        m_LSPS1 = 0xE2;
        m_LSPS2 = 0x80;
        m_LS3 = 0xA8;
        m_PS3 = 0xA9;
        m_LS = 0x2028;
        m_PS = 0x2029;
        m_NEL = 0x85;
        m_SolarisAndClocale = false;
#else
        m_NEL = StrFromUTF8("\xc2\x85")[0];
        m_SolarisAndClocale = (SCXLocaleContext::GetCtypeName() == L"C");
#endif
        m_NEL1 = 0xC2;
        m_NEL2 = 0x85;
    }

    void tearDown(void)
    {
        /* CUSTOMIZE: This method will be called once after each test function with no regard of success or failure. */
    }

    void TestReadLineNewlines()
    {
        SCXStream::NLF nlf;
        std::wostringstream data;
        wchar_t cr = data.widen(m_CR);
        wchar_t lf = data.widen(m_LF);
        wchar_t vt = data.widen(m_VT);
        wchar_t ff = data.widen(m_FF);
        data << L"Row 1" << cr   << L"Row 2" << lf   << L"Row 3" << cr << lf;
        data << L"Row 4" << vt   << L"Row 5" << ff;
        if (!m_SolarisAndClocale)
        {
            data << L"Row 6" << m_NEL;
        }
#if !defined(sun)
        data << L"Row 7" << m_LS << L"Row 8" << m_PS;
#endif
        data << L"Row 9";
        wstring line(L"Dummydata");
        std::wistringstream  source(data.str());
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 1" && nlf == SCXStream::eCR);
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 2" && nlf == SCXStream::eLF);
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 3" && nlf == SCXStream::eCRLF);
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 4" && nlf == SCXStream::eVT);
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 5" && nlf == SCXStream::eFF);
        if (!m_SolarisAndClocale)
        {
            SCXStream::ReadLine(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 6" && nlf == SCXStream::eNEL);
        }
#if !defined(sun)
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 7" && nlf == SCXStream::eLS);
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 8" && nlf == SCXStream::ePS);
#endif
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 9" && nlf == SCXStream::eUnknown);
    }

    void TestReadLineNoLine() {
        SCXStream::NLF nlf;
        std::wistringstream  source;
        wstring line;
        CPPUNIT_ASSERT_THROW(SCXStream::ReadLine(source, line, nlf), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestReadLineEmpty() {
        SCXStream::NLF nlf;
        std::wostringstream data;
        wchar_t lf = data.widen(m_LF);
        data << lf << L"Row 2";
        std::wistringstream  source(data.str());
        wstring line(L"Dummydata");
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"");
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 2");
    }

    void TestReadAllLinesMoreThanOne() {
        SCXStream::NLFs nlfs;
        std::wostringstream data;
        wchar_t cr = data.widen(m_CR);
        data << L"Row 1" << cr << L"Row 2";
        vector<wstring> linesRead;
        std::wistringstream  source(data.str());
        SCXStream::ReadAllLines(source, linesRead, nlfs);
        vector<wstring> linesExpected;
        linesExpected.push_back(L"Row 1");
        linesExpected.push_back(L"Row 2");
        CPPUNIT_ASSERT(linesRead == linesExpected);
    }

    void TestReadAllLinesNoLine() {
        try {
            SCXStream::NLFs nlfs;
            vector<wstring> linesRead;
            std::wistringstream  source;
            SCXStream::ReadAllLines(source, linesRead, nlfs);
            vector<wstring> linesExpected;
            CPPUNIT_ASSERT(linesRead == linesExpected);
        } catch (SCXException& e) {
            cout << StrToUTF8(e.What()) <<  " occured at " << StrToUTF8(e.Where());
            CPPUNIT_ASSERT(!"throws exception");
        }
    }

    void TestReadPartialLine() {
        SCXStream::NLF nlf;
        std::wostringstream target;
        target << L"Kalle Olle Lasse";
        std::wistringstream source(target.str());
        wstring line;
        SCXStream::ReadPartialLine(source, 6, line, nlf);
        CPPUNIT_ASSERT(line == L"Kalle " && nlf == SCXStream::eUnknown && source.peek() != WEOF);
        SCXStream::ReadPartialLine(source, 20, line, nlf);
        CPPUNIT_ASSERT(line == L"Olle Lasse" && nlf == SCXStream::eUnknown && source.peek() == WEOF);
    }

    void TestReadCharAsUTF8NoChar() {
        istringstream  source;
        CPPUNIT_ASSERT_THROW(SCXStream::ReadCharAsUTF8(source), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestReadCharAsUTF8ManyChars() {
        try {
            ostringstream data;
            data << 'A';
#if !defined(sun)
            data << m_LSPS1 << m_LSPS2 << m_LS3;
            data << m_LSPS1 << m_LSPS2 << m_PS3;
#endif
            data << m_NEL1 << m_NEL2;
            data << 'B';
            istringstream source(data.str());
            wchar_t unicodeChar;
            unicodeChar = SCXStream::ReadCharAsUTF8(source);
            CPPUNIT_ASSERT(unicodeChar == 'A');
#if !defined(sun)
            unicodeChar = SCXStream::ReadCharAsUTF8(source);
            CPPUNIT_ASSERT(unicodeChar == m_LS);
            unicodeChar = SCXStream::ReadCharAsUTF8(source);
            CPPUNIT_ASSERT(unicodeChar == m_PS);
#endif
            unicodeChar = SCXStream::ReadCharAsUTF8(source);
            CPPUNIT_ASSERT(unicodeChar == m_NEL);
            unicodeChar = SCXStream::ReadCharAsUTF8(source);
            CPPUNIT_ASSERT(unicodeChar == 'B');
        } catch (SCXException& e) {
            cout << StrToUTF8(e.What()) <<  " occured at " << StrToUTF8(e.Where());
            CPPUNIT_ASSERT(!"throws exception");
        }
    }

    void TestReadCharAsUTF8Errors() {
        ostringstream data;
        data << SCXStreamTest::m_NEL1;
        data << "A";
        data << m_NEL1;
        istringstream source(data.str());
        wchar_t unicodeChar;
        try {
            unicodeChar = SCXStream::ReadCharAsUTF8(source);
            CPPUNIT_ASSERT(!"Identified UTF8 problem");
        } catch (SCXLineStreamContentException& e1) {
            vector<unsigned char> expectedByteSequence;
            expectedByteSequence.push_back(m_NEL1);
            CPPUNIT_ASSERT(e1.GetByteSequence() == expectedByteSequence);
            unicodeChar = SCXStream::ReadCharAsUTF8(source);
            CPPUNIT_ASSERT(unicodeChar == static_cast<wchar_t>(source.widen('A')));
            try {
                unicodeChar = SCXStream::ReadCharAsUTF8(source);
                CPPUNIT_ASSERT(!"Identified UTF8 problem");
            } catch (SCXLineStreamContentException& e2) {
                CPPUNIT_ASSERT(e2.GetByteSequence() == expectedByteSequence);
            }
        }
    }

    void TestReadLineAsUTF8Newlines() {
        try {
            SCXStream::NLF nlf;
            ostringstream data;
            data << "Row 1" << m_CR << "Row 2" << m_LF << "Row 3" << m_CR << m_LF;
#if !defined(sun)
            data << "Row 4" << m_LSPS1 << m_LSPS2 << m_LS3;
            data << "Row 5" << m_LSPS1 << m_LSPS2 << m_PS3;
#endif
            if (!m_SolarisAndClocale)
            {
                data << "Row 6" << m_NEL1 << m_NEL2;
            }
            data << "Row 7" << m_VT << "Row 8" << m_FF << "Row 9";
            wstring line(L"Dummydata");
            istringstream  source(data.str());
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 1" && nlf == SCXStream::eCR);
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 2" && nlf == SCXStream::eLF);
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 3" && nlf == SCXStream::eCRLF);
#if !defined(sun)
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 4" && nlf == SCXStream::eLS);
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 5" && nlf == SCXStream::ePS);
#endif
            if (!m_SolarisAndClocale)
            {
                SCXStream::ReadLineAsUTF8(source, line, nlf);
                CPPUNIT_ASSERT(line == L"Row 6" && nlf == SCXStream::eNEL);
            }
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 7" && nlf == SCXStream::eVT);
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 8" && nlf == SCXStream::eFF);
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 9" && nlf == SCXStream::eUnknown);
        } catch (SCXException& e) {
            cout << StrToUTF8(e.What()) <<  " occured at " << StrToUTF8(e.Where());
            CPPUNIT_ASSERT(!"throws exception");
        }
    }

    void TestReadLineAsUTF8NoLine() {
        SCXStream::NLF nlf;
        istringstream  source;
        wstring line;
        CPPUNIT_ASSERT_THROW(SCXStream::ReadLineAsUTF8(source, line, nlf), SCXInvalidArgumentException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestReadLineAsUTF8Empty() {
        try {
            SCXStream::NLF nlf;
            ostringstream data;
            data << m_LF << "Row 2";
            istringstream  source(data.str());
            wstring line(L"Dummydata");
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"" && nlf == SCXStream::eLF);
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 2" && nlf == SCXStream::eUnknown);
        } catch (SCXException& e) {
            cout << StrToUTF8(e.What()) <<  " occured at " << StrToUTF8(e.Where());
            CPPUNIT_ASSERT(!"throws exception");
        }
    }

    void TestReadAllLinesAsUTF8MoreThanOne() {
        try {
            SCXStream::NLFs nlfs;
            ostringstream data;
            data << "Row 1" << m_CR << "Row 2";
            vector<wstring> linesRead;
            istringstream  source(data.str());
            SCXStream::ReadAllLinesAsUTF8(source, linesRead, nlfs);
            vector<wstring> linesExpected;
            linesExpected.push_back(L"Row 1");
            linesExpected.push_back(L"Row 2");
            CPPUNIT_ASSERT(linesRead == linesExpected && nlfs.count(SCXStream::eCR) == 1);
        } catch (SCXException& e) {
            cout << StrToUTF8(e.What()) <<  " occured at " << StrToUTF8(e.Where());
            CPPUNIT_ASSERT(!"throws exception");
        }
    }

    void TestReadAllLinesAsUTF8NoLine() {
        try {
            SCXStream::NLFs nlfs;
            vector<wstring> linesRead;
            istringstream  source;
            SCXStream::ReadAllLinesAsUTF8(source, linesRead, nlfs);
            vector<wstring> linesExpected;
            CPPUNIT_ASSERT(linesRead == linesExpected);
        } catch (SCXException& e) {
            cout << StrToUTF8(e.What()) <<  " occured at " << StrToUTF8(e.Where());
            CPPUNIT_ASSERT(!"throws exception");
        }
    }

    void TestReadPartialLineAsUTF8() {
        SCXStream::NLF nlf;
        ostringstream target;
        target << "Kalle Olle Lasse";
        istringstream source(target.str());
        wstring line;
        SCXStream::ReadPartialLineAsUTF8(source, 6, line, nlf);
        CPPUNIT_ASSERT(line == L"Kalle " && nlf == SCXStream::eUnknown && source.peek() != EOF);
        SCXStream::ReadPartialLineAsUTF8(source, 20, line, nlf);
        CPPUNIT_ASSERT(line == L"Olle Lasse" && nlf == SCXStream::eUnknown && source.peek() == EOF);
    }

    void TestWriteCharAsUTF8() {
        ostringstream target;
        SCXStream::WriteAsUTF8(target, 'A');
        if (!m_SolarisAndClocale)
        {
            SCXStream::WriteAsUTF8(target, m_NEL);
        }
#if !defined(sun)
        SCXStream::WriteAsUTF8(target, m_LS);
        SCXStream::WriteAsUTF8(target, m_PS);
#endif
        SCXStream::WriteAsUTF8(target, 'B');
        istringstream source(target.str());
        CPPUNIT_ASSERT(source.get() == 'A');
        if (!m_SolarisAndClocale)
        {
            CPPUNIT_ASSERT(source.get() == m_NEL1);
            CPPUNIT_ASSERT(source.get() == m_NEL2);
        }
#if !defined(sun)
        CPPUNIT_ASSERT(source.get() == m_LSPS1);
        CPPUNIT_ASSERT(source.get() == m_LSPS2);
        CPPUNIT_ASSERT(source.get() == m_LS3);
        CPPUNIT_ASSERT(source.get() == m_LSPS1);
        CPPUNIT_ASSERT(source.get() == m_LSPS2);
        CPPUNIT_ASSERT(source.get() == m_PS3);
#endif
        CPPUNIT_ASSERT(source.get() == 'B');
    }

    void TestWriteNewLineAsUTF8() {
        ostringstream target;
        SCXStream::WriteNewLineAsUTF8(target, SCXStream::eCR);
        SCXStream::WriteNewLineAsUTF8(target, SCXStream::eLF);
        SCXStream::WriteNewLineAsUTF8(target, SCXStream::eCRLF);
        SCXStream::WriteNewLineAsUTF8(target, SCXStream::eFF);
        SCXStream::WriteNewLineAsUTF8(target, SCXStream::eVT);
        if (!m_SolarisAndClocale)
        {
            SCXStream::WriteNewLineAsUTF8(target, SCXStream::eNEL);
        }
#if !defined(sun)
        SCXStream::WriteNewLineAsUTF8(target, SCXStream::eLS);
        SCXStream::WriteNewLineAsUTF8(target, SCXStream::ePS);
#endif
        istringstream source(target.str());
        CPPUNIT_ASSERT(source.get() == m_CR);
        CPPUNIT_ASSERT(source.get() == m_LF);
        CPPUNIT_ASSERT(source.get() == m_CR);
        CPPUNIT_ASSERT(source.get() == m_LF);
        CPPUNIT_ASSERT(source.get() == m_FF);
        CPPUNIT_ASSERT(source.get() == m_VT);
        if (!m_SolarisAndClocale)
        {
            CPPUNIT_ASSERT(source.get() == m_NEL1);
            CPPUNIT_ASSERT(source.get() == m_NEL2);
        }
#if !defined(sun)
        CPPUNIT_ASSERT(source.get() == m_LSPS1);
        CPPUNIT_ASSERT(source.get() == m_LSPS2);
        CPPUNIT_ASSERT(source.get() == m_LS3);
        CPPUNIT_ASSERT(source.get() == m_LSPS1);
        CPPUNIT_ASSERT(source.get() == m_LSPS2);
        CPPUNIT_ASSERT(source.get() == m_PS3);
#endif
    }

    void TestWriteAsUTF8() {
        try {
            ostringstream target;
            SCXStream::WriteAsUTF8(target, L"Row 1");
            SCXStream::WriteNewLineAsUTF8(target, SCXStream::eCRLF);
            SCXStream::WriteAsUTF8(target, L"Row 2");
            wstring line;
            SCXStream::NLF nlf;
            istringstream source(target.str());
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 1" && nlf == SCXStream::eCRLF);
            SCXStream::ReadLineAsUTF8(source, line, nlf);
            CPPUNIT_ASSERT(line == L"Row 2" && nlf == SCXStream::eUnknown);
        } catch (SCXException& e) {
            cout << StrToUTF8(e.What()) <<  " occured at " << StrToUTF8(e.Where());
            CPPUNIT_ASSERT(!"throws exception");
        }
    }

    void TestWriteNewLine() {
        std::wostringstream target;
        SCXStream::WriteNewLine(target, SCXStream::eCR);
        SCXStream::WriteNewLine(target, SCXStream::eLF);
        SCXStream::WriteNewLine(target, SCXStream::eCRLF);
        SCXStream::WriteNewLine(target, SCXStream::eFF);
        SCXStream::WriteNewLine(target, SCXStream::eVT);
        if (!m_SolarisAndClocale)
        {
            SCXStream::WriteNewLine(target, SCXStream::eNEL);
        }
#if !defined(sun)
        SCXStream::WriteNewLine(target, SCXStream::eLS);
        SCXStream::WriteNewLine(target, SCXStream::ePS);
#endif
        std::wistringstream source(target.str());
        CPPUNIT_ASSERT(source.get() == m_CR);
        CPPUNIT_ASSERT(source.get() == m_LF);
        CPPUNIT_ASSERT(source.get() == m_CR);
        CPPUNIT_ASSERT(source.get() == m_LF);
        CPPUNIT_ASSERT(source.get() == m_FF);
        CPPUNIT_ASSERT(source.get() == m_VT);
        if (!m_SolarisAndClocale)
        {
            CPPUNIT_ASSERT(static_cast<wchar_t>(source.get()) == m_NEL);
        }
#if !defined(sun)
        CPPUNIT_ASSERT(static_cast<wchar_t>(source.get()) == m_LS);
        CPPUNIT_ASSERT(static_cast<wchar_t>(source.get()) == m_PS);
#endif
    }

    void TestWrite() {
        std::wostringstream target;
        SCXStream::Write(target, L"Row 1");
        SCXStream::WriteNewLine(target, SCXStream::eCRLF);
        SCXStream::Write(target, L"Row 2");
        wstring line;
        SCXStream::NLF nlf;
        std::wistringstream source(target.str());
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 1" && nlf == SCXStream::eCRLF);
        SCXStream::ReadLine(source, line, nlf);
        CPPUNIT_ASSERT(line == L"Row 2" && nlf == SCXStream::eUnknown);
    }

    void TestEnvironmentLocale() {

        SCXCoreLib::SCXLocaleContext foo("");
        
        CheckEnvironmentLocaleConversion(L"A");
        CheckEnvironmentLocaleConversion(L"AB");
        CheckEnvironmentLocaleConversion(L"AbC");
        CheckEnvironmentLocaleConversion(L"ABCd");

        /* The C locale is not required to support these characters. */
        if ("C" != foo.name())
        {
            wstring intstr;
            try {
                SCXStream::NLF nlf;
                ifstream infs("./testfiles/env_loc_en_US.UTF-8.txt", ios_base::in);
                SCXStream::ReadLineAsUTF8(infs, intstr, nlf);
                infs.close();
            } catch (SCXException& e) {
                wostringstream ss;
                ss << e.What() << "--" << e.Where();
                CPPUNIT_FAIL( StrToUTF8(ss.str()) );
            }
            CheckEnvironmentLocaleConversion(intstr); 
        }
        else
        {
            SCXUNIT_WARNING(L"TestEnvironmentLocale--Environment Locale is C and  cannot test non-ascii charactor!");
        }
    }

    void CheckEnvironmentLocaleConversion(const wstring &text) {
        const string filename = "CheckEnvironmentLocaleConversion";
        // Make sure the file is deleted 
        SelfDeletingFilePath RAIIFile(StrFromUTF8(filename));
        wofstream outfs(filename.c_str(), ios_base::out);

        outfs << text.c_str();
        outfs.flush();
        outfs.close();
        
        wfstream infs(filename.c_str(), ios_base::in);
        wostringstream buf;
        wchar_t wc;
        while (infs.get(wc).good()) {
            buf << wc;
        }
        infs.close();
        CPPUNIT_ASSERT(text == buf.str());
    }

    /* This tests that the current locale has a facet that can correctly convert between
       UTF-8 as external representation and UCS-4 as the internal representation.
       Strictly speaking, this is only needed on HPUX since that's the only place where we
       run our own code conversion, but the other platforms should be able to pass this
       test anyways.
       However:
       - Linux has a strange quirk that makes the program hang if we compare two wchar_t's.
         This was discovered by be an issue with running the test in threads.
       - There is a boolean in the code that detemines if we are allowed to accept
         partial encodings as intermediate results. There is no reason that the conversions
         as we have set them up should need to produce partials, but our code_cvt facet
         for HPUX does exactly that. (Strangely enough, only sometimes.)
       - On HPUX our code_cvt facet doesn't handle NUL chars correctly.
         (But neither does Suns codecvt, so it's probably just pointless to fix.)
       - Many of our test host aren't set up to handle unicode. Either LANG=en_US.utf8,
         is not set, or else that entire codec is missing.
    */
    void TestConversionFacet()
    {
        // Currently disabled for the last reason outlined above.
#if 0
        SCXCoreLib::SCXLocaleContext temporary_context("");

        /*
         * We have defined the same string in two different encodings. We will
         * translate both of them into a buffers and compare the result with the
         * other predefined encoding. Both must match exactly.
         */

        // The characters ÅÄÖ encoded as a UTF-8 string.
        // Sun and HP can't have a 0 at the end.
        // const char mbstr[] = { (char)0xc3, (char)0x85, (char)0xc3, (char)0x84,
        //                        (char)0xc3, (char)0x96, 0x0};
        const char mbstr[] = { (char)0xc3, (char)0x85, (char)0xc3, (char)0x84,
                               (char)0xc3, (char)0x96};
        const size_t mblen = sizeof(mbstr);
        const char *mbstrend = mbstr + mblen;
        const char *mbnext = mbstr;

        // The characters ÅÄÖ encoded as a UCS4/UTF-32 string.
        // const wchar_t wcstr[] = { 0x00c5, 0x00c4, 0x00d6, 0x0000};  // Again, Sun and HP...
        const wchar_t wcstr[] = { 0x00c5, 0x00c4, 0x00d6};
        const size_t wclen = sizeof(wcstr) / sizeof(wchar_t);
        const wchar_t *wcstrend = wcstr + wclen;
        const wchar_t *wcnext = 0;

        // Buffer receiving wide characters
        const size_t wcbufsz = 128;
        wchar_t wcbuf[wcbufsz];
        wchar_t *wcbufend = wcbuf + wcbufsz;
        wchar_t *wcbufnext = wcbuf;

        // Buffer receiving multi-byte characters
        const size_t mbbufsz = 128;
        char mbbuf[mbbufsz];
        char *mbbufend = mbbuf + mbbufsz;
        char *mbbufnext = 0;

        locale loc;             // Initiated to the current locale
        const bool allow_partial_conversions = true;
        codecvt_base::result res;
        size_t i = 0;
        mbstate_t state;
        memchr((void*)&state, 0, sizeof(state));

        /* Sun's CC can't do functions that are templatized only on return type. */
#if defined(sun) && defined(_RWSTD_NO_TEMPLATE_ON_RETURN_TYPE)
        const std::codecvt<wchar_t, char, mbstate_t>& translator =
            use_facet(loc, static_cast<std::codecvt<wchar_t, char, mbstate_t>*>(0));
#else
        const std::codecvt<wchar_t, char, mbstate_t>& translator =
            use_facet<std::codecvt<wchar_t, char, mbstate_t> >(loc);
#endif /* sun */

        if (allow_partial_conversions) {
            const char *mbptr = mbstr;
            wchar_t *wcptr = wcbuf;
            do {
                res = translator.in(state, mbptr, mbstrend, mbnext,
                                    wcptr, wcbufend, wcbufnext);

                // Diagnostic output for debugging
                // cout << "Input characters consumed: " << (mbnext - mbstr)
                //     << ". Characters remaining: " << (mbstrend - mbnext) << endl;

                //cout << "Output characters produced: " << (wcbufnext - wcbuf)
                //     << ". Remaining buffer space: " << (wcbufend - wcbufnext) << endl;

                mbptr = mbnext;
                wcptr = wcbufnext;
            } while (res == codecvt_base::partial);
        } else {
            res = translator.in(state, mbstr, mbstrend, mbnext,
                                wcbuf, wcbufend, wcbufnext);
            CPPUNIT_ASSERT(res != codecvt_base::partial);
        }

        CPPUNIT_ASSERT(res == codecvt_base::ok);        // Conversion went well
        CPPUNIT_ASSERT(mbnext == mbstrend);             // All characters consumed
        CPPUNIT_ASSERT_EQUAL(wclen, (size_t)(wcbufnext - wcbuf));// Number of resulting chars

        // Test that all characters have the expected value
        for(i = 0; i < wclen; ++i) {
            CPPUNIT_ASSERT_EQUAL(wcstr[i], wcbuf[i]);
        }

        /* Now do the opposite conversion: UCS-4 to UTF-8. */

        memchr((void*)&state, 0, sizeof(state));

        if (allow_partial_conversions) {
            const wchar_t *wcptr = wcstr;
            char *mbptr = mbbuf;
            do {
                res = translator.out(state, wcptr, wcstrend, wcnext,
                                     mbptr, mbbufend, mbbufnext);

                // Diagnostic output for debugging
                //cout << "Input characters consumed: " << (wcnext - wcstr)
                //     << ". Characters remaining: " << (wcstrend - wcnext) << endl;

                //cout << "Output characters produced: " << (mbbufnext - mbbuf)
                //     << ". Remaining buffer space: " << (mbbufend - mbbufnext) << endl;

                wcptr = wcnext;
                mbptr = mbbufnext;
            } while (res == codecvt_base::partial);
        } else {
            res = translator.out(state, wcstr, wcstrend, wcnext, mbbuf, mbbufend, mbbufnext);
            CPPUNIT_ASSERT(res != codecvt_base::partial);
        }

        CPPUNIT_ASSERT(res == codecvt_base::ok); // Conversion went well
        CPPUNIT_ASSERT(wcnext == wcstrend); // All characters consumed
        CPPUNIT_ASSERT_EQUAL(mblen, (size_t)(mbbufnext - mbbuf));// Number of resulting chars

        // Test that all characters have the expected value
        for(i = 0; i < mblen; ++i) {
            CPPUNIT_ASSERT_EQUAL(mbstr[i], mbbuf[i]);
        }

// End of disabled code
#endif
    }

    void TestNarrowToWideOutStream() {
        std::wostringstream target;
        SCXWideAsNarrowStreamBuf adapterBuffer(target.rdbuf());
        std::ostream adapter(&adapterBuffer);
        std::wostringstream reference;
        for (unsigned length = 0; length < 255; length++) {
            reference.str(L"");
            for (int nr = length - 1; nr >= 0; nr--) {
                wchar_t c = 90 + length % 25;
                reference << c;
            }
            for (unsigned nr = 0; nr < length; nr++) {
                wchar_t c = 90 + length % 25;
                reference << c;
            }
            target.str(L"");
            adapter << StrToUTF8(reference.str()).c_str();
            adapter.flush();
            CPPUNIT_ASSERT(target.str() == reference.str());
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStreamTest ); /* CUSTOMIZE: Name must be same as classname */
