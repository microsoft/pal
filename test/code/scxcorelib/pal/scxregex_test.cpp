/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Tests the SCXRegex class.

    \date        2008-08-20 14:24:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxregex.h>
#include <testutils/scxunit.h>

class SCXRegexTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXRegexTest );
    CPPUNIT_TEST( TestIsMatch );
    CPPUNIT_TEST( SpaceMatchesSpace );
    CPPUNIT_TEST( SpaceDoesNotMatchNonspace );
    CPPUNIT_TEST( TestReturnMatch );
    CPPUNIT_TEST( TestReturnMatchPartialAndSub );
    CPPUNIT_TEST( TestReturnMatchEmptyString );
    CPPUNIT_TEST( TestReturnMatchZeroLarge );
    CPPUNIT_TEST( TestReturnMatchStop );
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXRegex *patternPtr;

public:
    void setUp(void)
    {
        patternPtr = NULL;
    }

    void tearDown(void)
    {
        if (patternPtr != NULL)
        {
            delete patternPtr;
            patternPtr = NULL;
        }
    }

    void TestIsMatch(void)
    {
        SCXCoreLib::SCXRegex r1(L"AA");
        CPPUNIT_ASSERT(   r1.IsMatch(L"AA"));
        CPPUNIT_ASSERT( ! r1.IsMatch(L"ABA"));
 
        SCXCoreLib::SCXRegex r2(L"A+");
        CPPUNIT_ASSERT( ! r2.IsMatch(L""));
        CPPUNIT_ASSERT(   r2.IsMatch(L"A"));
        CPPUNIT_ASSERT(   r2.IsMatch(L"AAA"));

        SCXCoreLib::SCXRegex r3(L"A*");
        CPPUNIT_ASSERT(   r3.IsMatch(L""));
        CPPUNIT_ASSERT(   r3.IsMatch(L"A"));
        CPPUNIT_ASSERT(   r3.IsMatch(L"AAA"));
        CPPUNIT_ASSERT(   r3.IsMatch(L"ABA"));
        CPPUNIT_ASSERT(   r3.IsMatch(L"BBB"));

        SCXCoreLib::SCXRegex r4(L"A.*A");
        CPPUNIT_ASSERT( ! r4.IsMatch(L"A"));
        CPPUNIT_ASSERT(   r4.IsMatch(L"AA"));
        CPPUNIT_ASSERT(   r4.IsMatch(L"ABA"));
        CPPUNIT_ASSERT(   r4.IsMatch(L"AAA"));

        CPPUNIT_ASSERT_THROW(SCXCoreLib::SCXRegex r5(L"*"), SCXCoreLib::SCXInvalidRegexException);
    }

    void SpaceMatchesSpace()
    {
        SCXCoreLib::SCXRegex r(L"[[:space:]]");
        CPPUNIT_ASSERT(r.IsMatch(L"a a"));
    }

    void SpaceDoesNotMatchNonspace()
    {
        SCXCoreLib::SCXRegex r(L"[[:space:]]");
        CPPUNIT_ASSERT(!r.IsMatch(L"aaa"));
    }

    void TestReturnMatch(void)
    {
        bool ret = true;
        std::vector<std::wstring> retMtch;

        //Compile it:
        try
        {
             patternPtr = new SCXCoreLib::SCXRegex(L"^Units =[^=]*=[ ]*([0-9]+)");
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            std::wcout << L"Exception caught during compile of regex2: " << e.What() << std::endl;
            ret = false;
        }

        CPPUNIT_ASSERT_MESSAGE("SCXRegex Constructor failed!",  ret );

        //Success case
        try
        {
           ret = patternPtr->ReturnMatch(L"Units = sectors of 1 * 512 = 512 bytes", retMtch, 0);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            std::wcout << L"Exception caught during First match of regex: " << e.What() << std::endl;
            ret = false;
        }

        CPPUNIT_ASSERT_MESSAGE("SCXRegex: Third Match failed!",  ret );
        CPPUNIT_ASSERT( retMtch[0] == L"Units = sectors of 1 * 512 = 512" );  
        CPPUNIT_ASSERT( retMtch[1] == L"512" );  
        retMtch.clear();


        //Failure case
        try
        {
           ret = patternPtr->ReturnMatch(L"Finders Keepers Blah Blah",retMtch, 0);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            std::wcout << L"Exception caught during compile or match of regex: " << e.What() << std::endl;
            ret = true;
        }

        CPPUNIT_ASSERT_MESSAGE("SCXRegex Second Match should not have matched!",  !ret );

        retMtch.clear();
        delete patternPtr;
        patternPtr = NULL;
        ret = true;


        //Testing part II //////////////////////////

        //Compile it:
        try
        {
           patternPtr = new SCXCoreLib::SCXRegex(L"bootpath:[ ]+[^ ]*(scsi|ide){1}[^:]*:([a-z]?)");
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            std::wcout << L"Exception caught during compile of regex: " << e.What() << std::endl;
            ret = false;
        }

        CPPUNIT_ASSERT_MESSAGE("SCXRegex Constructor failed!",  ret );

        //Success case
        try
        {
           ret = patternPtr->ReturnMatch(L"bootpath:  '/pci@1c,600000/scsi@2/disk@0,0:a'",retMtch, 0);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            std::wcout << L"Exception caught during Solaris match of regex: " << e.What() << std::endl;
            ret = false;
        }

        CPPUNIT_ASSERT_MESSAGE("SCXRegex: Third Match failed!",  ret );
        CPPUNIT_ASSERT( retMtch[1] == L"scsi" );  
        CPPUNIT_ASSERT( retMtch[2] == L"a" );  
        retMtch.clear();


        //Failure case
        try
        {
            ret = patternPtr->ReturnMatch(L"Finders Keepers Blah Blah",retMtch, 0);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            std::wcout << L"Exception caught during compile or match of regex: " << e.What() << std::endl;
            ret = true;
        }

        CPPUNIT_ASSERT_MESSAGE("SCXRegex Fourth Match should not have matched!",  !ret );

        retMtch.clear();

        delete patternPtr;
        patternPtr = NULL;
    }

    void TestReturnMatchPartialAndSub(void)
    {
        std::vector<SCXCoreLib::SCXRegExMatch> match;
        SCXCoreLib::SCXRegex re(L"A(B(CD))|E(F(GH))");

        CPPUNIT_ASSERT( re.ReturnMatch(L"ABCD", match, 5, 0) );
        CPPUNIT_ASSERT_EQUAL(5u, match.size());
        CPPUNIT_ASSERT_EQUAL(true, match[0].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"ABCD", match[0].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[1].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"BCD", match[1].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[2].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"CD", match[2].matchString);
        CPPUNIT_ASSERT_EQUAL(false, match[3].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[3].matchString);
        CPPUNIT_ASSERT_EQUAL(false, match[4].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[4].matchString);

        CPPUNIT_ASSERT( re.ReturnMatch(L"EFGH", match, 5, 0) );
        CPPUNIT_ASSERT_EQUAL(5u, match.size());
        CPPUNIT_ASSERT_EQUAL(true, match[0].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"EFGH", match[0].matchString);
        CPPUNIT_ASSERT_EQUAL(false, match[1].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[1].matchString);
        CPPUNIT_ASSERT_EQUAL(false, match[2].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[2].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[3].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"FGH", match[3].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[4].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"GH", match[4].matchString);
    }

    void TestReturnMatchEmptyString(void)
    {
        std::vector<SCXCoreLib::SCXRegExMatch> match;
        SCXCoreLib::SCXRegex re(L"AB()CD|EF()GH");

        CPPUNIT_ASSERT( re.ReturnMatch(L"ABCD", match, 3, 0) );
        CPPUNIT_ASSERT_EQUAL(3u, match.size());
        CPPUNIT_ASSERT_EQUAL(true, match[0].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"ABCD", match[0].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[1].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[1].matchString);
        CPPUNIT_ASSERT_EQUAL(false, match[2].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[2].matchString);

        CPPUNIT_ASSERT( re.ReturnMatch(L"EFGH", match, 3, 0) );
        CPPUNIT_ASSERT_EQUAL(3u, match.size());
        CPPUNIT_ASSERT_EQUAL(true, match[0].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"EFGH", match[0].matchString);
        CPPUNIT_ASSERT_EQUAL(false, match[1].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[1].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[2].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"", match[2].matchString);
    }

    void TestReturnMatchZeroLarge(void)
    {
        std::vector<SCXCoreLib::SCXRegExMatch> match;
        SCXCoreLib::SCXRegex re(L"(AB)CD");

        // Zero return size requested although there is a match.
        CPPUNIT_ASSERT( re.ReturnMatch(L"ABCD", match, 0, 0) );
        CPPUNIT_ASSERT_EQUAL(0u, match.size());

        // Returned vector of matches is larger than actual number of possible matches.
        // Remainder of returned vectors is set to "" and false.
        CPPUNIT_ASSERT( re.ReturnMatch(L"ABCD", match, 100, 0) );
        CPPUNIT_ASSERT_EQUAL(100u, match.size());
        CPPUNIT_ASSERT_EQUAL(true, match[0].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"ABCD", match[0].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[1].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"AB", match[1].matchString);
        size_t i;
        for(i = 2; i < match.size(); i++)
        {
            std::string msg = SCXCoreLib::StrToUTF8(L"For index [" + SCXCoreLib::StrFrom(i) + L"]");
            CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, false, match[i].matchFound);
            CPPUNIT_ASSERT_EQUAL_MESSAGE( msg, L"", match[i].matchString);
        }
    }

    void TestReturnMatchStop(void)
    {
        std::vector<SCXCoreLib::SCXRegExMatch> match;
        SCXCoreLib::SCXRegex re(L"(AB)((CD)|(EF))");

         // Stop returning matches after first nonmatch encountered.
        CPPUNIT_ASSERT( re.ReturnMatch(L"ABEF", match, 5, 0, true) );
        CPPUNIT_ASSERT_EQUAL(3u, match.size());
        CPPUNIT_ASSERT_EQUAL(true, match[0].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"ABEF", match[0].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[1].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"AB", match[1].matchString);
        CPPUNIT_ASSERT_EQUAL(true, match[2].matchFound);
        CPPUNIT_ASSERT_EQUAL(L"EF", match[2].matchString);
        // Last two entries are cut off.
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXRegexTest );
