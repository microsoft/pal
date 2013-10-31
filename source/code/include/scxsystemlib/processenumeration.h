/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief          Enumeration of Process Items
    \date           07-10-29 15:24:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef PROCESSENUMERATION_H
#define PROCESSENUMERATION_H

#include <map>

#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxthread.h>
#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/processinstance.h>

#include <errno.h>
#include <vector>

#if defined(linux) || defined(sun)
#include <sys/types.h>
#include <dirent.h>
#endif

#if defined(aix)

#if PF_MAJOR < 6
extern "C" int getprocs64 (void *procsinfo, int sizproc, void *fdsinfo, int sizfd, pid_t *index, int count);
#endif // PF_MAJOR < 6

#include <procinfo.h>

#endif // defined(aix)

#if defined(hpux)
#include <sys/pstat.h>
#endif


class ProcessPAL_Test;

namespace SCXSystemLib
{
    /** Time between each sample in seconds. */
    const int PROCESS_SECONDS_PER_SAMPLE = 60;

    /** Type of live process map. One pid corresponds to one process. */
    typedef std::map<scxpid_t, SCXCoreLib::SCXHandle<ProcessInstance> > ProcMap;

    /*----------------------------------------------------------------------------*/
    /**
        Class that represents a collection of Process:s.
        
        PAL Holding collection of Process:s.
    */
    class ProcessEnumeration : public EntityEnumeration<ProcessInstance>
    {
    public:
        static const wchar_t *moduleIdentifier;         //!< Module identifier

        ProcessEnumeration();
        ~ProcessEnumeration();
        virtual void Init();
        virtual void Update(bool updateInstances=true);
        virtual void CleanUp();

        size_t Size() const;

        const SCXCoreLib::SCXThreadLockHandle& GetLockHandle() const;
        void UpdateNoLock(SCXCoreLib::SCXThreadLock& lck, bool updateInstances=true);

        /* This one is public for testing purposes */
        void SampleData();

        SCXCoreLib::SCXHandle<ProcessInstance> Find(scxpid_t pid);
        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > Find(const std::wstring& name);
        static bool SendSignalByName(const std::wstring& name, int sig);
        static bool GetNumberOfProcesses(unsigned int& numberOfProcesses);

    private:
        SCXCoreLib::SCXLogHandle m_log;                         //!< Handle to log file 
        SCXCoreLib::SCXThreadLockHandle m_lock; //!< Handles locking in the process enumeration.

        SCXCoreLib::SCXHandle<SCXCoreLib::SCXThread> m_dataAquisitionThread; //!< Thread pointer.
        static void DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param);

        /** Map of active processes */
        ProcMap m_procs;

        int m_EnumErrorCount;    //!< Number of consecutive enumeration attempts with errors.
        int m_EnumGoodCount;     //!< Number of consecutive enumeration attempts without errors.
        SCXCoreLib::SCXLogSeverity m_EnumLogLevel;  //!< Log level to use when logging execption during instance update

    protected:
        friend class ::ProcessPAL_Test;

        /*
         * ProcLister definition / implmentation
         *
         * ProcLister is a convenience class that lists all current processes on
         * various platforms.  It is only used within processenumeration.cpp, but
         * is included in this header file for unit test purposes.
         */

#if defined(linux) || defined(sun)

        /**
         * Iterator for all directories under /proc that represents a process.
         * This is a local convenience class that encapsulates the opendir() and
         * readdir() methods.
         */
        class ProcLister {
        public:

            /** Starts a new process directory iterator */
            ProcLister() : ent(0) {
                d = opendir("/proc/");
                if (!d) { throw SCXCoreLib::SCXErrnoException(L"opendir", errno, SCXSRCLOCATION); }
            };

            /** Ends the iterator */
            ~ProcLister() {
                closedir(d);
            };

            /**
             * Advances the iterator and tests if there are any more processes.
             * \returns an indicator if we've reached the end. If this is true the
             *         getHandle() call can be used to retrieve the current entry
             */
            bool nextProc() {
                errno = 0;
                while((ent = readdir(d))) {
                    if (isdigit(ent->d_name[0])) {
                        return true;
                    }
                }
                // Test if readdir failed.
                if (errno != 0) { throw SCXCoreLib::SCXErrnoException(L"readdir", errno, SCXSRCLOCATION); }
                return false;
            }

            /**
             * Gets the name of the next process
             *  \returns the process direcory name
             */
            const char *getHandle() { return ent->d_name; }

            /**
             * Returns the pid of the next process.
             * \returns the process id
             * This method has some domain knowledge on the underlying type
             * of scxpid_t.
             */
            scxpid_t getPid() { return strtoul(ent->d_name, static_cast<char**>(0), 10); }

        private:
            DIR *d;                 //!< Internal representation of a directory
            struct dirent *ent;     //!< Internal representation of a directory entry
        };
#endif

#if defined(aix)
        /**
         * Iterator for process information entries on a HP/UX system. This is local
         * a convenience class that presents data from the getprocs64 interface in a
         * similar way to how we receive data from the /proc interface on Sun and Linux.
         */
        class ProcLister {
            static const size_t burst_size = 10; //!< Number of entries to read per burst

        public:
            /** Starts a new process iterator */
            ProcLister() : m_index(0), m_procIndex(0), m_burst(0), m_handle(0) {
                m_procs.reserve( burst_size );
                readburst();
            }

            /** Deallocates memory */
            ~ProcLister() {
            }

            /**
             * Advances iterator to next process.
             * \returns false if the iterator has reached the end
             */
            bool nextProc() {
                while (true)
                {
                    if (m_index >= m_burst) {
                        if (readburst() == false) {
                            return false;
                        }
                    }
                    // If we're a kernel process, skip (be compatible with 'ps')
                    if ((m_procs[m_index].pi_flags & SKPROC) != 0)
                    {
                    m_index++;
                    continue;
                    }
                    break;
                }

                m_handle = &m_procs[m_index++];
                return true;
            }

            /**
             * Gets data structure for next process
             * \returns process data in AIX specific format
             */
            struct procentry64 *getHandle() { return m_handle; }

            /**
             * Returns the pid of the next process.
             * \returns the process id
             */
            scxpid_t getPid() { return static_cast<scxpid_t>(m_handle->pi_pid); }

        private:
            /**
             * Reads a variable size burst of process data entries.
             * \returns true if method was able to read process data
             */
            bool readburst() {
                m_burst = getprocs64(&m_procs[0], sizeof(struct procentry64), 0, 0, &m_procIndex, burst_size);
                if (m_burst < 0) { throw SCXCoreLib::SCXErrnoException(L"getprocs64", errno, SCXSRCLOCATION); }
                m_index = 0;
                return m_burst != 0;
            }

            int m_index;                 //!< Next index within current m_procs array
            pid_t m_procIndex;           //!< Process identifier of required process table entry
            int m_burst;                 //!< Number of items in last read burst
            struct procentry64 *m_handle; //!< Pointer to data that is current in iterator
            std::vector< struct procentry64 > m_procs; //!< Array that holds the current burst for procentry64
        };
#endif

#if defined(hpux)
        /**
         * Iterator for process information entries on a HP/UX system. This is local
         * a convenience class that presents data from the pstat interface in a
         * similar way to how we receive data from the /proc interface on Sun and Linux.
         */
        class ProcLister {
            static const size_t burst_size = 150; //!< Number of entries to read per burst

        public:
            /** Starts a new process iterator */
            ProcLister() : m_index(0), m_idx(0), m_burst(0), m_handle(0) {
                pst.reserve( burst_size );
                readburst();
            }

            /** Deallocates memory */
            ~ProcLister() {
            }

            /**
             * Advances iterator to next process.
             * \returns false if the iterator has reached the end
             */
            bool nextProc() {
                if (m_index >= m_burst) {
                    if (readburst() == false) {
                        return false;
                    }
                }
                m_handle = &pst[m_index];
                m_index++;
                return true;
            }

            /**
             * Gets data structure for next process
             * \returns process data in HP/UX specific format
             */
            struct pst_status *getHandle() { return m_handle; }

            /**
             * Returns the pid of the next process.
             * \returns the process id
             */
            scxpid_t getPid() { return static_cast<scxpid_t>(m_handle->pst_pid); }

        private:
            /**
             * Reads a variable size burst of process data entries.
             * \returns true if method was able to read process data
             */
            bool readburst() {
                m_burst = pstat_getproc(&pst[0], sizeof(pst[0]), burst_size, m_idx);
                if (m_burst < 0) { throw SCXCoreLib::SCXErrnoException(L"pstat_getproc", errno, SCXSRCLOCATION); }
                m_idx = m_burst > 0 ? pst[m_burst - 1].pst_idx + 1 : -1;
                m_index = 0;
                return m_burst != 0;
            }

            int m_index;                 //!< Next index within current pst array
            int m_idx;                   //!< Next index after currect burst
            int m_burst;                 //!< Number of items in last read burst
            struct pst_status *m_handle; //!< Pointer to data that is current in iterator
            std::vector< struct pst_status > pst; //!< Array that holds the current burst
        };
#endif
    };
}

#endif /* PROCESSENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
