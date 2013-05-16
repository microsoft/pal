/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-07-24 14:01:13

    Log file configurator test
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include "scxcorelib/util/log/scxlogmediator.h"
#include "scxcorelib/util/log/scxlogfileconfigurator.h"
#include "scxcorelib/util/log/scxlogfilebackend.h"
#include "scxcorelib/util/log/scxlogstdoutbackend.h"
#include <testutils/scxunit.h>
#if defined(SCX_UNIX)
#include <scxcorelib/scxuser.h>
#endif
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxlogpolicy.h>

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif

using namespace SCXCoreLib;

class TestMediator : public SCXLogMediator
{
public:
    /*----------------------------------------------------------------------------*/
    /**
        A strict ordering is needed by the set template.
    */
    struct HandleCompare
    {
        /*----------------------------------------------------------------------------*/
        /**
            Compares two scxhandles.
            \param[in] h1 First handle to compare.
            \param[in] h2 Second handle to compare.
            \returns true if h1 is less than h2.
        */
        bool operator()(const SCXHandle<SCXLogItemConsumerIf> h1, const SCXHandle<SCXLogItemConsumerIf> h2) const
        {
            return h1.GetData() < h2.GetData();
        }
    };

    virtual void LogThisItem(const SCXLogItem&)
    {}

    virtual bool RegisterConsumer(SCXHandle<SCXLogItemConsumerIf> consumer)
    {
        m_Consumers.insert(consumer);
        return true;
    }

    virtual bool DeRegisterConsumer(SCXHandle<SCXLogItemConsumerIf> consumer)
    {
        return m_Consumers.erase(consumer) > 0;
    }

    virtual SCXLogSeverity GetEffectiveSeverity(const std::wstring&) const
    {
        return SCXCoreLib::eNotSet;
    }

    virtual ~TestMediator() {};

    std::set<SCXHandle<SCXLogItemConsumerIf>, HandleCompare> m_Consumers;
};


static void AddUserNameToPath(SCXFilePath& logfilepath)
{
#if defined(SCX_UNIX)
        SCXUser user;
        if (!user.IsRoot())
        {
            logfilepath.AppendDirectory(user.GetName());
        }
#endif
}

class SCXLogFileConfiguratorTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogFileConfiguratorTest );
    CPPUNIT_TEST( TestNoConfigurationFile );
    CPPUNIT_TEST( TestConfigurationFile );
    CPPUNIT_TEST( TestDefaultThresholdIsInfo );
    CPPUNIT_TEST( DefaultThresholdActiveAfterSetAndClearOfThreshold );
    CPPUNIT_TEST( TestThreeBackends );
    CPPUNIT_TEST( TestSetSeverity );
    CPPUNIT_TEST( TestThreadSafe );
    CPPUNIT_TEST( TestReconfigureNoConfigToSimpleConfig );
    CPPUNIT_TEST( TestAutomaticReconfigure );
    CPPUNIT_TEST( TestAutomaticReconfigureNoFile );
    CPPUNIT_TEST( TestInvalidConfigurationFile );
    SCXUNIT_TEST_ATTRIBUTE(TestAutomaticReconfigure, SLOW);
    CPPUNIT_TEST_SUITE_END();

public:

    void setUp(void)
    {
    }

    void tearDown(void)
    {
    }

    void TestNoConfigurationFile()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        SCXFilePath configFilePath(L"this_file_should_not_exist");
        SCXLogFileConfigurator configurator(testMediator, configFilePath);
        
        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);
        
        SCXHandle<SCXLogItemConsumerIf> backend = *(testMediator->m_Consumers.begin());
        CPPUNIT_ASSERT(backend != 0);
        SCXLogFileBackend* fbackend;
        fbackend = dynamic_cast<SCXLogFileBackend*>(backend.GetData());
        CPPUNIT_ASSERT(fbackend != 0);
        CPPUNIT_ASSERT(CustomLogPolicyFactory()->GetDefaultSeverityThreshold() == fbackend->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(CustomLogPolicyFactory()->GetDefaultLogFileName() == fbackend->GetFilePath());
    }

    void TestConfigurationFile()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal WARNING");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common HYSTERICAL");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common.entityenumeration INFO");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        SCXLogFileConfigurator configurator(testMediator, configFilePath);

        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);

        SCXHandle<SCXLogItemConsumerIf> backend = *(testMediator->m_Consumers.begin());
        CPPUNIT_ASSERT(backend != 0);
        SCXLogFileBackend* fbackend;
        fbackend = dynamic_cast<SCXLogFileBackend*>(backend.GetData());
        CPPUNIT_ASSERT(fbackend != 0);
        SCXFilePath logfilepath(L"/var/log/scx");
        AddUserNameToPath(logfilepath);
        CPPUNIT_ASSERT(logfilepath == fbackend->GetFilePath());

        // The easy ones first
        CPPUNIT_ASSERT(eTrace == fbackend->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal"));
        CPPUNIT_ASSERT(eHysterical == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common"));
        CPPUNIT_ASSERT(eInfo == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common.entityenumeration"));

        // Then some derived
        CPPUNIT_ASSERT(eTrace == fbackend->GetEffectiveSeverity(L"scxtest"));
        CPPUNIT_ASSERT(eTrace == fbackend->GetEffectiveSeverity(L"scxtest.core"));
        CPPUNIT_ASSERT(eTrace == fbackend->GetEffectiveSeverity(L"scxtest.core.common"));
        CPPUNIT_ASSERT(eTrace == fbackend->GetEffectiveSeverity(L"scxtest.core.common.notpal"));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system"));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.notcommon"));
        // Hysterical is not inherited.
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common.notentityenumeration"));
        CPPUNIT_ASSERT(eInfo == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common.entityenumeration.something"));

        SCXFile::Delete(configFilePath);
    }

    void TestDefaultThresholdIsInfo()
    {
        // The default severity is Info. Make sure the default severity is reset to that even
        // when having effective sev Error for a while.
        SCXHandle<TestMediator> testMediator( new TestMediator());
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal ERROR");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        SCXLogFileConfigurator configurator(testMediator, configFilePath);
        
        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);
        
        SCXHandle<SCXLogItemConsumerIf> backend = *(testMediator->m_Consumers.begin());
        CPPUNIT_ASSERT(backend != 0);
        SCXLogFileBackend* fbackend;
        fbackend = dynamic_cast<SCXLogFileBackend*>(backend.GetData());
        CPPUNIT_ASSERT(fbackend != 0);

        CPPUNIT_ASSERT(eInfo == fbackend->GetEffectiveSeverity(L""));

        SCXFile::Delete(configFilePath);
    }

    void DefaultThresholdActiveAfterSetAndClearOfThreshold()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator());
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        SCXLogFileConfigurator configurator(testMediator, configFilePath);

        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);

        // Verify the default:
        CPPUNIT_ASSERT_MESSAGE("Default min active threshold is not INFO", L"INFO" == configurator.GetMinActiveSeverityThreshold());

        configurator.SetSeverityThreshold(L"some.module", eHysterical);
        CPPUNIT_ASSERT_MESSAGE("Expected HYSTERICAL for min active threshold", L"HYSTERICAL" == configurator.GetMinActiveSeverityThreshold());
        configurator.ClearSeverityThreshold(L"some.module");
        CPPUNIT_ASSERT_MESSAGE("After clearing threshold, min active is not back to default.", L"INFO" == configurator.GetMinActiveSeverityThreshold());

        SCXFile::Delete(configFilePath);
    }

    void TestThreeBackends()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L")");
        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx2");
        confFileContent.push_back(L"MODULE: ERROR");
        confFileContent.push_back(L")");
        confFileContent.push_back(L"STDOUT (");
        confFileContent.push_back(L"MODULE: WARNING");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        SCXLogFileConfigurator configurator(testMediator, configFilePath);
        
        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 3);

        // The order of the backends is undefined.
        SCXLogFileBackend* b1 = 0;
        SCXLogFileBackend* b2 = 0;
        SCXLogStdoutBackend* bStdout = 0;
        SCXFilePath logfilepath1(L"/var/log/scx");
        SCXFilePath logfilepath2(L"/var/log/scx2");
        AddUserNameToPath(logfilepath1);
        AddUserNameToPath(logfilepath2);
        for (std::set<SCXHandle<SCXLogItemConsumerIf>, TestMediator::HandleCompare>::const_iterator iter = testMediator->m_Consumers.begin();
             iter != testMediator->m_Consumers.end();
             ++iter)
        {
            SCXLogStdoutBackend* b = dynamic_cast<SCXLogStdoutBackend*>(iter->GetData());
            if (b != 0)
            {
                bStdout = b;
            }
            else
            {
                SCXLogFileBackend* backend = dynamic_cast<SCXLogFileBackend*>(iter->GetData());
                CPPUNIT_ASSERT(backend != 0);
                if (logfilepath1 == backend->GetFilePath())
                {
                    b1 = backend;
                }
                else if (logfilepath2 == backend->GetFilePath())
                {
                    b2 = backend;
                }
            }
        }
        CPPUNIT_ASSERT(0 != b1 && 0 != b2);
        CPPUNIT_ASSERT(0 != bStdout);

        CPPUNIT_ASSERT(eTrace == b1->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(eError == b2->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(eWarning == bStdout->GetEffectiveSeverity(L""));
        
        SCXFile::Delete(configFilePath);
    }

    void TestSetSeverity()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L")");
        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx2");
        confFileContent.push_back(L"MODULE: ERROR");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        SCXLogFileConfigurator configurator(testMediator, configFilePath);

        unsigned int configVersionBefore = configurator.GetConfigVersion();
        CPPUNIT_ASSERT(configVersionBefore > 0);
        configurator.SetSeverityThreshold(L"", eWarning);
        CPPUNIT_ASSERT(configVersionBefore != configurator.GetConfigVersion());

        // Check that severities have been changed.
        std::set<SCXHandle<SCXLogItemConsumerIf>, TestMediator::HandleCompare>::const_iterator iter = testMediator->m_Consumers.begin();
        SCXLogFileBackend* backend = dynamic_cast<SCXLogFileBackend*>(iter->GetData());
        CPPUNIT_ASSERT(backend != 0);
        CPPUNIT_ASSERT(eWarning == backend->GetEffectiveSeverity(L""));
        ++iter;
        backend = dynamic_cast<SCXLogFileBackend*>(iter->GetData());
        CPPUNIT_ASSERT(backend != 0);
        CPPUNIT_ASSERT(eWarning == backend->GetEffectiveSeverity(L""));

        // If we do a "non-change" then the config version should not change.
        configVersionBefore = configurator.GetConfigVersion();
        configurator.SetSeverityThreshold(L"", eWarning);
        CPPUNIT_ASSERT(configVersionBefore == configurator.GetConfigVersion());
        
        // Now let's try changing severity level for a submodule.
        // This should actually be considered as a change.
        configVersionBefore = configurator.GetConfigVersion();
        configurator.SetSeverityThreshold(L"scx", eWarning);
        CPPUNIT_ASSERT(configVersionBefore != configurator.GetConfigVersion());
        // This should not be considered as a change.
        configVersionBefore = configurator.GetConfigVersion();
        configurator.SetSeverityThreshold(L"scx", eWarning);
        CPPUNIT_ASSERT(configVersionBefore == configurator.GetConfigVersion());

        // Now let's try to clear severity level for a submodule.
        // This should actually be considered as a change.
        configVersionBefore = configurator.GetConfigVersion();
        configurator.ClearSeverityThreshold(L"scx");
        CPPUNIT_ASSERT(configVersionBefore != configurator.GetConfigVersion());
        // This should not be considered as a change.
        configVersionBefore = configurator.GetConfigVersion();
        configurator.ClearSeverityThreshold(L"scx");
        CPPUNIT_ASSERT(configVersionBefore == configurator.GetConfigVersion());
        
        SCXFile::Delete(configFilePath);
    }
    
    void TestThreadSafe()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        SCXFilePath configFilePath(L"test_log_file_configurator");
        SCXCoreLib::SCXThreadLockHandle lockH = SCXCoreLib::ThreadLockHandleGet();
        SCXLogFileConfigurator configurator(testMediator, configFilePath, lockH);

        SCXCoreLib::SCXThreadLock lock(lockH);

        CPPUNIT_ASSERT_THROW(configurator.SetSeverityThreshold(L"something.something", eWarning), SCXCoreLib::SCXThreadLockHeldException);
        CPPUNIT_ASSERT_THROW(configurator.RestoreConfiguration(), SCXCoreLib::SCXThreadLockHeldException);
    }

    void TestReconfigureNoConfigToSimpleConfig()
    {
        // Start with default configuration.
        SCXHandle<TestMediator> testMediator( new TestMediator() );

        SCXFilePath configFilePath(L"test_log_file_configurator");
        SCXFile::Delete(configFilePath);
        SCXLogFileConfigurator configurator(testMediator, configFilePath);

        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);

        SCXHandle<SCXLogItemConsumerIf> backend = *(testMediator->m_Consumers.begin());
        CPPUNIT_ASSERT(backend != 0);
        SCXLogFileBackend* fbackend;
        fbackend = dynamic_cast<SCXLogFileBackend*>(backend.GetData());
        CPPUNIT_ASSERT(fbackend != 0);
        CPPUNIT_ASSERT(eInfo == fbackend->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(CustomLogPolicyFactory()->GetDefaultSeverityThreshold() == fbackend->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(CustomLogPolicyFactory()->GetDefaultLogFileName() == fbackend->GetFilePath());

        unsigned int configVersion = configurator.GetConfigVersion();

        // Then reconfigure the log framework.

        std::vector<std::wstring> confFileContent;
        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: test_log_file");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal WARNING");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common HYSTERICAL");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common.entityenumeration INFO");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        configurator.RestoreConfiguration();
        
        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);
        
        backend = *(testMediator->m_Consumers.begin());
        CPPUNIT_ASSERT(backend != 0);
        fbackend = dynamic_cast<SCXLogFileBackend*>(backend.GetData());
        CPPUNIT_ASSERT(fbackend != 0);
        CPPUNIT_ASSERT(L"test_log_file" == fbackend->GetFilePath().GetFilename());

        CPPUNIT_ASSERT(eTrace == fbackend->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal"));
        CPPUNIT_ASSERT(eHysterical == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common"));
        CPPUNIT_ASSERT(eInfo == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common.entityenumeration"));

        CPPUNIT_ASSERT(configVersion != configurator.GetConfigVersion());

        SCXFile::Delete(configFilePath);
    }

    void TestAutomaticReconfigure()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        SCXFilePath configFilePath(L"test_log_file_configurator");
        SCXFilePath configFilePathTmp(L"test_log_file_configurator_tmp");

        std::vector<std::wstring> confFileContent;
        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal WARNING");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common HYSTERICAL");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common.entityenumeration INFO");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        SCXThreadLockHandle lockH = ThreadLockHandleGet();
        SCXLogFileConfigurator configurator(testMediator, configFilePath, lockH, 100);

        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);

        SCXHandle<SCXLogItemConsumerIf> backend = *(testMediator->m_Consumers.begin());
        CPPUNIT_ASSERT(backend != 0);
        SCXLogFileBackend* fbackend;
        fbackend = dynamic_cast<SCXLogFileBackend*>(backend.GetData());
        CPPUNIT_ASSERT(fbackend != 0);
        SCXFilePath logfilepath(L"/var/log/scx");
        AddUserNameToPath(logfilepath);
        CPPUNIT_ASSERT(logfilepath == fbackend->GetFilePath());

        CPPUNIT_ASSERT(eTrace == fbackend->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal"));
        CPPUNIT_ASSERT(eHysterical == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common"));
        CPPUNIT_ASSERT(eInfo == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common.entityenumeration"));

        unsigned int configVersion = configurator.GetConfigVersion();
        confFileContent.clear();
        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: WARNING");
        confFileContent.push_back(L")\n");

#if defined(WIN32)
        // We have a timing window: If the file is deleted, and then the
        // configuration reader thread kicks off, it will sense no config
        // file and reload the default configuration.  However, this test
        // will then fail because we must wait another "update cycle"
        // (100ms for this test) before the new configuation file is loaded
        // (from the SCXFile::Move() command).  Solution: Wait for another
        // update cycle for the "final" configuration to be loaded.
        //
        // This could have been eliminated by doing an atomic rename of a
        // new log file config file to the "real" log file configuration,
        // but Windows doesn't allow atomic rename of files if the
        // destination file already exists.  So there, we delete the
        // existing configuration file first.  Note that we need time
        // for the log reader to pick up that change, but that's easily
        // handled by the 'SCXThread::Sleep(1000)' below (to wait for a
        // full second to elapse).
        //
        // See wi11088 for more information.

        SCXFile::Delete(configFilePath);
#endif

        // We need to get a new date on the file so we wait a second.
        SCXThread::Sleep(1000);

        SCXFile::WriteAllLinesAsUTF8(configFilePathTmp, confFileContent, std::ios_base::out);
        SCXFile::Move(configFilePathTmp, configFilePath);

#if defined(WIN32)
        // On Windows, we got a new "default" configuration by deleting
        // the log reader configuration file.  So wait for the final
        // configuration to get loaded.  This is needed because the
        // configuration version is already bumped due to the default
        // configuration being loaded.

        SCXThread::Sleep(150);
#endif

        for (int tries = 0; configVersion == configurator.GetConfigVersion(); ++tries)
        {
            SCXThread::Sleep(100);
            if (tries == 10)
            {
                CPPUNIT_FAIL("Configuration is not updated automatically");
            }
        }

        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);

        backend = *(testMediator->m_Consumers.begin());
        CPPUNIT_ASSERT(backend != 0);
        fbackend = dynamic_cast<SCXLogFileBackend*>(backend.GetData());
        CPPUNIT_ASSERT(fbackend != 0);

        if (logfilepath != fbackend->GetFilePath())
        {
            std::wcout << L"Logfilepath: " << logfilepath.DumpString()
                       << L", Version: " << configVersion << std::endl;
            std::wcout << L"fbackend->GetFilePath(): "
                       << fbackend->GetFilePath().DumpString() << L", Version: "
                       << configurator.GetConfigVersion() << std::endl;
        }
        CPPUNIT_ASSERT(logfilepath == fbackend->GetFilePath());

        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L""));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal"));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common"));
        CPPUNIT_ASSERT(eWarning == fbackend->GetEffectiveSeverity(L"scxtest.core.common.pal.system.common.entityenumeration"));
    }

    void TestAutomaticReconfigureNoFile()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        SCXFilePath configFilePath(L"this_file_should_not_exist");
        SCXThreadLockHandle lockH = ThreadLockHandleGet();
        SCXLogFileConfigurator configurator(testMediator, configFilePath, lockH, 100);

        unsigned int configVersion = configurator.GetConfigVersion();

        for (int tries = 0; tries < 3; ++tries)
        {
            SCXThread::Sleep(100);
            if (configVersion != configurator.GetConfigVersion())
            {
                CPPUNIT_FAIL("Configuration is updated when it should not");
            }
        }
    }

    void TestInvalidConfigurationFile()
    {
        SCXHandle<TestMediator> testMediator( new TestMediator() );
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal WARNING");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common HYSTERICAL");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common.entityenumeration INFO");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        SCXLogFileConfigurator configurator(testMediator, configFilePath);

        CPPUNIT_ASSERT(testMediator->m_Consumers.size() == 1);

        SCXFile::Delete(configFilePath);
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogFileConfiguratorTest );
