/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.
    
    Created date    2008-07-24 14:01:13

    Log file configurator test
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include "scxcorelib/util/log/scxlogconfigreader.h"
#include <testutils/scxunit.h>
#include <scxcorelib/scxfile.h>

using namespace SCXCoreLib;


struct BackendStub {
        
    bool IsInitialized() { return true;}
    bool SetProperty(const std::wstring& , const std::wstring& ){
        return true;
    }
    
};

struct ConfigConsumerInterfaceStub {
    int nCounter;
    std::wstring lastName;

    ConfigConsumerInterfaceStub() : nCounter(0){}
    SCXHandle< BackendStub > Create( const std::wstring& name ) 
    {
        if ( name == L"FILE (" ) {
            lastName = name;
            nCounter++;
            return SCXHandle< BackendStub >( new BackendStub );
        }
        return SCXHandle< BackendStub >(0);
    }
    
    void Add( SCXHandle< BackendStub >  ) 
    {
    }
    
    bool SetSeverityThreshold( SCXHandle< BackendStub > , const std::wstring& , SCXLogSeverity ) 
    {
        return true;
    }
    
};


class SCXLogConfigReaderTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXLogConfigReaderTest );
    CPPUNIT_TEST( TestNoConfigurationFile );
    CPPUNIT_TEST( TestConfigurationFile );
    CPPUNIT_TEST( TestInvalidConfigurationFile );
    CPPUNIT_TEST( TestInvalidConfigurationFile2 );
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
        ConfigConsumerInterfaceStub oConsumer;
        SCXLogConfigReader< BackendStub, ConfigConsumerInterfaceStub > oParser;
        
        SCXFilePath configFilePath(L"this_file_should_not_exist");
        
        CPPUNIT_ASSERT(!oParser.ParseConfigFile(configFilePath, &oConsumer ));
        
    }

    void TestConfigurationFile()
    {
        // parse basic valid config file
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


        ConfigConsumerInterfaceStub oConsumer;
        SCXLogConfigReader< BackendStub, ConfigConsumerInterfaceStub > oParser;
        
        CPPUNIT_ASSERT( oParser.ParseConfigFile(configFilePath, &oConsumer ) );
        CPPUNIT_ASSERT( oConsumer.nCounter == 1 );
        CPPUNIT_ASSERT( oConsumer.lastName == L"FILE (" );

        SCXFile::Delete(configFilePath);
    }

    void TestInvalidConfigurationFile()
    {
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"FILE (");
        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal WARNING");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common HYSTERICAL");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal.system.common.entityenumeration INFO");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        ConfigConsumerInterfaceStub oConsumer;
        SCXLogConfigReader< BackendStub, ConfigConsumerInterfaceStub > oParser;

        // since config file is incomplte, it should return "false", although it creates some entries
        CPPUNIT_ASSERT( !oParser.ParseConfigFile(configFilePath, &oConsumer ) );
        CPPUNIT_ASSERT( oConsumer.nCounter == 1 );
        CPPUNIT_ASSERT( oConsumer.lastName == L"FILE (" );
        

        SCXFile::Delete(configFilePath);
    }
    
    void TestInvalidConfigurationFile2()
    {
        std::vector<std::wstring> confFileContent;
        SCXFilePath configFilePath(L"test_log_file_configurator");

        confFileContent.push_back(L"PATH: /var/log/scx");
        confFileContent.push_back(L"MODULE: TRACE");
        confFileContent.push_back(L"MODULE: scxtest.core.common.pal WARNING");
        confFileContent.push_back(L")\n");
        SCXFile::WriteAllLinesAsUTF8(configFilePath, confFileContent, std::ios_base::out);

        ConfigConsumerInterfaceStub oConsumer;
        SCXLogConfigReader< BackendStub, ConfigConsumerInterfaceStub > oParser;

        // there are no "back-end-section-start" tag
        CPPUNIT_ASSERT( !oParser.ParseConfigFile(configFilePath, &oConsumer ) );
        CPPUNIT_ASSERT( oConsumer.nCounter == 0 );
        

        SCXFile::Delete(configFilePath);
    }
    
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXLogConfigReaderTest );
