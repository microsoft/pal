/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

    Created date    2007-05-24 08:10:00

    String aid method test class.

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxlocale.h>

#include <testutils/scxunit.h>

#include <string>
#include <vector>
#include <exception>
#include <stdexcept>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <langinfo.h> // To look up codepage (locale)


using namespace SCXCoreLib;

// A few test code points for the locale-independent upcase and downcase functions
struct TestInfo
{
    unsigned int Char;          // a test character
    unsigned int Converted;     // its upper or lowercase equivalent
};

// Some test characters and their upper case equivalents
static TestInfo UpcaseTestTable[] =
{
    { '\f',   '\f' },           // no case
    { '5',    '5' },            // no case
    { 'C',    'C' },            // already upper case
    { 'q',    'Q' },
    { 0x00A5, 0x00A5 },
    { 0x00A3, 0x00A3 },         // already upper case
    { 0x00FF, 0x0178 },
    { 0x0180, 0x0243 },
    { 0x01A0, 0x01A0 },         // no case
    { 0x0217, 0x0216 },
    { 0x023D, 0x023D },         // already upper case
    { 0x0280, 0x01A6 },
    { 0x02FF, 0x02FF },         // no case
    { 0x0377, 0x0376 },
    { 0x03CC, 0x038C },
    { 0x03D8, 0x03D8 },         // already upper case
    { 0x04A7, 0x04A6 },
    { 0x048A, 0x048A },         // already upper case
    { 0x04FF, 0x04FE },
    { 0x052D, 0x052D },         // no case
    { 0x0575, 0x0545 },
    { 0x0660, 0x0660 },         // no case
    { 0x1F97, 0x1F9F },
    { 0x1D7D, 0x2C63 },
    { 0x3089, 0x3089 },         // no case
    { 0x8080, 0x8080 },         // no case
    { 0xA78C, 0xA78B },
    { 0xFF41, 0xFF21 },
    { 0xFFD0, 0xFFD0 },         // no case
    { 0x00013478, 0x00013478 }  // no case
};

// Some test characters and their lower case equivalents
static TestInfo DowncaseTestTable[] =
{
    { 0x11,   0x11 },           // no case
    { '%',    '%' },            // no case
    { 'C',    'c' },
    { 'z',    'z' },            // already lower case
    { 0x00A6, 0x00A6 },         // no case
    { 0x00C3, 0x00E3 },
    { 0x0133, 0x0133 },         // no case
    { 0x0216, 0x0217 },
    { 0x0289, 0x0289 },         // already lower case
    { 0x02DE, 0x02DE },         // no case
    { 0x0376, 0x0377 },
    { 0x03B1, 0x03B1 },         // already lower case
    { 0x04C3, 0x04C4 },
    { 0x0512, 0x0513 },
    { 0x052D, 0x052D },         // no case
    { 0x0660, 0x0660 },         // no case
    { 0x1F9F, 0x1F97 },
    { 0x2C63, 0x1D7D },
    { 0x8088, 0x8088 },         // no case
    { 0xA78B, 0xA78C },
    { 0xC173, 0xC173 },         // no case
    { 0xFF25, 0xFF45 },
    { 0xFFE0, 0xFFE0 },         // no case
    { 0x000E43F5, 0x000E43F5 }  // no case
};

class SCXStringAid_Test : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXStringAid_Test  );

    CPPUNIT_TEST( testTrim );
    CPPUNIT_TEST( testStrip );
    CPPUNIT_TEST( testToUpper );
    CPPUNIT_TEST( testCompare );
    CPPUNIT_TEST( testIsPrefix );
    CPPUNIT_TEST( testAppend );
    CPPUNIT_TEST( testTokenize );
    CPPUNIT_TEST( testTokenizeStr );
    CPPUNIT_TEST( testToIntETC );
    CPPUNIT_TEST( testFrom );
    CPPUNIT_TEST( testUTF8Conversion );
    CPPUNIT_TEST( testUTF8ConversionFails );
    CPPUNIT_TEST( testMergeTokens );
    CPPUNIT_TEST( testTokenizeWithDelimiters );
    CPPUNIT_TEST( testFromMultibyte );
    CPPUNIT_TEST( testFromMultibyteNoThrow );
    CPPUNIT_TEST( testDumpStringException );
    CPPUNIT_TEST( testUtfUpDownCase );

// This test should not be run on HPUX 11iv2 since we know that vsnprintf does not behave well on that platform
#if (!defined(hpux) || !((PF_MAJOR==11) && (PF_MINOR < 31))) && (!defined(sun) || !((PF_MAJOR==5) && (PF_MINOR <= 9)))
    CPPUNIT_TEST( testUNIX03 );
#endif

    CPPUNIT_TEST( test_Empty_String );
    CPPUNIT_TEST( test_Nonquoted_String_OnlySpaces );
    CPPUNIT_TEST( test_Nonquoted_String_EmptyTokensOnly );
    CPPUNIT_TEST( test_Nonquoted_String );
    CPPUNIT_TEST( test_Nonquoted_String_ReturnEmptyTokens );
    CPPUNIT_TEST( test_QuotedQuotes_Ignored );
    CPPUNIT_TEST( test_Quoted_String );
    CPPUNIT_TEST( test_Quoted_String_Double );
    CPPUNIT_TEST( test_Quoted_SingleQuote );
    CPPUNIT_TEST( test_Quoted_SingleQuote_ReturnEmptyTokens );
    CPPUNIT_TEST( test_Quoted_SingleQuote_QuotedSpaces );
    CPPUNIT_TEST( test_Quoted_String_QuotedQuotes );
    CPPUNIT_TEST( test_Quoted_SingleQuotedElement );
    CPPUNIT_TEST( test_Quoted_UnterminatedQuote );

    CPPUNIT_TEST_SUITE_END();

protected:
    void testTrim()
    {
        std::wstring w_tst(L"\t  Test String  \t");
        std::wstring w_tst1(L"Test String  \t");
        std::wstring w_tst2(L"\t  Test String");
        std::wstring w_tst3(L"Test String");
        std::wstring w_empty(L"");
        std::wstring w_blank(L" \t  ");

        // Check that TrimL removes whitespace to the left of the string
        CPPUNIT_ASSERT(w_tst1 == SCXCoreLib::StrTrimL(w_tst));

        // Check that TrimR removes whitespace to the right of the string
        CPPUNIT_ASSERT(w_tst2 == SCXCoreLib::StrTrimR(w_tst));

        // Check that Trim removes whitespace at both sides of the string
        CPPUNIT_ASSERT(w_tst3 == SCXCoreLib::StrTrim(w_tst));

        // Check that Trim works on empty string
        CPPUNIT_ASSERT(w_empty == SCXCoreLib::StrTrim(w_empty));

        // Check that TrimL works on blank string
        CPPUNIT_ASSERT(w_empty == SCXCoreLib::StrTrimL(w_blank));

        // Check that TrimR works on blank string
        CPPUNIT_ASSERT(w_empty == SCXCoreLib::StrTrimR(w_blank));
    }

    void testStrip()
    {
        std::wstring w_tst(L"\n. Test String. \n");
        std::wstring w_tst1(L"Test String. \n");
        std::wstring w_tst2(L"\n. Test String");
        std::wstring w_tst3(L"Test String");
        std::wstring w_empty(L"");
        std::wstring w_blank(L".\n .");
        std::wstring w_what(L". \n");

        // Check that TrimL removes whitespace to the left of the string
        CPPUNIT_ASSERT(w_tst1 == SCXCoreLib::StrStripL(w_tst, w_what));

        // Check that TrimR removes whitespace to the right of the string
        CPPUNIT_ASSERT(w_tst2 == SCXCoreLib::StrStripR(w_tst, w_what));

        // Check that Trim removes whitespace at both sides of the string
        CPPUNIT_ASSERT(w_tst3 == SCXCoreLib::StrStrip(w_tst, w_what));

        // Check non-trimmable strings are not trimmed.
        CPPUNIT_ASSERT(w_tst3 == SCXCoreLib::StrStrip(w_tst3, w_what));

        // Check that Trim works on empty string
        CPPUNIT_ASSERT(w_empty == SCXCoreLib::StrStrip(w_empty, w_what));

        // Check that TrimL works on blank string
        CPPUNIT_ASSERT(w_empty == SCXCoreLib::StrStripL(w_blank, w_what));

        // Check that TrimR works on blank string
        CPPUNIT_ASSERT(w_empty == SCXCoreLib::StrStripR(w_blank, w_what));
    }

    void testToUpper()
    {
        std::wstring w_tst(L"A Small Test String");
        std::wstring w_tst1(L"A SMALL TEST STRING");

        // Test that ToUpper returns a string with all charachters converted to uppercase
        CPPUNIT_ASSERT(w_tst1 == SCXCoreLib::StrToUpper(w_tst));
    }

    void testCompare()
    {
        std::wstring w_tst(L"a small test string");
        std::wstring w_tst1(L"A SMALL TEST STRING");
        std::wstring w_tst2(L"A SMALL TEST STRING");

        // Test that a string is equal when compared with it self
        CPPUNIT_ASSERT(SCXCoreLib::StrCompare(w_tst, w_tst, true) == 0);
        // Test that two strings with different casing is case insensitive equal
        CPPUNIT_ASSERT(SCXCoreLib::StrCompare(w_tst, w_tst1, true) == 0);
        // Compare that two different strings with same content are equal
        CPPUNIT_ASSERT(SCXCoreLib::StrCompare(w_tst1, w_tst2, false) == 0);
    }

    void testIsPrefix()
    {
        std::wstring w_tst(L"a small test string");
        std::wstring w_tst1(L"a small");
        std::wstring w_tst2(L"A SMALL");

        // Test that an equally cased substring is prefix
        CPPUNIT_ASSERT(SCXCoreLib::StrIsPrefix(w_tst, w_tst1));
        // Test that an differently cased substring is not prefix
        CPPUNIT_ASSERT( ! SCXCoreLib::StrIsPrefix(w_tst, w_tst2));
        // Test that an equally cased substring is case insensitve prefix
        CPPUNIT_ASSERT(SCXCoreLib::StrIsPrefix(w_tst, w_tst1, true));
        // Test that an differently cased substring is case insensitve prefix
        CPPUNIT_ASSERT(SCXCoreLib::StrIsPrefix(w_tst, w_tst2, true));
    }

    void testAppend()
    {
        std::wstring w_tst(L"a small ");
        scxulong i = 4711;
        std::wstring w_tst3(L"a small 4711");

        // Test that appending an integer to a string results in the two being concatenated
        CPPUNIT_ASSERT(w_tst3 == SCXCoreLib::StrAppend(w_tst, i));
    }

    void testTokenize()
    {
        std::vector<std::wstring> tokens;

        // Test trimming and no empty tokens
        SCXCoreLib::StrTokenize(L"a small  test\nstring", tokens);
        CPPUNIT_ASSERT(tokens.size() == 4);
        CPPUNIT_ASSERT(tokens[0] == L"a");
        CPPUNIT_ASSERT(tokens[1] == L"small");
        CPPUNIT_ASSERT(tokens[2] == L"test");
        CPPUNIT_ASSERT(tokens[3] == L"string");

        // Test trimming and empty tokens
        SCXCoreLib::StrTokenize(L"a x smally x test zstring", tokens, L"xyz", true, true);
        CPPUNIT_ASSERT(tokens.size() == 5);
        CPPUNIT_ASSERT(tokens[0] == L"a");
        CPPUNIT_ASSERT(tokens[1] == L"small");
        CPPUNIT_ASSERT(tokens[2] == L"");
        CPPUNIT_ASSERT(tokens[3] == L"test");
        CPPUNIT_ASSERT(tokens[4] == L"string");

        // Test no trimming and no empty tokens
        SCXCoreLib::StrTokenize(L"a x smallyx test zstring", tokens, L"xyz", false, false);
        CPPUNIT_ASSERT(tokens.size() == 4);
        CPPUNIT_ASSERT(tokens[0] == L"a ");
        CPPUNIT_ASSERT(tokens[1] == L" small");
        CPPUNIT_ASSERT(tokens[2] == L" test ");
        CPPUNIT_ASSERT(tokens[3] == L"string");

        // Test no trimming and empty tokens
        SCXCoreLib::StrTokenize(L"a x smallyx test zstring", tokens, L"xyz", false, true);
        CPPUNIT_ASSERT(tokens.size() == 5);
        CPPUNIT_ASSERT(tokens[0] == L"a ");
        CPPUNIT_ASSERT(tokens[1] == L" small");
        CPPUNIT_ASSERT(tokens[2] == L"");
        CPPUNIT_ASSERT(tokens[3] == L" test ");
        CPPUNIT_ASSERT(tokens[4] == L"string");

        // Test that a string without separators are returned as a single token
        SCXCoreLib::StrTokenize(L"abc", tokens, L" ");
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"abc");

        // Test that an empty string results in a single empty token or no token at all (depending on parameters)
        SCXCoreLib::StrTokenize(L"", tokens, L" ", true, true);
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"");
        SCXCoreLib::StrTokenize(L"", tokens, L" ", true, false);
        CPPUNIT_ASSERT(tokens.size() == 0);

        // Test that an empty separator results in a single token (as if no separator found)
        SCXCoreLib::StrTokenize(L"a b c", tokens, L"");
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"a b c");

        // Test separators beginning/end of string
        SCXCoreLib::StrTokenize(L" abc ", tokens, L" ", false, true);
        CPPUNIT_ASSERT(tokens.size() == 3);
        CPPUNIT_ASSERT(tokens[0] == L"");
        CPPUNIT_ASSERT(tokens[1] == L"abc");
        CPPUNIT_ASSERT(tokens[2] == L"");
        SCXCoreLib::StrTokenize(L" abc ", tokens, L" ", true, false);
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"abc");
        SCXCoreLib::StrTokenize(L" abc ", tokens, L" ", false, false);
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"abc");

        // test only separators result in (no) tokens depending on parameter
        SCXCoreLib::StrTokenize(L";;", tokens, L";");
        CPPUNIT_ASSERT(tokens.size() == 0);
        SCXCoreLib::StrTokenize(L";;", tokens, L";", true, true);
        CPPUNIT_ASSERT(tokens.size() == 3);
        CPPUNIT_ASSERT(tokens[0] == L"");
        CPPUNIT_ASSERT(tokens[1] == L"");
        CPPUNIT_ASSERT(tokens[2] == L"");
    }

    void testTokenizeStr()
    {
        std::vector<std::wstring> tokens;

        // Test trimming and no empty tokens
        SCXCoreLib::StrTokenizeStr(L"a small small testsmallstring", tokens, L"small");
        CPPUNIT_ASSERT(tokens.size() == 3);
        CPPUNIT_ASSERT(tokens[0] == L"a");
        CPPUNIT_ASSERT(tokens[1] == L"test");
        CPPUNIT_ASSERT(tokens[2] == L"string");

        // Test trimming and empty tokens
        SCXCoreLib::StrTokenizeStr(L"a small small testsmallstring", tokens, L"small", true, true);
        CPPUNIT_ASSERT(tokens.size() == 4);
        CPPUNIT_ASSERT(tokens[0] == L"a");
        CPPUNIT_ASSERT(tokens[1] == L"");
        CPPUNIT_ASSERT(tokens[2] == L"test");
        CPPUNIT_ASSERT(tokens[3] == L"string");

        // Test no trimming and no empty tokens
        SCXCoreLib::StrTokenizeStr(L"a small small testsmallsmallstring", tokens, L"small", false, false);
        CPPUNIT_ASSERT(tokens.size() == 4);
        CPPUNIT_ASSERT(tokens[0] == L"a ");
        CPPUNIT_ASSERT(tokens[1] == L" ");
        CPPUNIT_ASSERT(tokens[2] == L" test");
        CPPUNIT_ASSERT(tokens[3] == L"string");

        // Test no trimming and empty tokens
        SCXCoreLib::StrTokenizeStr(L"a small small testsmallsmallstring", tokens, L"small", false, true);
        CPPUNIT_ASSERT(tokens.size() == 5);
        CPPUNIT_ASSERT(tokens[0] == L"a ");
        CPPUNIT_ASSERT(tokens[1] == L" ");
        CPPUNIT_ASSERT(tokens[2] == L" test");
        CPPUNIT_ASSERT(tokens[3] == L"");
        CPPUNIT_ASSERT(tokens[4] == L"string");

        // Test that a string without separators are returned as a single token
        SCXCoreLib::StrTokenizeStr(L"abc", tokens, L"cab");
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"abc");

        // Test that an empty string results in a single empty token or no token at all (depending on parameters)
        SCXCoreLib::StrTokenizeStr(L"", tokens, L"abc", true, true);
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"");
        SCXCoreLib::StrTokenizeStr(L"", tokens, L"abc", true, false);
        CPPUNIT_ASSERT(tokens.size() == 0);

        // Test that an empty separator results in a single token (as if no separator found)
        SCXCoreLib::StrTokenizeStr(L"a b c", tokens, L"");
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"a b c");

        // Test separators beginning/end of string
        SCXCoreLib::StrTokenizeStr(L"cababccab", tokens, L"cab", false, true);
        CPPUNIT_ASSERT(tokens.size() == 3);
        CPPUNIT_ASSERT(tokens[0] == L"");
        CPPUNIT_ASSERT(tokens[1] == L"abc");
        CPPUNIT_ASSERT(tokens[2] == L"");
        SCXCoreLib::StrTokenizeStr(L"cababccab", tokens, L"cab", false, false);
        CPPUNIT_ASSERT(tokens.size() == 1);
        CPPUNIT_ASSERT(tokens[0] == L"abc");

        // test only separators result in (no) tokens depending on parameter
        SCXCoreLib::StrTokenizeStr(L"cabcab", tokens, L"cab");
        CPPUNIT_ASSERT(tokens.size() == 0);
        SCXCoreLib::StrTokenizeStr(L"cabcab", tokens, L"cab", true, true);
        CPPUNIT_ASSERT(tokens.size() == 3);
        CPPUNIT_ASSERT(tokens[0] == L"");
        CPPUNIT_ASSERT(tokens[1] == L"");
        CPPUNIT_ASSERT(tokens[2] == L"");
    }

    void testToIntETC()
    {
      try {
        std::wstring w_tst(L"4711");
        std::wstring w_tst2(L" 4711 ");
        std::wstring w_tst3(L"-42");
        std::wstring w_tst4(L" -42 ");

        // StrToUInt
        CPPUNIT_ASSERT(4711 == SCXCoreLib::StrToUInt(w_tst));
        CPPUNIT_ASSERT(4711 == SCXCoreLib::StrToUInt(w_tst2));
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToUInt(L"Not a number"), SCXCoreLib::SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToUInt(w_tst3), SCXCoreLib::SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToUInt(w_tst4), SCXCoreLib::SCXNotSupportedException);
        // Note: We can't detect overflow. The following will succeed.
        //CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToUInt(L"999999999999999999999999999999"),
        //                   SCXCoreLib::SCXNotSupportedException);

        // StrToDouble
        CPPUNIT_ASSERT(SCXCoreLib::Equal(4711, SCXCoreLib::StrToDouble(w_tst), 0));
        CPPUNIT_ASSERT(SCXCoreLib::Equal(4711, SCXCoreLib::StrToDouble(w_tst2), 0));
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToDouble(L"Not a number"), SCXCoreLib::SCXNotSupportedException);
        CPPUNIT_ASSERT(SCXCoreLib::Equal(-42, SCXCoreLib::StrToDouble(w_tst3), 0));
        CPPUNIT_ASSERT(SCXCoreLib::Equal(-42, SCXCoreLib::StrToDouble(w_tst4), 0));

        // StrToLong
        CPPUNIT_ASSERT(4711 == SCXCoreLib::StrToLong(w_tst));
        CPPUNIT_ASSERT(4711 == SCXCoreLib::StrToLong(w_tst2));
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToLong(L"Not a number"), SCXCoreLib::SCXNotSupportedException);
        CPPUNIT_ASSERT(-42 == SCXCoreLib::StrToLong(w_tst3));
        CPPUNIT_ASSERT(-42 == SCXCoreLib::StrToLong(w_tst4));

        // StrToULong
        CPPUNIT_ASSERT(4711 == SCXCoreLib::StrToULong(w_tst));
        CPPUNIT_ASSERT(4711 == SCXCoreLib::StrToULong(w_tst2));
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToULong(L"Not a number"), SCXCoreLib::SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToULong(w_tst3), SCXCoreLib::SCXNotSupportedException);
        CPPUNIT_ASSERT_THROW(SCXCoreLib::StrToULong(w_tst4), SCXCoreLib::SCXNotSupportedException);
      } catch (SCXException& e) {
        std::wcout << L"\nException in testToIntETC: " << e.What()
                   << L" @ " << e.Where() << std::endl;
        CPPUNIT_ASSERT(!"Exception");
      }

    }

    void testFrom()
    {
        CPPUNIT_ASSERT(L"42" == SCXCoreLib::StrFrom(static_cast<unsigned int>(42)));
        CPPUNIT_ASSERT(L"42" == SCXCoreLib::StrFrom(static_cast<scxulong>(42)));
        CPPUNIT_ASSERT(L"-4711" == SCXCoreLib::StrFrom(static_cast<scxlong>(-4711)));
        CPPUNIT_ASSERT(L"-47.11" == SCXCoreLib::StrFrom(static_cast<double>(-47.11)));
        CPPUNIT_ASSERT(L"42" == SCXCoreLib::StrFrom(static_cast<double>(42)));
    }

    void testUTF8Conversion()
    {
        bool solarisAndClocale = false;

#if defined(sun)
        solarisAndClocale = (SCXLocaleContext::GetCtypeName() == L"C");
#endif

        CPPUNIT_ASSERT(
            SCXCoreLib::StrFromUTF8(
            SCXCoreLib::StrToUTF8(L"")) == L"");
        CPPUNIT_ASSERT(
            SCXCoreLib::StrFromUTF8(
            SCXCoreLib::StrToUTF8(L"Test string 1 - Simple")) == L"Test string 1 - Simple");
        if (!solarisAndClocale)
        {
            CPPUNIT_ASSERT(
                SCXCoreLib::StrFromUTF8(
                    SCXCoreLib::StrToUTF8(L"Test string 2 - With åäöÅÄÖ")) == L"Test string 2 - With åäöÅÄÖ");
            CPPUNIT_ASSERT(
                SCXCoreLib::StrFromUTF8(
                    SCXCoreLib::StrToUTF8(L"Hello world, Καλημ%Gα½³%@ρα κ%Gα½Ή%@σμε, コンニチハ")) == L"Hello world, Καλημ%Gα½³%@ρα κ%Gα½Ή%@σμε, コンニチハ");
        }
    }

    void testUTF8ConversionFails()
    {
        std::string utf8("AB");
        // Create an invalid UTF8 sequence:
        utf8[0] = (char)0xC3;
        utf8[1] = (char)0x00;
        SCXUNIT_ASSERT_THROWN_EXCEPTION(SCXCoreLib::StrFromUTF8(utf8), SCXCoreLib::SCXStringConversionException, L"Multibyte");
    }

    void testMergeTokens()
    {
        std::wstring s = L"this 'is' \"a string\" with (lot's  (sic!) of) ' variants ' 'for you'";
        std::vector<std::wstring> tokens;
        std::map<std::wstring,std::wstring> m;

        // Set up the merge identifier pairs
        m[L"\""] = L"\"";
        m[L"'"] = L"'";
        m[L"("] = L")";

        SCXCoreLib::StrTokenize(s, tokens);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(13), tokens.size());

        CPPUNIT_ASSERT(SCXCoreLib::StrMergeTokens(tokens, m, L" "));

        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(8), tokens.size());
        CPPUNIT_ASSERT(L"this" == tokens[0]);
        CPPUNIT_ASSERT(L"is" == tokens[1]);
        CPPUNIT_ASSERT(L"a string" == tokens[2]);
        CPPUNIT_ASSERT(L"with" == tokens[3]);
        CPPUNIT_ASSERT(L"lot's (sic!" == tokens[4]);
        CPPUNIT_ASSERT(L"of)" == tokens[5]);
        CPPUNIT_ASSERT(L"variants" == tokens[6]);
        CPPUNIT_ASSERT(L"for you" == tokens[7]);

        // test mismatch
        tokens.clear();
        SCXCoreLib::StrTokenize(L"a (b c", tokens);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), tokens.size());

        CPPUNIT_ASSERT( ! SCXCoreLib::StrMergeTokens(tokens, m, L" "));
    }

    void testTokenizeWithDelimiters()
    {
        std::vector<std::wstring> tokens;
        SCXCoreLib::StrTokenize(L"a=b  c", tokens, L" =", false, false, true);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(6), tokens.size());
        CPPUNIT_ASSERT(L"a" == tokens[0]);
        CPPUNIT_ASSERT(L"=" == tokens[1]);
        CPPUNIT_ASSERT(L"b" == tokens[2]);
        CPPUNIT_ASSERT(L" " == tokens[3]);
        CPPUNIT_ASSERT(L" " == tokens[4]);
        CPPUNIT_ASSERT(L"c" == tokens[5]);
    }

    void testFromMultibyte()
    {
        CPPUNIT_ASSERT(SCXCoreLib::StrFromUTF8("abc") == L"abc");
    }

    void testFromMultibyteNoThrow()
    {
        CPPUNIT_ASSERT(SCXCoreLib::StrFromMultibyteNoThrow("abc") == L"abc");

        // This test is sensitive to locale (don't run unless it's UTF-8)
        //
        // If the codepage isn't UTF-8, then method mbsrtowcs() doesn't throw,
        // but doesn't appear to generate something that std::wcout is able to
        // print, it seems.  This is impossible for us to detect, though, since
        // no error is returned and no exception is thrown.

        const char *localeStr = nl_langinfo(CODESET);
        if ( NULL != localeStr && strcasecmp(localeStr, "UTF8") != 0 && strcasecmp(localeStr, "UTF-8") != 0 )
        {
            SCXUNIT_WARNING( StrAppend(L"Test SCXStringAid_Test::testFromMultibyteNoThrow requires UTF-8 codepage to run properly, existing codpage: ", localeStr) );
            return;
        }

        // It's difficult to get this string in place via C++, so use a hammer
        // We have to replace the 'X' (at end of string) with hex C0, octal 300

        char badString[16];
        strncpy( badString, "alxapfs34X", sizeof(badString) );
        CPPUNIT_ASSERT( 'X' == badString[9] );
        badString[9] = static_cast<char> (0xC0);
        CPPUNIT_ASSERT(SCXCoreLib::StrFromMultibyteNoThrow(std::string(badString)) == L"alxapfs34?");
    }

    void testToUTF8()
    {
        CPPUNIT_ASSERT(SCXCoreLib::StrToUTF8(L"abc") == "abc");
    }


    class MyException : public std::exception {
    public:
        virtual const char *what() const throw()
        {
            return "problem";
        }
    };

    void testDumpStringException()
    {
        MyException e;
        std::wstring text(SCXCoreLib::DumpString(e));
        CPPUNIT_ASSERT(text.find(L"MyException") != std::wstring::npos &&
                       text.find(L"problem") != std::wstring::npos);
    }

    void testUtfUpDownCase()
    {
        unsigned int c;
        unsigned int uc1;
        unsigned int uc2;
        unsigned int Errors = 0;

        // upper case test
        for (size_t i = 0; i < sizeof UpcaseTestTable / sizeof (TestInfo); i++)
        {
            c = UpcaseTestTable[i].Char;
            uc1 = SCXCoreLib::UtfToUpper(c);
            if (uc1 != c)
            {
                uc2 = SCXCoreLib::UtfToLower(uc1);
            }
            else
            {
                uc2 = c;
            }
            if (uc2 != c || uc1 != UpcaseTestTable[i].Converted)
            {
                Errors++;
            }
        }
        CPPUNIT_ASSERT(Errors == 0);

        // lower case test
        Errors = 0;
        for (size_t i = 0; i < sizeof DowncaseTestTable / sizeof (TestInfo); i++)
        {
            c = DowncaseTestTable[i].Char;
            uc1 = SCXCoreLib::UtfToLower(c);
            if (uc1 != c)
            {
                uc2 = SCXCoreLib::UtfToUpper(uc1);
            }
            else
            {
                uc2 = c;
            }
            if (uc2 != c || uc1 != DowncaseTestTable[i].Converted)
            {
                Errors++;
            }
        }
        CPPUNIT_ASSERT(Errors == 0);

        return;
    }

    /** Here we test that vsnprintf() conforms to the UNIX03 specifiction as
        opposed to the conflicting definition from UNIX95. The core lib does not
        depend on this, but OpenWSMan does. OpenWSMan has no unit tests, so this
        seems like a resonable place to put the test. See also WI5724.
    */
    void testUNIX03()
    {
        CPPUNIT_ASSERT_MESSAGE("vsnprintf() does not conform to UNIX03. ",
                               u03helper("123%s", "456") == 6);
    }

    int u03helper(const char *fmt, ...)
    {
        va_list ap2;
        int size = 0;

        va_start(ap2, fmt);

        /* Should compute the size requirement under UNIX03,
           but returns 0 to indicate failure under UNIX95 */
        size = vsnprintf(NULL, 0, fmt, ap2);

        va_end(ap2);
        return size;
    }

    // Debugging routine
    void DumpVector(const std::vector<std::wstring>& vector)
    {
        std::wcout << std::endl;
        std::wcout << L"  Vector size: " << vector.size() << std::endl;
        for (size_t i = 0; i < vector.size(); ++i)
        {
            std::wcout << L"   Element " << i << L": \"" << vector[i] << L"\"" << std::endl;
        }
    }

    void test_Empty_String()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L"", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), tokens.size() );
    }

    void test_Nonquoted_String_OnlySpaces()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L"    ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), tokens.size() );
    }

    void test_Nonquoted_String_EmptyTokensOnly()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L"  ,  ,  ,   ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(0), tokens.size() );
    }

    void test_Nonquoted_String()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" A, B , ,  C  ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(3), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A"), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"B"), tokens[1] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"C"), tokens[2] );
    }

    void test_Nonquoted_String_ReturnEmptyTokens()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" A, B , ,  C  ", tokens, L",", true);
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(4), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A"), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"B"), tokens[1] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L""), tokens[2] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"C"), tokens[3] );
    }

    void test_QuotedQuotes_Ignored()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" A, B \\\" , ,  C  ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(3), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A"), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"B \\\""), tokens[1] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"C"), tokens[2] );
    }

    void test_Quoted_String()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" \"A, B \", ,  C  ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(2), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A, B "), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"C"), tokens[1] );
    }

    void test_Quoted_String_Double()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" \"A, B ', C, D' \", ,  E  ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(2), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A, B ', C, D' "), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"E"), tokens[1] );
    }

    void test_Quoted_SingleQuote()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" 'A, B ', ,  C  ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(2), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A, B "), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"C"), tokens[1] );
    }

    void test_Quoted_SingleQuote_ReturnEmptyTokens()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" 'A, B ', ,  C  ", tokens, L",", true);
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(3), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A, B "), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L""), tokens[1] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"C"), tokens[2] );
    }

    void test_Quoted_SingleQuote_QuotedSpaces()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" ' ', ,  A  ", tokens, L",", true);
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(3), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L" "), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L""), tokens[1] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A"), tokens[2] );
    }

    void test_Quoted_String_QuotedQuotes()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L"    \"A, B \\\" CD \\\" \"  , E ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(2), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A, B \\\" CD \\\" "), tokens[0] );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"E"), tokens[1] );
    }

    void test_Quoted_SingleQuotedElement()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" \"A, B \"  ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(1), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"A, B "), tokens[0] );
    }

    void test_Quoted_UnterminatedQuote()
    {
        std::vector<std::wstring> tokens;
        StrTokenizeQuoted(L" \"A, B ", tokens, L",");
        CPPUNIT_ASSERT_EQUAL( static_cast<size_t>(1), tokens.size() );
        CPPUNIT_ASSERT_EQUAL( std::wstring(L"\"A, B"), tokens[0] );
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXStringAid_Test );
