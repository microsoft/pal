/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        scxprocess.cpp

    \brief       Implements the process handling PAL.

    \date        2007-10-12 15:43:00

*/
/*----------------------------------------------------------------------------*/

#if defined(hpux)
#undef _XOPEN_SOURCE // Or else chroot is not defined.
#include <sys/time.h>
#endif
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxoserror.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxexception.h>

#include <iostream>
#include <stdio.h>
#include <vector>
#include <stdio.h>

#if defined(WIN32)
#include <process.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#endif

#include <errno.h>

namespace SCXCoreLib
{

    SignalBlock::SignalBlock(int sigmask) : m_sigmask(sigmask)
    {
        sigemptyset(&m_set);
        sigaddset(&m_set, m_sigmask);
        int ret = pthread_sigmask(SIG_BLOCK, &m_set, &m_oldset);
        if (ret != 0)
        {
            std::cerr << "pthread_sigmask failed with error: " << ret << std::endl;
        }
    }

    SignalBlock::~SignalBlock()
    {
        // Consume any pending signal but do not wait
        struct timespec timeout;
        timeout.tv_sec = timeout.tv_nsec = 0;
        sigtimedwait(&m_set, NULL, &timeout);

        // Restore signal mask
        int ret = pthread_sigmask(SIG_SETMASK, &m_oldset, NULL);
        if (ret != 0)
        {
            std::cerr << "pthread_sigmask failed with error : " << ret << std::endl;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the calling process', process id.

        \returns      SCXProcessId of the calling thread.

    */
    SCXProcessId SCXProcess::GetCurrentProcessID()
    {
#if defined(WIN32)
        return GetCurrentProcessId();
#elif defined(SCX_UNIX)
        return getpid();
#else
#error "Platform not supported"
#endif
    }

    //! Split a command into its separate parts
    //! \param[in]  command like those entered in a command shell
    //! \returns    Parts that was separated by space in "command"
    //! A part is allowed to contain spaces if those are quoted by single or double quotes.
    //! If a part itself consists of a single quote, that part may be quoted by a double quote,
    //! and vice versa.
    //!
    //! Additionally, quoted quotes are ignored.  So a command like:
    //!
    //!         /bin/sh -c "echo \"<?php phpinfo();?>\" > /tmp/index.php"
    //!
    //! should be broken into the following parts:
    //!
    //!         [0] -> /bin/sh
    //!         [1] -> -c
    //!         [2] -> echo \"<?php phpinfo();?>\" > /tmp/index.php
    //!
    //! \note   Parts are delimited by spaces, not the quotes. If one were to list all files
    //          of the directory /usr/local/apache-tomcat/, it could be written as follows:
    //!         ls "/usr/"local/'apache-tomcat/'. That is useful if a command consists of
    //!         single as well as double quotes.
    std::vector<std::wstring> SCXProcess::SplitCommand(const std::wstring &command) {
        std::vector<std::wstring> parts;

        std::wostringstream part;
        bool newPart = false, escape = false;
        wchar_t quote = 0;
        for (std::wstring::size_type i = 0; i < command.length(); i++)
        {
            wchar_t c = command.at(i);
            if (c == '\\')
            {
                // If we have something like \\\", then output a "\" for "\\"
                // (unless we're in a single quote - then output literally)
                if ( escape || ('\'' == quote) )
                {
                    part << c;
                    escape = false;
                }
                else
                {
                    escape = true;
                }
            }
            else if (c == ' ')
            {
                if (quote)
                {
                    part << c;
                }
                else if (newPart)
                {
                    parts.push_back(part.str());
                    part.str(L"");
                    newPart = false;
                }
                escape = false;
            }
            else if (c == '\'' || c == '"')
            {
                if (escape && c == '\'')
                {
                    part << "\\";
                    escape = false;    // Ignore quotes on apostrophe chars
                }

                if (!escape)
                {
                    if (quote == c)
                    {
                        quote = 0;
                    }
                    else if (quote)
                    {
                        part << c;
                    }
                    else
                    {
                        quote = c;
                    }
                }
                else
                {
                    part << c;
                    escape = false;
                }
            }
            else
            {
                part << c;
                newPart = true;
                escape = false;
            }
        }
        if (newPart) {
            parts.push_back(part.str());
        }
        return parts;
    }

#if defined(SCX_UNIX)

        /**********************************************************************************/
    //! Run a process by passing it arguments and streams for stdin, stdout and stderr
    //! \param[in]  command     Corresponding to what would be entered by a user in a command shell
    //! \param[in]  mystdin     stdin for the process
    //! \param[in]  mystdout    stdout for the process
    //! \param[in]  mystderr    stderr for the process
    //! \param[in]  timeout     Accepted number of millieconds to wait for return
    //! \param[in]  cwd         Directory to be set as current working directory for process.
    //! \param[in]  chrootPath  Directory to be used for chrooting process.
    //! \returns exit code of run process.
    //! \throws     SCXInternalErrorException           Failure that could not be prevented
    //! This call will block as long as the process writes to its stdout or stderr
    //! \note   Make sure that "mystdout" and "mystderr" does not block when written to.
    //!
    //! \date        2007-12-05 15:43:00
    int SCXProcess::Run(const std::wstring &command,
                        std::istream &mystdin,
                        std::ostream &mystdout,
                        std::ostream &mystderr,
                        unsigned timeout,
                        const SCXFilePath& cwd/* = SCXFilePath()*/,
                        const SCXFilePath& chrootPath /* = SCXFilePath() */)
    {
#if 0
        std::vector<std::wstring> splitCmd = SplitCommand(command);
        std::wcout << std::endl << L"Original command: " << command;
        std::wcout << std::endl << L"Split command results::";

        for (size_t i = 0; i < splitCmd.size(); i++)
            std::wcout << std::endl << L"\t" << L"splitCmd[" << i << L"]: " << splitCmd[i];

        return Run(splitCmd, mystdin, mystdout,mystderr, timeout, cwd, chrootPath);
#else
        return Run(SplitCommand(command), mystdin, mystdout,mystderr, timeout, cwd, chrootPath);
#endif
    }


    /**********************************************************************************/
    //! Class for running a process with an option to timeout
    class ProcessThreadParam : public SCXThreadParam
    {
    public:
        //! Constructor that starts a process
        //! \param[in]  process     Process to monitor
        //! \param[in]  timeout     Accepted wait before process terminates
        ProcessThreadParam(SCXProcess *process, unsigned timeout)
            : m_process(process), m_processTerminated(false), m_timeout(timeout)
        {
        }

        /**********************************************************************************/
        //! Wait for the process to terminate
        //! \param[in]  mystdin     Represents stdin of process
        //! \param[in]  mystdout    Represents stdout of process
        //! \param[in]  mystderr    Represents stderr of process
        //! \returns Return code from process being run.
        int WaitForReturn(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr)
        {
            int returnCode = -1;
            try
            {
                returnCode =  m_process->WaitForReturn(mystdin, mystdout, mystderr);
                m_processTerminated = true;
            }
            catch (...)
            {
                m_processTerminated = true;
                throw;
            }
            return returnCode;
        }

        /**********************************************************************************/
        //! Wait a limited amount of time for the process to terminate.
        void WaitForReturn()
        {
            // Good enough way of waiting for the process
            int timeoutLeft = m_timeout;
            while (timeoutLeft > 0 && !m_processTerminated)
            {
                const int timeBetweenChecks = 1000;
                SCXThread::Sleep(timeBetweenChecks);
                timeoutLeft -= timeBetweenChecks;
            }
            if (!m_processTerminated)
            {
                m_process->Kill();
            }
        }

    private:
        ProcessThreadParam(const ProcessThreadParam &);             //!< Prevent copying
        ProcessThreadParam &operator=(const ProcessThreadParam &);  //!< Prevent assignment

        SCXProcess *m_process;      //!< Process to run
        bool m_processTerminated;   //!< True iff the process has terminated
        unsigned m_timeout;              //!< Accepted wait before termination
    };

    /**********************************************************************************/
    //! Monitors the process
    //! \param[in]  handle      Parameters of thread
    //! \note Meant to be called in a separate thread by being passed as argument to Thread constructor
    void WaitForReturnFn(SCXCoreLib::SCXThreadParamHandle& handle)
    {
        ProcessThreadParam* param = static_cast<ProcessThreadParam *>(handle.GetData());
        param->WaitForReturn();
    }

    /**********************************************************************************/
    //! Helper function to determine effective timeout duration.
    //! \param[in]  timeout   The specified timeout duration set by the creator of the SCXProcess
    //! \returns A timeout that is adjusted by approximately the amount of time it took to set up the subprocess.
    unsigned SCXProcess::GetEffectiveTimeout(unsigned timeout)
    {
        return static_cast<unsigned>(std::max(0, static_cast<int>(timeout) - static_cast<int>(m_timeoutOverhead)));
    }

    /**********************************************************************************/
    //! Run a process by passing it arguments and streams for stdin, stdout and stderr
    //! \param[in]  myargv      Arguments corresponding to argv in the main function of the process
    //! \param[in]  mystdin     stdin for the process
    //! \param[in]  mystdout    stdout for the process
    //! \param[in]  mystderr    stderr for the process
    //! \param[in]  timeout     Max number of milliseconds the process is allowed to run (0 means no limit)
    //! \param[in]  cwd         Directory to be set as current working directory for process.
    //! \param[in]  chrootPath  Directory to be used for chrooting process.
    //! \returns exit code of run process.
    //! \throws     SCXInternalErrorException           Failure that could not be prevented
    //! This call will block as long as the process writes to its stdout or stderr
    //! \note   Make sure that "mystdout" and "mystderr" does not block when written to.
    //!
    //! \date        2007-12-05 15:43:00
    int SCXProcess::Run(const std::vector<std::wstring> &myargv,
                        std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr,
                        unsigned timeout,
                        const SCXFilePath& cwd/* = SCXFilePath()*/,
                        const SCXFilePath& chrootPath /* = SCXFilePath() */)
    {
        SCXProcess process(myargv, cwd, chrootPath);
        return Run(process, mystdin, mystdout, mystderr, timeout);
    }

    /**********************************************************************************/
    //! Run a process by passing it arguments and streams for stdin, stdout and stderr
    //! \param[in]  process     A process instance to execute.
    //! \param[in]  mystdin     stdin for the process
    //! \param[in]  mystdout    stdout for the process
    //! \param[in]  mystderr    stderr for the process
    //! \param[in]  timeout     Max number of milliseconds the process is allowed to run (0 means no limit)
    //! \returns exit code of run process.
    //! \throws     SCXInternalErrorException           Failure that could not be prevented
    //! This call will block as long as the process writes to its stdout or stderr
    //! \note   Make sure that "mystdout" and "mystderr" does not block when written to.
    //!
    //! \date        2009-05-13 10:20:00
    int SCXProcess::Run(SCXProcess& process,
                        std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr, unsigned timeout /*= 0*/)
    {
        int returnCode = -1;
        SignalBlock ignoreSigPipe = SignalBlock(SIGPIPE);

        if (timeout <= 0)
        {
            returnCode = process.WaitForReturn(mystdin, mystdout, mystderr);
        }
        else
        {
            ProcessThreadParam *ptp = new ProcessThreadParam(&process, timeout);
            //ptp = 0;
            SCXThreadParamHandle tph(ptp);

            SCXThread t(WaitForReturnFn, tph);
            returnCode = ptp->WaitForReturn(mystdin, mystdout, mystderr);
            t.Wait();
        }
        return returnCode;
    }

    /**********************************************************************************/
    //! Constructor
    //! \param[in]  myargv       Arguments to process
    //! \param[in]  cwd         Directory to be set as current working directory for process.
    //! \param[in]  chrootPath  Directory to be used for chrooting process.

    SCXProcess::SCXProcess(const std::vector<std::wstring> myargv,
                           const SCXFilePath& cwd/* = SCXFilePath()*/,
                           const SCXFilePath& chrootPath /* = SCXFilePath() */) :
            m_stdinChars(1000),
            m_buffer(1000),
            m_stdinCharCount(0),
            m_pid(-1),
            m_processExitCode(-1),
            m_waitCompleted(false),
            m_stdinActive(true),
            m_stdoutActive(true),
            m_stderrActive(true),
            m_timeoutOverhead(0)
    {
        // Convert arguments to types expected by the system function for running processes
        for (std::vector<char *>::size_type i = 0; i < myargv.size(); i++)
        {
            m_cargv.push_back(strdup(StrToUTF8(myargv[i]).c_str()));
        }
        m_cargv.push_back(0);

        // Create pipes for communicating with the child process
        if ( -1 == pipe(m_inForChild) ) {
               throw SCXInternalErrorException(L"Failed to open pipe for m_inForChild", SCXSRCLOCATION);
        }
        if (-1 == pipe(m_outForChild) ) {
               throw SCXInternalErrorException(L"Failed to open pipe for m_outForChild", SCXSRCLOCATION);
        }
        if ( -1 == pipe(m_errForChild) ) {
               throw SCXInternalErrorException(L"Failed to open pipe for m_errForChild", SCXSRCLOCATION);
        }

        // A 'magic number' that is passed to the parent to signify that this process has had its pgid set.
        const char * c_magicGUID = "b4360097-03d5-4d1d-9514-176428bcd88f";
        const ssize_t c_magicGUID_length = 36;

        m_pid = fork();                         // Create child process, duplicates file descriptors
        if (m_pid == 0)
        {
            char error_msg[1024];
            // Set the pgid of the forked process to be the same as the forked process's pid, so that
            // we can kill the forked process and all subprocesses (if necessary) by calling killpg.
            setpgid(0, 0);

            // Communicate with the parent process that the child process has set its process group id.
            ssize_t bytes_written = DoWrite(m_outForChild[W], c_magicGUID, c_magicGUID_length);

            // Child process.
            // The file descriptors are duplicates, created by "fork",  of those in the parent process
            dup2(m_inForChild[R], STDIN_FILENO);      // Make the child process read from the parent process
            close(m_inForChild[R]);                   // Close duplicate
            close(m_inForChild[W]);                   // The child only reads from its stdin
            dup2(m_outForChild[W], STDOUT_FILENO);    // Make the child process write to the parent process
            close(m_outForChild[R]);                  // The child only writes to its stdout
            close(m_outForChild[W]);                  // Close duplicate
            dup2(m_errForChild[W], STDERR_FILENO);    // Make the child process write to the parent process
            close(m_errForChild[R]);                  // The child only writes to its stdout
            close(m_errForChild[W]);                  // Close duplicate

            if (bytes_written == -1)
            {
                snprintf(error_msg,
                            sizeof(error_msg),
                            "Failed to communicate with the parent process errno=%d",
                            errno);
                // Still try to inform the parent of imminent death
                DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
                CloseAndDie();
            }

            if (L"" != chrootPath.Get())
            {
                if ( 0 != ::chroot(StrToUTF8(chrootPath.Get()).c_str()))
                {
                    snprintf(error_msg,
                             sizeof(error_msg),
                             "Failed to chroot '%s' errno=%d",
                             StrToUTF8(chrootPath.Get()).c_str(), errno);
                    DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
                    CloseAndDie();
                }
                if ( 0 != ::chdir("/"))
                {
                    snprintf(error_msg,
                             sizeof(error_msg),
                             "Failed to change root directory. errno=%d", errno);
                    DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
                    CloseAndDie();
                }
            }
            if (L"" != cwd.Get())
            {
                if (0 != ::chdir(StrToUTF8(cwd.Get()).c_str()))
                {
                    snprintf(error_msg,
                             sizeof(error_msg),
                             "Failed to change cwd. errno=%d", errno);
                    DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
                    CloseAndDie();
                }
            }

            // Close open file descriptors except stdin/out/err
            // (Some systems have UNLIMITED of 2^64; limit to something reasonable)

            int fdLimit = 0;
            fdLimit = getdtablesize();
            if (fdLimit > 2500)
            {
                fdLimit = 2500;
            }

            for (int fd = 3; fd < fdLimit; ++fd)
            {
                close(fd);
            }

            execvp(m_cargv[0], &m_cargv[0]);                // Replace the child process image
            snprintf(error_msg, sizeof(error_msg), "Failed to start child process '%s' errno=%d  ", m_cargv[0], errno);
            DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));
            CloseAndDie();                                  // Failed to load correct process.
        }
        else
        {
            // Same (parent) process.
            // All file descriptors were duplicated in the child process by "fork"
            close(m_inForChild[R]);                   // Only the child process reads from its stdin
            close(m_outForChild[W]);                  // Only the child process writes to its stdout
            close(m_errForChild[W]);                  // Only the child process writes to its stderr
            if (m_pid < 0)
            {
                // Free remaining resources held by the parent process
                // The child process manages its own resources
                close(m_inForChild[W]);
                close(m_outForChild[R]);
                close(m_errForChild[R]);

                // Free everything except the terminating NULL
                for (std::vector<char *>::size_type i = 0; i < m_cargv.size() - 1; i++)
                {
                    free(m_cargv[i]);
                }
                //  No child process was created
                throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
            }

            // Set non-blocking I/O for the output and error channels
            // We need to use this to insure we have all our data from
            // the subprocess ...

            if (-1 == fcntl(m_outForChild[R], F_SETFL, O_NONBLOCK))
            {
                throw SCXInternalErrorException(UnexpectedErrno(L"Failed to set non-blocking I/O on stdout pipe", errno), SCXSRCLOCATION);
            }

            if (-1 == fcntl(m_errForChild[R], F_SETFL, O_NONBLOCK))
            {
                throw SCXInternalErrorException(UnexpectedErrno(L"Failed to set non-blocking I/O on stderr pipe", errno), SCXSRCLOCATION);
            }

            // Block until c_magicGUID is written to the child's stdout. This is to prevent a race condition
            // where the parent process attempts to kill the child's process group before the child process
            // sets its process group.
            char readmagic[c_magicGUID_length+1];
            m_timeoutOverhead = 0;
            const size_t c_timeBetweenReads = 50;
            const size_t c_maxTimeout = 30000;
            ssize_t numOfBytesRead = 0;

            while(m_timeoutOverhead < c_maxTimeout)
            {
                ssize_t readVal = read(m_outForChild[R], readmagic + numOfBytesRead, c_magicGUID_length - numOfBytesRead);
                if ( (readVal + numOfBytesRead) == c_magicGUID_length)
                {
                    // Proper number of bytes read.  Let's make sure the strings match.
                    if (strncmp(readmagic, c_magicGUID, c_magicGUID_length) != 0)
                    {
                        // there was an error communicating with the subprocess
                        throw SCXInternalErrorException(L"Process communication failed: read data did not match", SCXSRCLOCATION);
                    }
                    break;
                }
                else if (readVal < 0)
                {
                    if (errno == EAGAIN)
                    {
                        continue;
                    }
                    else
                    {
                        // there was an error communicating with the subprocess
                        throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed: read returned an error", errno), SCXSRCLOCATION);
                    }
                }
                else
                {
                    // read returned an unexpected number of bytes, keep reading until the number of bytes matches
                    numOfBytesRead += readVal;
                }
                SCXThread::Sleep(c_timeBetweenReads);
                m_timeoutOverhead += c_timeBetweenReads;
            }
        }
    }

    /**********************************************************************************/
    //! Send input to process
    //! \param[in]  mystdin     Source
    //! \returns true if there is possibly more data to send (potentially more in stream)
    //! \throws SCXInternalErrorException On syscall failures.
    bool SCXProcess::SendInput(std::istream &mystdin) {
        // Modify to read all available input - synchronously (blocking)
        std::streamsize stdinCharsRead;

        mystdin.read(&m_stdinChars[m_stdinCharCount], m_stdinChars.size() - m_stdinCharCount);
        stdinCharsRead = mystdin.gcount();

        if (!mystdin.eof() && !mystdin.good())
        {
            throw SCXInternalErrorException(L"Process parent communication failed", SCXSRCLOCATION);
        }

        m_stdinCharCount += static_cast<int>(stdinCharsRead);
        ssize_t bytesWritten = 0;
        if (m_stdinCharCount > 0)
        {
            bytesWritten = DoWrite(m_inForChild[W], &m_stdinChars[0], m_stdinCharCount);
        }

        if (bytesWritten < 0)
        {
            if (EPIPE == errno)
            {
                // If pipe has closed, we're done (nothing to read)
                return false;
            }
            else
            {
                throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
            }
        }
        strncpy(&m_stdinChars[0], &m_stdinChars[bytesWritten], m_stdinCharCount - bytesWritten);
        m_stdinCharCount -= static_cast<int>(bytesWritten);

        return (0 != stdinCharsRead || 0 != m_stdinCharCount);
    }

    /**********************************************************************************/
    //! Perform I/O to/from stdin, stdout, and stderr for a process
    //! \param[in]  mystdin     Content that must be sent to the process stdin
    //! \param[in]  mystdout    Receiver of content that the process writes to stdout
    //! \param[in]  mystderr    Receiver of content that the process writes to stderr
    //! \returns true if there is possibly more data to fetch (stderr and/or stdout still open for read).
    bool SCXProcess::InternalPerformIO(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr) {
        const int pollTimeoutInSecs = 2;     // Timeout waiting for subprocess

        // Wait some time for something available to be read from stdout or
        // stderr of the child process.
        //
        // Only try and read stdout/stderr if we haven't previously received
        // an error. If we previously received an error, don't read that.

        // Short circuit and exit if we know that both output pipes are dead
        // (No point in checking input pipe if we can't get output anyway)
        if (!m_stdoutActive && !m_stderrActive)
        {
            return false;
        }

        // We use FDs for poll as follows:
        //  0: processInput (sense if we can write to child process)
        //  1: processOutput (sense if we can read from stdout of child)
        //  2: processError (sense if we can read from stderr of child)

        struct pollfd fds[3];
        fds[0].fd = fds[1].fd = fds[2].fd = -1;
        fds[0].events = POLLOUT;
        fds[1].events = fds[2].events = POLLIN;

        if (m_stdinActive)
        {
            fds[0].fd = m_inForChild[W];
        }
        if (m_stdoutActive)
        {
            fds[1].fd = m_outForChild[R];
        }
        if (m_stderrActive)
        {
            fds[2].fd = m_errForChild[R];
        }

        int pollStatus = poll(fds, 3, pollTimeoutInSecs * 1000);
        if (pollStatus < 0)             /* Error occurred */
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
        }

        if (pollStatus > 0)             /* No timeout */
        {
            // Check if we can write to the child's stdin channel
            if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                m_stdinActive = false;
            }
            else if (fds[0].revents & POLLOUT)
            {
                m_stdinActive = SendInput(mystdin);
            }

            // Check if we have data to read from streams
            // (If no data to read, check if stream has closed)
            if (fds[1].revents & POLLIN)
            {
                ReadToStream(m_outForChild[R], mystdout);
            }
            if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                m_stdoutActive = false;
            }

            if (fds[2].revents & POLLIN)
            {
                ReadToStream(m_errForChild[R], mystderr);
            }
            if (fds[2].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                m_stderrActive = false;
            }
        }

        return m_stdoutActive || m_stderrActive;
    }

    /**********************************************************************************/
    //! Perform I/O to/from stdin, stdout, and stderr for a process
    //! \param[in]  mystdin     Content that must be sent to the process stdin
    //! \param[in]  mystdout    Receiver of content that the process writes to stdout
    //! \param[in]  mystderr    Receiver of content that the process writes to stderr
    //! \returns true if there is possibly more data to fetch (process still executing
    //!          and stderr and/or stdout still open for read.
    bool SCXProcess::PerformIO(
        std::istream &mystdin,
        std::ostream &mystdout,
        std::ostream &mystderr)
    {
        // Just read until we're told that both mystdout and mystderr are dead

        bool retval = InternalPerformIO(mystdin, mystdout, mystderr);

        // On some systems (AIX, Solaris and HPUX) a process can die without closing STDERR
        if (DoWaitPID(NULL, false) != 0)
        {
            // On at least one of our systems (AIX 6.1 build host) we have seen that
            // we need to do one more try to select and read to get all of STDERR.
            InternalPerformIO(mystdin, mystdout, mystderr);
            return false;
        }

        return retval;
    }

    /**********************************************************************************/
    //! Wait for the process to return, that is, terminate normally or by beeing signaled
    //! \returns Exit code of process being waited for.
    //! \throws     SCXInternalErrorException       Did not succeed to wait for the process
    int SCXProcess::WaitForReturn()
    {
        int child_status = 0;
        if (m_pid != DoWaitPID(&child_status, true))
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"Failed to wait for child process", errno), SCXSRCLOCATION);
        }
        if (!WIFEXITED(child_status))
        {
            throw SCXInterruptedProcessException(SCXSRCLOCATION);
        }
        return WEXITSTATUS(child_status);
    }

    /**********************************************************************************/
    //! Interacts with the process while waiting for the it to return,
    //! that is, terminate normally or by beeing signaled
    //! \param[in]  mystdin     Stdin of process
    //! \param[in]  mystdout    Stdout of process
    //! \param[in]  mystderr    Stderr of process
    int SCXProcess::WaitForReturn(std::istream &mystdin, std::ostream &mystdout, std::ostream &mystderr)
    {
        bool fetched = true;
        while (fetched)
        {
            fetched = PerformIO(mystdin, mystdout, mystderr);
        }

        return WaitForReturn();
    }

    /**********************************************************************************/
    //! Destructor
    SCXProcess::~SCXProcess()
    {
        // Free remaining resources held by the parent process
        // The child process manages its own resources
        close(m_inForChild[W]);
        close(m_outForChild[R]);
        close(m_errForChild[R]);

        // Free everything except the terminating NULL
        for (std::vector<char *>::size_type i = 0; i < m_cargv.size() - 1; i++)
        {
            free(m_cargv[i]);
        }
    }

    /**********************************************************************************/
    //! Terminate the process
    void SCXProcess::Kill()
    {
        if (killpg(m_pid, SIGKILL) < 0 && errno != ESRCH)
        {
            throw SCXInternalErrorException(UnexpectedErrno(L"Unable to kill child process group", errno), SCXSRCLOCATION);
        }
    }

    /**********************************************************************************/
    //! Terminate the parent process, explicitly closing STDIN/OUT/ERR (so they flush)
    void SCXProcess::CloseAndDie()
    {
        /*
          Make an attempt to exit with proper error code, so parent will pick up the
          child exit as an exit, and not as a terminated (interrupted) process.
         */

        std::vector<char *> binExit;
        binExit.push_back( strdup("/bin/sh") );
        binExit.push_back( strdup("-c") );
        binExit.push_back( strdup("exit 1") );
        binExit.push_back( 0 );

        execvp(binExit[0], &binExit[0]);                // Replace the child process image

        /*
          We give up - we've tried to execv, and failed.  Just close and abort.
          This will be interpreted by the parent as an interrupted process.
        */

        char error_msg[1024];
        snprintf(error_msg, sizeof(error_msg), "Failed to start exit shell '%s' errno=%d  ", binExit[0], errno);
        DoWrite(STDERR_FILENO, error_msg, strlen(error_msg));

        // Close each of the channels that the parent has handles to
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        // Abort.  We do this so distructors don't run.  This routine is normally
        // called after a fork(), but when execv() fails.  In that state, objects
        // contructed from the parent may not distruct properly (mutexes, etc).
        abort();
    }

    /**********************************************************************************/
    //! Read available data from socket and put it in the stream.
    //! \param fd Socket/file descriptor to read from.
    //! \param stream Stream to write data to.
    //! \returns true if there might be more data to read or false if the socket is closed.
    //! \throws SCXInternalErrorException if the socket read fails.
    bool SCXProcess::ReadToStream(int fd, std::ostream& stream)
    {
        // The pipe is now opened non-blocking, so keep reading until no bytes
        // are read (we won't block). Non-blocking reads are necessary to avoid
        // a race condition at child process shutdown: If we did a partial read
        // and then the child process ended, remaining bytes can be discarded.

        ssize_t bytesRead;
        do
        {
            bytesRead = read(fd, &m_buffer[0], m_buffer.size());
        if (bytesRead == 0) {
            return false;
        } else if (bytesRead < 0) {
            // If no data is available for reading, just return
            if (EAGAIN == errno)
            {
                return true;
            }

            throw SCXInternalErrorException(UnexpectedErrno(L"Process communication failed", errno), SCXSRCLOCATION);
        }
            stream.write(&m_buffer[0], bytesRead);
        } while (true);

        return true;
    }

    /**********************************************************************************/
    //! Thin wrapper around write system call.
    //! \param fd File descriptor to write to.
    //! \param buf Buffer with data.
    //! \param size Size of data in buffer.
    //! \returns Number of bytes written or negative for errors.
    ssize_t SCXProcess::DoWrite(int fd, const void* buf, size_t size)
    {
        return write(fd, buf, size);
    }

    /**********************************************************************************/
    //! Wrapper around the waitpid systemcall. Caches the result so you can call this
    //! method several times.
    //! \param[out] pExitCode If not null the content of this pointer will be set to the
    //!                       exit code of the process if it has exited.
    //! \param[in] blocking True if caller want to block while waiting otherwise false.
    //! \returns The PID of the process being waited for if it has exited, zero if process
    //!          is still running or negative for errors (check errno).
    int SCXProcess::DoWaitPID(int* pExitCode, bool blocking)
    {
        SCXProcessId pid = 0;
        if (m_waitCompleted)
        {
            pid = m_pid;
        }
        else
        {
            pid = waitpid(m_pid, &m_processExitCode, blocking?0:WNOHANG);
            if (pid == m_pid)
            {
                m_waitCompleted = true;
            }
        }

        if (pExitCode != NULL)
        {
            *pExitCode = m_processExitCode;
        }
        return pid;
    }

#endif /* SCX_UNIX */


} /* namespace SCXCoreLib */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
