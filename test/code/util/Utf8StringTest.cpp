#include <Unicode.h>
#include <iostream>
#include <math.h>
#include <sstream>

#include <testutils/scxunit.h>

using namespace SCX::Util;

static Utf8String c_CDATA("[CDATA[");

struct ByteRange
{
    unsigned char Minima;
    unsigned char Maxima;
    
    void Print()
    {
        printf("ByteRange = {%02x, %02x}\n", (Minima & 0xff), (Maxima & 0xff));
    }

    bool GetTestByte(int testId, char& testByte)
    {
        bool expectedResult = false;
        
        if (Maxima == Minima)
        {
             if (testId >= 4)
             {
                 testId -= 3;
             }
        }

        switch (testId)
        {
        case 0:
            expectedResult = false;
            break;

        case 1:
            if ((((unsigned char)Minima) == 0x0) || (((unsigned char)Minima)== 0xC2))
            {
                testByte = static_cast<char>(Minima -1);
                expectedResult = false;
            }
            else
            {
                testByte = Minima;
                expectedResult = true;
            }
            break;
             
        case 2:
            testByte = Minima;
            expectedResult = true;
            break;

        case 3:
            if ((unsigned char) Minima == 0xED)
            {
                testByte = Minima;
                expectedResult = true;
            }
            else
            {
                testByte = static_cast<char>(Minima + 1);
                expectedResult = true;
            }

            if ((unsigned char)testByte >= 0xF5)
            {
                expectedResult = false;
            }

            break;

        case 4:
            testByte = static_cast<char>(Maxima - 1);
            expectedResult = true;
            break;

        case 5:
            testByte = Maxima;
            expectedResult = true;
            break;

        case 6:
            if (((unsigned char)Maxima == 0xEC) ||
                    ((unsigned char)Maxima == 0xF3))
            {
                testByte = Maxima;
                expectedResult = true;
            }
            else
            {
                testByte = static_cast<char>(Maxima + 1);
                expectedResult = false;
            }
            break;
        }

        return expectedResult;
    }
};

unsigned int byteSequenceLength[] = {1, 2, 3, 3, 3, 3, 4, 4, 4};

ByteRange Utf8ByteSequence[9][4] = {
    { {0x00, 0x7F} },
    { {0xC2, 0xDF}, {0x80, 0xBF} },
    { {0xE0, 0xE0}, {0xA0, 0xBF}, {0x80, 0xBF} },
    { {0xE1, 0xEC}, {0x80, 0xBF}, {0x80, 0xBF} },
    { {0xED, 0xED}, {0x80, 0x9F}, {0x80, 0xBF} },
    { {0xEE, 0xEF}, {0x80, 0xBF}, {0x80, 0xBF} },
    { {0xF0, 0xF0}, {0x90, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF} },
    { {0xF1, 0xF3}, {0x80, 0xBF}, {0x80, 0xBF}, {0x80, 0xBF} },
    { {0xF4, 0xF4}, {0x80, 0x8F}, {0x80, 0xBF}, {0x80, 0xBF} },
};

class TestString
{
public:
    TestString() : m_data()
    {}

    TestString(const char* str) : m_data(str)
    {

    }

    TestString& operator=(const char* str)
    {
        m_data.assign(str);
        return *this;
    }
    
    ~TestString()
    {
    }

private:
    std::string m_data;
};



class Utf8StringTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE (Utf8StringTest);
    
        CPPUNIT_TEST (EmptyCtorTest);
        CPPUNIT_TEST (CharArrayCtor_AsciiOnlyTest);
        CPPUNIT_TEST (StdStringCtor_AsiiOnlyTest);
        CPPUNIT_TEST (CharArrayCtor_DisallowedUtf8CharsTest);
        CPPUNIT_TEST (Ctor_HandleBOMTest);
        CPPUNIT_TEST (Ctor_HandleIncompleteBOMTest);
        CPPUNIT_TEST (CharArrayCtor_EmbeddedNullCharTest);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range1);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range2);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range3);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range4);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range5);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range6);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range7);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range8);
        CPPUNIT_TEST (Ctor_IllFormedSequenceTest_Range9);
        CPPUNIT_TEST (GetIteratorAndCodePointTest);
        CPPUNIT_TEST (SetCodePointTest);
        CPPUNIT_TEST (ReadFromStreamTest);
        CPPUNIT_TEST (ConvertFromUtf16Test1);
        CPPUNIT_TEST (ConvertFromUtf16Test2);
        CPPUNIT_TEST (EraseAsciiTest);
        CPPUNIT_TEST (AsciiSubStrTest);
        CPPUNIT_TEST (AsciiFindStrTest);
        CPPUNIT_TEST (AsciiCompareTest);
        CPPUNIT_TEST (AsciiAppendTest);
        CPPUNIT_TEST (AsciiTrimTest);
        CPPUNIT_TEST (IndexOperatorTest);
        CPPUNIT_TEST (ConvertExtendedAsciiLookingString);
        CPPUNIT_TEST (TestConvertFromUtf8ToWideString);
   CPPUNIT_TEST_SUITE_END ();
    
    
    public:

        void EmptyCtorTest()
        {
            Utf8String str;

            CPPUNIT_ASSERT(str.Empty());
            CPPUNIT_ASSERT(str.Size() == 0);

            Utf8String str2 = "";
            CPPUNIT_ASSERT(str2.Empty());
            CPPUNIT_ASSERT(str2.Size() == 0);

            Utf8String str3("");
            CPPUNIT_ASSERT(str3.Empty());
            CPPUNIT_ASSERT(str3.Size() == 0);

            std::string s;

            Utf8String str4(s);
            CPPUNIT_ASSERT(str4.Empty());
            CPPUNIT_ASSERT(str4.Size() == 0);
        }

        void CharArrayCtor_AsciiOnlyTest()
        {
            Utf8String str1 = "AbC";
            CPPUNIT_ASSERT(!str1.Empty());
            CPPUNIT_ASSERT_EQUAL((size_t)3, str1.Size());

            char str[5] = {'1', '2', '3', '\0' ,'5'};
            Utf8String str2(str);
            CPPUNIT_ASSERT(!str2.Empty());
            CPPUNIT_ASSERT_EQUAL((size_t)3, str2.Size());

            Utf8String str3("12345");
            CPPUNIT_ASSERT(!str3.Empty());
            CPPUNIT_ASSERT_EQUAL((size_t)5, str3.Size());
        }

        void StdStringCtor_AsiiOnlyTest()
        {
            std::string s = "1245";
            Utf8String str(s);
            CPPUNIT_ASSERT(!str.Empty());
            CPPUNIT_ASSERT_EQUAL((size_t)4, str.Size());
        }

        void CharArrayCtor_DisallowedUtf8CharsTest()
        {
            CPPUNIT_ASSERT_THROW(Utf8String str("\xC0\xAF"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("abcd\xC1sefgh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xD0\xB0\xF5sefgh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csef\xF6gh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xF7gh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xF8gh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xF9gh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xFAgh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xFBgh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xFCgh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xFDgh"), InvalidCodeUnitException);
            CPPUNIT_ASSERT_THROW(Utf8String str("ab\xEA\xBA\x8Csefadada\xFEgh"), InvalidCodeUnitException);
        }

        void CharArrayCtor_EmbeddedNullCharTest()
        {
            Utf8String str("\x0065\x0067\x0000\x0068\0x0079");
            CPPUNIT_ASSERT_EQUAL((size_t)2, str.Size());

            // just null
            Utf8String str1("\x0000");
            CPPUNIT_ASSERT_EQUAL((size_t)0, str1.Size());
            
            // bom with explit null
            Utf8String str2("\x00EF\x00BB\x00BF\x0000");
            CPPUNIT_ASSERT_EQUAL((size_t)0, str2.Size());

        }

        void Ctor_HandleBOMTest()
        {
            Utf8String str("\x00EF\x00BB\x00BFsabc");
            CPPUNIT_ASSERT_EQUAL(4, (int)str.Size());

            // Just the bom
            Utf8String str1("\x00EF\x00BB\x00BF");
            CPPUNIT_ASSERT_EQUAL(0, (int)str1.Size());
        }

        void Ctor_HandleIncompleteBOMTest()
        {
            CPPUNIT_ASSERT_THROW(Utf8String str("\x00EF\x00BBsabc"), InvalidCodeUnitException);
        }

        std::vector<int> ConvertToBase(int decimalNum, unsigned int base)
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
            
        void RunTestForSequence(int rangeNum)
        {
            int sequenceNum = rangeNum - 1;
            int byteCount = byteSequenceLength[sequenceNum];
            int expectedSize = 5;
            size_t obtainedSize = 0;
            int numTests = (int) pow(7.0, byteCount);
            int startTest = (int) pow(7.0, byteCount-1);
            ByteRange* range = Utf8ByteSequence[sequenceNum];

            std::vector<int> testId;
            std::string inputStr;
            char testChar = 0;
            bool expectedResult = false;
            bool gotException = false;
            for (int i = startTest; i < numTests; i++)
            {
                //printf(" %d\n", i);
                expectedResult = true;
                gotException = false;
                expectedSize = 5;
                inputStr = "ab";
                testId = ConvertToBase(i, 7);
                for(size_t j = 0; j < testId.size(); ++j)
                {
                    testChar = 0;
                    if (testId[j] != 0)
                    {
                        bool tempResult = range[j].GetTestByte(testId[j], testChar);
                        expectedResult = tempResult && expectedResult ;
                        inputStr += testChar;
                    }
                    else
                    {
                        if (!((i == 0)))
                        {
                            expectedResult = false;
                        }
                    }
                }

                if (i == 0 )
                {
                    expectedSize--;
                }

               inputStr += "cd";
               //printf("---\n");

                std::stringstream ss;
                ss << i << " : ";
                for (size_t k = 0; k < inputStr.size(); k++)
                {
                    ss <<  std::hex << std::setw(2) <<(unsigned short) (unsigned char) (inputStr[k] & 0xff) << " ";
                }
                ss << " : " <<  (expectedResult? "true" : "false") << "\n";
               //printf("%s", ss.str().c_str());

                try
               {
                   Utf8String str(inputStr);
                   obtainedSize = str.CodePoints();
               }
               catch (InvalidCodeUnitException& e)
               {
                   //printf("Exception = %s\n", SCXCoreLib::StrToUTF8(e.What()).c_str());
                   gotException = true;
               }

               if (expectedResult != gotException)
               {
                   // This is success 
                   if (expectedResult)
                   {
                       //printf("%s", ss.str().c_str());
                       CPPUNIT_ASSERT_EQUAL(expectedSize, (int)obtainedSize);
                   }

               }
               else
               {
                    for(size_t j = 0; j < testId.size(); ++j)
                    {
                        printf(" %d ", testId[j]);
                    }
                    printf("\n");
                   // print the problem 
                   std::string message = "Sequence Failed : " + ss.str(); 
                   CPPUNIT_FAIL(message.c_str());
               }
            }
         }
            


        void Ctor_IllFormedSequenceTest_Range1()
        {
            RunTestForSequence(1);
        }

        void Ctor_IllFormedSequenceTest_Range2()
        {
            RunTestForSequence(2);
        }

        void Ctor_IllFormedSequenceTest_Range3()
        {
            RunTestForSequence(3);
        }

        void Ctor_IllFormedSequenceTest_Range4()
        {
            RunTestForSequence(4);
        }

        void Ctor_IllFormedSequenceTest_Range5()
        {
            RunTestForSequence(5);
        }
        void Ctor_IllFormedSequenceTest_Range6()
        {
            RunTestForSequence(6);
        }

        void Ctor_IllFormedSequenceTest_Range7()
        {
            RunTestForSequence(7);
        }

        void Ctor_IllFormedSequenceTest_Range8()
        {
            RunTestForSequence(8);
        }

        void Ctor_IllFormedSequenceTest_Range9()
        {
            RunTestForSequence(9);
        }

        void GetIteratorAndCodePointTest()
        {
             Utf8String str("\x004D\x00D0\x00B0\x00E4\x00BA\x008C\x00F0\x0090\x008C\x0082");
             CPPUNIT_ASSERT_EQUAL(4, (int) str.CodePoints());
                

             unsigned int expectedCP[4] = {0x004D, 0x0430, 0x4E8C, 0x10302};
             unsigned int* expPtr = expectedCP;
             Utf8String::Iterator it = str.Begin();
             for (; it != str.End(); ++it, ++expPtr)
             {
                 CPPUNIT_ASSERT_EQUAL(*expPtr, GetCodePoint(it));
             }
        }

        void SetCodePointTest()
        {
#if 0
            try
            {
            CCM::Client::CodePoint testCP[] = {0x0, 0x1, 0x7E, 0x7F, 0x80, 0x7FE, 0x7FF, 0x800, 0x801, 0xFFFF, 0x10000, 0x10001, 0x10FFFE, 0x10FFFF,
                                   0x10FFFF, 0x10FFFE, 0x10001, 0x10000, 0xFFFF, 0x801, 0x800, 0x7FF, 0x7FE, 0x80, 0x7F, 0x7E, 0x1, 0x0 };
            CCM::Client::CodePoint expectedCP[] = { 0x10302, 0x0430, 0x004D, 0x4E8C};

            char arr[] = {0xF0, 0x90, 0x8C, 0x82, 0xD0, 0xB0, 0x4D, 0xE4, 0xBA, 0x8C, 0x0};

            for (int i = 0 ; i < 28; i++)
            {
                Utf8String str(arr);
                str[0] = testCP[i];
                str[1] = testCP[i];
                str[3] = testCP[i];

                CPPUNIT_ASSERT_EQUAL(4, (int) str.Size());
                for (size_t j = 0; j < 4; j++)
                {
                    if (j == 2)
                    {
                        CPPUNIT_ASSERT_EQUAL(expectedCP[2], str[j].GetCodePoint());
                    }
                    else
                    {
                        CPPUNIT_ASSERT_EQUAL(testCP[i], str[j].GetCodePoint());
                    }
                }
                
            }
            }
            catch (SCXCoreLib::SCXException& e)
            {
                CPPUNIT_FAIL(SCXCoreLib::StrToUTF8(e.What()).c_str());
            }
#endif
        }

        void ReadFromStreamTest()
        {
            std::vector<unsigned char> input;
            input.push_back(0xEF);
            input.push_back(0x00BB);
            input.push_back(0x00BF);
            // 4D D0 B0 E4 BA 8C F0 90 8C 82
            input.push_back(0x4D); 
            input.push_back(0xD0);
            input.push_back(0xB0);
            input.push_back(0xE4);
            input.push_back(0xBA);
            input.push_back(0x8C);
            input.push_back(0xF0);
            input.push_back(0x90);
            input.push_back(0x8C);
            input.push_back(0x82);

            Utf8String str(input);

             CPPUNIT_ASSERT_EQUAL(4, (int) str.CodePoints());
                

             unsigned int expectedCP[4] = {0x004D, 0x0430, 0x4E8C, 0x10302};
             unsigned int* expPtr = expectedCP;
             Utf8String::Iterator it = str.Begin();
             for (; it != str.End(); ++it, ++expPtr)
             {
                 CPPUNIT_ASSERT_EQUAL(*expPtr, GetCodePoint(it));
             }

        }

        void ConvertFromUtf16Test1()
        {
            Utf16Char arr[] = {0xFEFF, 0x004D, 0x0430, 0x4E8C, 0xD800, 0xDF02, 0x0};
            Utf16String u16str(arr);

            Utf8String u8str;

            u8str.Assign(u16str.Begin(), u16str.End());
            
            CPPUNIT_ASSERT_EQUAL(4, (int) u8str.CodePoints());

            unsigned int expectedCP[4] = {0x004D, 0x0430, 0x4E8C, 0x10302};
            unsigned int* expPtr = expectedCP;
            Utf8String::Iterator it = u8str.Begin();
            for (; it != u8str.End(); ++it, ++expPtr)
            {
                CPPUNIT_ASSERT_EQUAL(*expPtr, GetCodePoint(it));
            }
        }

        void ConvertFromUtf16Test2()
        {
            Utf16Char arr[] = {0xFEFF, 0xD800, 0xDF02,  0x4E8C, 0x004D, 0x0430,  0x0};
            Utf16String u16str(arr);

            Utf8String u8str;

            u8str.Assign(u16str.Begin(), u16str.End());

            std::string t(u8str.Str());
            
            CPPUNIT_ASSERT_EQUAL(4, (int) u8str.CodePoints());

             unsigned int expectedCP[4] = { 0x10302, 0x4E8C, 0x004D, 0x0430 };
             unsigned int* expPtr = expectedCP;
             Utf8String::Iterator it = u8str.Begin();
             for (; it != u8str.End(); ++it, ++expPtr)
             {
                 CPPUNIT_ASSERT_EQUAL(*expPtr, GetCodePoint(it));
             }
        }

        void EraseAsciiTest()
        {
            Utf8String str;
            size_t pos, count;

            // Pos = 0 Count = 0
            str = "0123456789";
            str.Erase();
            CPPUNIT_ASSERT_EQUAL(0, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String(""), str);

            // Pos = 0 0 < Pos < Length
            str = "0123456789";
            pos = 0; count = 3;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(7, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("3456789"), str);

             // Pos = 0 Pos = Length
            str = "0123456789";
            pos = 0; count = 10;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(0, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String(""), str);

            // Pos = 0 Pos > Length
            str = "0123456789";
            pos = 0; count = 20;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(0, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String(""), str);

            // 0 < Pos < Length  Count = 0
            str = "0123456789";
            pos = 2; count = 0;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(10, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("0123456789"), str);

            // 0 < Pos < Length  0 < Count < Length
            str = "0123456789";
            pos = 2; count = 3;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(7, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("0156789"), str);

            pos = 3; count = 4;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(3, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("015"), str);

            // 0 < Pos < Length  0 < Count < Length - SPECIAL
            str = "0123456789";
            pos = 2; count = 8;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(2, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("01"), str);

            // 0 < Pos < Length  0 < Count < Length - SPECIAL 2
            str = "0123456789";
            pos = 2; count = 9;
            str.Erase(pos, count);
            CPPUNIT_ASSERT_EQUAL(2, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("01"), str);

            // 0 < Pos < Length  SPECIAL 3
            str = "0123456789";
            pos = 4; 
            str.Erase(pos);
            CPPUNIT_ASSERT_EQUAL(4, (int) str.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("0123"), str);

            str = "0123456789";
            CPPUNIT_ASSERT_THROW(str.Erase(15, 1), SCXCoreLib::SCXIllegalIndexException<size_t>);
            SCXUNIT_ASSERTIONS_FAILED_ANY();
        }

        void AsciiSubStrTest()
        {
            Utf8String str, str2;
            size_t pos, count;

            // Pos = 0 Count = 0
            str = "0123456789";
            str2 = str.SubStr();
            CPPUNIT_ASSERT_EQUAL(10, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("0123456789"), str2);

            // Pos = 0 0 < Pos < Length
            str = "0123456789";
            pos = 0; count = 3;
            str2 = str.SubStr(pos, count);
            CPPUNIT_ASSERT_EQUAL(3, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("012"), str2);

             // Pos = 0 Pos = Length
            str = "0123456789";
            pos = 0; count = 10;
            str2 = str.SubStr(pos, count);
            CPPUNIT_ASSERT_EQUAL(10, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("0123456789"), str2);

            // Pos = 0 Pos > Length
            str = "0123456789";
            pos = 0; count = 20;
            str2 = str.SubStr(pos, count);
            CPPUNIT_ASSERT_EQUAL(10, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("0123456789"), str2);

            // 0 < Pos < Length  Count = 0
            str = "0123456789";
            pos = 2; count = 0;
            str2 = str.SubStr(pos, count);
            CPPUNIT_ASSERT_EQUAL(0, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String(""), str2);

            // 0 < Pos < Length  0 < Count < Length
            str = "0123456789";
            pos = 2; count = 3;
            str2 = str.SubStr(pos, count);
            CPPUNIT_ASSERT_EQUAL(3, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("234"), str2);

            pos = 3; count = 4;
            str2 = str.SubStr(pos, count);
            CPPUNIT_ASSERT_EQUAL(4, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("3456"), str2);

            // 0 < Pos < Length  0 < Count < Length - SPECIAL
            str = "0123456789";
            pos = 2; count = 8;
            str2 = str.SubStr(pos, count);
           CPPUNIT_ASSERT_EQUAL(8, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("23456789"), str2);

            // 0 < Pos < Length  0 < Count < Length - SPECIAL 2
            str = "0123456789";
            pos = 2; count = 9;
            str2 = str.SubStr(pos, count);
            CPPUNIT_ASSERT_EQUAL(8, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("23456789"), str2);

            // 0 < Pos < Length  SPECIAL 3
            str = "0123456789";
            pos = 4; 
            str2 = str.SubStr(pos);
             CPPUNIT_ASSERT_EQUAL(6, (int) str2.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("456789"), str2);

            str = "0123456789";
            CPPUNIT_ASSERT_THROW(str.SubStr(15, 1), SCXCoreLib::SCXIllegalIndexException<size_t>);
            SCXUNIT_ASSERTIONS_FAILED_ANY();
        }

        void AsciiFindStrTest()
        {
            Utf8String str1, str2;
            size_t pos, ret;

            // str1 = "" str2 ="" pos
            str1 = "";
            str2 = "";
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(std::string::npos, ret);

            // str1 = "" str2 pos
            str1 = "";
            str2 = "1234";
            pos = 1;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(std::string::npos, ret);

            // str1 < str2 str2 pos
            str1 = "12";
            str2 = "1234";
            pos = 1;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(std::string::npos, ret);

            // str1 > str2 str2 = "" pos
            str1 = "1234";
            str2 = "";
            pos = 1;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(std::string::npos, ret);

            // str1 > str2 str2  pos = 0
            str1 = "987123234";
            str2 = "123";
            pos = 0;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(3, (int)ret);

            // str1 > str2 str2  pos > 0 
            str1 = "987123234";
            str2 = "1";
            pos = 3;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(3, (int)ret);

            // str1 > str2 str2  pos > 0 - Special case 1 miss start matches
            str1 = "987123234";
            str2 = "123";
            pos = 4;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(3, (int)ret);

            // str1 > str2 str2  pos > 0 - Special case 1 no match
            str1 = "987123234";
            str2 = "99";
            pos = 4;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(std::string::npos, ret);

            // str1 > str2 str2  pos > 0 - Special case 2 end - Match 
            str1 = "987123234";
            str2 = "234";
            pos = 4;
            ret = str1.Find(str2);
            CPPUNIT_ASSERT_EQUAL(6, (int)ret);


            // str1 > str2 str2  pos > 0 - Special case More than one match
            str1 = "11111111111";
            str2 = "11";
            pos = 4;
            ret = str1.Find(str2, pos);
            CPPUNIT_ASSERT_EQUAL(4, (int)ret);
        }

        void AsciiCompareTest()
        {
            Utf8String str1, str2;

            // str1 ="" str2=""
            CPPUNIT_ASSERT(str1.Compare(str2));
            CPPUNIT_ASSERT(str1 == str2);
            CPPUNIT_ASSERT(!(str1 != str2));

            // str1 != "" str2=""
            str1 = "1234";
            CPPUNIT_ASSERT((!str1.Compare(str2)));
            CPPUNIT_ASSERT(!(str1 == str2));
            CPPUNIT_ASSERT((str1 != str2));

            // str1  "" str2 !=""
            str1 = "1234";
            str2 = "234";
            CPPUNIT_ASSERT((!str1.Compare(str2)));
            CPPUNIT_ASSERT(!(str1 == str2));
            CPPUNIT_ASSERT((str1 != str2));

            //str1 == str2
            str1 = "1234";
            str2 = "1234";
            CPPUNIT_ASSERT((str1.Compare(str2)));
            CPPUNIT_ASSERT((str1 == str2));
            CPPUNIT_ASSERT(!(str1 != str2));


            // Compare overload
            size_t pos, n;
            
            str1 = "999912349999";
            str2 = "1234";
            
            // str1 str2 pos = 0 n = 0 
            pos = 0;
            n = 0;
            CPPUNIT_ASSERT(!(str1.Compare(pos, n, str2)));

            // str1 str2 pos = 2 n = 0
            pos = 2;
            n = 0;
            CPPUNIT_ASSERT(!(str1.Compare(pos, n, str2)));

            // str1 str2 pos = 0 n = 2
            pos = 0;
            n = 2;
            CPPUNIT_ASSERT(!(str1.Compare(pos, n, str2)));
            
            // str1 str2 pos = 0 n = 2
            pos = 4;
            n = 4;
            CPPUNIT_ASSERT((str1.Compare(pos, n, str2)));

            // Start pos
            str1 = "12349999";
            str2 = "1234";
            
            pos = 0;
            n = 4;
            CPPUNIT_ASSERT((str1.Compare(pos, n, str2)));

            pos = 0;
            n = 3;
            CPPUNIT_ASSERT(!(str1.Compare(pos, n, str2)));

            pos = 0;
            n = 6;
            CPPUNIT_ASSERT(!(str1.Compare(pos, n, str2)));

            // --- 

            str1 = "99991234";
            str2 = "1234";
            pos = 4;
            n = 4;
            CPPUNIT_ASSERT((str1.Compare(pos, n, str2)));

            pos = 4;
            n = 3;
            CPPUNIT_ASSERT(!(str1.Compare(pos, n, str2)));

            pos = 4;
            n = 6;
            CPPUNIT_ASSERT((str1.Compare(pos, n, str2)));

            CPPUNIT_ASSERT_THROW(str1.Compare(15, 1, str2), SCXCoreLib::SCXIllegalIndexException<size_t>);

            str1 = "[CDATA[&&***#4<>";
            str2 = "[CDATA[";
            CPPUNIT_ASSERT((str1.Compare(0, str2.Size(), str2)));

            SCXUNIT_ASSERTIONS_FAILED_ANY();
        }

        void AsciiAppendTest()
        {
            // str1 = "" str2 = ""
            Utf8String str1, str2, str3;
            str3 = str1 + str2;
            CPPUNIT_ASSERT_EQUAL(Utf8String(""), str1.Append(str2));
            CPPUNIT_ASSERT_EQUAL(0, (int) str1.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String(""), str3);

            str1 = "12";
            str2 = "";
            str3 = "123";
            str3 = str1 + str2;
            CPPUNIT_ASSERT_EQUAL(Utf8String("12"), str1.Append(str2));
            CPPUNIT_ASSERT_EQUAL(2, (int) str1.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("12"), str3);

            str1 = "";
            str2 = "12";
            str3 = "123";
            str3 = str1 + str2;
            CPPUNIT_ASSERT_EQUAL(Utf8String("12"), str1.Append(str2));
            CPPUNIT_ASSERT_EQUAL(2, (int) str1.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("12"), str3);

            str1 = "12";
            str2 = "34";
            str3 = "123";
            str3 = str1 + str2;
            CPPUNIT_ASSERT_EQUAL(Utf8String("1234"), str1.Append(str2));
            CPPUNIT_ASSERT_EQUAL(4, (int) str1.Size());
            CPPUNIT_ASSERT_EQUAL(Utf8String("1234"), str3);

            str1 = "12";
            str2 = "023456";
            
            Utf8String::Iterator it = str2.Begin();
            ++it; 
            ++it;
            ++it;
            str1.Append(*it);
            CPPUNIT_ASSERT_EQUAL(Utf8String("124"), str1);

#if 0
            it = str2.Begin();
            Utf8String::Iterator it2 = str1.Begin();
            it->Insert(*it2);
            CPPUNIT_ASSERT_EQUAL(Utf8String("1023456"), str2);

            it = str2.End();
            it2++;
            it->Insert(*it2);
            CPPUNIT_ASSERT_EQUAL(Utf8String("10234562"), str2);
    
            str2 = "11234562";
            it = str2.Begin();
            ++it;++it;++it;
            it2++;
            it->Insert(*it2); 
            CPPUNIT_ASSERT_EQUAL(Utf8String("112434562"), str2);
#endif
        }

        void AsciiTrimTest()
        {
            Utf8String input[][2] = {
                                        {"", ""},
                                        {" abcd", "abcd"},
                                        {"   abcd", "abcd"},
                                        {"   a  b     cd", "a  b     cd"},
                                        {"abcd    ", "abcd"},
                                        {"a    bc   d    ", "a    bc   d"},
                                        {"       a    bc   d    ", "a    bc   d"},
                                    };

            for (int i = 0; i < 7; ++i)
            {
                Utf8String test(input[i][0]);
                test.Trim();
                CPPUNIT_ASSERT_EQUAL(input[i][1], test);
            }
        }

        Utf8String TestPass(Utf8String& p)
        {
            if (p[0] != 'c')
            {
            }

            p.Erase(0, 0);
            return p;
        }
        void IndexOperatorTest()
        {
            Utf8String s;
            
            s = "0123456789";
            TestPass(s);
            for (int i = 0 ; i < 10; i++)
            {
                CPPUNIT_ASSERT(s[i] == ('0' + i));
                CPPUNIT_ASSERT(s[i] != 'a');
            }
        }

         void ConvertExtendedAsciiLookingString()
        {
            // rpm -U
            Utf16Char arr[] = {0x0072, 0x0070, 0x006D, 0x0020, 0x2013, 0x0055, 0x0};
            Utf16String str(arr);
            Utf8String u8str;
            u8str.Assign(str.Begin(), str.End());
            
            Utf8String expectedStr = "rpm \x00e2\x0080\x0093U";
            CPPUNIT_ASSERT_EQUAL(expectedStr, u8str);

            Utf16String str2;
            str2.Assign(u8str.Begin(), u8str.End());

            CPPUNIT_ASSERT(str2==str);
        }

        void TestConvertFromUtf8ToWideString()
        {
            // Create UTF-8 String
            std::vector<unsigned char> input;
            input.push_back(0x74);
            input.push_back(0x65);
            input.push_back(0x73);
            input.push_back(0x74);
            input.push_back(0xC3);
            input.push_back(0xA9);
            input.push_back(0xC3);
            input.push_back(0xAB);
            input.push_back(0x2E);
            input.push_back(0x4E);
            input.push_back(0x51);
            input.push_back(0x62);
            input.push_back(0xC3);
            input.push_back(0xBF);
            input.push_back(0x00);
            Utf8String utf8str(input);

            // Convert
            std::wstring wString;
            utf8str.ToWideString(wString);

            // Check codepoints
            unsigned int expectedCP[12] = {0x0074,
                                           0x0065,
                                           0x0073,
                                           0x0074,
                                           0x00E9,
                                           0x00EB,
                                           0x002E,
                                           0x004E,
                                           0x0051,
                                           0x0062,
                                           0x00FF,
                                           0x0000};

            unsigned int* expPtr = expectedCP;
            std::wstring::iterator it = wString.begin();

            for ( ; it != wString.end(); ++it, ++expPtr)
            {
                CPPUNIT_ASSERT_EQUAL(*expPtr, (unsigned int) *it);
            }
        }
    };

CPPUNIT_TEST_SUITE_REGISTRATION (Utf8StringTest);
