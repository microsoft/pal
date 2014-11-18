/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.
    
  Created date    2014-09-29 15:15:00

  Tests for ConfigFile class.
    
*/
/*----------------------------------------------------------------------------*/

#include <ostream>
#include <string>
#include <vector>

#include <scxcorelib/scxconfigfile.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <testutils/scxunit.h>

using std::wstring;
using namespace SCXCoreLib;

class TestableSCXConfigFile : public SCXConfigFile
{
public:
    TestableSCXConfigFile(SCXFilePath path) : SCXConfigFile(path)
    {}

    bool GetLoadedFlag()
    {
        return m_configLoaded;
    }

    size_t GetNumberOfEntries()
    {
        return m_config.size();
    }
};

class SCXConfigFileTest : public CPPUNIT_NS::TestFixture 
{
    CPPUNIT_TEST_SUITE( SCXConfigFileTest );

    CPPUNIT_TEST( TestLoadFilePath );
    CPPUNIT_TEST( TestLoadEmpty );
    CPPUNIT_TEST( TestLoadEmptyKey );
    CPPUNIT_TEST( TestLoadDupKey );
    CPPUNIT_TEST( TestLoadSaveNonexistent );

    CPPUNIT_TEST( TestUseBeforeLoad );
    CPPUNIT_TEST( TestSave );
    CPPUNIT_TEST( TestKeyExists );
    CPPUNIT_TEST( TestGetValue );
    CPPUNIT_TEST( TestGetNonexistentValue );
    CPPUNIT_TEST( TestOverwriteValue );
    CPPUNIT_TEST( TestNewValue );
    CPPUNIT_TEST( TestSurroundingSpacesRemoved );
    CPPUNIT_TEST( TestDeleteEntry );
    CPPUNIT_TEST( TestDeleteNonexistentEntry );
    CPPUNIT_TEST( TestIteration );

    CPPUNIT_TEST_SUITE_END();
private:
    SCXFilePath pathTestEmptyFile, pathTestEmptyKey, pathTestUsual, pathTestDupKey;
    wstring tmp;

public:
    SCXConfigFileTest() : pathTestEmptyFile(L"testfiles/scxconfigfile-test-emptyfile.txt"),
                          pathTestEmptyKey(L"testfiles/scxconfigfile-test-emptykey.txt"),
                          pathTestUsual(L"testfiles/scxconfigfile-test-usual.txt"),
                          pathTestDupKey(L"testfiles/scxconfigfile-test-dupkey.txt")
    {}

    void setUp()
    {
        tmp = L"";
        std::vector<wstring> lines1;
        SCXFile::WriteAllLinesAsUTF8(pathTestEmptyFile, lines1, std::ios_base::out);
        
        lines1.push_back(L"=value");
        lines1.push_back(L"normal key=after empty key");
        SCXFile::WriteAllLinesAsUTF8(pathTestEmptyKey, lines1, std::ios_base::out);

        std::vector<wstring> lines2;
        lines2.push_back(L"key=value");
        lines2.push_back(L"equal=a=value");
        lines2.push_back(L" keystrip    =       valuestrip   ");
        lines2.push_back(L"key space=value space");
        lines2.push_back(L"empty=");
        lines2.push_back(L"");
        lines2.push_back(L"skip=line");
        SCXFile::WriteAllLinesAsUTF8(pathTestUsual, lines2, std::ios_base::out);
        
        lines2.push_back(L"key=value");
        SCXFile::WriteAllLinesAsUTF8(pathTestDupKey, lines2, std::ios_base::out);
    }

    void tearDown()
    {
        SCXFile::Delete(pathTestEmptyFile.Get());
        SCXFile::Delete(pathTestEmptyKey.Get());
        SCXFile::Delete(pathTestUsual.Get());
        SCXFile::Delete(pathTestDupKey.Get());
    }

