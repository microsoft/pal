/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       SystemInfo class (system information)
    \date        08-23-11 13:00:00

*/
/*----------------------------------------------------------------------------*/
#ifndef SYSTEMINFO_H
#define SYSTEMINFO_H

#include <scxcorelib/scxlog.h>
#include <string>

#if defined(aix)
#include <libperfstat.h>
#endif // defined(aix)


namespace SCXSystemLib
{
    //! Type of virtual machine
    enum eVmType {
        eVmUnknown = 0,         //!< Unsure if we're in a VM or not
        eVmNotDetected,         //!< Definitely not within a VM
        eVmDetected             //!< Definitely within a VM
    };


#if defined(linux)
    //! CPUID function calls (for virtual machine detection on Linux)
    typedef enum
    {
        eProcessorInfo        = 0x00000001,     //!< Processor "virtual bit"
        eHypervisorInfo       = 0x40000000,     //!< Hypervisor Information
        eHyperV_VendorNeutral = 0x40000001,     //!< Hyper-V: Vendor Neutral flag
        eHyperV_FeaturesID    = 0x40000003      //!< Hyper-V: Feature ID flag
    } CpuIdFunction;

    //! Specific virtual machine types for Linux
    typedef enum
    {
        eNoVmDetected = 0,                      //!< Physical machine
        eDetectedHyperV,                        //!< Hyper-V machine
        eDetectedVMware,                        //!< VMware machine
        eDetectedXen,                           //!< XEN machine
        eUnknownVmDetected = 0xffffffff         //!< VM, but of unknown type
    } LinuxVmType;


    /*------------------------------------------------------------------------*/
    /**
       This class provides a "container" for internal x86/x64 registers,
       which are accessed via assembly for virtual machine detection.
    */
    struct Registers
    {
    public:
        Registers()
            : m_eax(0), m_ebx(0), m_ecx(0), m_edx(0)
        {}

        u_int32_t m_eax;
        u_int32_t m_ebx;
        u_int32_t m_ecx;
        u_int32_t m_edx;

    private:
        Registers(const Registers & source);                    //!< Intentionally not implemented
        Registers & operator=(const Registers & source);        //!< Intentionally not implemented
    };
#endif


    /*----------------------------------------------------------------------------*/
    /**
       Class representing all external dependencies from the CPU PAL.

    */
    class SystemInfoDependencies
    {
    public:
        virtual ~SystemInfoDependencies() {};

        virtual char *getenv(const char *name) const;
        virtual uid_t geteuid() const;

#if defined(linux)
        virtual void CallCPUID(CpuIdFunction function, Registers& registers);

        // These aren't typically overridden, and thus aren't virtual
        LinuxVmType DetermineLinuxVirtualMachineState();
        bool IsHypervisorPresent();
        bool IsVendorNeutral();
        bool IsCreatePartitionsEnabled();
#elif defined(aix)
        virtual int perfstat_partition_total(perfstat_id_t* name,
					     perfstat_partition_total_t* userbuff,
					     size_t sizeof_struct,
					     int desired_number);
#endif // defined(linux)
    };


    /*----------------------------------------------------------------------------*/
    /**
       Class that represents the common set of System parameters.

       This class only implements the total instance and has no
       collection threrad.
    */
    class SystemInfo
    {
        static const wchar_t *moduleIdentifier;         //!< Shared module string

    public:
        explicit SystemInfo( SCXCoreLib::SCXHandle<SystemInfoDependencies> = SCXCoreLib::SCXHandle<SystemInfoDependencies>( new SystemInfoDependencies()) );
        virtual ~SystemInfo();

        virtual const std::wstring DumpString() const;
        std::wstring DumpVmType(eVmType e) const;

        static const wchar_t *GetModuleIdentifier() { return moduleIdentifier; }

        /* Properties returned by System Information */
        bool GetNativeBitSize(unsigned short& bitsize) const;
        bool GetVirtualMachineState(eVmType& vmType) const;

        std::wstring GetDefaultSudoPath() const;
        std::wstring GetShellCommand(const std::wstring& command) const;
        std::wstring GetElevatedCommand(const std::wstring& command) const;

#if defined(aix)
        bool GetAIX_IsInWPAR(bool& isInWPAR) const;
#endif
#if defined(sun)
        bool GetSUN_IsInGlobalZone(bool& isInGlobalZone) const;
#endif

    protected:
        virtual void Update();

        unsigned short DetermineNativeBitSize();
        eVmType DetermineVirtualMachineState();

    private:
        SCXCoreLib::SCXHandle<SystemInfoDependencies> m_deps; //!< Collects external depedencies of this class
        SCXCoreLib::SCXLogHandle m_log;		//!< Log handle.

        unsigned short m_nativeBitSize;         //!< Native bit size on machine
        eVmType m_vmType;                       //!< Virtual machine status
        std::wstring m_defaultSudoPath;         //!< Default sudo location for this platform
#if defined(aix)
        bool m_isInWPAR;                        //!< AIX: Are we in a Workload Partition?
#endif
#if defined(sun)
        bool m_isInGlobalZone;                  //!< SUN: Are we in the global zone?
#endif

        //
        // Virtual machine support functions (Platform-Specific)
        //
#if defined(linux)
    private:
        LinuxVmType m_linuxVmType;
#endif
    };
}

#endif /* SYSTEMINFO_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
