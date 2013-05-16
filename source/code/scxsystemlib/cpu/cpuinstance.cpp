/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       PAL representation of a CPU

    \date        07-05-21 12:00:00

    \date        08-05-28 14:43:00
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <string>
#include <sstream>
#include <vector>

#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxmath.h>

#include <scxsystemlib/cpuinstance.h>

using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Constructor

        Parameters:  procNumber - Number of processor, used as base for instance name
                     isTotal - Whether the instance represents a Total value of a collection
    */
    CPUInstance::CPUInstance(unsigned int procNumber, bool isTotal) : EntityInstance(isTotal)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.cpu.cpuinstance");

        if (isTotal)
        {
            m_procName = L"_Total";
        }
        else
        {
            // The name of an instance is the processor number
            m_procName = StrFrom(procNumber);
        }

        SCX_LOGTRACE(m_log, wstring(L"CPUInstance default constructor - ").append(m_procName));

        m_procNumber = procNumber;

        // Init data
        m_processorTime = 0;
        m_idleTime = 0;
        m_userTime = 0;
        m_niceTime = 0;
        m_privilegedTime = 0;
        m_iowaitTime = 0;
        m_interruptTime = 0;
        m_dpcTime = 0;
        m_queueLength = 0;
    }


    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    */
    CPUInstance::~CPUInstance()
    {
        SCX_LOGTRACE(m_log, wstring(L"CPUInstance destructor - ").append(m_procName));
    }

    /*----------------------------------------------------------------------------*/
#if defined(aix)
    /**
       Updates data sampler members from raw data.

       \param raw Pointer to raw data for this CPU instance

       This is called periodically from the updater thread.

       \note This is the object oriented way of doing updates. Let the instance
       have the knowledge on how to update itself.
    */
    void CPUInstance::UpdateDataSampler(perfstat_cpu_t *raw)
    {
        m_UserCPU_tics.AddSample(raw->user);
        m_SystemCPUTime_tics.AddSample(raw->sys);
        m_IdleCPU_tics.AddSample(raw->idle);
        m_IOWaitTime_tics.AddSample(raw->wait);
        m_queueLength = raw->runque;            // Threads on runqueue
    }

    /**
       Updates data sampler members from raw data.

       \param raw Pointer to raw data for the total instance

       This is called periodically from the updater thread. This version is exclusive
       for the total instance. The is because the total instance is collected in a
       different structure.
    */
    void CPUInstance::UpdateDataSampler(perfstat_cpu_total_t *raw)
    {
        m_UserCPU_tics.AddSample(raw->user);
        m_SystemCPUTime_tics.AddSample(raw->sys);
        m_IdleCPU_tics.AddSample(raw->idle);
        m_IOWaitTime_tics.AddSample(raw->wait);
        m_queueLength = raw->runque;            // Processes on runqueue
    }
#endif

    /*----------------------------------------------------------------------------*/
    /**
        Get processor name

        Retval:      name of processor instance
    */
    const wstring& CPUInstance::GetProcName() const
    {
        return m_procName;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor number

        Retval:      number of processor instance
    */
    unsigned int CPUInstance::GetProcNumber() const
    {
        return m_procNumber;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor time

        Parameters:  processorTime - RETURN: time of processor instance

        Retval:      true if a value is supported by the implementation
    */
    bool CPUInstance::GetProcessorTime(scxulong& processorTime) const
    {
        processorTime = m_processorTime;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor idle time

        Parameters:  idleTime - RETURN: idle time of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetIdleTime(scxulong& idleTime) const
    {
        idleTime = m_idleTime;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor user time

        Parameters:  userTime - RETURN: user time of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetUserTime(scxulong& userTime) const
    {
        userTime = m_userTime;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor nice time

        Parameters:  niceTime - RETURN: nice time of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetNiceTime(scxulong& niceTime) const
    {
#if defined(linux) || defined(hpux)
        niceTime = m_niceTime;
        return true;
#else
        niceTime = niceTime;
        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor priviledged time

        Parameters:  privilegedTime - RETURN: priviledged time of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetPrivilegedTime(scxulong& privilegedTime) const
    {
        privilegedTime = m_privilegedTime;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor iowait time

        Parameters:  iowaitTime - RETURN: iowait time of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetIowaitTime(scxulong& iowaitTime) const
    {
        iowaitTime = m_iowaitTime;
        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor interrupt time

        Parameters:  interruptTime - RETURN: interrupt time of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetInterruptTime(scxulong& interruptTime) const
    {
#if defined(linux)
        interruptTime = m_interruptTime;
        return true;
#else
        interruptTime = interruptTime;
        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor dpc time

        Parameters:  dpcTime - RETURN: dpc time of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetDpcTime(scxulong& dpcTime) const
    {
#if defined(linux)
        dpcTime = m_dpcTime;
        return true;
#else
        dpcTime = dpcTime;
        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get processor queue length

        Parameters:  procName - RETURN: queue length of processor instance

        Retval:      true if a value is supported by this implementation
    */
    bool CPUInstance::GetQueueLength(scxulong& queueLength) const
    {
#if defined(aix)
        queueLength = m_queueLength;
        return true;
#else
        queueLength = queueLength; // Work around a potential warning.
        return false;
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update values

    */
    void CPUInstance::Update()
    {
        SCX_LOGTRACE(m_log, wstring(L"CPUInstance::Update() - ").append(m_procName));

#if defined(linux) || defined(sun) || defined(hpux)
        scxulong total_delta_tics = m_Total_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong idle_delta_tics = m_IdleCPU_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong user_delta_tics = m_UserCPU_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong system_delta_tics = m_SystemCPUTime_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong nice_delta_tics = m_NiceCPU_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong iowait_delta_tics = m_IOWaitTime_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong irq_delta_tics = m_IRQTime_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong softirq_delta_tics = m_SoftIRQTime_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);

        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    total count = ", m_Total_tics.GetNumberOfSamples()));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    total delta = ", total_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    idle delta = ", idle_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    user delta = ", user_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    nice delta = ", nice_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    system delta = ", system_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    iowait delta = ", iowait_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    irq delta = ", irq_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    softirq delta = ", softirq_delta_tics));

        m_processorTime = GetPercentageSafe(idle_delta_tics,   total_delta_tics, true);
        m_idleTime      = GetPercentageSafe(idle_delta_tics,   total_delta_tics);
        m_userTime      = GetPercentageSafe(user_delta_tics,   total_delta_tics);
        m_niceTime      = GetPercentageSafe(nice_delta_tics,   total_delta_tics);
        m_privilegedTime= GetPercentageSafe(system_delta_tics, total_delta_tics);
        m_iowaitTime    = GetPercentageSafe(iowait_delta_tics, total_delta_tics);
        m_interruptTime = GetPercentageSafe(irq_delta_tics,    total_delta_tics);
        m_dpcTime       = GetPercentageSafe(softirq_delta_tics,total_delta_tics);

#elif defined(aix)

        /* We have a somewhat different strategy on AIX: Compute deltas for all sample
           items. Add them, and compute the respective percentage. (I should note that
           this number is entirely consistent with what you would get if you divide the
           deltas with the sample interval multiplied with with the clock rate. This has
           been tested. The result may be different on a partitioned system.)
        */

        scxulong user_delta_tics = m_UserCPU_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong system_delta_tics = m_SystemCPUTime_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong iowait_delta_tics = m_IOWaitTime_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);
        scxulong idle_delta_tics = m_IdleCPU_tics.GetDelta(MAX_CPUINSTANCE_DATASAMPER_SAMPLES);

        scxulong total_delta_tics = user_delta_tics + system_delta_tics
            + iowait_delta_tics + idle_delta_tics;


        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    user delta = ", user_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    system delta = ", system_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    iowait delta = ", iowait_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    idle delta = ", idle_delta_tics));
        SCX_LOGHYSTERICAL(m_log, StrAppend(L"    total delta = ", total_delta_tics));


        m_userTime = GetPercentageSafe(user_delta_tics, total_delta_tics);
        m_privilegedTime = GetPercentageSafe(system_delta_tics, total_delta_tics);
        m_iowaitTime = GetPercentageSafe(iowait_delta_tics, total_delta_tics);
        m_idleTime = GetPercentageSafe(idle_delta_tics, total_delta_tics);
        // m_processorTime is sum of non-idle time. (Adding the percentages gives rounding err)
        m_processorTime = GetPercentageSafe(user_delta_tics + system_delta_tics
                                            + iowait_delta_tics, total_delta_tics);

#else
#error "Implement this!"
#endif
    }

    /*----------------------------------------------------------------------------*/
    /**
        Calculate tic percentages of total elapsed tics.

        Parameters:  tic_delta - Difference of old and new value of the counter
        Parameters:  tot_delta - Difference of old and new value of the total counter
        Parameters:  inverse - Inverse the calculation
        Returns:     Number of percent tic has taken of tot
        Created:     2007-06-11 09:15:00

        Wraps the percentage calculation from scxmath in order to ignore border cases
        like when counter wraps or first time used in the provider.

    */
    scxulong CPUInstance::GetPercentageSafe(const scxulong tic_delta,
                                                  const scxulong tot_delta,
                                                  const bool inverse /* = false */) const
    {
        return GetPercentage(0, tic_delta, 0, tot_delta, inverse);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the User ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetUserLastTick() const
    {
        size_t last = m_UserCPU_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_UserCPU_tics[0];
        } else
        {
            return 0;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the Nice ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetNiceLastTick() const
    {
        size_t last = m_NiceCPU_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_NiceCPU_tics[0];
        } else
        {
            return 0;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the System ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetPrivilegedLastTick() const
    {
        size_t last = m_SystemCPUTime_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_SystemCPUTime_tics[0];
        } else
        {
            return 0;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the Idle ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetIdleLastTick() const
    {
        size_t last = m_IdleCPU_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_IdleCPU_tics[0];
        } else
        {
            return 0;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the Wait ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetIowaitLastTick() const
    {
        size_t last = m_IOWaitTime_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_IOWaitTime_tics[0];
        } else
        {
            return 0;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the Interrupt ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetInterruptLastTick() const
    {
        size_t last = m_IRQTime_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_IRQTime_tics[0];
        } else
        {
            return 0;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the SW Interrupt ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetSWInterruptLastTick() const
    {
        size_t last = m_SoftIRQTime_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_SoftIRQTime_tics[0];
        } else
        {
            return 0;
        }
    }
    /*----------------------------------------------------------------------------*/
    /**
        Retrieve the last sample of the Total ticks performance counter.

        \returns    The last sample or 0 if no samples exists.
    */
    scxulong CPUInstance::GetTotalLastTick() const
    {
        size_t last = m_Total_tics.GetNumberOfSamples();
        if (last > 0)
        {
            return m_Total_tics[0];
        } else
        {
            return 0;
        }
    }
}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
