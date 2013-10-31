/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Enumeration of Process Items

   \date        07-10-29 15:22:00

*/
/*----------------------------------------------------------------------------*/
#include <errno.h>
#include <sys/time.h>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxcondition.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>

#include <scxsystemlib/processenumeration.h>
#include <scxsystemlib/processinstance.h>

#include <unistd.h>
#include <vector>

using namespace std;
using namespace SCXCoreLib;


namespace SCXSystemLib
{
    /** Module name string */
    const wchar_t *ProcessEnumeration::moduleIdentifier = L"scx.core.common.pal.system.process.processenumeration";

    /*----------------------------------------------------------------------------*/
    /**
       Class that represents values passed between the threads of the process instance.

       Representation of values passed between the threads of the process instance.

       I really don't know why we have this class, but all our pals have it so we'd
       better have it too or be outcasts.
    */
    class ProcessEnumerationThreadParam : public SCXThreadParam
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Constructor

           \param[in] processenum Pointer to process enumeration associated with the thread.
        */
        ProcessEnumerationThreadParam(ProcessEnumeration *processenum)
            : SCXThreadParam(), m_processEnum(processenum)
        {}

        /*----------------------------------------------------------------------------*/
        /**
           Retrieves the process enumeration parameter.

           \returns Pointer to process enumeration associated with the thread.
        */
        ProcessEnumeration* GetProcessEnumeration()
        {
            return m_processEnum;
        }
    private:
        ProcessEnumeration* m_processEnum; //!< Pointer to process enumeration associated with the thread.
    };

    /*==================================================================================*/

    /**
       Default constructor
    */
    ProcessEnumeration::ProcessEnumeration()
        : EntityEnumeration<ProcessInstance>(),
          m_lock(SCXCoreLib::ThreadLockHandleGet()),
          m_dataAquisitionThread(0),
          m_EnumErrorCount(0),
          m_EnumGoodCount(0),
          m_EnumLogLevel(eError)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);

        SCX_LOGTRACE(m_log, L"ProcessEnumeration default constructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor. This destructor must remove the elements from various
       containers that have elements that are pointers to classes. Also kills
       the sampler thread hard if not shut down gracefully (by using CleanUp).
    */
    ProcessEnumeration::~ProcessEnumeration()
    {
        SCX_LOGTRACE(m_log, L"ProcessEnumeration::~ProcessEnumeration()");

        if (0 != m_dataAquisitionThread)
        {
            if (m_dataAquisitionThread->IsAlive())
            {
                CleanUp();
            }
            m_dataAquisitionThread = 0;
        }

        // Remove these pointers so that we don't try to delete them twice
        Clear();

        m_procs.clear();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Starts collection thread which creates process instances.
    */
    void ProcessEnumeration::Init()
    {
        SCX_LOGTRACE(m_log, L"ProcessEnumeration Init()");

        // There is no total instance
        SetTotalInstance(SCXCoreLib::SCXHandle<ProcessInstance>(0));

        // Start collection thread.
        if (NULL == m_dataAquisitionThread)
        {
            ProcessEnumerationThreadParam* params = new ProcessEnumerationThreadParam(this);
            m_dataAquisitionThread = new SCXCoreLib::SCXThread(ProcessEnumeration::DataAquisitionThreadBody, params);
        }
        SCXCoreLib::SCXThread::Sleep(500);      // Give us some time to start up
    }

    /*----------------------------------------------------------------------------*/
    /**
       Release the resources allocated.

       Must be called before deallocating this object. Will wait when stopping
       the sampler thread.

    */
    void ProcessEnumeration::CleanUp()
    {
        if (0 != m_dataAquisitionThread)
        {
            m_dataAquisitionThread->RequestTerminate();
            m_dataAquisitionThread->Wait();
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
       Readies all process data for reading.

       \param updateInstances Has no meaning

       \throws SCXInternalErrorException If any instance is not a ProcessInstance

       Update first clobbers the instance list. It then iterates over the
       map of current process instances and adds them to the instance list that's
       inherited from entity enumeration. 
    */ 
    void ProcessEnumeration::Update(bool updateInstances)
    {
        // Inhibit data sampler from running for the duration of this function
        SCX_LOGHYSTERICAL(m_log, L"Update - Aquire lock ");
        SCXCoreLib::SCXThreadLock lock(m_lock);
        SCX_LOGHYSTERICAL(m_log, L"Update - Lock aquired, get data ");

        // std::cout << "ProcessEnumeration::Update()" << std::endl;

        UpdateNoLock(lock, updateInstances);
    }
 
    /**
       Readies all process data for reading.

       \param lck A previously taken lock that belongs to this PAL
       \param updateInstances Has no meaning

       \throws SCXInternalErrorException If any instance is not a ProcessInstance

       Update first clobbers the instance list. It then iterates over the
       map of current process instances and adds them to the instance list that's
       inherited from entity enumeration. 

       This is a version of Update() that does not actively lock the enumeration lock for
       processes. The caller is responsible for getting the lock handle with the 
       member function GetLockHandle() and creating the lock with SCXThreadLock.
       The lock must be supplied as "proof" that the lock was taken.
    */ 
    void ProcessEnumeration::UpdateNoLock(SCXCoreLib::SCXThreadLock&, bool)
    {
        Clear();                // Only removes pointers to instances from vector

        /*
         * Iterate over processes that were alive when latest sample were taken.
         * Add (a pointer to) each one to the vector of instances
         * But first compute the time-dependent values.
         */
        SCX_LOGTRACE(m_log, StrAppend(L"Update(): Number of live processes : ",m_procs.size()));

        ProcMap::iterator pi;
        for (pi = m_procs.begin(); pi != m_procs.end(); ++pi) {
            SCXCoreLib::SCXHandle<ProcessInstance> p = pi->second;
            p->UpdateTimedValues();
            AddInstance(p);
            SCX_LOGHYSTERICAL(m_log, StrAppend(L"Adding live pid: ", p->DumpString()));
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Returns the number of elements in the enumeration.

       \returns Number of elements in enumeration collection.

       \date        08-01-03 14:45

       This method overides the base class implementation in order to make it thread
       safe, i.e. the size returned should not be affected by ongoing updates.
       Since this method might be called when the lock is already in place, it first 
       checks to see if it has the lock before trying to aquire it.
    */
    size_t ProcessEnumeration::Size() const
    {
        SCX_LOGHYSTERICAL(m_log, L"Size - Aquire lock ");
        SCXCoreLib::SCXThreadLock lock(m_lock, false);
        if ( ! lock.HaveLock()) // Guard against locking multiple times...
        {
            lock.Lock();
        }
        SCX_LOGHYSTERICAL(m_log, L"Size - Lock aquired, get data ");
        return EntityEnumeration<ProcessInstance>::Size();
    }
        
    /*----------------------------------------------------------------------------*/
    /**
       Return lock handle of the enumeration.

       \returns Enumeration lock handle.

       \date        08-01-03 14:45
    */
    const SCXCoreLib::SCXThreadLockHandle& ProcessEnumeration::GetLockHandle() const
    {
        return m_lock;
    }

    /*=============================================================================*/
    /* Only code that run in local thread beyond this point.                       */
    /*=============================================================================*/

    /**
       Thread body for local updater thread.

       \param  param Must contain a parameter named "ParamValues" of type ProcessEnumerationThreadParam*

       This is a loop that runs continiously until the process enumeration
       goes out of scope.  It lists the processes at a regular interval,
       tests if these processes correspond to those we already know about.
       If a new process is found it's added to the list. If there exists an
       old process in out internal list that doesen't exists in the system
       list, that process is moved to a special list of dead processes.
    */
    void ProcessEnumeration::DataAquisitionThreadBody(SCXCoreLib::SCXThreadParamHandle& param)
    {
        SCXLogHandle log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGTRACE(log, L"ProcessEnumeration::DataAquisitionThreadBody()");

        ProcessEnumerationThreadParam* p = static_cast<ProcessEnumerationThreadParam*>(param.GetData());
        SCXASSERT(0 != p);

        ProcessEnumeration *processEnum = p->GetProcessEnumeration();
        SCXASSERT(0 != processEnum);

        int countup = 0;        // Consequtive number of good enumerations
        int countdown = 3;      // Consequtive number of exceptions before stop logging errors
        bool bUpdate = true;
        SCXLogSeverity logsev = eError;

        p->m_cond.SetSleep(PROCESS_SECONDS_PER_SAMPLE * 1000);
        {
            SCXConditionHandle h(p->m_cond);
            while( ! p->GetTerminateFlag() )
            {
                // Be sure to update the first time through
                if ( bUpdate )
                {
                    try {
                        processEnum->SampleData();
                        SCX_LOGHYSTERICAL(log, L"ProcessEnumeration DataAquisition - Sleep ");
                        // If we have had 10 consecutive enumerations without problems, reset 
                        // number of allowed consecutive error logs and set log severity to Error
                        if (countup > 9)
                        {
                            countdown = 3;
                            logsev = eError;
                        }
                        else
                        {
                            countup++;
                        }
                    } catch (SCXException& e) {
                        countup = 0;
                        // If we have had all allowed consecutive error logs set log severity to Trace
                        if (countdown > 0)
                        {
                            --countdown;
                        }
                        else
                        {
                            logsev = eTrace;
                        }
                        SCX_LOG(log, logsev, e.Where() + L" : " + e.What());
                    }
                    bUpdate = false;
                }

                enum SCXCondition::eConditionResult r = h.Wait();
                if (SCXCondition::eCondTimeout == r)
                {
                    bUpdate = true;
                }
            }
        }

        SCX_LOGHYSTERICAL(log, L"ProcessEnumeration DataAquisition - Ending ");
    }

    /**
       Makes a periodical sampling of process data.
       This method is run at a regular interval and updates existing process instances
       according to the system view. Newly created processes are added to the list
       of instances.
    */
    void ProcessEnumeration::SampleData()
    {
        ProcLister pl;          // Iterator for external list
        scxpid_t pid;
        ProcMap::iterator pos;
        struct timeval realtime;
        bool goterror = false;

        // Lock common data structures so that Update() don't get partial data
        SCX_LOGHYSTERICAL(m_log, L"SampleData - Aquire lock ");
        SCXCoreLib::SCXThreadLock lock(m_lock);
        SCX_LOGHYSTERICAL(m_log, L"SampleData - Lock aquired, get data ");

        /* Compute real time once to save some time. */
        gettimeofday(&realtime, 0);

        /* Walk through process iterator to see all live processes */
        while (pl.nextProc()) {

            pid = pl.getPid();
            /* Look for pid in process map */
            pos = m_procs.find(pid);

            try 
            {
                if (pos != m_procs.end()) {
                    /* If it was found, update it and mark it as found. */
                    bool stillExists = pos->second->UpdateInstance(pl.getHandle(), false);
                    if (!stillExists) { continue; } // Died before or during UpdateInstance()
                    pos->second->UpdateDataSampler(realtime);
                } else {
                    /* If it wasn't found, add it. */
                    SCXCoreLib::SCXHandle<ProcessInstance> inst( new ProcessInstance(pid, pl.getHandle()) );
                    bool stillExists = inst->UpdateInstance(pl.getHandle(), true);
                    if (!stillExists) { continue; } // Already gone. Not added.
                    inst->UpdateDataSampler(realtime);
                    m_procs.insert(std::make_pair(pid, inst));
                }
            } catch (SCXException& e) {
                goterror = true;
                SCX_LOG(m_log, m_EnumLogLevel, e.Where() + L" : " + e.What());
            }
        }

        // We log with severity Error only for 4 cosecutive enumerations, then we start logging Trace
        // until there has been 10 consecutive enumerations without a problem, then we return again to
        // use severity Error for problems.
        if (goterror)
        {
            m_EnumGoodCount = 0;
            if (m_EnumErrorCount < 3)
            {
                m_EnumErrorCount++;
            }
            else
            {
                m_EnumLogLevel = eTrace;
            }
        }
        else
        {
            m_EnumErrorCount = 0;
            if (m_EnumGoodCount < 9)
            {
                m_EnumGoodCount++;
            }
            else
            {
                m_EnumLogLevel = eError;
            }
        }

        /* Iterate over known processes and delete those who weren't
           present in "external list".
           That procedure will also reset the found-flag.
        */
        ProcMap::iterator pi;
        for (pi = m_procs.begin(); pi != m_procs.end(); ) {
            if (!pi->second->WasFound()) {             
                m_procs.erase(pi++); // Don't saw off branch!
            } else {
                ++pi;
            }
        }
    }

    /**
       Finds a process based on its pid.

       \param   pid     Process id to find
       \returns Handle to a process instance, or NULL if pid is not present in list.

       \note The returned process instance is guaranteed to be valid only until
       the next time that the SampleData() process runs. This means that you shouldn't
       use this call when the updater thread is running, unless you've taken steps 
       to lock that thread first. See ProcessEnumeration::GetLockHandle().
     */
    SCXCoreLib::SCXHandle<ProcessInstance> ProcessEnumeration::Find(scxpid_t pid) 
    { 
        ProcMap::iterator pos = m_procs.find(pid); 
        if (pos != m_procs.end()) {
            return pos->second;
        }
        return SCXCoreLib::SCXHandle<ProcessInstance>(0);
    }

    /**
       Finds a process based on its name.
       Multiple matching processes can be found.

       \param   name    Name of process that we're searching for
       \returns A vector with process instance pointer. The vector is empty is
       no processes with a matching name were found

       \note The returned process instance is guaranteed to be valid only until
       the next time that the SampleData() process runs. This means that you shouldn't
       use this call when the updater thread is running, unless you've taken steps 
       to lock that thread first. See ProcessEnumeration::GetLockHandle().
     */
    std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > ProcessEnumeration::Find(const wstring& name)
    {
        ProcMap::iterator pi;
        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > retval;
        const unsigned short Terminated = 7;
        unsigned short state;
        std::string pname;
        const std::string fname(SCXCoreLib::StrToUTF8(name));

        for (pi = m_procs.begin(); pi != m_procs.end(); ++pi) {
            if (pi->second->GetExecutionState(state) && (state != Terminated) &&
                pi->second->GetName(pname) && (pname == fname)) 
            {
                retval.push_back(pi->second);
            }
        }
        return retval;
    }

    /**
       Sends a signal (i.e. the POSIX kill() call) to one or more processes
       that has a certain name.
       \param  name     The process name without parameters or path.
       \param  sig      The POSIX signal name. Choose one from <signal.h>.
       \returns true if at least one process receives the signal
    */
    bool ProcessEnumeration::SendSignalByName(const wstring& name, int sig)
    {
        SCXCoreLib::SCXHandle<ProcessEnumeration> procEnum( new ProcessEnumeration() );
        /* No Init(), we do manual updates. */
        procEnum->SampleData();
        procEnum->Update(true);

        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > proclist = procEnum->Find(name);

        bool found = false;
        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> >::iterator pos;
        for(pos = proclist.begin(); pos != proclist.end(); ++pos) {
            found |= (*pos)->SendSignal(sig);
        }

        return found;
    }

    /**
       Static method that retrievs the number of processes running on the system.
       \param[out]  numberOfProcesses   Number of processes running.
       \returns true if this method is supported on the platform.
    */
    bool ProcessEnumeration::GetNumberOfProcesses(unsigned int& numberOfProcesses)
    {
#if defined(hpux)

        struct pst_dynamic pstd;
        if (pstat_getdynamic(&pstd, sizeof(pstd), 1, 0) != 1) 
        {
            return false;
        }

        numberOfProcesses = pstd.psd_activeprocs;
        return true;

#else

        ProcLister pl;
        int procCount = 0;

        while (pl.nextProc())
        {
            procCount++;
        }

        numberOfProcesses = procCount;

        return true;

#endif
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
