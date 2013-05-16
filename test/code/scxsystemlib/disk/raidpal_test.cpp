/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Tests for the RAID PAL.

    \date        2008-03-28 09:50:00

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/scxraid.h>
#include <cppunit/extensions/HelperMacros.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>

class SCXRaidTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXRaidTest );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( TestRaidCfgParserDefaults );
    CPPUNIT_TEST( TestMD_CF_ParsingConcatStripes );
    CPPUNIT_TEST( TestMD_CF_ParsingMirrors );
    CPPUNIT_TEST( TestMD_CF_WI15110 );
    CPPUNIT_TEST( TestMD_CF_ParsingHotSpares );
    CPPUNIT_TEST( TestMD_CF_ParsingTransMetaDevices );
    CPPUNIT_TEST( TestMD_CF_ParsingRAID5 );
    CPPUNIT_TEST( TestMD_CF_ParsingSoftPartitions_CompleteDisk );
    CPPUNIT_TEST( TestMD_CF_ParsingSoftPartitions_Extents );
    CPPUNIT_TEST( TestMD_CF_ParsingSoftPartitions_ExtentsOnMetaDevice );
    CPPUNIT_TEST( TestMD_CF_FilesFromTAPusers );
    CPPUNIT_TEST( TestMD_CF_SoftPartitionConfigFromCustomer1a_bug14262 );
    CPPUNIT_TEST( TestMD_CF_SoftPartitionConfigFromCustomer1b_bug14262 );
    CPPUNIT_TEST( TestMD_CF_SoftPartitionConfigFromCustomer2_bug14557 );
    CPPUNIT_TEST( TestMD_CF_ParsingStateDatabaseReplicas );
    CPPUNIT_TEST( TestMD_CF_ParsingInvalidOptionsShouldIgnoreLine );
    CPPUNIT_TEST( TestMD_CF_ParsingInvalidLineShouldIgnoreLine );
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaid> m_pRaid;

    class TestRaidCfgParser : public SCXSystemLib::SCXRaidCfgParserDefault
    {
    public:
        SCXCoreLib::SCXFilePath m_path;
        bool m_keepFile;

        TestRaidCfgParser()
            : m_path(L"test_md.cf"), m_keepFile(false)
        {
        }

        TestRaidCfgParser(const std::wstring& path)
            : m_path(path), m_keepFile(true)
        {
        }

        virtual ~TestRaidCfgParser()
        {
            if ( ! m_keepFile && SCXCoreLib::SCXFile::Exists(m_path))
                SCXCoreLib::SCXFile::Delete(m_path);
        }

        virtual const SCXCoreLib::SCXFilePath& GetConfPath() const 
        {
            return m_path;
        }
    };

    // Returns a TestRaidCfgParser and writes the temporary test file we want to use,
    SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> PrepareTest(const std::wstring& cfg)
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new TestRaidCfgParser() );
        SCXCoreLib::SCXHandle<std::wfstream> fs = SCXCoreLib::SCXFile::OpenWFstream(deps->GetConfPath(), std::ios_base::out | std::ios_base::trunc);
        *fs << cfg;
        fs->close();
        return deps;
    }

    // Will remove items from vectors that are found in both vectors.
    void VerifyVector(std::vector<std::wstring>& expected, std::vector<std::wstring>& test)
    {
        std::vector<std::wstring>::iterator e = expected.begin();
        while (e != expected.end())
        {
            bool found = false;
            for (std::vector<std::wstring>::iterator t = test.begin();
                 ! found && t != test.end(); ++t)
            {
                if (*e == *t)
                {
                    test.erase(t);
                    e = expected.erase(e);
                    found = true;
                }
            }
            if ( ! found)
            {
                ++e;
            }
        }
    }

    std::string PrintableVector(const std::vector<std::wstring>& v)
    {
        std::wstring s = L"";
        for (std::vector<std::wstring>::const_iterator it = v.begin();
             it != v.end(); ++it)
        {
            if (s.length() > 0)
            {
                s.append(L", ");
            }
            s.append(*it);
        }
        return SCXCoreLib::StrToUTF8(s);
    }

public:
    void setUp(void)
    {
        m_pRaid = 0;
    }

    void tearDown(void)
    {
        m_pRaid = 0;
    }

    void callDumpStringForCoverage()
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new SCXSystemLib::SCXRaidCfgParserDefault() );
        m_pRaid = new SCXSystemLib::SCXRaid(deps);
        CPPUNIT_ASSERT(m_pRaid->DumpString().find(L"SCXRaid") != std::wstring::npos);
    }

    void TestRaidCfgParserDefaults()
    {
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new SCXSystemLib::SCXRaidCfgParserDefault() );
        CPPUNIT_ASSERT(deps->GetConfPath().Get() == L"/etc/lvm/md.cf");
    }

    void TestMD_CF_ParsingConcatStripes(void)
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"#complete line comment\nd1 1 2 d111 d112 -i 32k # comment at end of line\nd2 4 1 d211 1 d221 1 d231 1 d241\nd3 2 3  d311 d312 d313 -i 16k \\\n3 d321 d322 d323 -i 32k\nd4 3 1 d411 \\\n\\\n     1 d421 \\ #Comment!\n\t2 d431 d432\n";


        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), md.size());
        v.push_back(L"d1");
        v.push_back(L"d2");
        v.push_back(L"d3");
        v.push_back(L"d4");        
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"d111");
        v.push_back(L"d112");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d2", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), md.size());
        v.push_back(L"d211");
        v.push_back(L"d221");
        v.push_back(L"d231");
        v.push_back(L"d241");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d3", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(6), md.size());
        v.push_back(L"d311");
        v.push_back(L"d312");
        v.push_back(L"d313");
        v.push_back(L"d321");
        v.push_back(L"d322");
        v.push_back(L"d323");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d4", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), md.size());
        v.push_back(L"d411");
        v.push_back(L"d421");
        v.push_back(L"d431");
        v.push_back(L"d432");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_ParsingMirrors(void)
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d50 -m d51 d52 1 # Last one seen in customer config\n";
        cfg.append(L"d51 1 1 c0t0d0s5\n");
        cfg.append(L"d52 1 1 c0t1d0s5\n");

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), md.size());
        v.push_back(L"d50");
        v.push_back(L"d51");
        v.push_back(L"d52");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d50", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"c0t0d0s5");
        v.push_back(L"c0t1d0s5");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d51", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c0t0d0s5");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d52", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c0t1d0s5");
    }

    void TestMD_CF_WI15110(void)
    {
        std::vector<std::wstring> v;

        std::wstring cfg = L"d0 -m d10 d11 1\n";
        cfg.append(L"d10 1 1 c1t0d0s0\n");
        cfg.append(L"d11 1 1 c1t1d0s0\n");
        cfg.append(L"d1 -m d20 d21 1\n");
        cfg.append(L"d20 1 1 c1t0d0s1\n");
        cfg.append(L"d21 1 1 c1t1d0s1\n");
        cfg.append(L"d3 -m d30 d31 1\n");
        cfg.append(L"d30 1 1 c1t0d0s3\n");
        cfg.append(L"d31 1 1 c1t1d0s3\n");
        cfg.append(L"d4 -m d40 d41 1\n");
        cfg.append(L"d40 1 1 c1t0d0s4\n");
        cfg.append(L"d41 1 1 c1t1d0s4\n");
        cfg.append(L"d7 -m d70 d71 1\n");
        cfg.append(L"d70 1 1 c1t0d0s7\n");
        cfg.append(L"d71 1 1 c1t1d0s7\n");
        cfg.append(L"hsp001\n");

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        v.push_back(L"d0");
        v.push_back(L"d10");
        v.push_back(L"d11");
        v.push_back(L"d1");
        v.push_back(L"d20");
        v.push_back(L"d21");
        v.push_back(L"d3");
        v.push_back(L"d30");
        v.push_back(L"d31");
        v.push_back(L"d4");
        v.push_back(L"d40");
        v.push_back(L"d41");
        v.push_back(L"d7");
        v.push_back(L"d70");
        v.push_back(L"d71");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d0", md));
        v.push_back(L"c1t0d0s0");
        v.push_back(L"c1t1d0s0");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d10", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t0d0s0");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d11", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t1d0s0");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        v.push_back(L"c1t0d0s1");
        v.push_back(L"c1t1d0s1");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d20", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t0d0s1");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d21", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t1d0s1");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d3", md));
        v.push_back(L"c1t0d0s3");
        v.push_back(L"c1t1d0s3");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d30", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t0d0s3");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d31", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t1d0s3");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d4", md));
        v.push_back(L"c1t0d0s4");
        v.push_back(L"c1t1d0s4");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d40", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t0d0s4");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d41", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t1d0s4");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d7", md));
        v.push_back(L"c1t0d0s7");
        v.push_back(L"c1t1d0s7");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d70", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t0d0s7");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d71", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"c1t1d0s7");
    }

    void TestMD_CF_ParsingHotSpares()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d1 1 1 d111 -h hsp1\n";
        cfg.append(L"hsp1 h1 h2\n");

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), md.size());
        v.push_back(L"d111");
        v.push_back(L"h1");
        v.push_back(L"h2");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_ParsingTransMetaDevices()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d1 -t d11 d12\n";
        cfg.append(L"d11 -m d111 \n");
        cfg.append(L"d111 1 1 d1111\n");
        cfg.append(L"d112 1 1 d1121 # Prepared but not used\n");
        cfg.append(L"d12 -m d121 \n");
        cfg.append(L"d121 1 1 d1211\n");
        cfg.append(L"d122 1 1 d1221 # Prepared but not used\n");

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(7), md.size());
        v.push_back(L"d1");
        v.push_back(L"d11");
        v.push_back(L"d12");
        v.push_back(L"d111");
        v.push_back(L"d112");
        v.push_back(L"d121");
        v.push_back(L"d122");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"d1111");
        v.push_back(L"d1211");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d11", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1111");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d111", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1111");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d112", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1121");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d12", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1211");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d121", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1211");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d122", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1221");
    }

    void TestMD_CF_ParsingRAID5()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d0 -r d1 d2 d3 -i 20k\n";
        
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d0");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d0", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), md.size());
        v.push_back(L"d1");
        v.push_back(L"d2");
        v.push_back(L"d3");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_FilesFromTAPusers()
    {
        std::vector<std::wstring> v;
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new TestRaidCfgParser(L"./testfiles/tap1.md.cf") );

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(18), md.size());
        v.push_back(L"d100");
        v.push_back(L"d50");
        v.push_back(L"d30");
        v.push_back(L"d10");
        v.push_back(L"d0");
        v.push_back(L"d60");
        v.push_back(L"d101");
        v.push_back(L"d102");
        v.push_back(L"d51");
        v.push_back(L"d52");
        v.push_back(L"d31");
        v.push_back(L"d32");
        v.push_back(L"d11");
        v.push_back(L"d12");
        v.push_back(L"d1");
        v.push_back(L"d2");
        v.push_back(L"d61");
        v.push_back(L"d62");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        
        // Only testing the mirrored devices not mirror parts.
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d100", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"c0t2d0s0");
        v.push_back(L"c0t3d0s0");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d50", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"c0t0d0s5");
        v.push_back(L"c0t1d0s5");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d30", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"c0t0d0s3");
        v.push_back(L"c0t1d0s3");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d10", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"c0t0d0s1");
        v.push_back(L"c0t1d0s1");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d0", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"c0t0d0s0");
        v.push_back(L"c0t1d0s0");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d60", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"c0t0d0s6");
        v.push_back(L"c0t1d0s6");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_ParsingSoftPartitions_CompleteDisk()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d85 -p -e c3t4d0 9g";

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        v.push_back(L"d85");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d85", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        v.push_back(L"c3t4d0");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_ParsingSoftPartitions_Extents()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d1 -p c0t3d0s0 -o 20483 -b 20480 -o 135398 -b 20480";

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        v.push_back(L"d1");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        v.push_back(L"c0t3d0s0");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_ParsingSoftPartitions_ExtentsOnMetaDevice()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d1 -p d2 -o 1 -b 1\n";
        cfg.append(L"d2 -m d21 d22 1\n");
        cfg.append(L"d21 1 1 dev1\n");
        cfg.append(L"d22 1 1 dev2\n");

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(4), md.size());
        v.push_back(L"d1");
        v.push_back(L"d2");
        v.push_back(L"d21");
        v.push_back(L"d22");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
        v.push_back(L"dev1");
        v.push_back(L"dev2");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_SoftPartitionConfigFromCustomer1a_bug14262()
    {
        std::vector<std::wstring> v;
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new TestRaidCfgParser(L"./testfiles/bug14262a.md.cf") );

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(12), md.size());
        v.push_back(L"d0");
        v.push_back(L"d10");
        v.push_back(L"d20");
        v.push_back(L"d132");
        v.push_back(L"d2");
        v.push_back(L"d12");
        v.push_back(L"d22");
        v.push_back(L"d122");
        v.push_back(L"d112");
        v.push_back(L"d102");
        v.push_back(L"d95");
        v.push_back(L"d142");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        
        // Only testing soft partitions and not their parts
        std::wstring softs[] = { L"d132", L"d122", L"d112", L"d102", L"d95", L"d142", L"" };
        for (int i=0; softs[i].size() > 0; i++)
        {
            CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(softs[i], md));
            CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
            v.push_back(L"c0t0d0s6");
            v.push_back(L"c0t1d0s6");
            VerifyVector(v, md);
            CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
            CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        }
    }
    
    void TestMD_CF_SoftPartitionConfigFromCustomer1b_bug14262()
    {
        std::vector<std::wstring> v;
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new TestRaidCfgParser(L"./testfiles/bug14262b.md.cf") );

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(12), md.size());
        v.push_back(L"d1");
        v.push_back(L"d11");
        v.push_back(L"d21");
        v.push_back(L"d0");
        v.push_back(L"d10");
        v.push_back(L"d20");
        v.push_back(L"d122");
        v.push_back(L"d2");
        v.push_back(L"d12");
        v.push_back(L"d22");
        v.push_back(L"d112");
        v.push_back(L"d102");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        
        // Only testing soft partitions and not their parts
        std::wstring softs[] = { L"d122", L"d112", L"d102", L"" };
        for (int i=0; softs[i].size() > 0; i++)
        {
            CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(softs[i], md));
            CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
            v.push_back(L"c0t0d0s6");
            v.push_back(L"c0t1d0s6");
            VerifyVector(v, md);
            CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
            CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        }
    }
    
    void TestMD_CF_SoftPartitionConfigFromCustomer2_bug14557()
    {
        std::vector<std::wstring> v;
        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps( new TestRaidCfgParser(L"./testfiles/bug14557.md.cf") );

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(16), md.size());
        v.push_back(L"d123");
        v.push_back(L"d13");
        v.push_back(L"d23");
        v.push_back(L"d1");
        v.push_back(L"d11");
        v.push_back(L"d21");
        v.push_back(L"d120");
        v.push_back(L"d10");
        v.push_back(L"d20");
        v.push_back(L"d33");
        v.push_back(L"d30");
        v.push_back(L"d4");
        v.push_back(L"d6");
        v.push_back(L"d16");
        v.push_back(L"d26");
        v.push_back(L"d5");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        
        // Only testing soft partitions and not their parts
        std::wstring softs[] = { L"d4", L"d5", L""};
        for (int i=0; softs[i].size() > 0; i++)
        {
            CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(softs[i], md));
            CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), md.size());
            v.push_back(L"c0t0d0s6");
            v.push_back(L"c0t1d0s6");
            VerifyVector(v, md);
            CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
            CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
        }
    }

    void TestMD_CF_ParsingStateDatabaseReplicas()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"mddb01 -c 3 dev1 dev2 dev3";

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        v.push_back(L"mddb01");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"mddb01", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), md.size());
        v.push_back(L"dev1");
        v.push_back(L"dev2");
        v.push_back(L"dev3");
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);
    }

    void TestMD_CF_ParsingInvalidOptionsShouldIgnoreLine()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d1 -t d11 d12\n";
        cfg.append(L"d11 -m d111 \n");
        cfg.append(L"d111 1 1 d1111\n");
        cfg.append(L"d112 1 1 d1121 -x invalid\n"); // invalid line
        cfg.append(L"d12 -m d121 \n");
        cfg.append(L"d121 1 1 d1211\n");
        cfg.append(L"d122 1 1 d1221 # Prepared but not used\n");

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        v.push_back(L"d1");
        v.push_back(L"d11");
        v.push_back(L"d12");
        v.push_back(L"d111");
        v.push_back(L"d121");
        v.push_back(L"d122");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        v.push_back(L"d1111");
        v.push_back(L"d1211");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d11", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1111");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d111", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1111");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d12", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1211");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d121", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1211");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d122", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1221");
    }

    void TestMD_CF_ParsingInvalidLineShouldIgnoreLine()
    {
        std::vector<std::wstring> v;
        std::wstring cfg = L"d1 -t d11 d12\n";
        cfg.append(L"d11 -m d111 \n");
        cfg.append(L"d111 1 1 d1111\n");
        cfg.append(L"d112 d1\n"); // Invalid line
        cfg.append(L"d12 -m d121 \n");
        cfg.append(L"d121 1 1 d1211\n");
        cfg.append(L"d122 1 1 d1221 # Prepared but not used\n");

        SCXCoreLib::SCXHandle<SCXSystemLib::SCXRaidCfgParser> deps = PrepareTest(cfg);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid = new SCXSystemLib::SCXRaid(deps));
        
        std::vector<std::wstring> md;
        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetMetaDevices(md));
        v.push_back(L"d1");
        v.push_back(L"d11");
        v.push_back(L"d12");
        v.push_back(L"d111");
        v.push_back(L"d121");
        v.push_back(L"d122");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d1", md));
        v.push_back(L"d1111");
        v.push_back(L"d1211");
        CPPUNIT_ASSERT_EQUAL(v.size(), md.size());
        VerifyVector(v, md);
        CPPUNIT_ASSERT_MESSAGE("All expected devices not returned: " + PrintableVector(v), v.size() == 0);
        CPPUNIT_ASSERT_MESSAGE("More devices returned than expected: " + PrintableVector(md), md.size() == 0);

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d11", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1111");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d111", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1111");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d12", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1211");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d121", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1211");

        CPPUNIT_ASSERT_NO_THROW(m_pRaid->GetDevices(L"d122", md));
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), md.size());
        CPPUNIT_ASSERT(md[0] == L"d1221");
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXRaidTest );
