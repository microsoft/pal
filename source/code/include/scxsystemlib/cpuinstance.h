/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       PAL representation of a CPU

   \date        07-05-21 12:00:00

   \date        08-05-28 14:43:00
*/
/*----------------------------------------------------------------------------*/
#ifndef CPUINSTANCE_H
#define CPUINSTANCE_H

#include <string>

#if defined(aix)
#include <libperfstat.h>
#endif

#include <scxsystemlib/entityinstance.h>
#include <scxsystemlib/datasampler.h>
#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{
    /** Number of samples collected in the datasampler for CPU. */
    const size_t MAX_CPUINSTANCE_DATASAMPER_SAMPLES = 6;

    /** Datasampler for CPU information. */
#if defined(aix)
    typedef DataSampler<u_longlong_t> CPUInstanceDataSampler;
#else
    typedef DataSampler<scxulong> CPUInstanceDataSampler;
#endif
    /*----------------------------------------------------------------------------*/
    /**
       Class that represents a colletion of instances.

       Concrete implementation of an instance of a CPU

    */
    class CPUInstance : public EntityInstance
    {
        friend class CPUEnumeration;

    public:

        CPUInstance(unsigned int procNumber, size_t sampleSize, bool isTotal = false);
        virtual ~CPUInstance();

        const std::wstring& GetProcName() const;
        unsigned int GetProcNumber() const;

        virtual void Update();

        // Return values indicate whether the implementation for this platform
        // supports the value or not.
        bool GetProcessorTime(scxulong& processorTime) const ;
        bool GetIdleTime(scxulong& idleTime) const;
        bool GetUserTime(scxulong& userTime) const;
        bool GetNiceTime(scxulong& niceTime) const;
        bool GetPrivilegedTime(scxulong& privilegedTime) const;
        bool GetIowaitTime(scxulong& iowaitTime) const;
        bool GetInterruptTime(scxulong& interruptTime) const;
        bool GetDpcTime(scxulong& dpcTime) const;
        bool GetQueueLength(scxulong& queueLength) const;

#if defined(aix)
        void UpdateDataSampler(perfstat_cpu_t *raw);
        void UpdateDataSampler(perfstat_cpu_total_t *raw);
#endif

        scxulong GetUserLastTick() const;
        scxulong GetIdleLastTick() const;
        scxulong GetNiceLastTick() const;
        scxulong GetPrivilegedLastTick() const;
        scxulong GetIowaitLastTick() const;
        scxulong GetInterruptLastTick() const;
        scxulong GetSWInterruptLastTick() const;
        scxulong GetTotalLastTick() const;

    private:
        scxulong GetPercentageSafe(const scxulong tic_delta,
                                         const scxulong tot_delta,
                                         const bool inverse = false) const;

    private:

        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle

        std::wstring m_procName;         //!< Processor name
        unsigned int m_procNumber;       //!< Processor number

        scxulong m_processorTime;        //!< Processor time.
        scxulong m_idleTime;             //!< Processor idle time.
        scxulong m_userTime;             //!< Processor user time.
        scxulong m_niceTime;             //!< Processor nice time.
        scxulong m_privilegedTime;       //!< Processor privileged time.
        scxulong m_iowaitTime;           //!< Processor io wait time.
        scxulong m_interruptTime;        //!< Processor interrupt time.
        scxulong m_dpcTime;              //!< Processor dpc time.
        scxulong m_queueLength;          //!< Processor queue length.

        // NB: Not all of these are used on every platform
        CPUInstanceDataSampler m_UserCPU_tics;       //!< Data sampler for user time.
        CPUInstanceDataSampler m_NiceCPU_tics;       //!< Data sampler for nice time.
        CPUInstanceDataSampler m_SystemCPUTime_tics; //!< Data sampler for system time.
        CPUInstanceDataSampler m_IdleCPU_tics;       //!< Data sampler for idle time.
        CPUInstanceDataSampler m_IOWaitTime_tics;    //!< Data sampler for IO wait time.
        CPUInstanceDataSampler m_IRQTime_tics;       //!< Data sampler for IRQ time.
        CPUInstanceDataSampler m_SoftIRQTime_tics;   //!< Data sampler for soft IRQ time
        CPUInstanceDataSampler m_Total_tics;         //!< Data sampler for total time.
    };

}

#endif /* CPUINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
