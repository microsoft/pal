#include <Unicode.h>
#include <iostream>
#include <algorithm>
#include <map>

#include <testutils/scxunit.h>

using namespace SCX::Util;

struct CodePointRange
{
    Utf16Char Min;
    Utf16Char Max;

    void GetTestData(int testId, Utf16Char& testData, bool& inSequence, bool& isSurrogate)
    {
        switch (testId)
        {
        case 0:
            testData = 0;
            inSequence = false;
            break;
        case 1:
            testData = static_cast<Utf16Char>(Min - 1);
            inSequence = false;
            break;
        case 2:
            testData = Min;
            inSequence = true;
            break;
        case 3:
            testData = static_cast<Utf16Char>(Min + 1);
            inSequence = true;
            break;
        case 4:
            testData = static_cast<Utf16Char>(Max - 1);
            inSequence = true;
            break;
        case 5:
            testData = Max;
            inSequence = true;
            break;
        case 6:
            testData = static_cast<Utf16Char>(Max + 1);
            inSequence = false;
            break;
        }

        isSurrogate = ((testData >= 0xD800) && (testData <= 0xDFFF));
     }
};

CodePointRange SurrogateHighCodeUnit = {0xD800, 0xDBFF};
CodePointRange SurrogateLowCodeUnit =  {0xDC00, 0xDFFF};

class Utf16StringTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE (Utf16StringTest);
        // CPPUNIT_TEST (TestCharSize);
        CPPUNIT_TEST (EmptyCtorTest);
        CPPUNIT_TEST (CharArrayCtor_SimpleTest);
        CPPUNIT_TEST (Ctor_HandleBOMTest);
        CPPUNIT_TEST (Ctor_NonSurrogateCodePointsTest);
        CPPUNIT_TEST (Ctor_InvalidSurrogateCodePointsTest);
        CPPUNIT_TEST (Ctor_ValidSurrogateCodePointsTest);
        CPPUNIT_TEST (IteratorAndCodePointTest);
        CPPUNIT_TEST (SetCodePointTest);
        CPPUNIT_TEST (ReadFromStreamTest);
        CPPUNIT_TEST (ConvertFromUtf8Test);
    CPPUNIT_TEST_SUITE_END ();
    
    
    public:

        void TestCharSize()
        {
            char arr1[] = {0x65, 0x66, 0x67, 0x00};
            for (size_t i = 0; i < sizeof(arr1); i++)
            {
                printf(" 0x%x", arr1[i]);
            }

            Utf16Char arr2[] = {0x0065, 0x0066, 0x0067, 0x0};

            std::basic_string<char> str1(arr1);
            std::basic_string<Utf16Char> str2(arr2);
           
            std::basic_string<Utf16Char> str3;
            str3 += 0x0065;
            str3 += 0x0066;
            str3 += 0x0067;
            str3 += Utf16Char(0x0);
           
            std::string str4("ABC");

            printf("\nstr1 = %lu\n", static_cast<unsigned long>(str1.size()));
            printf("\nstr2 = %lu\n", static_cast<unsigned long>(str2.size()));
            printf("\nstr3 = %lu\n", static_cast<unsigned long>(str3.size()));
            printf("\nstr4 = %lu\n", static_cast<unsigned long>(str4.size()));

            for (size_t i = 0; i < 4; i++)
            {
                printf(" 0x%x", str1[i]);
            }

            std::string s = "ABCDXY";
            s[4] = 'Z';
            s[5] = 'Q';
            for (size_t i = 0; i < s.size() ; i++)
            {
                printf(" 0x%x", s[i]);
            }

            printf("\ns %lu = %s :\n", static_cast<unsigned long>(s.size()), s.c_str());
            printf("size s = %lu\n", static_cast<unsigned long>(s.length()));

            std::string s2;
            char a1[] = {'A', 'B', 'C'};
            std::copy(a1, a1+2, s2.begin());
            s2 += 'D';
            printf("s2 %lu = %s :\n", static_cast<unsigned long>(s2.size()), s2.c_str());

        }

        void EmptyCtorTest()
        {
            
            Utf16String str;
            
            CPPUNIT_ASSERT(str.Empty());
            CPPUNIT_ASSERT(str.Size() == 0);
            
            std::basic_string<Utf16Char> str3;
            Utf16String str2(str3);
            CPPUNIT_ASSERT(str2.Empty());
            CPPUNIT_ASSERT(str2.Size() == 0);
        }

        void CharArrayCtor_SimpleTest()
        {
            Utf16Char arr[] = {0x0065, 0x0066, 0x0067, 0x0};
            Utf16String str(arr);
            CPPUNIT_ASSERT(!str.Empty());
            CPPUNIT_ASSERT_EQUAL(3, (int)str.Size());
        }

        void Ctor_HandleBOMTest()
        {
            Utf16Char arr[] = {0xFEFF, 0x0065, 0x0066, 0};
            Utf16String str(arr);
            CPPUNIT_ASSERT(!str.Empty());
            CPPUNIT_ASSERT_EQUAL(2, (int)str.Size());

            Utf16Char arr1[] = {0xFE, 0x0065, 0x0066, 0x0};
            Utf16String str1(arr1);
            CPPUNIT_ASSERT(!str1.Empty());
            CPPUNIT_ASSERT_EQUAL(3, (int)str1.Size());
        
        }

        void Ctor_NonSurrogateCodePointsTest()
        {
            {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0x0};
                Utf16String str(arr);
                CPPUNIT_ASSERT(!str.Empty());
                CPPUNIT_ASSERT_EQUAL(3, (int)str.Size());
            }

            {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xD7FF, 0x0};
                Utf16String str(arr);
                CPPUNIT_ASSERT(!str.Empty());
                CPPUNIT_ASSERT_EQUAL(4, (int)str.Size());
            }

            {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xE000, 0x0};
                Utf16String str(arr);
                CPPUNIT_ASSERT(!str.Empty());
                CPPUNIT_ASSERT_EQUAL(4, (int)str.Size());
            }
        }

        void Ctor_InvalidSurrogateCodePointsTest()
        {
            // Higher Order without lower Order
            {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xD800, 0x4E8C, 0x0};
                CPPUNIT_ASSERT_THROW(Utf16String str(arr), InvalidCodeUnitException);
            }

            
            {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xDBFF, 0x0};
                CPPUNIT_ASSERT_THROW(Utf16String str(arr), InvalidCodeUnitException);
            }

            // Lower Order without Higher Order
            {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xDC00, 0x0};
                CPPUNIT_ASSERT_THROW(Utf16String str(arr), InvalidCodeUnitException);
            }
            
            {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xDFFF, 0x0};
                CPPUNIT_ASSERT_THROW(Utf16String str(arr), InvalidCodeUnitException);
            }
            
            // Reverse order 
             {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xDFFF,0xD800, 0x0};
                CPPUNIT_ASSERT_THROW(Utf16String str(arr), InvalidCodeUnitException);
            }

             // Last character is surrogate high
             {
                Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xDFFF,0xD800, 0x0};
                CPPUNIT_ASSERT_THROW(Utf16String str(arr), InvalidCodeUnitException);
             }
            
        }

        std::vector<int> ConvertToBase(int decimalNum, int base)
        {
            std::vector<int> baseNum;
            std::vector<int>::iterator it;
            it = baseNum.begin();
            do
            {
                baseNum.insert(it, decimalNum % base);
                it = baseNum.begin();
                decimalNum = decimalNum / base;
            }while (decimalNum != 0);
            return baseNum;
        }

        void Ctor_ValidSurrogateCodePointsTest()
        {
            const int numberOfCombinations = 7;
            int numTest = numberOfCombinations * numberOfCombinations;

            std::basic_string<Utf16Char> input;
            bool expectedResult = true;
            size_t expectedSize = 3;
            for (int i = 0; i < numTest; i++)
            {
                input.clear();
                expectedResult = true;
                expectedSize = 3;
                Utf16Char testValue = 0;
                
                bool inSequence1 = false;
                bool inSequence2 = false;
                bool isSurrogate1 = false;
                bool isSurrogate2 = false;

                input += 0xFEFF;
                input += 0x004D;
                input += 0x0430;

                std::vector<int> TestSequence = ConvertToBase(i, numberOfCombinations);
               
                // Add high Surrogate
                SurrogateHighCodeUnit.GetTestData(TestSequence[0], testValue, inSequence1, isSurrogate1);
                if (testValue != 0)
                {
                    input += testValue;
                    expectedSize++;
                }
                
                 // Add low Surrogate
                (SurrogateLowCodeUnit.GetTestData(TestSequence[1], testValue, inSequence2, isSurrogate2));
                if (testValue != 0)
                {
                    input += testValue;
                    expectedSize++;
                }
               
                if ((!isSurrogate1) && (!isSurrogate2))
                {
                    expectedResult = true;
                }
                else if (inSequence1 & inSequence2 & isSurrogate1 & isSurrogate2)
                {
                    expectedResult = true;
                    expectedSize = 4;
                }
                else
                {
                    expectedResult = false;
                }
                

                input += 0x4E8C;
                
                /*
                printf("%ld :", i);
                for (size_t k = 0; k < input.size(); k++)
                {
                    printf(" 0x%x", input[k]);
                }
                printf(" : %d : (%d %d) (%d %d)\n", expectedResult, inSequence1, isSurrogate1, inSequence2, isSurrogate2);
                */

                if (expectedResult)
                {
                    try
                    {
                        Utf16String str(input);
                        CPPUNIT_ASSERT_EQUAL(expectedSize, str.CodePoints());
                    }
                    catch (InvalidCodeUnitException& e)
                    {
                        printf("\nEx = %s\n", SCXCoreLib::StrToUTF8(e.What()).c_str());
                        CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(e.What()).c_str());
                    }
                    
                }
                else
                {
                    CPPUNIT_ASSERT_THROW(Utf16String str(input), InvalidCodeUnitException);
                }
            }
        }

        void IteratorAndCodePointTest()
        {
            SCX::Util::Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xD800, 0xDF02, 0x0};
            Utf16String str(arr);
            CPPUNIT_ASSERT_EQUAL(4, (int)str.CodePoints());

            CodePoint expectedCP[] = {0x004D, 0x0430, 0x4E8C, 0x10302};
            unsigned int* expPtr = expectedCP;

            Utf16String::Iterator it = str.Begin();
            Utf16String::Iterator end = str.End();
            for (; it != end; ++it, ++expPtr)
            {
                CPPUNIT_ASSERT_EQUAL(*expPtr, GetCodePoint(it));
            }

        }

        void DumpUtf16String(const Utf16String& str)
        {
            std::basic_string<Utf16Char> bstr = str.Str();
            for (std::basic_string<Utf16Char>::iterator i = bstr.begin(); i != bstr.end();  ++i)
            {
                printf("%x ", *i);
            }
            printf("\n");
        }

        void SetCodePointTest()
        {
            try
                    {
            Utf16Char arr[] = {0xFEFF, 0xD800, 0xDF02, 0x004D, 0x0430, 0x4E8C, 0x0};
            

            CodePoint expectedCP[] = { 0x10302, 0x004D, 0x0430, 0x4E8C};
            CodePoint testCP[] = {0x0, 0x0065, 0x5555, 0xFFFF, 0x10000, 0xFFFFF ,0x10FFFF, 0xFFFEE, 0x11111, 0x4444, 0x0097, 0x1};

            for (int i = 0; i < 12; i++)
            {
                Utf16String str(arr);
                str.SetCodePointAtIndex(0, testCP[i]);
                str.SetCodePointAtIndex(3, testCP[i]);
                CPPUNIT_ASSERT_EQUAL(4, (int)str.CodePoints());
                CPPUNIT_ASSERT_EQUAL(testCP[i], str.GetCodePointAtIndex(0));
                
                
                for (int j = 1; j < 3; j++)
                {
                    CPPUNIT_ASSERT_EQUAL(expectedCP[j], str.GetCodePointAtIndex(j));
                }
                CPPUNIT_ASSERT_EQUAL(testCP[i], str.GetCodePointAtIndex(3));
            }

            }
            catch (SCXCoreLib::SCXException& e)
            {
                CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(e.What()).c_str());
            }
        }

        void ReadFromStreamTest()
        {
             //Utf16Char arr[] = {0x00FF, 0x00FE, 0x004D, 0x0430, 0x4E8C, 0xD800, 0xDF02, 0x0};
            std::vector<unsigned char> input;
            input.push_back(0xFF);
            input.push_back(0xFE);
            input.push_back(0x4d);
            input.push_back(0x0);
            input.push_back(0x30);
            input.push_back(0x04);
            input.push_back(0x8C);
            input.push_back(0x4e);
            input.push_back(0x00);
            input.push_back(0xD8);
            input.push_back(0x02);
            input.push_back(0xDF);
            
            Utf16String str(input);
            /*
            std::basic_string<Utf16Char> strt = str.Str();
            for (size_t i = 0; i < strt.size(); ++i)
            {
                printf(" %x ", strt[i]);
            }
            printf("\n");
            */
            CPPUNIT_ASSERT_EQUAL(4, (int)str.CodePoints());

            CodePoint expectedCP[] = {0x004D, 0x0430, 0x4E8C, 0x10302};
            unsigned int* expPtr = expectedCP;

            Utf16String::Iterator it = str.Begin();
            Utf16String::Iterator end = str.End();
            for (; it != end; ++it, ++expPtr)
            {
                CPPUNIT_ASSERT_EQUAL(*expPtr, GetCodePoint(it));
            }
        }

        void ConvertFromUtf8Test()
        {
            Utf8String str("\x004D\x00D0\x00B0\x00E4\x00BA\x008C\x00F0\x0090\x008C\x0082");
            Utf16String u16Str;
            u16Str.Assign(str.Begin(), str.End());
            unsigned int expectedCP[4] = {0x004D, 0x0430, 0x4E8C, 0x10302};
            unsigned int* expPtr = expectedCP;
            Utf16String::Iterator it = u16Str.Begin();
            for (; it != u16Str.End(); ++it, ++expPtr)
            {
                CPPUNIT_ASSERT_EQUAL(*expPtr, GetCodePoint(it));
            }

            std::vector<unsigned char> stream;
            u16Str.Write(stream);
            // Compare the stream
            unsigned char expectedStream[] = {0xFF, 0xFE, 0x4D, 0x00, 0x30, 0x04, 0x8C, 0x4E, 0x00, 0xD8, 0x02, 0xDF};

            CPPUNIT_ASSERT_EQUAL(12, (int)stream.size());

            std::pair<std::vector<unsigned char>::iterator,unsigned char*> ret = std::mismatch(stream.begin(), stream.end(), expectedStream);
            CPPUNIT_ASSERT(ret.first == stream.end());
        }
};

CPPUNIT_TEST_SUITE_REGISTRATION (Utf16StringTest);
