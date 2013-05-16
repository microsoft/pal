/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Tests the persistence framework

    \date        2008-08-21 12:49:46

*/
/*----------------------------------------------------------------------------*/
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxpersistence.h>
#include <scxcorelib/scxfile.h>
#include <testutils/scxunit.h>
#include <sys/wait.h>
#if defined(aix)
#include <unistd.h>
#endif
#include <string>
#include "scxcorelib/util/persist/scxfilepersistmedia.h"

using namespace SCXCoreLib;

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif


class SCXPersistenceTest : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SCXPersistenceTest );
    CPPUNIT_TEST( TestName );
    CPPUNIT_TEST( TestVersion );
    CPPUNIT_TEST( TestEmptyGroup );
    CPPUNIT_TEST( TestSingleValue );
    CPPUNIT_TEST( TestSubgroupsAndValues );
    CPPUNIT_TEST( TestWriteEndNonOpenGroup );
    CPPUNIT_TEST( TestWriteAllGroupsMustEnd );
    CPPUNIT_TEST( TestReadEndNonOpenGroup );
    CPPUNIT_TEST( TestReadStartGroupWithWrongName );
    CPPUNIT_TEST( TestReadValueWithWrongName );
    CPPUNIT_TEST( TestReadStartGroupWhenEndGroupIsNext );
    CPPUNIT_TEST( TestReadStartGroupWhenValueIsNext );
    CPPUNIT_TEST( TestReadValueWhenEndGroupIsNext );
    CPPUNIT_TEST( TestReadValueWhenStartGroupIsNext );
    CPPUNIT_TEST( TestReadEndGroupWhenStartGroupIsNext );
    CPPUNIT_TEST( TestReadEndGroupWhenValueIsNext );
    CPPUNIT_TEST( TestReadTruncatedFile );
    CPPUNIT_TEST( TestReadCorruptedFile );
    CPPUNIT_TEST( TestUnPersist );
    CPPUNIT_TEST( TestConflictingPaths );
    CPPUNIT_TEST( TestValueWithSpace );
    CPPUNIT_TEST( TestGroupNameWithSpace );
    CPPUNIT_TEST( TestValueNameWithSpace );
    CPPUNIT_TEST( TestValueWithQuote );
    CPPUNIT_TEST( TestGroupNameWithQuote );
    CPPUNIT_TEST( TestValueNameWithQuote );
    CPPUNIT_TEST( TestNonTrivialUTF8Names );
    CPPUNIT_TEST( TestReadXMLEncodedLT );
    CPPUNIT_TEST( TestReadXMLEncodedAMP );
    CPPUNIT_TEST( TestReadXMLEncodedAPOS );
    CPPUNIT_TEST( TestReadXMLEncodedQUOT );
    CPPUNIT_TEST( TestReadXMLEncodedNUM );
    CPPUNIT_TEST( TestReadXMLEncodedInvalid );
    CPPUNIT_TEST( TestReadXMLEncodedInvalidEmptyString );
    CPPUNIT_TEST( TestCreateWriterInNonExistingDirectoryFails );
    SCXUNIT_TEST_ATTRIBUTE(TestReadTruncatedFile, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestReadCorruptedFile, SLOW);
    CPPUNIT_TEST_SUITE_END();

private:
    SCXHandle<SCXPersistMedia> m_pmedia;

