/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        scxprocess.h

    \brief       Defined the process PAL.
    
    \date        2007-10-12 15:43:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXPROCESS_H
#define SCXPROCESS_H

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfilepath.h>

#if defined(WIN32)
#include <windows.h>
#elif defined(linux) || defined(sun) || defined(hpux)
#include <unistd.h>
#endif
 
#include <iostream>
#include <vector>
#include <string>
#include <sys/types.h>
#include <signal.h>

#if defined(sun) || defined(hpux)
#include <sys/wait.h>
#endif

namespace SCXCoreLib
{
    /** Definition of a process id. */
#if defined(WIN32)
    typedef DWORD SCXProcessId;
#elif defined(SCX_UNIX)
    typedef pid_t SCXProcessId;
#else
#error Not implemented on this platform 
#endif


    /*----------------------------------------------------------------------------*/
    /**
     * Process did not run to completion, it terminated prematurally and there was
     * not return code
     */
    class SCXInterruptedProcessException : public SCXException
    {
    public:
        //! Constructor
        //! \param[in]  location    Where the exception occurred
        SCXInterruptedProcessException(const SCXCodeLocation& location) 
           : SCXException(location)  {};
                                                         
        virtual std::wstring What() const { return L"Process interrupted"; }
    
    };

    /*----------------------------------------------------------------------------*/
    // RAII encapsulation to block certain signals ( Like SIGPIPE )
    class SignalBlock
    {
    public:
        //! Constructor
        //! \param[in]   Bitfield mask of signals to ignore
        SignalBlock(int sigmask);

        //! Destructor
        ~SignalBlock();
    private:
        sigset_t m_set, m_oldset;
        int m_sigmask;
    };

    /*----------------------------------------------------------------------------*/
    /**
        Represents a reference to a process. 
    
        Provide a reference to process instances.
    
    */
    class SCXProcess
    {
    public:
        static SCXProcessId GetCurrentProcessID();
        static std::vector<std::wstring> SplitCommand(const std::wstring &command);

#if defined(SCX_UNIX)

        static int Run(const std::wstring &command, std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr, 
                       unsigned timeout = 0, const SCXFilePath& cwd = SCXFilePath(), const SCXFilePath& chrootPath = SCXFilePath());
        static int Run(const std::vector<std::wstring> &myargv, std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr, 
                       unsigned timeout = 0, const SCXFilePath& cwd = SCXFilePath(), const SCXFilePath& chrootPath = SCXFilePath());
        static int Run(SCXProcess& process, std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr, unsigned timeout = 0);
        
        SCXProcess(const std::vector<std::wstring> myargv, const SCXFilePath& cwd = SCXFilePath(), const SCXFilePath& chrootPath = SCXFilePath());
        bool SendInput(std::istream &mystdin);
        bool PerformIO(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr);
        int WaitForReturn();
        int WaitForReturn(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr); 
        virtual ~SCXProcess();
        void Kill();
        unsigned GetEffectiveTimeout(unsigned timeout);

    protected:
        virtual bool ReadToStream(int fd, std::ostream& stream);
        virtual ssize_t DoWrite(int fd, const void* buf, size_t size);
        virtual int DoWaitPID(int* pExitCode, bool blocking);
        virtual bool InternalPerformIO(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr);
        virtual void CloseAndDie();

    private:
        //!< Index of file descriptors for a pipe
        enum Direction {
            R,           //!< in
            W            //!< out
        };

        int m_inForChild[2];                  //!< Pipe for stdin of the process
        int m_outForChild[2];                 //!< Pipe for stdput of the process
        int m_errForChild[2];                 //!< Pipe for stderr of the process
        std::vector<char *> m_cargv;          //!< Process arguments as null terminated char arrays
        std::vector<char> m_stdinChars;       //!< Buffer when reading from stdin
        std::vector<char> m_buffer;           //!< Buffer when writing to stdout and stderr
        int m_stdinCharCount;                 //!< Number of buffered stdin characters
        SCXProcessId m_pid;                   //!< Pid (process ID) of process
        int m_processExitCode;                //!< The process exit code
        bool m_waitCompleted;                 //!< Indicates if the process has been waited for.
        bool m_stdinActive;                   //!< The child process may read from its stdin
        bool m_stdoutActive;                  //!< The child process may write to its stdout
        bool m_stderrActive;                  //!< The child process may write to its stderr
        size_t m_timeoutOverhead;             //!< The amount of overhead in waiting for the child process to begin.
#endif
    };
} /* namespace SCXCoreLib */

#endif /* FILENAME_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
