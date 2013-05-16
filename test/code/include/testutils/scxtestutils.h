/*----------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file        

    \brief       Utility classes for Unit testing in the SCX project.

    \date        2008-10-31 21:17:46

    This is a file for adding small unit testing utilities.

*/
/*----------------------------------------------------------------------*/
#ifndef SCXTESTUTILS_H
#define SCXTESTUTILS_H

#if defined(SCX_UNIX)
#include <unistd.h>
#endif
#include <scxcorelib/scxfile.h>

namespace
{
#if defined(SCX_UNIX)
    /*
      Synchronization class.

      This is used so two processes can "sync" with one another (while there are
      several approaches, this one uses a pipe).  Note that pipes can sometimes
      be bidirectional, but this isn't portable.  For bi-directional use, you
      should create two pipes (one for reading, one for writing).

      This should be much more reliable than interlaced sleeps hoping that we're
      scheduled when we should be ...

      To use this class:
      The reading end should call SignifyReader() before doing any I/O
      The writing end should call SignifyWriter() before doing any I/O
    */

    class SynchronizeProcesses
    {
    public:
        enum eChannels {
            ReadChannel = 0,        //!< Read channel
            WriteChannel = 1        //!< Write channel
        };

        SynchronizeProcesses()
            : m_bSignified(false)
        {
            // Create the pipe, be certain both R/W channels not zero
            // (We use zero to signify that the channel is closed)
            CPPUNIT_ASSERT(0 == pipe(m_pipeFDs));
            CPPUNIT_ASSERT(0 != m_pipeFDs[ReadChannel]);
            CPPUNIT_ASSERT(0 != m_pipeFDs[WriteChannel]);
        }

        ~SynchronizeProcesses()
        {
            // Close whatever channels remain open from the pipe
            if (m_pipeFDs[ReadChannel]) CPPUNIT_ASSERT(0 == close(m_pipeFDs[ReadChannel]));
            if (m_pipeFDs[WriteChannel]) CPPUNIT_ASSERT(0 == close(m_pipeFDs[WriteChannel]));
        }

        void SignifyWriter()
        {
            // Close unused read channel
            CPPUNIT_ASSERT(0 == close(m_pipeFDs[ReadChannel]));
            m_pipeFDs[ReadChannel] = 0;
            m_bSignified = true;
        }

        void SignifyReader()
        {
            // Close unused write channel
            CPPUNIT_ASSERT(0 == close(m_pipeFDs[WriteChannel]));
            m_pipeFDs[WriteChannel] = 0;
            m_bSignified = true;
        }

        void ReadMarker(char c)
        {
            // Read a character from the pipe, make sure it's what we expected
            char buf;
            CPPUNIT_ASSERT(m_bSignified);
            CPPUNIT_ASSERT(read(m_pipeFDs[ReadChannel], &buf, 1) > 0);
            CPPUNIT_ASSERT(buf == c);
        }

        void WriteMarker(char c)
        {
            // Write a character to the pipe
            CPPUNIT_ASSERT(m_bSignified);
            CPPUNIT_ASSERT(-1 != write(m_pipeFDs[WriteChannel], &c, 1));
        }

    private:
        bool m_bSignified;
        int m_pipeFDs[2];
    };
#endif
}

namespace SCXCoreLib
{

    /*----------------------------------------------------------------------------*/
    /**
        This is a small utility to create a file path that will delete the file
        once it goes out of scope.
    */
    class SelfDeletingFilePath : public SCXCoreLib::SCXFilePath
    {
    public:
        /**
            Simple constructor
            \param[in] path File path.
        */
        SelfDeletingFilePath(const std::wstring& path) : SCXCoreLib::SCXFilePath(path)
        {}
        
        /** Destructor which deletes the file */
        virtual ~SelfDeletingFilePath()
        {
            SCXFile::Delete(*this);
        }
    };
}


#endif /* SCXTESTUTILS_H */
