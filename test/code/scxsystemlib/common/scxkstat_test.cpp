/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
    
  Created date    2007-08-22 11:55:09

  Test class for SCXKstat
    
*/
/*----------------------------------------------------------------------------*/

// Only for solaris!

#if defined(sun)

#include <cppunit/extensions/HelperMacros.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>
#include <scxsystemlib/scxkstat.h>
#include <scxcorelib/scxexception.h>
#include <iostream>


class TestKstatDependencyThatAlwaysFail : public SCXSystemLib::SCXKstatDependencies
{
public:
    virtual ~TestKstatDependencyThatAlwaysFail() {}
    
    virtual kstat_ctl_t* Open() { return 0; }
    virtual kstat_t* Lookup(kstat_ctl_t*, char*, int, char*) { return 0; }
    virtual int Read(kstat_ctl_t*, kstat_t*, void*) { return -1; }
    virtual kid_t Update(kstat_ctl_t*) { return -1; }
};

class TestKstatDependencyThatSometimesFail : public SCXSystemLib::SCXKstatDependencies
{
public:
    TestKstatDependencyThatSometimesFail()
        : m_readFailCount(0)
    { }
    virtual ~TestKstatDependencyThatSometimesFail() {}
    
    virtual kstat_ctl_t* Open() { return (kstat_ctl_t *) (4711); }
    virtual kstat_t* Lookup(kstat_ctl_t*, char*, int, char*) { return (kstat_t *) 47111; }
    virtual int Read(kstat_ctl_t*, kstat_t*, void*)
    {
        if (m_readFailCount)
        {
            m_readFailCount--;

            //std::wcout << std::endl << L"Simulating error from kstat_read() ...";
            errno = ENXIO;
            return -1; 
        }

        return 0;
    }
    void SetReadFailCount(int x) { m_readFailCount = x; }
    virtual kid_t Update(kstat_ctl_t*) { return -1; }
    virtual void Close(kstat_ctl_t*) {}

private:
    int m_readFailCount;
};

class TestKstatDependencyWithSensing : public SCXSystemLib::SCXKstatDependencies
{
public:
    virtual ~TestKstatDependencyWithSensing() {}

    TestKstatDependencyWithSensing() :
        m_UpdateCalledTimes(0),
        m_errno(0)
    {}

    virtual kstat_ctl_t* Open() { return (kstat_ctl_t *) (4711); }
    virtual kstat_t* Lookup(kstat_ctl_t*, char*, int, char*) { return 0; }
    virtual int Read(kstat_ctl_t*, kstat_t*, void*) { return -1; }

    virtual kid_t Update(kstat_ctl_t*)
    {
        ++m_UpdateCalledTimes;
        if (m_errno == 0)
        {
            return 4711;
        }
        else
        {
            errno = m_errno;
            return -1;
        }
    }
    virtual void Close(kstat_ctl_t*) {}

    void SetFail(int error)
    {
        m_errno = error;
    }

    int m_UpdateCalledTimes;
    int m_errno;
};

class TestKstatDependencyWithKnownValues : public SCXSystemLib::SCXKstatDependencies
{
public:
    virtual ~TestKstatDependencyWithKnownValues() {}

    TestKstatDependencyWithKnownValues()
        : m_chain(0)
        {}

    virtual kstat_ctl_t* Open() { return m_chain; }
    virtual kstat_t* Lookup(kstat_ctl_t*, char*, int, char*) { return 0; }
    virtual int Read(kstat_ctl_t*, kstat_t*, void*) { return 0; }
    virtual kid_t Update(kstat_ctl_t*) { return 0; }
    virtual void Close(kstat_ctl_t*) { }

    void SetKstat(kstat_ctl_t* p) { m_chain = p; }
private:
    kstat_ctl_t* m_chain;
};

class TestKstat : public SCXSystemLib::SCXKstat
{
public:
    TestKstat(const SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstatDependencies>& deps)
        : SCXKstat(deps)
    {
        Init();
    }
};

/**
   Provide behavior of a pipe FILE* with autoclosing when the class goes out
   of scope.
 */
class NicePipe {
    FILE * m_file;

public:
    /**
       CTOR
       \f FILE* created by popen(2).
    */
    NicePipe(FILE * f) : m_file(f)
    {}

    /**
       DTOR
    */
    ~NicePipe()
    {
        int rv = pclose(m_file);
        if (-1 == rv)
        {
            int e = errno;
            std::wcerr << L"pclose() failed returning " << e << std::endl;
        }
        else if (0 != rv)
        {
            std::wcerr << L"pclose() returned " << rv << std::endl;
        }
    }

    /**
       FILE * operator
       \returns The FILE pointer.
    */
    operator FILE*() const
    {
        return m_file;
    }
};

class SCXKstatTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXKstatTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( callDumpStringForCoverageWithModuleOnly );
    CPPUNIT_TEST( TestKstatOpenFails );
    CPPUNIT_TEST( TestExistingInstanceAny );
    CPPUNIT_TEST( TestExistingInstanceSpecified );
    CPPUNIT_TEST( TestExistingValueRaw );
    CPPUNIT_TEST( TestExistingValueNamed );
    CPPUNIT_TEST( TestExistingValueNamedWithModuleOnly );
    //CPPUNIT_TEST( TestExistingValueIntr );
    CPPUNIT_TEST( TestExistingValueIO );
    //CPPUNIT_TEST( TestExistingValueTimer );
    CPPUNIT_TEST( TestNonExistingModule );
    CPPUNIT_TEST( TestNonExistingName );
    CPPUNIT_TEST( TestNonExistingInstance );
    CPPUNIT_TEST( TestNonExistingValueRaw );
    CPPUNIT_TEST( TestNonExistingValueNamed );
    //CPPUNIT_TEST( TestNonExistingValueIntr );
    CPPUNIT_TEST( TestNonExistingValueIO );
    //CPPUNIT_TEST( TestNonExistingValueTimer );
    CPPUNIT_TEST( TestConstructorDoesNotCallUpdate );
    CPPUNIT_TEST( TestUpdateCallsUpdate );
    CPPUNIT_TEST( TestUpdateErrnoEAGAINDoesNotThrow );
    CPPUNIT_TEST( TestUpdateErrnoENXIOThrows );
    CPPUNIT_TEST( TestInternalIterator );
    CPPUNIT_TEST( TestReadWillRetryOnError );
    CPPUNIT_TEST( TestReadWillFailWithTooManyErrors );
    CPPUNIT_TEST_SUITE_END();

private:
    // If the instance parameter is set to -1, the first instance found is used. The instance number
    // of the found instance is returned in the instance parameter.
    scxulong GetCompareKstatValue(const char* module, const char* name, const char* statistic, int& instance)
    {
        std::stringstream command("");
        command << "kstat -p -m " << module << " -n " << name << " -s " << statistic;
        if (-1 != instance)
        {
            command << " -i " << instance;
        }
        NicePipe file(popen(command.str().c_str(), "r"));
        CPPUNIT_ASSERT(NULL != file);

        char buf[256];
        if (0 == fgets(buf, 256, file))
        {
            return 0;
        }
        std::wstring output = SCXCoreLib::StrFromUTF8(buf);
        std::vector<std::wstring> tokens;
        SCXCoreLib::StrTokenize(output, tokens, L" \t");

        if (2 != tokens.size())
        {
            return 0;
        }

        std::vector<std::wstring> parts;
        SCXCoreLib::StrTokenize(tokens[0], parts, L":");
        if (4 != parts.size())
        {
            return 0;
        }
        instance = static_cast<int> (SCXCoreLib::StrToLong(parts[1]));

        return SCXCoreLib::StrToULong(tokens[1]);
    }

    bool CompareWithMargin(scxulong d1, scxulong d2, scxulong margin=5)
    {
        /*
         * In this particular case it is ok to use long instead of scxlong. 
         * When using scxlong, the compiler does not know what abs to use. (long or int)
         * This code is test code that is only run on Solaris.
         */
        bool result = (static_cast<scxulong>(abs(static_cast<long>(d1) - static_cast<long>(d2))) <= margin);
        if ( ! result)
        {
            std::wcout << std::endl << L"CompareWithMargin: " << d1 << L", " << d2 << L", " << margin << std::endl;
        }
        return result;
    }

    void GetDiskNames(std::vector<std::wstring>& disks)
    {
        char buf[256];
        std::stringstream command("kstat -l -c disk");

        disks.clear();
        FILE *fp = popen(command.str().c_str(), "r");
        if (0 != fp)
        {
            while ( ! feof(fp))
            {
                memset(buf, 0, sizeof(buf));
                fgets(buf, sizeof(buf), fp);
                std::wstring line = SCXCoreLib::StrFromUTF8(buf);
                disks.push_back(line);
            }
            pclose(fp);
        }
    }

public:
    SCXSystemLib::SCXKstat* m_pKstat;

    void setUp(void)
    {
        m_pKstat = 0;
    }

    void tearDown(void)
    {
        if (0 != m_pKstat)
        {
            delete m_pKstat;
            m_pKstat = 0;
        }
    }

    void callDumpStringForCoverage()
    {
        m_pKstat = new SCXSystemLib::SCXKstat();
        m_pKstat->Lookup(L"cpu_stat", L"cpu_stat0");
        CPPUNIT_ASSERT(m_pKstat->DumpString().find(L"SCXKstat") != std::wstring::npos);
    }

    void callDumpStringForCoverageWithModuleOnly()
    {
        m_pKstat = new SCXSystemLib::SCXKstat();
        m_pKstat->Lookup(L"cpu_stat");
        CPPUNIT_ASSERT(m_pKstat->DumpString().find(L"SCXKstat") != std::wstring::npos);
    }

    void TestKstatOpenFails()
    {
        SCXUNIT_RESET_ASSERTION();
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXKstatDependencies> deps(new TestKstatDependencyThatAlwaysFail());
        SCXUNIT_ASSERT_THROWN_EXCEPTION(m_pKstat = new TestKstat(deps), SCXSystemLib::SCXKstatErrorException, L"kstat_open");
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestExistingInstanceAny() 
    {
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_stat"));
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_stat", L"cpu_stat0"));
    }
    
    void TestExistingInstanceSpecified() 
    {
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(std::wstring(L"cpu_stat"), 0));
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_stat", L"cpu_stat0", 0));
    }
    
    void TestExistingValueRaw() 
    {
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_stat", L"cpu_stat0"));
        CPPUNIT_ASSERT_THROW(m_pKstat->GetValue(L"pgin"), SCXCoreLib::SCXNotSupportedException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }
    
    void TestExistingValueNamed() 
    {
        SCXUNIT_RESET_ASSERTION();
        int instance = -1; // Ask for any instance
        scxulong before = GetCompareKstatValue("cpu_info", "cpu_info0", "state_begin", instance);
        scxulong value = before - 1; // make sure test will fail if value is not set.
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_info", L"cpu_info0", instance));
        CPPUNIT_ASSERT_NO_THROW(value = m_pKstat->GetValue(L"state_begin"));
        CPPUNIT_ASSERT_EQUAL(before, value);
        SCXUNIT_ASSERTIONS_FAILED(0);
    }
    
    void TestExistingValueNamedWithModuleOnly()
    {
        SCXUNIT_RESET_ASSERTION();
        int instance = -1; // Ask for any instance
        scxulong before = GetCompareKstatValue("cpu_info", "cpu_info0", "state_begin", instance);
        scxulong value = before - 1; // make sure test will fail if value is not set.
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_info", instance));
        CPPUNIT_ASSERT_NO_THROW(value = m_pKstat->GetValue(L"state_begin"));
        CPPUNIT_ASSERT_EQUAL(before, value);
        SCXUNIT_ASSERTIONS_FAILED(0);
    }
    
    void TestExistingValueIntr() 
    {
        CPPUNIT_FAIL("No known kstat instance of type INTR");
    }
    
    void TestExistingValueIO() 
    {
        SCXUNIT_RESET_ASSERTION();
        // Need to dynamically check for disks since they can have any name
        std::vector<std::wstring> disks;
        GetDiskNames(disks);
        scxulong compA,compB;
        int kstatsFound = 0;
        for (std::vector<std::wstring>::const_iterator it = disks.begin();
             it != disks.end() && 0 == kstatsFound; ++it)
        {
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(*it, parts, L":");

            if (parts.size() > 3)
            {
                try {
                    int instance = -1; // Ask for any instance
                    std::string module = SCXCoreLib::StrToUTF8(parts[0]);
                    std::string name = SCXCoreLib::StrToUTF8(parts[2]);
                    compB = GetCompareKstatValue(module.c_str(), name.c_str(), "reads", instance);
                    CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
                    CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(parts[0], parts[2], instance));
                    compA = GetCompareKstatValue(module.c_str(), name.c_str(), "reads", instance);
                    SCXUNIT_ASSERT_BETWEEN(m_pKstat->GetValue(L"reads"), compB, compA);
                    ++kstatsFound;
                }
                catch (SCXSystemLib::SCXKstatNotFoundException&) {}
            }
        }
        CPPUNIT_ASSERT_MESSAGE("Could not find any disks in kstat.", kstatsFound > 0);
        SCXUNIT_ASSERTIONS_FAILED(0);
    }
    
    void TestExistingValueTimer() 
    {
        CPPUNIT_FAIL("No known kstat instance of type TIMER");
    }
    
    // Non-existing module, but name existing in some module.
    void TestNonExistingModule() 
    {
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_THROW(m_pKstat->Lookup(L"ThisIsNotNamedFooSoItWillPassPolicheck", L"cpu_info0"), SCXSystemLib::SCXKstatNotFoundException);
    }
    
    // Existing module but name does not exist.
    void TestNonExistingName() 
    {
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_THROW(m_pKstat->Lookup(L"cpu_info", L"ThisIsNotNamedBarSoItWillPassPolicheck"), SCXSystemLib::SCXKstatNotFoundException);
    }

    void TestNonExistingInstance() 
    {
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_THROW(m_pKstat->Lookup(L"cpu_info", L"cpu_info0", 42), SCXSystemLib::SCXKstatNotFoundException);
    }
    
    void TestNonExistingValueRaw() 
    {
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_stat", L"cpu_stat0"));
        CPPUNIT_ASSERT_THROW(m_pKstat->GetValue(L"ThisIsNotNamedFooThisIsNotNamedBarSoItWillPassPolicheckSoItWillPassPolicheck"), SCXCoreLib::SCXNotSupportedException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }
    
    void TestNonExistingValueNamed() 
    {
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
        CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(L"cpu_info", L"cpu_info0"));
        CPPUNIT_ASSERT_THROW(m_pKstat->GetValue(L"ThisIsNotNamedFooThisIsNotNamedBarSoItWillPassPolicheckSoItWillPassPolicheck"), SCXSystemLib::SCXKstatStatisticNotFoundException);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }
    
    void TestNonExistingValueIntr() 
    {
        CPPUNIT_FAIL("No known kstat instance of type INTR");
    }
    
    void TestNonExistingValueIO() 
    {
        SCXUNIT_RESET_ASSERTION();
                // Need to dynamically check for disks since they can have any name
        std::vector<std::wstring> disks;
        GetDiskNames(disks);
        int kstatsFound = 0;
        for (std::vector<std::wstring>::const_iterator it = disks.begin();
             it != disks.end() && 0 == kstatsFound; ++it)
        {
            std::vector<std::wstring> parts;
            SCXCoreLib::StrTokenize(*it, parts, L":");

            if (parts.size() > 3)
            {
                try {
                    int instance = -1; // Ask for any instance
                    std::string module = SCXCoreLib::StrToUTF8(parts[0]);
                    std::string name = SCXCoreLib::StrToUTF8(parts[2]);
                    scxulong dummy = GetCompareKstatValue(module.c_str(), name.c_str(), "reads", instance);
                    CPPUNIT_ASSERT_NO_THROW(m_pKstat = new SCXSystemLib::SCXKstat);
                    CPPUNIT_ASSERT_NO_THROW(m_pKstat->Lookup(parts[0], parts[2], instance));
                    CPPUNIT_ASSERT_THROW(m_pKstat->GetValue(L"ThisIsNotNamedFooThisIsNotNamedBarSoItWillPassPolicheckSoItWillPassPolicheck"), SCXSystemLib::SCXKstatStatisticNotFoundException);
                    ++kstatsFound;
                }
                catch (SCXSystemLib::SCXKstatNotFoundException&) {}
            }
        }
        CPPUNIT_ASSERT_MESSAGE("Could not find any disks in kstat.", kstatsFound > 0);
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }
    
    void TestNonExistingValueTimer() 
    {
        CPPUNIT_FAIL("No known kstat instance of type TIMER");
    }
    
    void TestConstructorDoesNotCallUpdate()
    {
        SCXCoreLib::SCXHandle<TestKstatDependencyWithSensing> deps(new TestKstatDependencyWithSensing());
        m_pKstat = new TestKstat(deps);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The constructor should not call the updage method", 0, deps->m_UpdateCalledTimes);
    }

    void TestUpdateCallsUpdate()
    {
        SCXCoreLib::SCXHandle<TestKstatDependencyWithSensing> deps(new TestKstatDependencyWithSensing());
        m_pKstat = new TestKstat(deps);
        m_pKstat->Update();
        CPPUNIT_ASSERT_EQUAL_MESSAGE("The update method should call update on the dependency object.", 1, deps->m_UpdateCalledTimes);
    }

    void TestUpdateErrnoEAGAINDoesNotThrow()
    {
        SCXCoreLib::SCXHandle<TestKstatDependencyWithSensing> deps(new TestKstatDependencyWithSensing());
        m_pKstat = new TestKstat(deps);
        deps->SetFail(EAGAIN);

        CPPUNIT_ASSERT_NO_THROW_MESSAGE("The update method should not throw if update fails and errno is EAGAIN", m_pKstat->Update());
    }
    
    void TestUpdateErrnoENXIOThrows()
    {
        SCXCoreLib::SCXHandle<TestKstatDependencyWithSensing> deps(new TestKstatDependencyWithSensing());
        m_pKstat = new TestKstat(deps);
        deps->SetFail(ENXIO);

        CPPUNIT_ASSERT_THROW_MESSAGE("The update method should throw if update fails and errno is not EAGAIN", m_pKstat->Update(), SCXSystemLib::SCXKstatErrorException);
    }

    void TestInternalIterator()
    {
        kstat_t kstatArray[10];
        for(int i = 0; i < 10; i++)
        {
            memset(&kstatArray[i], 0, sizeof(kstat_t));
            strcpy(kstatArray[i].ks_module, "test");
            strcpy(kstatArray[i].ks_class, "net");
            strcpy(kstatArray[i].ks_name, "obj");
            kstatArray[i].ks_instance = i;

            kstatArray[i].ks_next = (i == 9) ? NULL : &kstatArray[i+1];
        }

        kstat_ctl_t chain;
        memset(&chain, 0, sizeof(kstat_ctl_t));
        chain.kc_chain = &kstatArray[0];
        chain.kc_chain_id = 100;
        
        SCXCoreLib::SCXHandle<TestKstatDependencyWithKnownValues> deps(new TestKstatDependencyWithKnownValues());
        deps->SetKstat(&chain);
        m_pKstat = new TestKstat(deps);

        kstat_t* p = m_pKstat->ResetInternalIterator();
        CPPUNIT_ASSERT_MESSAGE("Wrong return from ResetInternalIterator", &kstatArray[0] == p);

        for(int i = 1; i < 10; i++)
        {
            p = m_pKstat->AdvanceInternalIterator();
            CPPUNIT_ASSERT_MESSAGE("Advancing the iterator didn't follow the array", &kstatArray[i] == p);
        }    
            
        p = m_pKstat->AdvanceInternalIterator();
        CPPUNIT_ASSERT_MESSAGE("Advancing the iterator didn't follow the array", NULL == p);
            
        p = m_pKstat->AdvanceInternalIterator();
        CPPUNIT_ASSERT_MESSAGE("Advancing the iterator didn't follow the array", NULL == p);
    }

    void TestReadWillRetryOnError()
    {
        SCXCoreLib::SCXHandle<TestKstatDependencyThatSometimesFail> deps(new TestKstatDependencyThatSometimesFail());
        deps->SetReadFailCount(2);
        m_pKstat = new TestKstat(deps);

        CPPUNIT_ASSERT_NO_THROW_MESSAGE("The Lookup method should not throw if read fails two times",
                                        m_pKstat->Lookup(L"cpu_stat", L"cpu_stat0"));
    }

    void TestReadWillFailWithTooManyErrors()
    {
        SCXCoreLib::SCXHandle<TestKstatDependencyThatSometimesFail> deps(new TestKstatDependencyThatSometimesFail());
        deps->SetReadFailCount(3);
        m_pKstat = new TestKstat(deps);

        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW_MESSAGE("The Lookup method should throw read fails too many times",
                                     m_pKstat->Lookup(L"cpu_stat", L"cpu_stat0"),
                                     SCXSystemLib::SCXKstatErrorException);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXKstatTest );

#endif