public:
    void setUp(void)
    {
        m_pmedia = GetPersistMedia();
        SCXFilePersistMedia* m = dynamic_cast<SCXFilePersistMedia*> (m_pmedia.GetData());
        CPPUNIT_ASSERT(m != 0);
        m->SetBasePath(L"./");
    }

    void tearDown(void)
    {
        try
        {
            m_pmedia->UnPersist(L"MyProvider");
            m_pmedia->UnPersist(L"MyProvider1");
            m_pmedia->UnPersist(L"TemporaryName");
        }
        catch (PersistDataNotFoundException&)
        {
            // Ignore.
        }
    }

    void TestName(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->DoneWriting();
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXUNIT_ASSERT_THROWN_EXCEPTION(m_pmedia->CreateReader(L"NoPersistedMediaWithThisName"), PersistDataNotFoundException, L"NoPersistedMediaWithThisName");
        CPPUNIT_ASSERT_NO_THROW(m_pmedia->CreateReader(L"MyProvider"));
    }

    void TestVersion(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->DoneWriting();
            pwriter = m_pmedia->CreateWriter(L"MyProvider1", 17);
            pwriter->DoneWriting();
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->GetVersion() == 0);
        preader = m_pmedia->CreateReader(L"MyProvider1");
        CPPUNIT_ASSERT(preader->GetVersion() == 17);
    }

    void TestEmptyGroup(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestEmptyGroup");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestEmptyGroup"));
        CPPUNIT_ASSERT(preader->ConsumeEndGroup()); // Closing TestEmptyGroup
    }

    void TestSingleValue(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteValue(L"TestValue", L"4711");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"4711" == preader->ConsumeValue(L"TestValue"));
    }

    void TestSubgroupsAndValues(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestRecursiveGroup");
            pwriter->WriteValue(L"TestValue1", L"4711");
            pwriter->WriteValue(L"TestValue2", L"oof");
            pwriter->WriteStartGroup(L"TestSubGroup");
            pwriter->WriteValue(L"TestValue3", L"rab");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestSubGroup
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestRecursiveGroup
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestRecursiveGroup"));
        CPPUNIT_ASSERT(L"4711" == preader->ConsumeValue(L"TestValue1"));
        std::wstring value;
        CPPUNIT_ASSERT(preader->ConsumeValue(L"TestValue2", value));
        CPPUNIT_ASSERT(L"oof" == value);
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestSubGroup"));
        CPPUNIT_ASSERT(L"rab" == preader->ConsumeValue(L"TestValue3"));
        CPPUNIT_ASSERT(preader->ConsumeEndGroup()); // Closing TestSubGroup
        CPPUNIT_ASSERT(preader->ConsumeEndGroup()); // Closing TestRecursiveGroup
    }

    void TestWriteEndNonOpenGroup(void)
    {
        SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
        // Should not be able to close a group when no group is open.
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW(pwriter->WriteEndGroup(), SCXInvalidStateException);
        SCXUNIT_ASSERTIONS_FAILED(1);
        CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
    }

    void TestWriteAllGroupsMustEnd(void)
    {
        SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
        pwriter->WriteStartGroup(L"TestEmptyGroup");
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW(pwriter->DoneWriting(), SCXInvalidStateException); // All groups not closed.
        SCXUNIT_ASSERTIONS_FAILED(1);
        CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
        pwriter->WriteStartGroup(L"TestRecursiveGroup");
        pwriter->WriteStartGroup(L"TestSubGroup");
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW(pwriter->DoneWriting(), SCXInvalidStateException); // All groups not closed.
        SCXUNIT_ASSERTIONS_FAILED(1);
        CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestSubGroup
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW(pwriter->DoneWriting(), SCXInvalidStateException); // All groups not closed.
        SCXUNIT_ASSERTIONS_FAILED(1);
        CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestRecursiveGroup
        CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
    }

    void TestReadEndNonOpenGroup(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        // Should not be able to look for a group end when no group is open.
        SCXUNIT_RESET_ASSERTION();
        CPPUNIT_ASSERT_THROW(preader->ConsumeEndGroup(), SCXInvalidStateException);
        SCXUNIT_ASSERTIONS_FAILED(1);
    }

    void TestReadStartGroupWithWrongName(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestGroup");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        SCXUNIT_ASSERT_THROWN_EXCEPTION(preader->ConsumeStartGroup(L"ThisGroupIsNotNext", true), PersistUnexpectedDataException, L"ThisGroupIsNotNext");
    }

    void TestReadValueWithWrongName(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteValue(L"TestValue", L"4711");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT_THROW(preader->ConsumeValue(L"ThisIsNotTheNameOfTheNextValue"), PersistUnexpectedDataException);
    }

    void TestReadStartGroupWhenEndGroupIsNext(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestGroup");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestGroup"));
        CPPUNIT_ASSERT_THROW(preader->ConsumeStartGroup(L"ThisGroup", true), PersistUnexpectedDataException);
        CPPUNIT_ASSERT(preader->ConsumeEndGroup());
    }

    void TestReadStartGroupWhenValueIsNext(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteValue(L"TestValue", L"4711");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT_THROW(preader->ConsumeStartGroup(L"TestValue", true), PersistUnexpectedDataException);
    }

    void TestReadValueWhenEndGroupIsNext(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestGroup");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestGroup"));
        CPPUNIT_ASSERT_THROW(preader->ConsumeValue(L"TestGroup"), PersistUnexpectedDataException);
        CPPUNIT_ASSERT(preader->ConsumeEndGroup());
    }

    void TestReadValueWhenStartGroupIsNext(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestGroup");
            pwriter->WriteStartGroup(L"TestGroup");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestGroup"));
        CPPUNIT_ASSERT_THROW(preader->ConsumeValue(L"TestGroup"), PersistUnexpectedDataException);
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestGroup"));
        CPPUNIT_ASSERT(preader->ConsumeEndGroup());
        CPPUNIT_ASSERT(preader->ConsumeEndGroup());
    }

    void TestReadEndGroupWhenStartGroupIsNext(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestGroup");
            pwriter->WriteStartGroup(L"TestGroup");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestGroup"));
        CPPUNIT_ASSERT_THROW(preader->ConsumeEndGroup(L"TestGroup"), PersistUnexpectedDataException);
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestGroup"));
        CPPUNIT_ASSERT(preader->ConsumeEndGroup());
        CPPUNIT_ASSERT(preader->ConsumeEndGroup());
    }

    void TestReadEndGroupWhenValueIsNext(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestGroup");
            pwriter->WriteValue(L"TestGroup", L"4711");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestGroup"));
        CPPUNIT_ASSERT_THROW(preader->ConsumeEndGroup(true), PersistUnexpectedDataException);
    }

    // This test fails in strange ways on AIX. See WI9509
    void TestReadTruncatedFile(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing. Write a fairly complex structure.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestRecursiveGroup");
            pwriter->WriteValue(L"TestValue1", L"4711");
            pwriter->WriteValue(L"TestValue2", L"oof");
            pwriter->WriteStartGroup(L"TestSubGroup");
            pwriter->WriteValue(L"TestValue3", L"rab");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestSubGroup
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestRecursiveGroup
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        // Now we will read the generated file, keep content in memory,
        // write back only the first n characters of it and then let
        // the reader parse it. This should generate an exception.

        SCXHandle<std::wfstream> instream = SCXFile::OpenWFstream(L"./MyProvider", std::ios_base::in);
        std::wstringstream original;
        original << instream->rdbuf();

        for (SCXHandle<std::wfstream> outstream = SCXFile::OpenWFstream(L"./MyProvider", std::ios_base::out);
             ! original.eof();
             *outstream << (wchar_t) original.get())
        {
            outstream->flush();
            try
            {
                SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
                CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestRecursiveGroup", true));
                CPPUNIT_ASSERT(L"4711" == preader->ConsumeValue(L"TestValue1"));
                std::wstring value;
                CPPUNIT_ASSERT(preader->ConsumeValue(L"TestValue2", value, true));
                CPPUNIT_ASSERT(L"oof" == value);
                CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestSubGroup", true));
                CPPUNIT_ASSERT(L"rab" == preader->ConsumeValue(L"TestValue3"));
                CPPUNIT_ASSERT(preader->ConsumeEndGroup(true)); // Closing TestSubGroup
                CPPUNIT_ASSERT(preader->ConsumeEndGroup(true)); // Closing TestRecursiveGroup
                
                // If we came all the way here we have been able to read everything that was
                // written without exceptions. This will happen in the last couple of loop
                // iterations.
            }
            catch (PersistUnexpectedDataException&)
            {
                // This is what should usually happen except for the last couple of iterations
            }
        }
    }

    void TestReadCorruptedFile(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing. Write a fairly complex structure.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"TestRecursiveGroup");
            pwriter->WriteValue(L"TestValue1", L"4711");
            pwriter->WriteValue(L"TestValue2", L"oof");
            pwriter->WriteStartGroup(L"TestSubGroup");
            pwriter->WriteValue(L"TestValue3", L"rab");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestSubGroup
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup()); // Closing TestRecursiveGroup
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        // Now we will read the generated file, keep content in memory,
        // write back the whole file but with a space at the nth position. Then let
        // the reader parse it.
        // If the space is in a harmless position, the file should be parseable as normal.
        // If it is in a harmfull position it should generate an exception.

        SCXHandle<std::wfstream> instream = SCXFile::OpenWFstream(L"./MyProvider", std::ios_base::in);
        std::wstringstream original;
        original << instream->rdbuf();

        std::wstring firstPart(L"");
        for (std::wstring secondPart = original.str();
             secondPart.size() > 0;
             secondPart.erase(0, 1))
        {
            SCXHandle<std::wfstream> outstream = SCXFile::OpenWFstream(L"./MyProvider", std::ios_base::out);
            *outstream << firstPart << L" " << secondPart;
            outstream->flush();
            try
            {
                SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
                CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestRecursiveGroup", true));
                std::wstring value = preader->ConsumeValue(L"TestValue1");
                CPPUNIT_ASSERT(L"4711" == value ||
                               L" 4711" == value ||
                               L"4 711" == value ||
                               L"47 11" == value ||
                               L"471 1" == value ||
                               L"4711 " == value);
                CPPUNIT_ASSERT(preader->ConsumeValue(L"TestValue2", value, true));
                CPPUNIT_ASSERT(L"oof" == value ||
                               L" oof" == value ||
                               L"o of" == value ||
                               L"oo f" == value ||
                               L"oof " == value);
                CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"TestSubGroup", true));
                value = preader->ConsumeValue(L"TestValue3");
                CPPUNIT_ASSERT(L"rab" == value ||
                               L" rab" == value ||
                               L"r ab" == value ||
                               L"ra b" == value ||
                               L"rab " == value);
                CPPUNIT_ASSERT(preader->ConsumeEndGroup(true)); // Closing TestSubGroup
                CPPUNIT_ASSERT(preader->ConsumeEndGroup(true)); // Closing TestRecursiveGroup
                // If we came all the way here we have been able to read everything that was
                // written without exceptions. This will happen if the space was in a 
                // harmless position.
            }
            catch (PersistUnexpectedDataException&)
            {
                // This is what should usually happen except for when the space
                // is in a harmless position.
            }
            firstPart.push_back(secondPart[0]);
        }
    }
    
    void TestUnPersist(void)
    {
        CPPUNIT_ASSERT_THROW( m_pmedia->UnPersist(L"MyProvider"), PersistDataNotFoundException);
        
        SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
        CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
        
        CPPUNIT_ASSERT_NO_THROW( m_pmedia->UnPersist(L"MyProvider"));
        CPPUNIT_ASSERT_THROW( m_pmedia->UnPersist(L"MyProvider"), PersistDataNotFoundException);
    }
    
    void TestConflictingPaths(void)
    {
        {
            SCXHandle<SCXPersistDataWriter> pwriter1 = m_pmedia->CreateWriter(L"/This/_is/a/file_path.log");
            SCXHandle<SCXPersistDataWriter> pwriter2 = m_pmedia->CreateWriter(L"/This__is/a_file/path.log");
            pwriter1->WriteValue(L"TestValue", L"4711");
            pwriter2->WriteValue(L"TestValue", L"4712");
            pwriter1->DoneWriting();
            pwriter2->DoneWriting();
        }
        
        SCXHandle<SCXPersistDataReader> preader1 = m_pmedia->CreateReader(L"/This/_is/a/file_path.log");
        SCXHandle<SCXPersistDataReader> preader2 = m_pmedia->CreateReader(L"/This__is/a_file/path.log");

        CPPUNIT_ASSERT(L"4711" == preader1->ConsumeValue(L"TestValue"));
        CPPUNIT_ASSERT(L"4712" == preader2->ConsumeValue(L"TestValue"));

        m_pmedia->UnPersist(L"/This/_is/a/file_path.log");
        m_pmedia->UnPersist(L"/This__is/a_file/path.log");
    }

    void TestValueWithSpace(void)
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteValue(L"TestValue", L"oof rab");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"oof rab" == preader->ConsumeValue(L"TestValue"));
    }
    void TestGroupNameWithSpace()
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"Test Group");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"Test Group"));
        CPPUNIT_ASSERT(preader->ConsumeEndGroup()); // Closing TestEmptyGroup
    }
    void TestValueNameWithSpace()
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteValue(L"Test Value", L"oof rab");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"oof rab" == preader->ConsumeValue(L"Test Value"));
    }
    void TestValueWithQuote()
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteValue(L"TestValue", L"oof\"rab");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"oof\"rab" == preader->ConsumeValue(L"TestValue"));
    }
    void TestGroupNameWithQuote()
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteStartGroup(L"Test\"Group");
            CPPUNIT_ASSERT_NO_THROW(pwriter->WriteEndGroup());
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(preader->ConsumeStartGroup(L"Test\"Group"));
        CPPUNIT_ASSERT(preader->ConsumeEndGroup()); // Closing TestEmptyGroup
    }
    void TestValueNameWithQuote()
    {
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider");
            pwriter->WriteValue(L"Test\"Value", L"oof rab");
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"oof rab" == preader->ConsumeValue(L"Test\"Value"));
    }

    void TestNonTrivialUTF8Names()
    {
        std::wstring testvalue(L"oof");
        // Codepoint "Å√Å§"/228/E4 "LATIN SMALL LETTER A WITH DIAERESIS"
#if defined(sun)
        testvalue += StrFromUTF8("\xc3\xa5");
#else
        testvalue += wchar_t(228);
#endif
        testvalue += L"rab";

        std::wstring testvaluename(L"Test");
        // Codepoint "Å√ñ"/214/D6 "LATIN CAPITAL LETTER O WITH DIAERESIS"
#if defined(sun)
        testvaluename += StrFromUTF8("\xc3\x96");
#else
        testvaluename += wchar_t(214);
#endif
        testvaluename += L"Value";

        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process will do the writing.
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"MyProvider1");
            pwriter->WriteValue(testvaluename, testvalue);
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
            exit(0);
        }

        // Parent process will do the reading after child has finished.
        waitpid(pid, 0, 0);
   
        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider1");
        CPPUNIT_ASSERT(testvalue == preader->ConsumeValue(testvaluename));
    }

    void GivenPersistedDataWithSpecialValue(const std::wstring& persistDataName,
                                            const std::wstring& valueName,
                                            const std::wstring& value)
    {
        // First write a standard persistance file.
        const std::wstring replaceText = L"ReplaceThisText";
        {
            SCXHandle<SCXPersistDataWriter> pwriter = m_pmedia->CreateWriter(L"TemporaryName");
            pwriter->WriteValue(valueName, replaceText);
            CPPUNIT_ASSERT_NO_THROW(pwriter->DoneWriting());
        }
        
        // Then open the file and replace the text with what we want to have.
        // This ensures that the value is not tampered with by the persistence writer.
        {
            SCXHandle<std::fstream> instream = SCXFile::OpenFstream(L"TemporaryName", std::ios_base::in);
            SCXHandle<std::fstream> outstream = SCXFile::OpenFstream(persistDataName, std::ios_base::out);
            
            SCXStream::NLF nlf;
            std::wstring line;
            for (SCXStream::ReadLineAsUTF8(*instream, line, nlf);
                 SCXStream::IsGood(*instream);
                 SCXStream::ReadLineAsUTF8(*instream, line, nlf))
            {
                std::wstring::size_type pos = line.find(replaceText);
                if (std::wstring::npos != pos)
                {
                    line.replace(pos, replaceText.size(), value);
                }
                SCXStream::WriteAsUTF8(*outstream, line);
                SCXStream::WriteNewLineAsUTF8(*outstream, nlf);
            }
        }
    }

    void TestReadXMLEncodedLT()
    {
        GivenPersistedDataWithSpecialValue(L"MyProvider", L"TestValue", L"&lt;");

        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"<" == preader->ConsumeValue(L"TestValue"));
    }

    void TestReadXMLEncodedAMP()
    {
        GivenPersistedDataWithSpecialValue(L"MyProvider", L"TestValue", L"&amp;");

        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"&" == preader->ConsumeValue(L"TestValue"));
    }

    void TestReadXMLEncodedAPOS()
    {
        GivenPersistedDataWithSpecialValue(L"MyProvider", L"TestValue", L"&apos;");

        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"\'" == preader->ConsumeValue(L"TestValue"));
    }

    void TestReadXMLEncodedQUOT()
    {
        GivenPersistedDataWithSpecialValue(L"MyProvider", L"TestValue", L"&quot;");

        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"\"" == preader->ConsumeValue(L"TestValue"));
    }

    void TestReadXMLEncodedNUM()
    {
        GivenPersistedDataWithSpecialValue(L"MyProvider", L"TestValue", L"&#83;");

        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT(L"S" == preader->ConsumeValue(L"TestValue"));
    }

    void TestReadXMLEncodedInvalid()
    {
        GivenPersistedDataWithSpecialValue(L"MyProvider", L"TestValue", L"&something;");

        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT_THROW(preader->ConsumeValue(L"TestValue"), PersistUnexpectedDataException);
    }

    void TestReadXMLEncodedInvalidEmptyString()
    {
        GivenPersistedDataWithSpecialValue(L"MyProvider", L"TestValue", L"&;");

        SCXHandle<SCXPersistDataReader> preader = m_pmedia->CreateReader(L"MyProvider");
        CPPUNIT_ASSERT_THROW(preader->ConsumeValue(L"TestValue"), PersistUnexpectedDataException);
    }

    void TestCreateWriterInNonExistingDirectoryFails()
    {
        SCXFilePersistMedia* m = dynamic_cast<SCXFilePersistMedia*> (m_pmedia.GetData());
        CPPUNIT_ASSERT(m != 0);
        m->SetBasePath(L"./non/exisiting/folder/");

        SCXUNIT_ASSERT_THROWN_EXCEPTION(m_pmedia->CreateWriter(L"MyProvider"), PersistMediaNotAvailable, L"non/exisiting/folder");
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXPersistenceTest );
