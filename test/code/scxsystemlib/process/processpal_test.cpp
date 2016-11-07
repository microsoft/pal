/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

  Created date    2007-10-31 11:27:00

  Process enumeration test class

*/
/*----------------------------------------------------------------------------*/
#include <errno.h>
#include <sys/wait.h>
#include <iostream>
#include <unistd.h>

// The following two are required by kill(2)
#include <sys/types.h>
#include <signal.h>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxthreadlock.h>

#include <scxsystemlib/processenumeration.h>
#include <scxsystemlib/processinstance.h>

#include <scxsystemlib/osenumeration.h>
#include <scxsystemlib/osinstance.h>

#include <testutils/scxunit.h>
#include <testutils/scxtestutils.h>

// dynamic_cast fix - wi 11220
#ifdef dynamic_cast
#undef dynamic_cast
#endif

using namespace SCXCoreLib;
using namespace SCXSystemLib;
using namespace std;


class ProcessPAL_ThreadParam : public SCXThreadParam
{
public:
    ProcessPAL_ThreadParam(SCXCoreLib::SCXHandle<ProcessEnumeration> procEnum)
    {
        m_procEnum = procEnum;
    }

    SCXCoreLib::SCXHandle<ProcessEnumeration> GetProcEnum() const { return m_procEnum; }

protected:
    SCXCoreLib::SCXHandle<ProcessEnumeration> m_procEnum;
};

class TestProcessEnumeration : public ProcessEnumeration
{
public:
    TestProcessEnumeration() : ProcessEnumeration() { }

protected:
    virtual void AddInstance(SCXCoreLib::SCXHandle<ProcessInstance> instance)
    {
        ProcessEnumeration::AddInstance(instance);
        SCXThread::Sleep(1);
    }
};

#if defined(sun) && ((PF_MAJOR > 5) || (PF_MAJOR == 5 && PF_MINOR >= 10))
// Test class to validate proper behavior for enumeration of process in global vs. non-global zones

namespace SCXSystemLib
{
    class TestProcessInstance : public ProcessInstance
    {
    public:
        TestProcessInstance(bool inGlobal, zoneid_t zoneID)
            : ProcessInstance(SCXProcess::GetCurrentProcessID(), StrToUTF8(StrFrom(SCXProcess::GetCurrentProcessID())).c_str()),
              m_inGlobalZone(inGlobal),
              m_zoneID(zoneID)
        { }

        bool DoUpdateInstance()
        {
            return UpdateInstance("", false);
        }

    protected:
        virtual bool isInGlobalZone()
        {
            m_psinfo.pr_zoneid = m_zoneID;
            return m_inGlobalZone;
        }

    private:
        bool m_inGlobalZone;
        zoneid_t m_zoneID;
    };
}
#endif // defined(sun) && ((PF_MAJOR > 5) || (PF_MAJOR == 5 && PF_MINOR >= 10))

extern "C" {
    /* Some stuff for signal testing.

       Please note that we've set the default action of the 
       SIGUSR1 signal to "ignore". That change was made in testrunner.cpp.
    */

    static bool sigUSR1Received = false;

    static void usr1Handler(int sig)
    {
        if (sig == SIGUSR1) {
            sigUSR1Received = true;
        }
    }

    static void (*oldHandler)(int);// Note: SunStudio insists that this has extern "C" linkage.
}

class ProcessPAL_Test : public CPPUNIT_NS::TestFixture
{
    // Note: Some tests *must not* be run in a subthread since forking from a non-main
    // thread wrecks havoc with process IDs on SLES9.
    CPPUNIT_TEST_SUITE( ProcessPAL_Test  );
    CPPUNIT_TEST( callDumpStringForCoverage );
    CPPUNIT_TEST( testNoTotalInstanceExists );
    CPPUNIT_TEST( testAtleastOneProcess );
    CPPUNIT_TEST( testCurrentPIDFound );
    CPPUNIT_TEST( testCurrentProcessValues );
    CPPUNIT_TEST( testPIDandNamesFound );
    CPPUNIT_TEST( testForSanity );
    CPPUNIT_TEST( testRandomProcessValues );
    CPPUNIT_TEST( testTerminatingProcess );
    SCXUNIT_TEST( testZombie, 0 );
    SCXUNIT_TEST( testZombieName, 0 );
    SCXUNIT_TEST( testParentPID, 0 );
    CPPUNIT_TEST( testUsedMemory );
    CPPUNIT_TEST( testThreadSafeSizeCallableWithLockHeld );
    CPPUNIT_TEST( testBug2277 );
    CPPUNIT_TEST( testNamedFind );
    CPPUNIT_TEST( testKillByName );
    CPPUNIT_TEST( testProcessPriorities );
    SCXUNIT_TEST( testGetParameters, 0 );
#ifndef hpux
    SCXUNIT_TEST( testGetParametersGreaterThan1024, 0 );
#endif
    CPPUNIT_TEST( testProcNameWithSpace );
    CPPUNIT_TEST( testSymbolicLinksReturnSymbolicName );
    CPPUNIT_TEST( testProcLister );
#if defined(sun) && ((PF_MAJOR > 5) || (PF_MAJOR == 5 && PF_MINOR >= 10))
    CPPUNIT_TEST( testSolaris10_GlobalZone_ProcessInGlobalZone );
    CPPUNIT_TEST( testSolaris10_GlobalZone_ProcessInNonGlobalZone );
    CPPUNIT_TEST( testSolaris10_NotGlobalZone_ProcessInNonGlobalZone );
#endif // defined(sun) && ((PF_MAJOR > 5) || (PF_MAJOR == 5 && PF_MINOR >= 10))

    SCXUNIT_TEST_ATTRIBUTE(callDumpStringForCoverage, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testNoTotalInstanceExists, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testAtleastOneProcess, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testCurrentPIDFound, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testCurrentProcessValues, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testPIDandNamesFound, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testRandomProcessValues, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testTerminatingProcess, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testZombie, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testZombieName, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testParentPID, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testUsedMemory, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testBug2277, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testNamedFind, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testKillByName, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testProcessPriorities, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testGetParameters, SLOW);
#ifndef hpux
    SCXUNIT_TEST_ATTRIBUTE(testGetParametersGreaterThan1024, SLOW);
#endif
    SCXUNIT_TEST_ATTRIBUTE(testProcNameWithSpace, SLOW);
    SCXUNIT_TEST_ATTRIBUTE(testSymbolicLinksReturnSymbolicName, SLOW);
    CPPUNIT_TEST_SUITE_END();

private:
    SCXCoreLib::SCXHandle<ProcessEnumeration> m_procEnum;

    static const bool fBrokenTestRepaired
#if defined(sun)
             = false;
#else
             = true;
#endif

    bool VerifyPIDSanity(scxulong pid)
    {
#if defined(aix) || defined(ppc)
        // AIX doesn't assign PIDs sequentially so there isn't an obvious range that PIDs fall into
        (void)pid; 
        return true;
#elif defined(linux)
        return pid <= 65535;
#else
        return pid <= 999999L;
#endif
    }

    void AssertPIDSanity(scxulong pid, const string& msg)
    {
        CPPUNIT_ASSERT_MESSAGE(msg.c_str(), VerifyPIDSanity(pid));
    }

public:

    void setUp(void)
    {
        //cout << "\nsetUp" << endl;
        m_procEnum = 0;
            // Bypass need for root access
        ProcessInstance::m_inhibitAccessViolationCheck = true;
    }

    void tearDown(void)
    {
        //cout << "\ntearDown" << endl;

        ProcessInstance::m_inhibitAccessViolationCheck = false;
        if (0 != m_procEnum)
        {
            m_procEnum->CleanUp();
            m_procEnum = 0;
        }
    }

    void callDumpStringForCoverage()
    {
        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(SCXCoreLib::SCXProcess::GetCurrentProcessID());
        CPPUNIT_ASSERT(0 != inst);
        CPPUNIT_ASSERT(inst->DumpString().find(L"testrunner") != wstring::npos);
    }

    void testNoTotalInstanceExists()
    {
        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> instance = m_procEnum->GetTotalInstance();
        CPPUNIT_ASSERT_EQUAL(static_cast<ProcessInstance*>(0), instance.GetData() );
    }

    void testAtleastOneProcess()
    {
        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();
        m_procEnum->Update(true);

        CPPUNIT_ASSERT(0 < m_procEnum->Size());
    }

    /* XXX Merge with above? */
    void testCurrentPIDFound()
    {
        scxulong pid;
        scxulong curpid = SCXCoreLib::SCXProcess::GetCurrentProcessID();
        string cmdname;

        map<scxulong, string> psOutput;
        map<scxulong, string>::iterator pos;

        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();

        GetPSEData(psOutput);
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(curpid);
        CPPUNIT_ASSERT(0 != inst );
        CPPUNIT_ASSERT(inst->GetPID(pid));
        CPPUNIT_ASSERT(pid == inst->getpid());
        CPPUNIT_ASSERT(inst->GetName(cmdname));
        // Test that pid is present in both sets
        pos = psOutput.find(pid);
        CPPUNIT_ASSERT(pos != psOutput.end());
        // Test that the command name is the same
        ostringstream msg;
        msg << "pos->second: " << pos->second << ", cmdname: " << cmdname << endl;
        CPPUNIT_ASSERT_MESSAGE (msg.str().c_str(), compareCmdNames(pos->second, cmdname));
    }

    /** Compares the process enumeration with the output from 'ps -el' and see to that
        there aren't too many differences. There may be some because 'ps' has its own
        opinion of names, and processes may die or be created in-between.
    */
    void testPIDandNamesFound()
    {
        scxulong pid;
        string cmdname;
        int pidsMissing = 0;
        unsigned int namesWrong = 0;
        ostringstream msg;

        map<scxulong, string> psOutput;
        map<scxulong, string>::iterator pos;

        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();

        GetPSEData(psOutput);
        m_procEnum->Update(true);

        for (ProcessEnumeration::EntityIterator iter = m_procEnum->Begin(); iter != m_procEnum->End(); ++iter) {
            SCXCoreLib::SCXHandle<ProcessInstance> inst = *iter;
            CPPUNIT_ASSERT(inst->GetPID(pid));
            CPPUNIT_ASSERT(inst->GetName(cmdname));

            // Test that pid is present in both sets (but ignore defunct processes)
            // (Our agent reports defunct process names differently than ps does)
            pos = psOutput.find(pid);
            if (pos != psOutput.end())
            {
                unsigned short procState;

                // Test that the command name is the same
                // If process name differs, verify that:
                //      Process state is NOT available
                //   OR Process state <> 7 (terminated) - terminated == defunct
                if ( ! compareCmdNames(pos->second, cmdname)
                     && (!inst->GetExecutionState(procState) || procState != 7)) {
                    msg << "Warning - Process names differ for pid: " 
                        << pid << " (" << pos->second << " != >" << cmdname << "<)" << std::endl;
                    ++namesWrong;
                }
            }
            else
            {
                msg << "PID not found " << pid << std::endl;
                ++pidsMissing;
            }

            // And then test that nothing dumps core
            SweepProcessInstance(inst);
        }

        // Assuming no more than ten processes have been killed/created between real
        // snapshot and verification snapshot.
        CPPUNIT_ASSERT_DOUBLES_EQUAL(0, pidsMissing, 10);

        CPPUNIT_ASSERT_MESSAGE("All process names are wrong",
                               namesWrong < m_procEnum->Size());

        // cout << "Number of wrong names: " << namesWrong << endl;

        // Sometimes processes change names. More specifically when exec() is called
#if defined(aix)
        // AIX platform APIs behave somewhat differently than 'ps' does; be slightly more forgiving
        // (HP does too, but since HP ignores all sh/bash processes, it doesn't need special treatment)
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str(), 0, namesWrong, 10);
#else
        CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE(msg.str(), 0, namesWrong, 2);
#endif
    }

    /** Do some sanity checking on some of the values returned from the process
        PAL.  Note: These tests use arbitrary limits (i.e. it's likely that a
        process size matches:  100,000bytes <= process size <= 2GB.

        If one of these sanity tests fail, feel free to bump the test to a
        rational limit.  This test is solely for sanity testing (i.e. process
        size isn't going to be hundreds of gigabytes in size).
     */
    void testForSanity()
    {
        ostringstream errStream;

        /*
         * We need to get some times (from boot) in a variety of formats
         */

        // First, seconds since system boot (or settle for epoch)

        scxulong ulUptime = 0;

        SCXCoreLib::SCXHandle<OSEnumeration> osEnum(new OSEnumeration());
        osEnum->Init();
        osEnum->Update(true);
        SCXCoreLib::SCXHandle<OSInstance> osInst = osEnum->GetTotalInstance();

        if (!osInst->GetSystemUpTime(ulUptime)) {
            // We can't get the real system uptime, so fake it
            ulUptime = time(NULL);
        }

        CPPUNIT_ASSERT(static_cast<time_t> (ulUptime) != static_cast<time_t> (-1));

        // Next: get the system boot time as an SCXCalendarTime

        scxulong ulBootTime = time(NULL) - ulUptime;
        SCXCalendarTime ctBootTime = SCXCalendarTime::FromPosixTime(ulBootTime);

        /*
          Now run through each process and test for sanity
        */

        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();
        m_procEnum->Update(true);

        SCXCalendarTime ctCurrentTime = SCXCalendarTime::CurrentUTC();

        // WI 11296: Adjust by a "fudge factor" to try and avoid failures from time changes

        SCXCoreLib::SCXRelativeTime timeFudge(0, 0, 0, 0, 5, 0.0);
        if (ulBootTime > (5 * 60)) {
            ctBootTime -= timeFudge;
        }
        ctCurrentTime += timeFudge;

        errStream << std::endl;
        errStream << "Boot time: " << StrToUTF8(ctBootTime.ToExtendedISO8601()) << std::endl;
        errStream << "Cur  time: " << StrToUTF8(ctCurrentTime.ToExtendedISO8601()) << std::endl;

        for (ProcessEnumeration::EntityIterator iter = m_procEnum->Begin(); iter != m_procEnum->End(); ++iter) {
            ostringstream subStream;
            SCXCoreLib::SCXHandle<ProcessInstance> inst = *iter;

            subStream << errStream.str();

            bool result;
            unsigned short usVal;
            int iVal;
            unsigned int uiVal;
            scxulong ulVal;
            std::string strVal;
            wstring wstrVal;
            SCXCoreLib::SCXCalendarTime ctTime;

            /*
             * Properties in SCX_UnixProcess
             */

            CPPUNIT_ASSERT(inst->GetPID(ulVal));
            subStream << "Pid=" << ulVal;
            AssertPIDSanity(ulVal,subStream.str());

            CPPUNIT_ASSERT(inst->GetName(strVal));
            CPPUNIT_ASSERT(strVal.size());

            result = inst->GetNormalizedWin32Priority(uiVal);
            CPPUNIT_ASSERT( result );
            subStream << ", PrioNorm=" << uiVal;
            CPPUNIT_ASSERT_MESSAGE(subStream.str().c_str(), uiVal <= 31);

            result = inst->GetNativePriority(iVal);
            CPPUNIT_ASSERT( result );
            subStream << ", PrioNat=" << iVal;
#if defined(linux)
                SCXUNIT_ASSERT_BETWEEN_MESSAGE(subStream.str().c_str(), iVal, -40, 99);
#elif defined(sun)
                SCXUNIT_ASSERT_BETWEEN_MESSAGE(subStream.str().c_str(), iVal, 0, 169);
#elif defined(hpux)
                SCXUNIT_ASSERT_BETWEEN_MESSAGE(subStream.str().c_str(), iVal, -512, 255);
#elif defined(aix)
                SCXUNIT_ASSERT_BETWEEN_MESSAGE(subStream.str().c_str(), iVal, 0, 255);
#endif

            CPPUNIT_ASSERT(inst->GetExecutionState(usVal));
            subStream << ", ExecState=" << usVal;
            CPPUNIT_ASSERT_MESSAGE(subStream.str().c_str(), usVal <= 11);

            CPPUNIT_ASSERT(inst->GetCreationDate(ctTime));
            ctTime.MakeUTC();
            subStream << ", CreationTime=" << StrToUTF8(ctTime.ToExtendedISO8601());
            // WI 18796: Don't bother validating process creation date/time.  In a VM enviroment,
            // it's just wrong too much, and never worked right for SLES 9 anyway.

            if (inst->GetTerminationDate(ctTime))
            {
                // If supported, it should be between boot time and now
                ctTime.MakeUTC();

                subStream << ", TermTime=" << StrToUTF8(ctTime.ToExtendedISO8601());
                SCXUNIT_ASSERT_BETWEEN_MESSAGE(subStream.str().c_str(), ctTime, ctBootTime, ctCurrentTime);
            }

            CPPUNIT_ASSERT(inst->GetParentProcessID(iVal));
            subStream << ", ParentPID=" << iVal;
            AssertPIDSanity(static_cast<scxulong>(iVal),subStream.str());

            CPPUNIT_ASSERT(inst->GetRealUserID(ulVal));
            subStream << ", RealUserID=" << ulVal;

            CPPUNIT_ASSERT(inst->GetProcessGroupID(ulVal));
            subStream << ", ProcessGroupID=" << ulVal;
            CPPUNIT_ASSERT_MESSAGE(subStream.str().c_str(), VerifyPIDSanity(ulVal) || static_cast<long> (ulVal) == -1);

            CPPUNIT_ASSERT(inst->GetProcessNiceValue(uiVal));
            /*
              Since NICE can be negative on Linux, ProcessProvider
              must offset this value by + 20 to avoid returning things like:
                  ProcessNiceValue = 4294967292
            */
            subStream << ", Nice=" << uiVal;
            CPPUNIT_ASSERT_MESSAGE(subStream.str().c_str(), uiVal < 100);

            /* 
             * Properties in SCX_UnixProcess, Phase 2
             */

            if (inst->GetOtherExecutionDescription(wstrVal))
            {
                // If platform supports this, it should return something
                subStream << ", ExecDesc=\"" << StrToUTF8(wstrVal) << "\"";
                CPPUNIT_ASSERT_MESSAGE(subStream.str().c_str(), wstrVal.size());
            }

            // Kernel time, in milliseconds
            // (We don't test this anymore; didn't account for multiple processors, and OM doesn't consume this anyway)
            CPPUNIT_ASSERT(inst->GetKernelModeTime(ulVal));
            subStream << ", KernelTime=" << ulVal;

            // User mode time, in milliseconds
            // (We don't test this anymore; didn't account for multiple processors, and OM doesn't consume this anyway)
            CPPUNIT_ASSERT(inst->GetUserModeTime(ulVal));
            subStream << ", UserTime=" << ulVal;

            // Not supported on any platforms
            CPPUNIT_ASSERT(!inst->GetWorkingSetSize(ulVal));

            /*
              TO-DO: Fill in remaining PAL sanity testing here!
            */

            subStream << std::endl;
        }
    }

    /* Tests that all processes that is found by 'ps' also can be found by the process
       enumeration.
       This test isn't ready for prime-time. Fails on AIX sometimes, and Linux always.
    */
    void testBeforeAndAfter()
    {
        string cmdname;
        map<scxulong, string> psBefore;
        map<scxulong, string> sampled;
        map<scxulong, string> psAfter;
        map<scxulong, string>::iterator pos;
        map<scxulong, string>::iterator apos;
        map<scxulong, string>::iterator spos;

        m_procEnum = new ProcessEnumeration();

        /* Get ps data before process enumeration. */
        GetPSEData(psBefore);

        /* Do process enumeration */
        m_procEnum->SampleData();
        m_procEnum->Update(true);

        /* Get ps data after process enumeration. */
        GetPSEData(psAfter);

        CPPUNIT_ASSERT(0 != m_procEnum);

        /* Iterate over enumerated processes and put them in associative array
           for convenience.
        */
        for (ProcessEnumeration::EntityIterator iter = m_procEnum->Begin();
             iter != m_procEnum->End(); ++iter)
        {
            scxulong testpid;
            string cmdstr;
            SCXCoreLib::SCXHandle<ProcessInstance> inst = *iter;
            CPPUNIT_ASSERT(inst->GetPID(testpid));
            CPPUNIT_ASSERT(inst->GetName(cmdstr));
            sampled[testpid] = cmdstr;
        }

        // cout << "Size of ps before == " << psBefore.size() << endl;
        // cout << "Size of ps after  == " << psAfter.size() << endl;
        // cout << "Number of enumerated processes == " << m_procEnum->Size() << endl;

        // Iterate over pids in psAfter. Check if these are present in psBefore.
        // If so, the *should* also be present in 'sampled'.
        for(apos = psAfter.begin(); apos != psAfter.end(); ++apos) {
            scxulong testpid = apos->first;
            pos = psBefore.find(testpid);
            if (pos != psBefore.end()) {
                /* Present in both sets. */
                spos = sampled.find(testpid);
                if (spos == sampled.end()) {
                    /* PID not present in sampled list */
                    ostringstream msg;
                    msg << "Missing pid = " << testpid << " cmd = " << apos->second;
                    CPPUNIT_FAIL(msg.str().c_str());
                }
            } else {
                /* Added after psBefore was collected */
                // cout << "Added: pid = " << testpid << " cmd = " << apos->second << endl;
            }
        }
    }


    /** Tests the current process, that is the unit test process. */
    void testCurrentProcessValues()
    {
        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(SCXCoreLib::SCXProcess::GetCurrentProcessID());
        VerifyProcessInstance(inst);

        SCXCoreLib::SCXCalendarTime creation_time;
        CPPUNIT_ASSERT( ! inst->GetTerminationDate(creation_time));

#if !(defined(PF_DISTRO_SUSE) && PF_MAJOR==9)
        // Suse 9 appears to have an OS bug that sets the process creation time 60 seconds
        // into the future.

        CPPUNIT_ASSERT(inst->GetCreationDate(creation_time));
        SCXCoreLib::SCXCalendarTime now(SCXCoreLib::SCXCalendarTime::CurrentLocal());

        // We would like to do this test:
        // CPPUNIT_ASSERT(creation_time < now);
        // However:
        // Process creation time on linux can differ by seconds, so there is a risk that
        // 'now' comes before process start. It should not be more that the odd second, though.
        // On other systems this test fails occasionally, too.
        // It would make sense to run this in a separate fork since we measure the time from
        // process start.

        SCXCoreLib::SCXAmountOfTime tolerance;
        tolerance.SetSeconds(2);

        CPPUNIT_ASSERT((creation_time < now) || (creation_time - now) < tolerance);

#else
        SCXUNIT_WARNING(wstring(L"We don't test GetCreationDate() on SLES9 since it's known to drift. This is an OS bug in kernel version 2.6.5."));
#endif /* SLES9 */

        // XXX Work here!
        // XXX make assertions for many more values

        // Get the CPU percentage.
        unsigned int cpupercent;
        CPPUNIT_ASSERT(inst->GetCPUTime(cpupercent));
        CPPUNIT_ASSERT(cpupercent <= 100);

        // Blocks per second
        scxulong blockRpS, blockWpS, blockTpS;
#if defined(hpux) || defined(sun)
        CPPUNIT_ASSERT(inst->GetBlockReadsPerSecond(blockRpS));
        CPPUNIT_ASSERT(inst->GetBlockWritesPerSecond(blockWpS));
        CPPUNIT_ASSERT(inst->GetBlockTransfersPerSecond(blockTpS));
        // TODO: Must test underlying values (potentially) increase.
#elif defined(linux) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetBlockReadsPerSecond(blockRpS));
        CPPUNIT_ASSERT( ! inst->GetBlockWritesPerSecond(blockWpS));
        CPPUNIT_ASSERT( ! inst->GetBlockTransfersPerSecond(blockTpS));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif
        // Time percentages
        scxulong userTime, privilegedTime;
        CPPUNIT_ASSERT(inst->GetPercentUserTime(userTime));
        CPPUNIT_ASSERT(inst->GetPercentPrivilegedTime(privilegedTime));
        CPPUNIT_ASSERT(userTime <= 100);
        CPPUNIT_ASSERT(privilegedTime <= 100);

        scxulong vText, vData, vStack;
        // Get the RealXXX values.
#if defined(hpux)
        CPPUNIT_ASSERT(inst->GetRealText(vText));
        CPPUNIT_ASSERT(inst->GetRealData(vData));
        CPPUNIT_ASSERT(inst->GetRealStack(vStack));

#if 0
        // Can't test. Some processes may be "swapped out".
        CPPUNIT_ASSERT(0 < vText);
        CPPUNIT_ASSERT(0 < vData);
        CPPUNIT_ASSERT(0 < vStack);
#endif

#elif defined(linux) || defined(sun) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetRealText(vText));
        CPPUNIT_ASSERT( ! inst->GetRealData(vData));
        CPPUNIT_ASSERT( ! inst->GetRealStack(vStack));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif

        // Get the VirtualXXX values.
#ifndef aix
        CPPUNIT_ASSERT(inst->GetVirtualText(vText));
#endif
        CPPUNIT_ASSERT(inst->GetVirtualData(vData));
#if !defined(linux)
        CPPUNIT_ASSERT(inst->GetVirtualStack(vStack));
#else
        // Not supported on Linux
        CPPUNIT_ASSERT(!inst->GetVirtualStack(vStack));
#endif

#if 0
        // We can't test these for random processes. While they are true for most
        // processes, most of the time. They fail now and then depending on peculiarities
        // for certain processes.
        CPPUNIT_ASSERT(0 < vText);
        CPPUNIT_ASSERT(0 < vData);
        CPPUNIT_ASSERT(0 < vStack);
#endif
        scxulong memMapFileSize, memShared;
#if defined(hpux)
        CPPUNIT_ASSERT(inst->GetVirtualMemoryMappedFileSize(memMapFileSize));
        //CPPUNIT_ASSERT(0 < memMapFileSize); //TODO: Is this really true for all processes?NO!
#elif defined(linux) || defined(sun) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetVirtualMemoryMappedFileSize(memMapFileSize));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif
#if defined(hpux) || defined(linux)
        CPPUNIT_ASSERT(inst->GetVirtualSharedMemory(memShared));
        //CPPUNIT_ASSERT(0 < memShared); // TODO: Is this really true for all processes? NO!
#elif defined(sun) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetVirtualSharedMemory(memShared));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif

        // TimeDeadChildren
        scxulong cpuTDC, sysTDC;
        CPPUNIT_ASSERT(inst->GetCpuTimeDeadChildren(cpuTDC));
        CPPUNIT_ASSERT(inst->GetSystemTimeDeadChildren(sysTDC));
        //CPPUNIT_ASSERT(0 < cpuTDC); // TODO: Is this really true for all processes? NO!
        //CPPUNIT_ASSERT(0 < sysTDC); // TODO: Is this really true for all processes? NO!
    }

    /** Tests values for a process chosen at random. There are many values that we
        can't assert to any certain about.
    */
    void testRandomProcessValues()
    {
        if (!fBrokenTestRepaired)
        {
            return;
        }

        SCXCoreLib::SCXHandle<ProcessInstance> inst(0);
        m_procEnum = new ProcessEnumeration(); // Don't call Init() since we want manual update.
        m_procEnum->SampleData();
        m_procEnum->Update(true);

        // Try with max three different random pid:s since a given process might have terminated during the test
        srand(static_cast<unsigned int>(time(0)));
        for (int i=0; i<3; i++)
        {
            size_t r = rand() % m_procEnum->Size();
            inst = m_procEnum->GetInstance(r);
            if (VerifyProcessInstance(inst, i<2?true:false))
            {
                break;
            }
            if (i < 2)
            {
                cout << endl << "Test of random process with index " << r << " failed. Trying another process" << endl;
            }
        }

        // Get the CPU percentage.
        unsigned int cpupercent;
        CPPUNIT_ASSERT(inst->GetCPUTime(cpupercent));
        CPPUNIT_ASSERT(cpupercent <= 100);

        // XXX Remove those unused

        // Blocks per second
        scxulong blockRpS, blockWpS, blockTpS;
#if defined(hpux) || defined(sun)
        CPPUNIT_ASSERT(inst->GetBlockReadsPerSecond(blockRpS));
        CPPUNIT_ASSERT(inst->GetBlockWritesPerSecond(blockWpS));
        CPPUNIT_ASSERT(inst->GetBlockTransfersPerSecond(blockTpS));
        // TODO: Must test underlying values (potentially) increase.
#elif defined(linux) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetBlockReadsPerSecond(blockRpS));
        CPPUNIT_ASSERT( ! inst->GetBlockWritesPerSecond(blockWpS));
        CPPUNIT_ASSERT( ! inst->GetBlockTransfersPerSecond(blockTpS));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif
        // Time percentages
        scxulong userTime, privilegedTime;
        CPPUNIT_ASSERT(inst->GetPercentUserTime(userTime));
        CPPUNIT_ASSERT(inst->GetPercentPrivilegedTime(privilegedTime));
        CPPUNIT_ASSERT(userTime <= 100);
        CPPUNIT_ASSERT(privilegedTime <= 100);

        scxulong vText, vData, vStack;
        // Get the RealXXX values.
#if defined(hpux)
        CPPUNIT_ASSERT(inst->GetRealText(vText));
        CPPUNIT_ASSERT(inst->GetRealData(vData));
        CPPUNIT_ASSERT(inst->GetRealStack(vStack));

#if 0
        // Can't test. Some processes may be "swapped out".
        CPPUNIT_ASSERT(0 < vText);
        CPPUNIT_ASSERT(0 < vData);
        CPPUNIT_ASSERT(0 < vStack);
#endif

#elif defined(linux) || defined(sun) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetRealText(vText));
        CPPUNIT_ASSERT( ! inst->GetRealData(vData));
        CPPUNIT_ASSERT( ! inst->GetRealStack(vStack));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif

        // Get the VirtualXXX values.
#ifndef aix
        CPPUNIT_ASSERT(inst->GetVirtualText(vText));
#endif
        CPPUNIT_ASSERT(inst->GetVirtualData(vData));
#if !defined(linux)
        CPPUNIT_ASSERT(inst->GetVirtualStack(vStack));
#else
        // Not supported on Linux
        CPPUNIT_ASSERT(!inst->GetVirtualStack(vStack));
#endif

#if 0
        // We can't test these for random processes. While they are true for most
        // processes, most of the time. They fail now and then depending on peculiarities
        // for certain processes.
        CPPUNIT_ASSERT(0 < vText);
        CPPUNIT_ASSERT(0 < vData);
        CPPUNIT_ASSERT(0 < vStack);
#endif
        scxulong memMapFileSize, memShared;
#if defined(hpux)
        CPPUNIT_ASSERT(inst->GetVirtualMemoryMappedFileSize(memMapFileSize));
        //CPPUNIT_ASSERT(0 < memMapFileSize); //TODO: Is this really true for all processes?NO!
#elif defined(linux) || defined(sun) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetVirtualMemoryMappedFileSize(memMapFileSize));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif
#if defined(hpux) || defined(linux)
        CPPUNIT_ASSERT(inst->GetVirtualSharedMemory(memShared));
        //CPPUNIT_ASSERT(0 < memShared); // TODO: Is this really true for all processes? NO!
#elif defined(sun) || defined(aix)
        CPPUNIT_ASSERT( ! inst->GetVirtualSharedMemory(memShared));
#else
        CPPUNIT_FAIL("Platform not supported");
#endif

        // TimeDeadChildren
        scxulong cpuTDC, sysTDC;
        CPPUNIT_ASSERT(inst->GetCpuTimeDeadChildren(cpuTDC));
        CPPUNIT_ASSERT(inst->GetSystemTimeDeadChildren(sysTDC));
        //CPPUNIT_ASSERT(0 < cpuTDC); // TODO: Is this really true for all processes? NO!
        //CPPUNIT_ASSERT(0 < sysTDC); // TODO: Is this really true for all processes? NO!
    }

    /** Creates and kills a process and tests that the process is not returned
        gets resonable values.
    */
    void testTerminatingProcess()
    {
        m_procEnum = new ProcessEnumeration();
        /* We don't start an update thread here: m_procEnum->Init(); */

        // SCXCoreLib::SCXCalendarTime pre_fork = SCXCoreLib::SCXCalendarTime::CurrentLocal();
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        { // Child process
            SCXCoreLib::SCXThread::Sleep(1973);
            exit(0);
        }

        m_procEnum->SampleData(); // Manual update of data
        m_procEnum->Update(true);
        // SCXCoreLib::SCXCalendarTime pre_wait = SCXCoreLib::SCXCalendarTime::CurrentLocal();
        waitpid(pid, 0, 0);
        // SCXCoreLib::SCXCalendarTime post_wait = SCXCoreLib::SCXCalendarTime::CurrentLocal();

        m_procEnum->SampleData(); // Manual update of data
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT(0 == inst);

    }

    /** Creates a process that exits immediately. Tests if the zombie process is found.
    */
    void testZombie()
    {
#if defined(aix)
        /* First see if this test has any chance of succeeding. */
        if (detectAIX61ProcBug()) {
            SCXUNIT_WARNING(wstring(L"Can't test for zombie processes on this system. ProcessPAL_Test::testZombie test was disabled."));
            return;
        }
#endif

        const unsigned short Terminated = 7;
        pid_t testapp_pid = SCXCoreLib::SCXProcess::GetCurrentProcessID();
        m_procEnum = new ProcessEnumeration();
        /* We don't start an update thread here: m_procEnum->Init(); */

        SCXCoreLib::SCXCalendarTime pre_fork = SCXCoreLib::SCXCalendarTime::CurrentLocal();

        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process exits immediately and leaves a zombie
            exit(4711);
        }

        // cout << endl << "**** testZombie - " << pid << endl;

        SCXCoreLib::SCXThread::Sleep(500);
        m_procEnum->SampleData(); // Manual update of data
        m_procEnum->Update(true);
        SCXCoreLib::SCXCalendarTime pre_wait = SCXCoreLib::SCXCalendarTime::CurrentLocal();

        // Tests that the process is there
        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT(0 != inst);
        int ppid;
        CPPUNIT_ASSERT(inst->GetParentProcessID(ppid));
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(testapp_pid), ppid);

        unsigned short execution_state;
        CPPUNIT_ASSERT(inst->GetExecutionState(execution_state));
        CPPUNIT_ASSERT_EQUAL(execution_state, Terminated);

        // The termination date should be set and between pre_fork and pre_wait
        SCXCoreLib::SCXCalendarTime termination_date;
        CPPUNIT_ASSERT(inst->GetTerminationDate(termination_date));

        CPPUNIT_ASSERT(pre_fork <= termination_date);
        CPPUNIT_ASSERT(termination_date <= pre_wait);

        // Now reap it
        waitpid(pid, 0, 0);
        // SCXCoreLib::SCXCalendarTime post_wait = SCXCoreLib::SCXCalendarTime::CurrentLocal();

        m_procEnum->SampleData(); // Manual update of data
        m_procEnum->Update(true);

        inst = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT(0 == inst);
    }

    /** Creates a process that exits immediately. Tests if the zombie process name is what we expect.
    */
    void testZombieName()
    {
#if defined(aix)
        /* First see if this test has any chance of succeeding. */
        if (detectAIX61ProcBug()) {
            SCXUNIT_WARNING(wstring(L"Can't test for zombie processes on this system. ProcessPAL_Test::testZombieName test was disabled."));
            return;
        }
#endif

        const unsigned short Terminated = 7;
        pid_t testapp_pid = SCXCoreLib::SCXProcess::GetCurrentProcessID();
        m_procEnum = new ProcessEnumeration();
        /* We don't start an update thread here: m_procEnum->Init(); */

        // SCXCoreLib::SCXCalendarTime pre_fork = SCXCoreLib::SCXCalendarTime::CurrentLocal();

        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        {
            // Child process exits immediately and leaves a zombie
            exit(4711);
        }

        // cout << endl << "**** testZombie - " << pid << endl;

        SCXCoreLib::SCXThread::Sleep(500);
        m_procEnum->SampleData(); // Manual update of data
        m_procEnum->Update(true);
        // SCXCoreLib::SCXCalendarTime pre_wait = SCXCoreLib::SCXCalendarTime::CurrentLocal();

        // Get the process name of us ('testrunner' or whatever)
        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(testapp_pid);
        CPPUNIT_ASSERT(0 != inst);

        std::string testapp_name;
        CPPUNIT_ASSERT(inst->GetName(testapp_name));

        // Get the instance we care about, verify it exists
        inst = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT(0 != inst);

        // Verify that it's really terminated ...
        unsigned short execution_state;
        CPPUNIT_ASSERT(inst->GetExecutionState(execution_state));
        CPPUNIT_ASSERT_EQUAL(execution_state, Terminated);

        // Verify that the name is as expected
        // On Linux:    [process-name] <defunct>
        // On Mac OS:   (process-name)
        // All others:  <defunct>
        //
        // (All others, at this time, are: Solaris x86 & Sparc, AIX, HP Itanium & PA-Risc

        std::string process_name;
        CPPUNIT_ASSERT(inst->GetName(process_name));
        cout << ": " << process_name;
#if defined(linux)
        std::string linux_name = "[" + testapp_name + "] <defunct>";
        CPPUNIT_ASSERT(process_name.compare(linux_name) == 0);
#elif defined(macos)
        std::string macos_name = "(" + testapp_name + ")";
        CPPUNIT_ASSERT(process_name.compare(macos_name) == 0);
#else
        CPPUNIT_ASSERT(process_name.compare("<defunct>") == 0);
#endif

        // Now reap it
        waitpid(pid, 0, 0);
    }

    /** Tests that a newly created process gets its parents ppid value. */
    void testParentPID()
    {
        m_procEnum = new ProcessEnumeration();
        /* No Init(), we do manual updates. */

        /* Create child process and let it run until killed */
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid) { // Child process
            for (;;)
            {
                SCXCoreLib::SCXThread::Sleep(5000);
            }
            exit(0);
        }

        m_procEnum->SampleData();
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(pid);
        if (0 != inst)  {
            int ppid;
            CPPUNIT_ASSERT(inst->GetParentProcessID(ppid));
            CPPUNIT_ASSERT_EQUAL(static_cast<int>(SCXCoreLib::SCXProcess::GetCurrentProcessID()), ppid);
            kill(pid, SIGKILL);
            return;
        }

        kill(pid, SIGKILL);
        CPPUNIT_FAIL("Could not find child process in enumeration.");
    }

    /** Tests memory allocation parameters. Allocates a large amount of memory and
        test that "used memory" goes up.
    */
    void testUsedMemory()
    {
        // XXX Note: This could be improved. There are some subtle issues on when we
        // take the samples and what has happened at that instance in time.

        ostringstream errStream;
        SynchronizeProcesses parentSync, childSync; // parentSync written by parent, childSync written by child

        scxulong umem1, umem2, pumem1, pumem2, prs1, prs2;
        m_procEnum = new ProcessEnumeration();
        // No not call Init() here since we run without update thread

        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid)
        { // Child process
            parentSync.SignifyReader();
            childSync.SignifyWriter();

            // First do simple handshake for no other reason than for sanity
            parentSync.ReadMarker(1);
            childSync.WriteMarker(1);

            parentSync.ReadMarker(2);           // Parent is ready to have memory allocated

            char* lots_of_mem = new char[4711*1024*10];

            childSync.WriteMarker(2);           // Child has allocated memory

            // Do something on string to prevent compiler from removing this during optimization.
            strcpy(lots_of_mem, "Something");
            //cout << lots_of_mem << endl;

            parentSync.ReadMarker(127);         // Verify we're ready to go bye bye
            exit(0);
        }

        parentSync.SignifyWriter();
        childSync.SignifyReader();

        // Simple handshake simply for sanity test
        parentSync.WriteMarker(1);
        childSync.ReadMarker(1);

        m_procEnum->SampleData();
        m_procEnum->Update(true);
        SCXCoreLib::SCXHandle<ProcessInstance> inst1 = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT(0 != inst1);

        CPPUNIT_ASSERT(inst1->GetUsedMemory(umem1));
        CPPUNIT_ASSERT(inst1->GetPercentUsedMemory(pumem1));
        CPPUNIT_ASSERT(inst1->GetVirtualData(prs1));

        parentSync.WriteMarker(2);              // Tell child to allocate memory
        childSync.ReadMarker(2);                // ... and verify it is done

        m_procEnum->SampleData();
        m_procEnum->Update(true);
        SCXCoreLib::SCXHandle<ProcessInstance> inst2 = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT(0 != inst2);

        CPPUNIT_ASSERT(inst2->GetUsedMemory(umem2));
        CPPUNIT_ASSERT(inst2->GetPercentUsedMemory(pumem2));
        CPPUNIT_ASSERT(inst2->GetVirtualData(prs2));

        errStream << endl;
        errStream << "  Used memory before alloc: " << umem1 << endl;
        errStream << "  And after: " << umem2 << endl;
        // We can't control paging, so this can't be reliably tested
        //CPPUNIT_ASSERT_MESSAGE(errStream.str().c_str(), umem1 <= umem2);

        errStream << "  Percent used memory before alloc: " << pumem1 << endl;
        errStream << "  And after: " << pumem2 << endl;

        // We can't control paging, so this can't be reliably tested
        //CPPUNIT_ASSERT_MESSAGE(errStream.str().c_str(), pumem1 <= pumem2);

        errStream << "  VirtualData before alloc: " << prs1 << endl;
        errStream << "  And after:                " << prs2 << endl;
        CPPUNIT_ASSERT_MESSAGE(errStream.str().c_str(), prs1 <= prs2);

        parentSync.WriteMarker(127);            // Child can die now
        // kill(pid, SIGTERM);
        waitpid(pid, 0, 0);
    }

    void testThreadSafeSizeCallableWithLockHeld()
    {
        m_procEnum = new ProcessEnumeration();
        SCXCoreLib::SCXThreadLock lock(m_procEnum->GetLockHandle());
        CPPUNIT_ASSERT_NO_THROW(m_procEnum->Size());
    }

    /* This method is dependant on a special TestProcessEnumeration instance which delays updates
       forcing a context switch during an update. That delay must be "large enough". */
    void testBug2277()
    {
        // WI6482: This test takes a VERY long time to complete on HPUX for some strange timing reason.
        // Disable for now since the test is not platform specific.

#if defined(hpux)

        SCXUNIT_WARNING(L"The testBug2277 test is not executed on HPUX - see WI 6482");
        return;

#else

        for (int nr = 0; nr < 10; nr++) // Reproduce Bug 2968 more often
        {
            m_procEnum = new TestProcessEnumeration(); // Using a derived process enumerator with AddInstance delays.
            // No not call Init() here since we run without update thread
            m_procEnum->SampleData();
            m_procEnum->Update(true);

            size_t expected = m_procEnum->Size();
            size_t result = 0;

            SCXThread updater(updateProcessesThreadBody, new ProcessPAL_ThreadParam(m_procEnum));

            updater.RequestTerminate();
            const int cMIN_TRIES = 10000;
            int tries = 0;
            while ((tries++ < cMIN_TRIES) || updater.IsAlive())
            {
                size_t size = m_procEnum->Size();

                if ((1 == tries) || (size < result))
                {
                    result = size;
                }
                else if (Abs(size-result) > 10)
                {
                    break;
                }
            }
            updater.Wait();
            CPPUNIT_ASSERT_DOUBLES_EQUAL(static_cast<double>(expected), static_cast<double>(result), 5);
        }

#endif /* hpux */

    }


#if defined(linux)      
    /* Start a new process with the specified name and try to find that name in the process list. */
    void testFindingProcByName(const string& procname)
    {
        //cout << "Testing for process name: '" << procname << "'";

        m_procEnum = new ProcessEnumeration();
        /* No Init(), we do manual updates. */

        string cmd = "cp -f /bin/sleep ./\"" + procname + "\"";
        string msg = "Error executing command: " + cmd;
        CPPUNIT_ASSERT_MESSAGE(msg, system(cmd.c_str()) == 0);
        cmd = "./\"" + procname + "\" 5 &";
        msg = "Error executing command: " + cmd;
        CPPUNIT_ASSERT_MESSAGE(msg, system(cmd.c_str()) == 0);

        // Sleep for a short time to make sure the process has time to get started
        SCXCoreLib::SCXThread::Sleep(500);

        /* Do process enumeration */
        m_procEnum->SampleData();
        m_procEnum->Update(true);

        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > namelist =
            m_procEnum->Find(StrFromUTF8(procname));
        msg = "Failed to find process: '" + procname + "'";
        CPPUNIT_ASSERT_MESSAGE(msg.c_str(), namelist.size() == 1);

        //cout << " - pid: " << namelist[0]->getpid() << endl;

        cmd = "rm -f ./\"" + procname + "\"";
        msg = "Error executing command: " + cmd;
        CPPUNIT_ASSERT_MESSAGE(msg, system(cmd.c_str()) == 0);
    }
#endif

    /* This is test to see if we can handle processes with a space in them. */
    void testProcNameWithSpace()
    {
#if defined(linux)
        //cout << endl;

        testFindingProcByName("my sleep ");
        testFindingProcByName("my ( sleep ");
        testFindingProcByName("my \' sleep ");
        testFindingProcByName("my \\ sleep ");
        // This does not work right now. We need to solve WI 12794 first.
        // testFindingProcByName("my ) sleep ");
#endif
    }

#if defined(sun) && ((PF_MAJOR > 5) || (PF_MAJOR == 5 && PF_MINOR >= 10))
    /* Test Global zone, global process */
    void testSolaris10_GlobalZone_ProcessInGlobalZone()
    {
        TestProcessInstance pi(true, 0);
        CPPUNIT_ASSERT( pi.DoUpdateInstance() );
    }

    /* Test Global zone, non-global process */
    void testSolaris10_GlobalZone_ProcessInNonGlobalZone()
    {
        TestProcessInstance pi(true, 1);
        CPPUNIT_ASSERT( !pi.DoUpdateInstance() );
    }

    /* Test Non-Global zone, non-global process */
    void testSolaris10_NotGlobalZone_ProcessInNonGlobalZone()
    {
        TestProcessInstance pi(false, 1);
        CPPUNIT_ASSERT( pi.DoUpdateInstance() );
    }
#endif // defined(sun) && ((PF_MAJOR > 5) || (PF_MAJOR == 5 && PF_MINOR >= 10))

    /* This is basically a test for the new Find(cmdname) call. */
    void testNamedFind()
    {
        string cmdname;
        scxulong pid;
        scxulong curpid = SCXCoreLib::SCXProcess::GetCurrentProcessID();
        m_procEnum = new ProcessEnumeration();
        /* No Init(), we do manual updates. */

        /* Do process enumeration */
        m_procEnum->SampleData();
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(curpid);
        CPPUNIT_ASSERT(inst != 0);

        CPPUNIT_ASSERT(inst->GetPID(pid));
        CPPUNIT_ASSERT(inst->GetName(cmdname));

        CPPUNIT_ASSERT(pid == curpid);

        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > namelist =
            m_procEnum->Find(StrFromUTF8(cmdname));
        CPPUNIT_ASSERT(namelist.size() > 0);

        int found = 0;
        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> >::iterator pos;
        for(pos = namelist.begin(); pos != namelist.end(); ++pos) {
            CPPUNIT_ASSERT((*pos)->GetPID(pid));
            if (pid == curpid) { found++; }
        }
        CPPUNIT_ASSERT(found >= 1); // There could be multiple testrunners going...
    }

    /* Here we test the KillByName call by sending the USR1 signal to ourselves.
       This code assumes that the process is called "testrunner" and will fail otherwise.
    */
    void testKillByName()
    {
        /* Install new signal handler */
        oldHandler = signal(SIGUSR1, &usr1Handler);
        CPPUNIT_ASSERT(oldHandler != SIG_ERR);

        /* Assert that we not yet have received the signal */
        CPPUNIT_ASSERT(sigUSR1Received == false);

        /* Signal ourselves */
        CPPUNIT_ASSERT(ProcessEnumeration::SendSignalByName(L"testrunner", SIGUSR1));

        /* Apparently the signals aren't synchronous so allow some time for delivery */
        SCXCoreLib::SCXThread::Sleep(300);

        /* Restore signal handler */
        CPPUNIT_ASSERT(signal(SIGUSR1, oldHandler) != SIG_ERR);

        /* Assert that we have received the signal */
        CPPUNIT_ASSERT(sigUSR1Received == true);
    }


    void testGetParameters()
    {
        const char* estr[] = { "sh", "-c", "sleep\t15;cat\t/dev/null", "AAAAAAAAAAAAAAAAAAAA",
                               "BBBBBBBBBBBBBBBBBBBB", "CCCCCCCCCCCCCCCCCCCC",
                               "DDDDDDDDDDDDDDDDDDDD", 0 };

        /* Note: The TAB character may look funny, but is neccessary as not to make
           this test too messy. This is because on some platforms (HP!) we get the
           parameter list as a string from the system, and all that separates the
           command line arguments is a blank character. By using a TAB, the command line
           interpreter of the shell can see that the "sleep" and the "5" are separated,
           but we still get them as one unit from GetParameters.

           Note2: The cat /dev/null may also look funny. It's there as a portable NOP.
           It turns out that at least bash is smart enough to figure out that, if cat
           wasn't there, sleep would be the last command to execute an thus it can be
           exec'ed directly, and not forked as a separate sub-process to sh.

           Note3: The AAAA... etc strings are there just to make the command string
           longer. They are never parsed. The total command length should be more that 64
           chars to properly test a HPUX feature.
        */

        m_procEnum = new ProcessEnumeration();
        /* No Init(), we do manual updates. */

        /* Forrrk off a command whose parameters we can control and measure */
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid) {
#if defined(sun)
            // The ordinary shell on Solaris does strange things to the parameters
            execv("/usr/bin/bash", const_cast<char* const*>(estr)); // Ugly cast...
#else
            execv("/bin/sh", const_cast<char* const*>(estr)); // Ugly cast...
#endif
            exit(0);            // Ok, won't be reached...
        }

        SCXCoreLib::SCXThread::Sleep(500);
        m_procEnum->SampleData();
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT( 0 != inst);

        /* Extract the parameters on those platforms that supports it */
        std::vector<std::string> params;
        if (inst->GetParameters(params)) {

            CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of parameters", sizeof(estr)/sizeof(char*)-1, params.size());

            std::vector<std::string>::iterator pos;

            int i = 0;
            for(pos = params.begin(); pos != params.end(); ++pos, ++i) {
                //cout << *pos << ", " << estr[i] << endl;
                CPPUNIT_ASSERT_EQUAL_MESSAGE("Command line parameters don't match",
                                             string(estr[i]), string(*pos));
            }

        }
        kill(pid, SIGKILL);     // Dispose of test subject
     }

    /* Start a new process with the specified name and try to find that name in the process list. */
    void testSymbolicLinksReturnSymbolicName()
    {
        string procname = "sleep-softlink";

        //ostringstream errStream;
        //errStream << endl << "Testing for symbolic link name: '" << procname << "'";

        m_procEnum = new ProcessEnumeration();
        /* No Init(), we do manual updates. */

        string cmd = "ln -s /bin/sleep ./" + procname;
        CPPUNIT_ASSERT_EQUAL(0, system(cmd.c_str()) & 0xFF);
        cmd = "./" + procname + " 5 &";
        CPPUNIT_ASSERT_EQUAL(0, system(cmd.c_str()) & 0xFF);
        
        // Sleep for a short time to make sure the process has time to get started
        SCXCoreLib::SCXThread::Sleep(500);

        /* Do process enumeration */
        m_procEnum->SampleData();
        m_procEnum->Update(true);

        std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > namelist =
            m_procEnum->Find(StrFromUTF8(procname));
        string msg = "Failed to find process: '" + procname + "'";
        CPPUNIT_ASSERT_MESSAGE(msg.c_str(), namelist.size() == 1);

        //errStream << " - pid: " << namelist[0]->getpid() << endl;

        string name;
        CPPUNIT_ASSERT(namelist[0]->GetName(name));
        msg = "Unexpected process name found: '" + name;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(msg.c_str(), procname, name);

        cmd = "rm -f ./" + procname;
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }

    // Verify that ProcLister interface returns values close to what 'ps' returns
    void testProcLister()
    {
        // First get the count from ProcLister

        int countPL = 0;
        ProcessEnumeration::ProcLister pl;

        while ( pl.nextProc() )
        {
            countPL++;
        }

        // Now get the count from ps

        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        int returnCode = SCXCoreLib::SCXProcess::Run(L"sh -c 'ps -ef | wc -l'", input, output, error);
        CPPUNIT_ASSERT_EQUAL( 0, returnCode );
        CPPUNIT_ASSERT_EQUAL( "", error.str() );
        // Convert stdout to numeric form
        stringstream ss( output.str() );
        int countPS(0);
        ss >> countPS;
        bool conv_result = !ss.fail();
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Failure converting 'ps -ef | wc -l' to numeric form", true, conv_result );
        // Subtract 4 due to overhead from 'ps' command:
        //   The 'ps' command sequence creates three new processes (sh, ps, and wc)
        //   The 'ps' command includes one header line
        countPS -= 4;

        // Now we compare the process count from ProcLister and the 'ps' command
        //
        // Since ProcLister and the 'ps' command sequence runs at different
        // times, other system activity could have created/deleted processes.  For
        // this reason, we have a 'fudge factor' (number of processes that can be
        // "off" while still passing the unit test).
        //
        // This "fudge factor" can easily be exceeded by heavy process creation
        // and deletion between our two count steps.  Not much we can do about
        // this (other than have a fudge factor and a meaningful error message).

        const int fudgeFactor = 5;
        ostringstream msg;
        msg << "Value from ProcLister: " << countPL
            << ", Value from 'ps -ef | wc -l': " << countPS
            << ", Fudge factor: " << fudgeFactor
            << ".  We expect that ProcLister count (countPL) is between 'ps' count (countPS)  as follows: "
            << "\"countPS - fudgeFactor <= countPL <= countPS + fudgeFactor\". "
            << "If this test fails, run it again to verify no transient failure due to process creation/deletion.";
        SCXUNIT_ASSERT_BETWEEN_MESSAGE(msg.str().c_str(), countPL, countPS - fudgeFactor, countPS + fudgeFactor);
    }


    /*=============================================================================*/
    /* Utilities from here on                                                      */
    /*=============================================================================*/

#if defined(aix)

    /* AIX 6.1 has a problem with the /proc filesystem for processes that has become
       zombies. This function tests if that bug applies to the running system.
    */
    bool detectAIX61ProcBug()
    {
        char buf[30];
        struct stat bstat;
        int res = 0;
        pid_t pid = fork();

        if (0 == pid)
        {
            // Child process exits immediately and leaves a zombie
            exit(4711);
        }

        snprintf(buf, sizeof(buf), "/proc/%d/psinfo", (int)pid);

        res = stat(buf, &bstat);
        int eno = errno;

        /* If stat() insists that psinfo is there then wait a second and see if it
           comes to the same conclusion again then. This is rather ad-hoc but has proved to
           catch problems in real sitations.
        */
        if (res >= 0) {
            sleep(1);
            res = stat(buf, &bstat);
            eno = errno;
        }

        waitpid(pid, 0, 0);

        if (res >= 0) {
            return false;       // We're ok
        }

        if (eno == ENOENT) {
            return true;        // We have the bug
        }

        cout << "Unknown error = " << eno << endl;
        return true;
    }
#endif

    static void updateProcessesThreadBody(SCXThreadParamHandle& param)
    {
        const int cMIN_TRIES = 5;
        ProcessPAL_ThreadParam* pl = dynamic_cast<ProcessPAL_ThreadParam*>(param.GetData());
        int tries = 0;
        while ((tries++ < cMIN_TRIES) || ( ! pl->GetTerminateFlag()))
        {
                //SCXThread::Sleep(1);
            pl->GetProcEnum()->Update(true);
        }
    }

    /**
     * Tests if the command name as received from 'ps' can be considered equal
     * with the full name that is stored in the process instance.
     * We need to relax the equality criteria a bit for the following reasons:
     * On Sun and HP since there's always lots of <defunct> processes sloshing around.
     * At least solaris truncates the ps-output to 8 characters.
     * There is also a risk that a process changes it name due to an exec() call,
     * but that we don't account for that in this function.
     */
    bool compareCmdNames(string psoutput, string instanceName)
    {
        if (psoutput == "<defunct>") return true;

#if defined(aix)
        // sshd processes don't show up right at all ...
        if (psoutput == "sshd") return true;
#endif

#if defined(hpux)
        // On HP platform, shell scripts and shell commands (echo, etc) are
        // returned by 'ps', but our agent will just return 'bash' or 'sh'

        if ((instanceName == "bash") || (instanceName == "sh"))
            return true;
#endif

#if defined(aix) || defined(hpux)
        // On AIX & HP, we have differences like ps="-bash", but agent="bash"
        // Be a little more forgiving for these sorts of differences
        //
        // The second compare will match on things like "-bash" vs. "bash"
        // The find will find substrings (i.e. "ksh" vs. "master.ksh")

        return (instanceName.compare(0, psoutput.length(), psoutput) == 0
                || psoutput.compare(1, psoutput.length()-1, instanceName) == 0
                || instanceName.find(psoutput) != string::npos
                || psoutput.find(instanceName) != string::npos);
#else
        return (instanceName.compare(0, psoutput.length(), psoutput) == 0);
#endif
//         Debug non-equal names
//         if (instanceName.compare(0, psoutput.length(), psoutput) == 0)
//             return true;
//         else {
//             cout << "compareCmdNames::" << psoutput << "!=" << instanceName << endl;
//             return false;
//         }
    }

    SCXCoreLib::SCXHandle<ProcessInstance> FindProcessInstanceFromPID(scxulong pid)
    {
        CPPUNIT_ASSERT(0 != m_procEnum);

//         for (ProcessEnumeration::EntityIterator iter = m_procEnum->Begin(); iter != m_procEnum->End(); ++iter)
//         {
//             scxulong testpid;
//             SCXCoreLib::SCXHandle<ProcessInstance> inst = *iter;
//             CPPUNIT_ASSERT(inst->GetPID(testpid));

//             if (pid == testpid)
//             {
//                 return inst;
//             }
//         }

#if !defined(aix)
        return m_procEnum->Find(pid);

#else /* aix only */

        SCXCoreLib::SCXHandle<ProcessInstance> inst = m_procEnum->Find(pid);
        if (inst != NULL) { return inst; }

        /* Workaround for AIX.
           Problem: Recently created processes won't show up immediately in the process
           directory if the load on the machine is high. This is not much of a problem
           for daily use, unless you, like the unit tests, try to find a specific
           process and it isn't there.

           By doing up to eight calls to Update() we hope to flush the /proc filesystem.
        */

        for(int i = 0; i < 8; i++)
        {
            sleep(0.25);

            /* OK, try again. */
            m_procEnum->SampleData();
            m_procEnum->Update(true);

            inst = m_procEnum->Find(pid);
            if (inst != NULL) { return inst; }
        }

        return SCXCoreLib::SCXHandle<ProcessInstance>(0);
#endif
    }

    bool VerifyProcessInstance(SCXCoreLib::SCXHandle<ProcessInstance> inst, bool checkfirst = false)
    {
        string icmdstr, jcmdstr;
        unsigned short istate; int ippid;
        scxulong ipid, iuid; unsigned int inice;
        int juid, jppid, jpri, jnice; char jstate;

        CPPUNIT_ASSERT(0 != inst);

        CPPUNIT_ASSERT(inst->GetPID(ipid));
        GetPSELData(static_cast<int>(ipid), jstate, juid, jppid, jpri, jnice, jcmdstr);

        CPPUNIT_ASSERT(inst->GetName(icmdstr));

        if (checkfirst)
        {
            if (!compareCmdNames(jcmdstr, icmdstr))
            {
                return false;
            }
        }

        CPPUNIT_ASSERT(inst->GetExecutionState(istate));
        CPPUNIT_ASSERT(inst->GetParentProcessID(ippid));
        CPPUNIT_ASSERT(inst->GetRealUserID(iuid));
        CPPUNIT_ASSERT(inst->GetProcessNiceValue(inice));

        // Compare with data from ps -el
        ostringstream msg;
        msg << "jcmdstr: " << jcmdstr << ", icmdstr: " << icmdstr << endl;
        CPPUNIT_ASSERT_MESSAGE(msg.str().c_str(), compareCmdNames(jcmdstr, icmdstr));

        CPPUNIT_ASSERT(istate < 12);
        CPPUNIT_ASSERT_EQUAL(jppid, ippid);
        // Observation: iuid is unexpectedly zero sometimes. Maybe because of setuid?
        // CPPUNIT_ASSERT_EQUAL(juid, (int)iuid);

#if !defined(linux)
        CPPUNIT_ASSERT_EQUAL(jnice, static_cast<int>(inice));
#else
        // Linux's GetProcessNiceValue offsets by 20 to guarentee results >= 0
        // (CIM model stipulates that this is an unsigned value)
        CPPUNIT_ASSERT_EQUAL(jnice, static_cast<int>(inice) - 20);
#endif

        return true;
    }

    void SweepProcessInstance(SCXCoreLib::SCXHandle<ProcessInstance> inst)
    {
        // Just access all methods so we can see that nothing fails fatal

        SCXCalendarTime ASCXCalendarTime;
        int Aint;
        scxulong Ascxulong;
        string Astring;
        unsigned int Aunsignedint;
        unsigned short Aunsignedshort;
        vector<string> Avector;
        wstring Ascxstring;

        inst->GetPID(Ascxulong);
        inst->GetName(Astring);
        inst->GetNormalizedWin32Priority(Aunsignedint);
        inst->GetNativePriority(Aint);
        inst->GetExecutionState(Aunsignedshort);
        inst->GetCreationDate(ASCXCalendarTime);
        inst->GetTerminationDate(ASCXCalendarTime);
        inst->GetParentProcessID(Aint);
        inst->GetRealUserID(Ascxulong);
        inst->GetProcessGroupID(Ascxulong);
        inst->GetProcessNiceValue(Aunsignedint);

        inst->GetOtherExecutionDescription(Ascxstring);
        inst->GetKernelModeTime(Ascxulong);
        inst->GetUserModeTime(Ascxulong);
        inst->GetWorkingSetSize(Ascxulong);
        inst->GetProcessSessionID(Ascxulong);
        inst->GetProcessTTY(Astring);
        inst->GetModulePath(Astring);
        inst->GetParameters(Avector);
        inst->GetProcessWaitingForEvent(Astring);

        inst->GetCPUTime(Aunsignedint);
        inst->GetBlockWritesPerSecond(Ascxulong);
        inst->GetBlockReadsPerSecond(Ascxulong);
        inst->GetBlockTransfersPerSecond(Ascxulong);
        inst->GetPercentUserTime(Ascxulong);
        inst->GetPercentPrivilegedTime(Ascxulong);
        inst->GetUsedMemory(Ascxulong);
        inst->GetPercentUsedMemory(Ascxulong);
        inst->GetPagesReadPerSec(Ascxulong);

        inst->GetRealText(Ascxulong);
        inst->GetRealData(Ascxulong);
        inst->GetRealStack(Ascxulong);
        inst->GetVirtualText(Ascxulong);
        inst->GetVirtualData(Ascxulong);
        inst->GetVirtualStack(Ascxulong);
        inst->GetVirtualMemoryMappedFileSize(Ascxulong);
        inst->GetVirtualSharedMemory(Ascxulong);
        inst->GetCpuTimeDeadChildren(Ascxulong);
        inst->GetSystemTimeDeadChildren(Ascxulong);
    }


#if defined(aix) || defined(hpux) || defined(sun)
    /**
       Strips path information from the filename passed in by parameter.
       If no path information is found, then no modifications are made.

       \param[out]  name Return parameter for the command name
       \returns     void
     */
    static void StripPathInfo(std::string& name)
    {
        size_t pos = name.rfind('/');
        if (pos != string::npos)
        {
            name.erase(0, pos + 1);
        }
    }
#endif
    /**
     * Retrieves mapping from process pid to command name by running ps -e in a subshell.
     */
    void GetPSEData(std::map<scxulong, std::string>& psValues)
    {
#if defined(linux)
        char psecmd[] = "/bin/ps -eo \"pid,comm\"";
#elif defined(sun)
        // On sun, we sometimes get a "Broken Pipe"
        // fname truncates to 8 chars and comm includes the path. Standards!
        char psecmd[] = "trap '' PIPE;/bin/ps -eo \"pid,s,args\"";
#elif defined(hpux)
        // Note: The 'ps' in the standard unix environment is useless since it
        // returns different strings in the COMMAND field than those that we look for.
        // Unfortunately we can't select what fields to display in the non-standard
        // enviroment. Useless: char psecmd[] = "UNIX_STD=2003 /bin/ps -eo \"pid,comm\"";
        char psecmd[] = "UNIX95= /bin/ps -eo 'pid,args'";
#elif defined(aix)
        char psecmd[] = "/bin/ps -Aeo \"pid,comm\"";
#else
        #error "More work for U!"
#endif
        char buf[256];
        int eno = 0;
        int mypid;
        std::string cmdstr;

        errno = 0;
        FILE* psEFile = popen(psecmd, "r");
        if (psEFile == 0) {
            CPPUNIT_ASSERT_MESSAGE("Can't do popen", psEFile);
            CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(errno), errno == 0);
            return;
        }

        //cout << "Looking for " << pid << endl;

        // Get rid of first line, then iterate over rest of lines until no more
        if (fgets(buf, sizeof(buf), psEFile)) {
            // cout << "\nPID\tCMD" << endl;
            while(fgets(buf, sizeof(buf), psEFile)) {
                istringstream scan(buf);

#if defined(hpux)
                // With ps -e, we get output like:
                //      19003 ?         0:00 sshd
                //
                // With UNIX95= /bin/ps -eo 'pid,args', we get output like:
                //      23939 ps -eo pid,args
                //
                
                scan >> mypid >> cmdstr;

                // If we have a shell, try and find parameter 1 (the actual process being run)
                // This should help eliminate some false positives ...
                //
                // This is error prone (particularly for things like "sh -c 'cd foo; ./bar'",
                // but it does help a little bit, which is all we need for these tests to pass

                //std::cout << std::endl << "    For PID " << mypid << ", we got: " << cmdstr;

                if (cmdstr == "/bin/bash" || cmdstr == "/bin/sh" || cmdstr == "/sbin/sh")
                {
                    scan >> cmdstr;

                    // Ugh ... get rid of this common parameter to the shell
                    if (cmdstr == "-c")
                    {
                        scan >> cmdstr;
                    }

                    //std::cout << " (modified to: " << cmdstr << ")";
                }

                StripPathInfo(cmdstr);
#elif defined(sun)
                // On some Solaris machines (5.8) zombie processes have no name. On most machines however they have the name "<defunct>".
                string state;
                scan >> mypid >> state;
                if ("Z" == state)
                {
                    cmdstr = "<defunct>";
                }
                else
                {
                    scan >> cmdstr;
                }

                StripPathInfo(cmdstr);
#else
                scan >> mypid >> cmdstr;
#endif

                // cout << mypid << "\t" << cmdstr << endl;
                psValues[mypid] = cmdstr;
            }
        }

        /* If this wasn't end of file then save errno. */
        if (!feof(psEFile)) { eno = errno; }
        pclose(psEFile);
        errno = eno;            // Just for documentation purposes!
        CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(eno), errno == 0);
    }

    /**
     * Runs a 'ps' command in a subshell and captures the output for the line with PID
     * equal to pid.
     */
    void GetPSELData(int pid, char& state, int& uid, int& ppid, int& pri,
                     int& nice, std::string& cmdstr)
    {
#if defined(linux)
        char psecmd[] = "/bin/ps -eo \"state,uid,pid,ppid,pri,nice,comm\"";
#elif defined(sun)
        // On sun, we sometimes get a "Broken Pipe"
        // fname truncates to 8 chars and comm includes the path. Standards!
        char psecmd[] = "trap '' PIPE;/bin/ps -eo \"s,uid,pid,ppid,pri,nice,fname\"";
#elif defined(hpux)
        // The UNIX95 variable enables the Standard Unix behaviour for ps on both v2 and v3.
        // UNIX_STD=2003 on the other hand, works only in v3.
        char psecmd[] = "UNIX95=yes /bin/ps -eo \"state,uid,pid,ppid,pri,nice,comm\"";
#elif defined(aix)
        char psecmd[] = "/bin/ps -Aeo \"state,uid,pid,ppid,pri,nice,comm\"";
#else
        #error "More work for U!"
#endif
        char buf[256];
        int eno = 0;
        int mypid;
        bool done = false;

        errno = 0;
        FILE* psEFile = popen(psecmd, "r");
        if (psEFile == 0) {
            CPPUNIT_ASSERT_MESSAGE("Can't do popen", psEFile);
            CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(errno), errno == 0);
            return;
        }

        char nicest[10];        // Temp holding space for nice value.

        //cout << "Looking for " << pid << endl;

        // Get rid of first line, then iterate over rest of lines until no more
        if (fgets(buf, sizeof(buf), psEFile)) {
            // cout << "\nPID\tST\tUID\tPPID\tPRI\tNI\tCMD" << endl;
            while(fgets(buf, sizeof(buf), psEFile)) {
                istringstream scan(buf);

                scan >> state >> uid >> mypid >> ppid >> pri >> nicest >> cmdstr;

                nice = atoi(nicest);
                // cout << mypid << "\t" << state << "\t" << uid << "\t" << ppid << "\t" << pri << "\t" << nice << "\t" << cmdstr << endl;

                if (pid == mypid) { done = true; break; }
            }
        }

        /* If this wasn't end of file then save errno. */
        if (!feof(psEFile)) { eno = errno; }
        pclose(psEFile);
        errno = eno;            // Just for documentation purposes!
        CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(eno), errno == 0);
        CPPUNIT_ASSERT_MESSAGE("Didn't find pid", done);
    }

    /*
      Verifies that reported process priorities are close to what "ps" command reports. Note that OS may modify the
      priority value so for that reason we allow for some slack.
    */
    void testProcessPriorities()
    {
        // First we get the collection of processes and their prioriries from "ps" command line.
        //
#if defined(linux)
        char psecmd[] = "/bin/ps -el";
#elif defined(sun)
        // On sun, we sometimes get a "Broken Pipe"
        // fname truncates to 8 chars and comm includes the path. Standards!
        char psecmd[] = "trap '' PIPE;/bin/ps -eo \"pid,pri\"";
#elif defined(hpux)
        char psecmd[] = "/bin/ps -el";
#elif defined(aix)
        char psecmd[] = "/bin/ps -Aeo \"pid,pri\"";
#else
        #error "Unsupported platform"
#endif
        errno = 0;
        FILE* psEFile = popen(psecmd, "r");
        if (psEFile == 0)
        {
            CPPUNIT_ASSERT(psEFile);
            CPPUNIT_ASSERT_MESSAGE(SCXCoreLib::strerror(errno), errno == 0);
            return;
        }

        std::stringstream errMsg;
        std::map<scxulong,int> prioritiesFromPS;
        char buf[256];
        // Get rid of first line, then iterate over rest of lines until no more.
        CPPUNIT_ASSERT(fgets(buf, sizeof(buf), psEFile));
        CPPUNIT_ASSERT(feof(psEFile) == 0);
        CPPUNIT_ASSERT(ferror(psEFile) == 0);
        CPPUNIT_ASSERT(strlen(buf) < 255);
        errMsg << "Processes reported by 'PS'" << std::endl << "PID\tPRI" << std::endl;
        while(fgets(buf, sizeof(buf), psEFile))
        {
            CPPUNIT_ASSERT(feof(psEFile) == 0);
            CPPUNIT_ASSERT(ferror(psEFile) == 0);
            CPPUNIT_ASSERT(strlen(buf) < 255);

            scxulong pid;
            int pri;
#if defined(linux) || defined(hpux)
            string skip;
            istringstream scan(buf);
            // On Linux and HPUX the output from 'ps' is in the format:
            // F S   UID   PID  PPID  C PRI  NI ADDR SZ WCHAN  TTY          TIME CMD            
            // 4 S     0     1     0  0  80   0 -   252 -      ?        00:00:49 init           
            scan >> skip >> skip >> skip >> pid >> skip >> skip >> pri;
            CPPUNIT_ASSERT(scan.good());
#elif defined(sun)
            istringstream scan(buf);
            // On Solaris and AIX the output from 'ps' is in the format:
            //  PID PRI
            //    0  96
            scan >> pid;
            CPPUNIT_ASSERT(scan.good());
            scan >>  pri;
            CPPUNIT_ASSERT(scan.eof() || scan.good());
#elif defined(aix)
            istringstream scan(buf);
            string aixPriority;
            // On Solaris and AIX the output from 'ps' is in the format:
            //  PID PRI
            //    0  96
            //
            // However, on AIX, PRI may be "-". If we read into an integer, the stream is considered "bad".
            scan >> pid;
            CPPUNIT_ASSERT(scan.good());
            scan >>  aixPriority;
            CPPUNIT_ASSERT(scan.eof() || scan.good());
            pri = atoi(aixPriority.c_str());
#else
#error Uncrecognized platform
#endif
            errMsg << pid << '\t' << pri << std::endl;

            std::pair<std::map<scxulong,int>::iterator, bool> ret;
            ret = prioritiesFromPS.insert(std::pair<scxulong, int>(pid, pri));
            ostringstream msg;
            msg << "process id repeats, pid = " << pid;
            CPPUNIT_ASSERT_MESSAGE(msg.str(), ret.second);
        }
        
        errMsg << "Processes reported by PAL" << std::endl << "PID\tPRI" << std::endl;
        // Now we have the collection of processes and priorities from "ps" command. Loop through processes reported by
        // the provider and verify the values are ok.
        m_procEnum = new ProcessEnumeration();
        m_procEnum->Init();
        m_procEnum->Update(true);
        vector<scxulong> palPid;
        vector<int> palPri;
        for (ProcessEnumeration::EntityIterator iter = m_procEnum->Begin(); iter != m_procEnum->End(); ++iter)
        {
            SCXCoreLib::SCXHandle<ProcessInstance> inst = *iter;
            scxulong pid;
            CPPUNIT_ASSERT(inst->GetPID(pid));
            int pri;
            CPPUNIT_ASSERT(inst->GetNativePriority(pri));
            palPid.push_back(pid);
            palPri.push_back(pri);
            errMsg << pid << '\t' << pri << std::endl;
        }
        size_t noPSProcessCount = 0;
        size_t priorityMismatchCount = 0;
        size_t i;
        for( i = 0; i < palPid.size(); i++)
        {
            scxulong pid = palPid[i];
            int pri = palPri[i];
            if(prioritiesFromPS.count(pid) == 0)
            {
                // This process did not exist when PS command was called. We allow only few occurances.
                noPSProcessCount++;
                CPPUNIT_ASSERT_MESSAGE("Too many processes not reported by PS command.\n" + errMsg.str(),
                    noPSProcessCount <= 5);
                continue;
            }
            int pspri = prioritiesFromPS[pid];

            unsigned int priDelta = abs(pri - pspri);
            // OS can modify the process priority. Only if priority is off by more than 2 we count it as a mismatch.
            if (priDelta > 2)
            {
                priorityMismatchCount++;
                // We allow for up to 9 processes to be off by more than 2.
                CPPUNIT_ASSERT_MESSAGE(
                    "Too many processes priorities differ from priorities reported by PS command.\n" + errMsg.str(),
                    priorityMismatchCount <= 9);
            }
        }
    }

    /*
      This unit test is to ensure that for a large command line, greater than 1024 but less than 4096
      that the GetParameters function performs correctly.
      The 4096 byte limit is imposed by SLES, RH, AIX. The limit is not on the size of the parameters 
      used to execute the process but rather by the retrieval of the parameters from the underlying OS.
    */
    void testGetParametersGreaterThan1024()
    {
        // actual command line size is 2587 bytes
        const char* estr[] = { 
            "sh", 
            "-c", 
            "sleep\t15;cat\t/dev/null",
            "/IBM/WebSphere/AppServer/java/bin/java",
            "-Declipse.security",
            "-Dwas.status.socket=38537",
            "-Dosgi.install.area=/IBM/WebSphere/AppServer",
            "-Dosgi.configuration.area=/IBM/WebSphere/AppServer/profiles/AppSrv01/configuration",
            "-Djava.awt.headless=true",
            "-Dosgi.framework.extensions=com.ibm.cds,com.ibm.ws.eclipse.adaptors",
            "-Xshareclasses:name=webspherev70_%g,groupAccess,nonFatal",
            "-Xscmx50M",
            "-Xbootclasspath/p:/IBM/WebSphere/AppServer/java/jre/lib/ext/ibmorb.jar:/IBM/WebSphere/AppServer/java/jre/lib/ext/ibmext.jar",
            "-classpath",
            "/IBM/WebSphere/AppServer/profiles/AppSrv01/properties:/IBM/WebSphere/AppServer/properties:/IBM/WebSphere/AppServer/lib/startup.jar:/IBM/WebSphere/AppServer/lib/bootstrap.jar:/IBM/WebSphere/AppServer/lib/jsf-nls.jar:/IBM/WebSphere/AppServer/lib/lmproxy.jar:/IBM/WebSphere/AppServer/lib/urlprotocols.jar:/IBM/WebSphere/AppServer/deploytool/itp/batchboot.jar:/IBM/WebSphere/AppServer/deploytool/itp/batch2.jar:/IBM/WebSphere/AppServer/java/lib/tools.jar",
            "-Dibm.websphere.internalClassAccessMode=allow",
            "-Xms50m",
            "-Xmx256m",
            "-Dws.ext.dirs=/IBM/WebSphere/AppServer/java/lib:/IBM/WebSphere/AppServer/profiles/AppSrv01/classes:/IBM/WebSphere/AppServer/classes:/IBM/WebSphere/AppServer/lib:/IBM/WebSphere/AppServer/installedChannels:/IBM/WebSphere/AppServer/lib/ext:/IBM/WebSphere/AppServer/web/help:/IBM/WebSphere/AppServer/deploytool/itp/plugins/com.ibm.etools.ejbdeploy/runtime:/IBM/WebSphere/AppServer/deploytool/itp/plugins/com.ibm.etools.ejbdeploy/runtime:/IBM/WebSphere/AppServer/deploytool/itp/plugins/com.ibm.etools.ejbdeploy/runtime",
            "-Dderby.system.home=/IBM/WebSphere/AppServer/derby",
            "-Dcom.ibm.itp.location=/IBM/WebSphere/AppServer/bin",
            "-Djava.util.logging.configureByServer=true",
            "-Duser.install.root=/IBM/WebSphere/AppServer/profiles/AppSrv01",
            "-Djavax.management.builder.initial=com.ibm.ws.management.PlatformMBeanServerBuilder",
            "-Dwas.install.root=/IBM/WebSphere/AppServer",
            "-Dpython.cachedir=/IBM/WebSphere/AppServer/profiles/AppSrv01/temp/cachedir",
            "-Djava.util.logging.manager=com.ibm.ws.bootstrap.WsLogManager",
            "-Dserver.root=/IBM/WebSphere/AppServer/profiles/AppSrv01",
            "-Dcom.ibm.security.jgss.debug=off",
            "-Dcom.ibm.security.krb5.Krb5Debug=off",
            "-Djava.security.auth.login.config=/IBM/WebSphere/AppServer/profiles/AppSrv01/properties/wsjaas.conf",
            "-Djava.security.policy=/IBM/WebSphere/AppServer/profiles/AppSrv01/properties/server.policy",
            "com.ibm.wsspi.bootstrap.WSPreLauncher",
            "-nosplash",
            "-application",
            "com.ibm.ws.bootstrap.WSLauncher",
            "com.ibm.ws.runtime.WsServer",
            "/IBM/WebSphere/AppServer/profiles/AppSrv01/config",
            "scxjet-aix71-01Node01Cell",
            "scxjet-aix71-01Node01",
            "server1", 0};

        m_procEnum = new ProcessEnumeration();
        /* No Init(), we do manual updates. */

        /* Forrrk off a command whose parameters we can control and measure */
        pid_t pid = fork();
        CPPUNIT_ASSERT(-1 != pid);
        if (0 == pid) {
#if defined(sun)
            // The ordinary shell on Solaris does strange things to the parameters
            execv("/usr/bin/bash", const_cast<char* const*>(estr)); // Ugly cast...
#else
            execv("/bin/sh", const_cast<char* const*>(estr)); // Ugly cast...
#endif
            exit(0);            // Ok, won't be reached...
        }

        SCXCoreLib::SCXThread::Sleep(500);
        m_procEnum->SampleData();
        m_procEnum->Update(true);

        SCXCoreLib::SCXHandle<ProcessInstance> inst = FindProcessInstanceFromPID(pid);
        CPPUNIT_ASSERT( 0 != inst);

        /* Extract the parameters on those platforms that supports it */
        std::vector<std::string> params;
        if (inst->GetParameters(params)) 
        {
            size_t size = params.size();
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Wrong number of parameters", sizeof(estr)/sizeof(char*)-1, size);

            CPPUNIT_ASSERT_EQUAL_MESSAGE("Command line parameters don't match",params[size-1],(string)"server1");
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Command line parameters don't match",params[size-2],(string)"scxjet-aix71-01Node01");
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Command line parameters don't match",params[size-3],(string)"scxjet-aix71-01Node01Cell");
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Command line parameters don't match",params[size-4],(string)"/IBM/WebSphere/AppServer/profiles/AppSrv01/config");
        }
        kill(pid, SIGKILL);     // Dispose of test subject
     }

};

CPPUNIT_TEST_SUITE_REGISTRATION( ProcessPAL_Test );
