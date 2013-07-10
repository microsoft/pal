#include <Unicode.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <iostream>
#include <math.h>
#include <sstream> 
#include <time.h> 
#include <XElement.h>

#include <testutils/scxunit.h>

using namespace SCX::Util;
using namespace SCX::Util::Xml;

const size_t c_ONE_MB = 1024 * 1024;

#ifdef CONFIG_OS_HPUX
    //static const clockid_t CLOCK_ID = CLOCK_VIRTUAL;
#else
    //static const clockid_t CLOCK_ID = CLOCK_PROCESS_CPUTIME_ID;
#endif


class UtfCharRef
{
public:
    UtfCharRef(Utf8String& str) : m_ref(str)
    {}
private:
   Utf8String& m_ref;

};

class UtfCharPtr
{
public:
    UtfCharPtr(Utf8String* strPtr) : m_ptr(strPtr)
    {}
private:
   Utf8String* m_ptr;

};

class Utf8StringPerfTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE (Utf8StringPerfTest);
    
    CPPUNIT_TEST (ConstructorTest);
    CPPUNIT_TEST (XmlLoadTest);
    CPPUNIT_TEST (RefVsPtrTest);
    
   CPPUNIT_TEST_SUITE_END ();
    
    
    public:

        timespec DiffTimeSpec(timespec time1, timespec time2)
        {
            timespec diff = {0, 0};
            if (time1.tv_sec > time2.tv_sec)
            {
                timespec temp = time1;
                time1 = time2;
                time2 = temp;
            }

            if (time2.tv_nsec < time1.tv_nsec) 
            {
                diff.tv_sec  = time2.tv_sec - time1.tv_sec - 1;
                diff.tv_nsec = (1000000000 + time2.tv_nsec) - time1.tv_nsec;
            } 
            else
            {
                diff.tv_sec = time2.tv_sec - time1.tv_sec;
                diff.tv_nsec = time2.tv_nsec - time1.tv_nsec;
            }
            return diff;
        }

        double CalculateIncrease(timespec time1, timespec time2)
        {
            const long SCX_NS_PER_SEC = 1000000000;
            long time1nsec = time1.tv_sec * SCX_NS_PER_SEC + time1.tv_nsec;
            long time2nsec = time2.tv_sec * SCX_NS_PER_SEC + time2.tv_nsec;

            double change = static_cast<double>(time2nsec - time1nsec) /
                static_cast<double>(time1nsec);
            return change;
        }

        void PrintTimeSpec(timespec time)
        {
            printf("Sec = %u NSec = %ld\n", (unsigned int)time.tv_sec, time.tv_nsec);
        }

        timespec ProfileFunction(void (*TestFunc)(void))
        {
            timespec start = {0, 0};
            timespec end   = {0, 0};

            int ret = 0;

            //ret = clock_gettime(CLOCK_ID, &start);
            //printf("START : ");PrintTimeSpec(start);
            if (ret != 0)
            {
                throw "Error getting clock";
                
            }

            (*TestFunc)();

            //ret = clock_gettime(CLOCK_ID, &end);
            //printf("END : ");PrintTimeSpec(end);
            if (ret != 0)
            {
                throw "Error getting clock";
            }
            
            timespec diff = DiffTimeSpec(start, end);
            return diff;
        }


        static void ConstructStdString()
        {
            for (int i = 0 ; i < 1000; i++)
            {
                std::string temp(c_ONE_MB, 'a');
                std::string test(temp.c_str());
            }
        }

        static void ConstructUtf8String()
        {
            for (int i = 0 ; i < 1000; i++)
            {
                std::string temp(c_ONE_MB, 'a');
                Utf8String test(temp.c_str());
            }
        }

        void ConstructorTest()
        {
            // RHEL 5 x64 : 1.6s
            timespec diff1 = ProfileFunction(&ConstructStdString);
            printf("\n"); 
            PrintTimeSpec(diff1);

            // RHEL 5 x64 :
            // Without Optmizations : 76.15s
            // With Optmizations : 12.28s
            timespec diff2 = ProfileFunction(&ConstructUtf8String);
            printf("\n"); 
            PrintTimeSpec(diff2);

            double per = CalculateIncrease(diff1, diff2);
            printf("Increase : %0.2f times\n", per);
            CPPUNIT_ASSERT(per <= 10.0);
        }

        static void CreateOneMbXml(std::string& xml)
        {
            std::string tempXml = "<node attribute=\"value\"></node>";
            
            std::stringstream ss;

            const int size = 32768;
            ss << "<root>";
            for (int i = 0; i < size; i++)
            {
                ss << tempXml;
            }
            ss << "</root>";
            xml = ss.str();;
        }

        static void XElementWithUtf8String()
        {
            std::string xml;
            CreateOneMbXml(xml);
            Utf8String uXml(xml);
            for (int i = 0; i < 1; i++)
            {
                XElementPtr root;
                XElement::Load(uXml, root);
            }

        }

        void XmlLoadTest()
        {
            // RHEL 5 x64 
            // Without Optimization : 127.8s
            // With Optimization : 58.8s
            timespec diff1 = ProfileFunction(&XElementWithUtf8String);
            printf("\n"); 
            PrintTimeSpec(diff1);
         }

        static void CharRef()
        {
            std::string temp(1024, 'a');
            Utf8String test(temp);
            for (int i = 0; i < 1000000; i++)
            {
                UtfCharRef c(test);
            }
        }

        static void CharPtr()
        {
            std::string temp(1024, 'a');
            Utf8String test(temp);
            for (int i = 0; i < 1000000; i++)
            {
                UtfCharPtr c(&test);
            }
        }

        void RefVsPtrTest()
        {
            timespec time1 = ProfileFunction(&CharRef);
            printf("\n"); 
            PrintTimeSpec(time1);
            
            time1 = ProfileFunction(&CharPtr);
            printf("\n"); 
            PrintTimeSpec(time1);
        }
};

// Disabling this for normal runs
//CPPUNIT_TEST_SUITE_REGISTRATION (Utf8StringPerfTest);