    void TestLoadFilePath()
    {
        TestableSCXConfigFile config(pathTestUsual);
        CPPUNIT_ASSERT_NO_THROW(config.LoadConfig());
        CPPUNIT_ASSERT_EQUAL(true, config.GetLoadedFlag());
        CPPUNIT_ASSERT_EQUAL((size_t) 6, config.GetNumberOfEntries());
    }

    void TestLoadEmpty()
    {
        TestableSCXConfigFile config(pathTestEmptyFile);
        CPPUNIT_ASSERT_NO_THROW(config.LoadConfig());
        CPPUNIT_ASSERT_EQUAL(true, config.GetLoadedFlag());
        CPPUNIT_ASSERT_EQUAL((size_t) 0, config.GetNumberOfEntries());
    }

    void TestLoadEmptyKey()
    {
        TestableSCXConfigFile config(pathTestEmptyKey);
        
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.LoadConfig(), SCXInvalidConfigurationFile, L"Empty key");
        
        // The config should still load if there is a parsing error
        CPPUNIT_ASSERT_EQUAL(true, config.GetLoadedFlag());
        CPPUNIT_ASSERT_EQUAL((size_t) 1, config.GetNumberOfEntries());
    }

    void TestLoadDupKey()
    {
        TestableSCXConfigFile config(pathTestDupKey);
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.LoadConfig(), SCXInvalidConfigurationFile, L"Duplicate key");

        // The config should still load if there is a parsing error
        CPPUNIT_ASSERT_EQUAL(true, config.GetLoadedFlag());
        CPPUNIT_ASSERT_EQUAL((size_t) 6, config.GetNumberOfEntries());
    }

    void TestLoadSaveNonexistent()
    {
        TestableSCXConfigFile config(SCXFilePath(L"imaginaryDirectory/config"));
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.LoadConfig(), SCXFilePathNotFoundException, L"No item found");
        CPPUNIT_ASSERT_EQUAL(true, config.GetLoadedFlag());
        CPPUNIT_ASSERT_EQUAL((size_t) 0, config.GetNumberOfEntries());
    }

    void TestUseBeforeLoad()
    {
        TestableSCXConfigFile config(pathTestUsual);
        // Forget to load the config file
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.SaveConfig(), SCXInvalidStateException, L"loaded before");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.GetValue(L"key", tmp), SCXInvalidStateException, L"loaded before");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.SetValue(L"key", L"nope"), SCXInvalidStateException, L"loaded before");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.DeleteEntry(L"key"), SCXInvalidStateException, L"loaded before");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.KeyExists(L"key"), SCXInvalidStateException, L"loaded before");
        SCXUNIT_ASSERTIONS_FAILED(5);

        CPPUNIT_ASSERT_EQUAL(false, config.GetLoadedFlag());
        CPPUNIT_ASSERT_EQUAL((size_t)0, config.GetNumberOfEntries());
    }

    // Returns a single wstring representing the vector for debug and comparison purposes
    wstring vectorString(const std::vector<wstring> &v)
    {
        std::wostringstream ss;
        for (unsigned int i=0; i<v.size(); i++)
            ss<<v[i]<<std::endl;
        return ss.str();
    }

    void TestSave()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();
        config.SetValue(L"key", L"new value");
        config.SetValue(L"new key", L"other value");
        config.SaveConfig();
        
        std::vector<wstring> lines_expected;
        lines_expected.push_back(L"empty=");
        lines_expected.push_back(L"equal=a=value");
        lines_expected.push_back(L"key=new value");
        lines_expected.push_back(L"key space=value space");
        lines_expected.push_back(L"keystrip=valuestrip");
        lines_expected.push_back(L"new key=other value");
        lines_expected.push_back(L"skip=line");

        std::vector<wstring> lines_actual;
        SCXStream::NLFs nlfs;
        SCXFile::ReadAllLinesAsUTF8(pathTestUsual, lines_actual, nlfs);
        CPPUNIT_ASSERT_EQUAL(vectorString(lines_expected), vectorString(lines_actual));
    }

    void TestKeyExists() 
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();

        CPPUNIT_ASSERT_EQUAL(true, config.KeyExists(L"key"));
        config.DeleteEntry(L"key");
        CPPUNIT_ASSERT_EQUAL(false, config.KeyExists(L"key"));
        
        CPPUNIT_ASSERT_EQUAL(false, config.KeyExists(L"newkey")); 
        config.SetValue(L"newkey", L"newval");
        CPPUNIT_ASSERT_EQUAL(true, config.KeyExists(L"newkey"));
    }

    void TestGetValue()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();

        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"key", tmp));
        CPPUNIT_ASSERT_EQUAL(L"value", tmp );

        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"key space", tmp));
        CPPUNIT_ASSERT_EQUAL(L"value space", tmp );

        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"empty", tmp));
        CPPUNIT_ASSERT_EQUAL(L"", tmp);

        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"equal", tmp));
        CPPUNIT_ASSERT_EQUAL(L"a=value", tmp);

        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"skip", tmp));
        CPPUNIT_ASSERT_EQUAL(L"line", tmp);
    }

    void TestGetNonexistentValue()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();
        
        CPPUNIT_ASSERT_EQUAL(false, config.GetValue(L"not a key", tmp));
    }

    void TestOverwriteValue()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();
        
        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"key", tmp));
        CPPUNIT_ASSERT_EQUAL(L"value", tmp );

        config.SetValue(L"key", L"new value");
        
        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"key", tmp));
        CPPUNIT_ASSERT_EQUAL(L"new value", tmp );
    }

    void TestNewValue()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();

        config.SetValue(L"new key", L"new value");
        CPPUNIT_ASSERT_EQUAL(true, config.KeyExists(L"new key"));
        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"new key", tmp));
        CPPUNIT_ASSERT_EQUAL(L"new value", tmp );
    }

    void TestSurroundingSpacesRemoved()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();

        CPPUNIT_ASSERT_EQUAL(true, config.GetValue(L"keystrip", tmp));
        CPPUNIT_ASSERT_EQUAL(L"valuestrip", tmp );
    }

    void TestDeleteEntry()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();

        CPPUNIT_ASSERT_EQUAL(true, config.KeyExists(L"key"));
        CPPUNIT_ASSERT_NO_THROW(config.DeleteEntry(L"key"));
        CPPUNIT_ASSERT_EQUAL(false, config.KeyExists(L"key"));
    }

    void TestDeleteNonexistentEntry()
    {
        SCXConfigFile config(pathTestUsual);
        config.LoadConfig();
        SCXUNIT_ASSERT_THROWN_EXCEPTION(config.DeleteEntry(L"not a key"), SCXInvalidArgumentException, L"not found");
        SCXUNIT_ASSERTIONS_FAILED_ANY();
    }

    void TestIteration()
    {
        TestableSCXConfigFile config(pathTestUsual);
        CPPUNIT_ASSERT_NO_THROW(config.LoadConfig());
        CPPUNIT_ASSERT_EQUAL(true, config.GetLoadedFlag());
        CPPUNIT_ASSERT_EQUAL((size_t) 6, config.GetNumberOfEntries());

        std::vector<wstring> expectedKeys;
        expectedKeys.push_back(L"empty");
        expectedKeys.push_back(L"equal");
        expectedKeys.push_back(L"key");
        expectedKeys.push_back(L"key space");
        expectedKeys.push_back(L"keystrip");
        expectedKeys.push_back(L"skip");

        int counter = 0;
        for (std::map<wstring,wstring>::const_iterator it = config.begin(); it != config.end(); ++it)
        {
            CPPUNIT_ASSERT_EQUAL_MESSAGE( "Differing key for element " + StrToUTF8(StrFrom(counter)),
                                          expectedKeys[counter], it->first );
            counter++;
        }
    }

};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXConfigFileTest );
