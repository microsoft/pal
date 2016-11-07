/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       PAL representation of the system interfaces

   \date        08-23-04 13:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/logsuppressor.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/stringaid.h>

#include <scxsystemlib/scxsysteminfo.h>

#include <errno.h>
#include <unistd.h>
#include <sys/utsname.h>

#if defined(aix)
  #include <libperfstat.h>
  #if PF_MAJOR >= 6
    #include <sys/wpar.h>
  #endif // PF_MAJOR >= 6
#endif // defined(aix)

#if defined(sun)
  #include <sys/systeminfo.h>
  #if (PF_MAJOR > 5 || (PF_MAJOR == 5 && PF_MINOR >= 10))
    #define _SUN_HAS_ZONE_SUPPORT_
    #include <zone.h>
  #endif // (PF_MAJOR > 5 || (PF_MAJOR == 5 && PF_MINOR >= 10))
#endif // defined(sun)

#if defined(macos)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif


// Find a "reasonable default" for sudo location
// To use sudo via RunAs provider, we must have our kit installed.  Since our
// kit now creates a sudo link, we can depend on that.

const std::wstring s_defaultSudoPath = L"/etc/opt/microsoft/scx/conf/sudodir/sudo";


using namespace std;
using namespace SCXCoreLib;

namespace SCXSystemLib
{
    //
    // Definitions for SystemInfoDependencies
    //

    char *SystemInfoDependencies::getenv(const char *name) const
    {
        return ::getenv(name);
    }

    uid_t SystemInfoDependencies::geteuid() const
    {
        return ::geteuid();
    }

    //
    // Definitions for SystemInfo
    //

    const wchar_t *SystemInfo::moduleIdentifier = L"scx.core.common.pal.system.systeminfo";

    /*----------------------------------------------------------------------------*/
    /**
       Constructor
    */
    SystemInfo::SystemInfo(SCXCoreLib::SCXHandle<SystemInfoDependencies> deps) :
        m_deps(deps),
        m_nativeBitSize(0),
        m_vmType(eVmUnknown),
        m_defaultSudoPath()
#if defined(aix)
        , m_isInWPAR(false)
#endif
#if defined(sun)
        , m_isInGlobalZone(true)
#endif
    {
        m_log = SCXLogHandleFactory::GetLogHandle(moduleIdentifier);
        SCX_LOGTRACE(m_log, L"SystemInfo constructor");
        Update();
    }

    /*----------------------------------------------------------------------------*/
    /**
       Destructor
    */
    SystemInfo::~SystemInfo()
    {
        SCX_LOGTRACE(m_log, L"SystemInfo destructor");
    }

    /*----------------------------------------------------------------------------*/
    /**
       Updates instance with latest data in preparation for read of individual 
       properties.
    */
    void SystemInfo::Update()
    {
        SCX_LOGTRACE(m_log, L"SystemInfo Update()");

        // Look up the native size of the machine
        m_nativeBitSize = DetermineNativeBitSize();

        // Figure out if we're in a virtual machine or not
        m_vmType = DetermineVirtualMachineState();

        // For now, default sudo path is hard-coded based on platform.  If we
        // allowed configuration, something smarter could be done here ...
        m_defaultSudoPath = s_defaultSudoPath;

#if defined(aix) && PF_MAJOR >= 6
        /*
         * Determine if we're in a WPAR (only supported on AIX 6 and above)
         */

        if ( wpar_getcid() != 0 )
        {
            m_isInWPAR = true;
        }
#endif

#if defined(sun)
#if defined(_SUN_HAS_ZONE_SUPPORT_)
        zoneid_t zid = getzoneid();

        if (-1 != zid)
        {
            m_isInGlobalZone = (GLOBAL_ZONEID == zid);
        }
        else
        {
            throw SCXErrnoException(L"getzoneid() function call failed", errno, SCXSRCLOCATION);
        }
#else // defined(_SUN_HAS_ZONE_SUPPORT_)
        m_isInGlobalZone = true;
#endif // defined(_SUN_HAS_ZONE_SUPPORT_)
#endif // defined(sun)
    }


    /*----------------------------------------------------------------------------*/
    /**
       Dump object as string (for logging).

       \returns     The object represented as a string suitable for logging.
    */
    const wstring SystemInfo::DumpString() const
    {
        return SCXDumpStringBuilder("SystemInfo")
        .Scalar("nativeBitSize", m_nativeBitSize)
        .Scalar("vmType", DumpVmType(m_vmType))
        .Text("defaultSudoPath", m_defaultSudoPath)
#if defined(aix)
        .Scalar("isInWPAR", m_isInWPAR)
#endif
#if defined(sun)
        .Scalar("isInGlobalZone", m_isInGlobalZone)
#endif
        ;
    }

    /**
       Provides the text name of the enum value for virtual machine state.

       \param e The enum value to translate.
       \return The text name of the enum value.
    */
    wstring  SystemInfo::DumpVmType(eVmType e) const
    {
        wstring  vmType;
        switch (e)
        {
        case eVmDetected:       vmType = L"VmDetected"; break;
        case eVmNotDetected:    vmType = L"VmNotDetected"; break;
        case eVmUnknown:        vmType = L"VmUnknown"; break;
        }

        if (vmType.length())
        {
#if defined(linux)
            // Append subtype of the virtual machine for Linux
            switch (m_linuxVmType)
            {
            case eNoVmDetected:         break;
            case eDetectedHyperV:       vmType.append(L"(Hyper-V)"); break;
            case eDetectedVMware:       vmType.append(L"(VMware)");  break;
            case eDetectedXen:          vmType.append(L"(Xen)");     break;

            case eUnknownVmDetected:
            default:
                vmType.append(L"(Unknown)"); break;
            }
#endif // defined(linux)

            return vmType;
        }

        wostringstream ws;
        ws << L"Undefined enum value for eTagSource." << e << std::ends;
        throw SCXInternalErrorException(ws.str().c_str(), SCXSRCLOCATION);
    }

    /**
       Determine native bit size for machine
    */
    unsigned short SystemInfo::DetermineNativeBitSize()
    {
        unsigned short bitSize = 0;

        // For now, we just detect Linux - otherwise, assume 64 bits (except for OpenSolaris)
        // See wi7734 for more information

#if defined(linux)

        struct utsname uname_buf;
        int rc = uname( & uname_buf );
        if (-1 == rc)
        {
            throw SCXErrnoException(L"uname() function call failed", errno, SCXSRCLOCATION);
        }

        wstring  strMachine = StrFromUTF8(uname_buf.machine);
        if (StrCompare(strMachine, L"x86_64") == 0 || StrCompare(strMachine, L"ppc64le") == 0)
        {
            bitSize = 64;
        }
        else if (StrCompare(strMachine, L"i386") == 0 || StrCompare(strMachine, L"i586") == 0 || StrCompare(strMachine, L"i686") == 0)
        {
            bitSize = 32;
        }
        else {
            wostringstream ws;
            ws << L"Unexpected return value for uname->machine: " << strMachine << std::ends;
            throw SCXInternalErrorException(ws.str().c_str(), SCXSRCLOCATION);
        }

#elif defined(aix)

        long ret = sysconf (_SC_AIX_KERNEL_BITMODE);
        if (ret < 0)
        {
            throw SCXErrnoException(L"sysconf(_SC_AIX_KERNEL_BITMODE) failed", errno, SCXSRCLOCATION);
        }
        bitSize = static_cast<unsigned short> (ret);

#elif defined(hpux)

        long ret = sysconf (_SC_KERNEL_BITS);
        if (ret < 0)
        {
            throw SCXErrnoException(L"sysconf(_SC_KERNEL_BITS) failed", errno, SCXSRCLOCATION);
        }
        bitSize = static_cast<unsigned short> (ret);

#elif defined(macos)
        /*
           For Mac OS Intel platforms, the history is this:

           As of Jan 2006, Apple released machines with the "Intel Core Duo" processor.
           This was a 32-bit processor, and was not capable of 64-bits.  Intel support
           was first available as a bugfix release to Tiger (10.4.4).

           As of April 2006 (but more completely by July 2006), Apple moved to the
           "Intel Core 2 Duo" processor, which supports 64-bit.

           To determine if the machine is 32-bit or 64-bit, we use sysctl().  According
           to the manpage, if the item is unknown, it should be treated as unavailable.

           Since hw.optional.x86_64 isn't defined in the header files, we need to look
           it up by name.
         */

        // Would like to use "hw.cpu64bit_capable", but that does not exist on 10.4.
        // So we use "hw.optional.x86_64" that is Intel-specific

        int supports64bit = 0;
        size_t bitLength = sizeof(supports64bit);

        if (0 == sysctlbyname("hw.optional.x86_64", &supports64bit, &bitLength, NULL, 0))
        {
            if (supports64bit)
            {
                bitSize = 64;
            }
            else
            {
                bitSize = 32;
            }
        }
        else
        {
            // Unknown?  Must not have 64-bit capabilities
            bitSize = 32;
        }
#elif defined(sun)
        bitSize = 32;
        std::vector<char> buf(256);
        /*
           On Solaris 5.10 there is a simple way to check this using SI_ARCHITECTURE_64.
           If the sysinfo call returns a non-negative value then the architecture supports
           64 bits.
         */
#if defined(SI_ARCHITECTURE_64)
        long ret = sysinfo(SI_ARCHITECTURE_64, &buf[0], buf.size());
        if (ret != -1)
        {
            bitSize = 64;
        }
#else
        /*
           Earlier versions of Solaris don't support the SI_ARCHITECTURE_64 so we need
           to fall back on parsing output from sysinfo(SI_ISALIST).
         */
        long ret = sysinfo(SI_ISALIST, &buf[0], buf.size());
        if (ret < 0)
        {
            throw SCXErrnoException(L"sysinfo(SI_ISALIST) failed", errno, SCXSRCLOCATION);
        }
        std::string isalist(&buf[0]);
        if (isalist.find("sparcv9") != std::string::npos ||
            isalist.find("amd64") != std::string::npos)
        {
            bitSize = 64;
        }
#endif
#else
        #error Platform not supported
#endif

        SCXASSERT( bitSize == 32 || bitSize == 64 );
        return bitSize;
    }


/*----------------------------------------------------------------------------*/
//
// Support code for virtual machine detection
//
/*----------------------------------------------------------------------------*/

#if defined(linux) && !defined(ppc)

    // this is the 4 1-byte characters "Hv#1" as a 4-byte unsigned little endian integer

    static const unsigned int HyperV_VendorNeutralID = 0x31237648;

    bool SystemInfoDependencies::IsHypervisorPresent()
    {
        Registers registers;
        CallCPUID(eProcessorInfo, registers);

        return (((registers.m_ecx >> 31) & 0x1) != 0);
    }

    /**
       Call the CPUID function on x86/x64 platforms (virtual machine detection)

       Note: This is virtual for unit test purposes.

       \param   function        [IN] CPUID function to perform
       \param   registers       [OUT] CPU registers from resulting CPUID call
    */
    void SystemInfoDependencies::CallCPUID(CpuIdFunction function, Registers& registers)
    {
        // Use an inline assembly template to call the CPUID opcode
        //
        // Note: Take care with PIC (position independent code).  PIC in i386
        //       uses a register to store the GOT (global offset table) address.
        //       This register is usually %ebx, making it unavailable for use by
        //       inline assembly.
        //
        //       If we're linking i386 & PIC, save %ebx & restore it when done.
        //       All other cases (non-PIC, x64, etc), don't bother with %ebx.

#if defined(__i386__) && defined(__PIC__)
        asm volatile
        (
            "pushl %%ebx    \n\t"   /* Save %ebx - necessary to generate PIC */
            "cpuid          \n\t"   /* opcode, a.k.a. the assembler template */
            "movl %%ebx, %1 \n\t"   /* Save what cpuid just put in %ebx */
            "popl %%ebx     \n\t"   /* Restore the old %ebx */
            :          /* output operands                       */
                "=a" (registers.m_eax),
                "=r" (registers.m_ebx),
                "=c" (registers.m_ecx),
                "=d" (registers.m_edx)
            :          /* input operands                        */
                "a" (static_cast< unsigned int >(function))
        );
#else
        asm
        (
            "cpuid"    /* opcode, a.k.a. the assembler template */
            :          /* output operands                       */
                "=a" (registers.m_eax),
                "=b" (registers.m_ebx),
                "=c" (registers.m_ecx),
                "=d" (registers.m_edx)
            :          /* input operands                        */
                "a" (static_cast< unsigned int >(function))
        );
#endif // defined(__i386__) && defined(__PIC__)
        return;
    }

    bool SystemInfoDependencies::IsVendorNeutral()
    {
        Registers registers;
        CallCPUID(eHyperV_VendorNeutral, registers);

        return (registers.m_eax == HyperV_VendorNeutralID);
    }

    bool SystemInfoDependencies::IsCreatePartitionsEnabled()
    {
        Registers registers;
        CallCPUID(eHyperV_FeaturesID, registers);

        return ((registers.m_ebx & 0x1) != 0);
    }

    LinuxVmType SystemInfoDependencies::DetermineLinuxVirtualMachineState()
    {
        static const std::string HyperVSignature = "Microsoft Hv";
        static const std::string VMwareSignature = "VMwareVMware";
        static const std::string XenSignature    = "XenVMMXenVMM";

        LinuxVmType result;
        bool isHypervisorPresent = IsHypervisorPresent();

        if (isHypervisorPresent)
        {
            Registers registers;
            CallCPUID(eHypervisorInfo, registers);

            // The 12 character vendor ID is stored in registers EBX, ECX and EDX ...
            std::string signature;
            signature.assign(reinterpret_cast< const char * >(&registers.m_ebx), sizeof(registers.m_ebx));
            signature.append(reinterpret_cast< const char * >(&registers.m_ecx), sizeof(registers.m_ecx));
            signature.append(reinterpret_cast< const char * >(&registers.m_edx), sizeof(registers.m_edx));

            if (0 == signature.compare(VMwareSignature))
            {
                result = eDetectedVMware;
            }
            else if (0 == signature.compare(HyperVSignature))
            {
                if (IsVendorNeutral() == true)
                {
                    if (IsCreatePartitionsEnabled() == false)
                    {
                        result = eDetectedHyperV;
                    }
                    else
                    {
                        // No Hypervisor after all ...
                        result = eNoVmDetected;
                    }
                }
                else
                {
                    // Note: Perhaps this could only happen on older, unsupported
                    //       versions of Hyper-V?  We don't know that for sure, so
                    //       log it (once) and then just say unknown ...

                    result = eUnknownVmDetected;

                    static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(L"HyperV_NoVendorNeutral"));
                    SCX_LOG(SCXLogHandleFactory::GetLogHandle(SystemInfo::GetModuleIdentifier()),
                            severity,
                            L"VM detection error: Hyper-V detected without VendorNeutral");
                }
            }
            else if (signature.compare(XenSignature) == 0)
            {
                result = eDetectedXen;
            }
            else
            {
                // Uh oh; some sort of signature we don't recognize
                result = eUnknownVmDetected;

                wstring  wsSignature = StrFromUTF8(signature);

                static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
                SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(wsSignature));
                SCX_LOG(SCXLogHandleFactory::GetLogHandle(SystemInfo::GetModuleIdentifier()),
                        severity,
                        StrAppend(L"Unknown hypervisor signature: ", wsSignature));
            }
        }
        else
        {
            // The hypervisor bit was not set - must not be a virtual machine

            result = eNoVmDetected;
        }

        return result;
    }
#elif defined(aix)
    /**
       Call the perfstat_parition_total function on AIX platforms

       Note: This is virtual for unit test purposes.

       \param   name            [IN] Must be set to NULL
       \param   userbuff        [IN/OUT] Points to the memory area to be filled
       \param   sizeof_struct   [IN] Specifies the size of the 'userbuff' structure
       \param   desired_number  [IN] Must be set to 1

       \returns Number of structures filled, or -1 if error (errno is set in this case)
    */
    int SystemInfoDependencies::perfstat_partition_total(perfstat_id_t* name,
                                                         perfstat_partition_total_t* userbuff,
                                                         size_t sizeof_struct,
                                                         int desired_number)
    {
        return ::perfstat_partition_total(name, userbuff, sizeof_struct, desired_number);
    }
#endif // defined(linux)

    /**
       Determine virtual machine state for machine

       TODO: Complete implementation for all platforms.  Until then, just return unknown.
    */
#if defined(linux) && !defined(ppc)
    eVmType SystemInfo::DetermineVirtualMachineState()
    {
        // Save the last copy we fetched (shouldn't normally change)
        // This allows us to show details for Dump() purposes ...
        m_linuxVmType = m_deps->DetermineLinuxVirtualMachineState();

        switch (m_linuxVmType)
        {
            case eNoVmDetected:
                return eVmNotDetected;

            case eDetectedHyperV:
            case eDetectedVMware:
            case eDetectedXen:
                return eVmDetected;

            case eUnknownVmDetected:
            default:
                return eVmUnknown;
        }
    }
#elif defined(linux) && defined(ppc)
    eVmType SystemInfo::DetermineVirtualMachineState()
    {
        // Linux is always virtualized on PowerPC
        return eVmDetected;
    }
#elif defined(aix)
    eVmType SystemInfo::DetermineVirtualMachineState()
    {
        // According to IBM on PMR number 13089,550:
        //
        // A physical machine will look like this (from 'lparstat' command):
        //   System configuration: type=Dedicated mode=Capped smt=On lcpu=2 mem=1904
        //
        //   %user %sys %wait %idle
        //   ----- ---- ----- -----
        //   0.9   0.4  0.1   98.6
        //
        // An LPAR machine will look like this:
        //   System configuration: type=Shared mode=Uncapped smt=On lcpu=2 mem=2560MB psize=1 ent=0.50
        //   %user %sys %wait %idle physc %entc lbusy app vcsw phint %nsp
        //   ----- ---- ----- ----- ----- ----- ----- --- ---- ----- ----
        //   6.0   52.9 0.0   41.1  0.00  0.6   0.1   0.99 357813532 1788069 99
        //
        // The key differences here:
        //   . "type=Dedicated" vs. "type=Shared"
        //   . No "psize=" vs. "psize=#"
        //   . LPAR-specific statistics
        //
        // According to 'man lparstat', psize=# of online physical processors in pool.
        // This seems like an error; other flags should be sufficient to determine LPAR

        perfstat_partition_total_t lparStats;
        if (! m_deps->perfstat_partition_total(NULL, &lparStats, sizeof(perfstat_partition_total_t), 1))
        {
            static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(L"perfstat_partition_total"));
            SCX_LOG(SCXLogHandleFactory::GetLogHandle(moduleIdentifier),
                    severity,
                    StrAppend(L"Error calling perfstat_partition_total() - unable to determine VM state; errno=",
                              errno));

            return eVmUnknown;
        }

        // 'shared_enabled' determines if Dedicated (false) or Shared (true)
        // 'shared_enabled || donate_enabled' determines if LPAR statistics appear
        if (lparStats.type.b.shared_enabled || lparStats.type.b.donate_enabled)
        {
            return eVmDetected;
        }

        // If we know we're not an LPAR, we still can't be sure if we're physical
        // because we could be a PowerVM/VIOS hosted VM.  Thus, say unknown ...
        return eVmUnknown;
    }
#elif defined(hpux)
    eVmType SystemInfo::DetermineVirtualMachineState()
    {
        return eVmUnknown;
    }
#elif defined(sun)
    eVmType SystemInfo::DetermineVirtualMachineState()
    {
        return eVmUnknown;
    }
#else
#error Not implemented on this platform
#endif


    /**
       Gets the Native Bit Size of the machine

       \param[out]  bitsize
       \returns     true if this value is supported by the implementation
    */
    bool SystemInfo::GetNativeBitSize(unsigned short& bitsize) const
    {
        bitsize = m_nativeBitSize;
        return (bitsize != 0);
    }

    /**
       Get the virtualization state of a machine (Unknown, Virtual, NotVirtual).

       \param[out]  Virtualization type of the machine (enum)
       \returns     true if this value is supported by the implementation
    */
    bool SystemInfo::GetVirtualMachineState(eVmType& vmType) const
    {
        vmType = m_vmType;
#if defined(linux) || defined(aix)
        return true;
#elif defined(hpux) || defined(sun)
        return false;
#else
  #error Platform not defined
#endif
    }

    /**
       Returns the default sudo path for this platform.
       Note that this just returns a reasonable default; sudo may not exist
       at that path if the customer has modified their system.

       \returns Default path for the 'sudo' program
    */
    wstring SystemInfo::GetDefaultSudoPath() const
    {
        return m_defaultSudoPath;
    }

    /**
       Returns a command formatted to be executed by the current shell.
       If no shell is currently defined, we use /bin/sh (which exists on all platforms)

       \returns Command to execute for elevation of input command.
    */
    wstring SystemInfo::GetShellCommand(const wstring& command) const
    {
        // Figure out the proper shell to use (default to current shell)
        //
        // Note that command must be executed with single quotes.  For example:
        //      /bin/sh -c 'command-to-execute'
        //
        // This is so that, if command has $ substitution, it happens in the
        // context of the elevated command, and not in the context of the command
        // to be elevated.

        const char* shell = m_deps->getenv("SHELL");
        wstring shellCommand;

        if (NULL == shell || '\0' == *shell)
        {
            shellCommand = L"/bin/sh -c \"";
        }
        else
        {
            shellCommand = StrFromUTF8(shell);
            shellCommand.append(L" -c \"");
        }
        shellCommand.append(command);
        shellCommand.append(L"\"");

        return shellCommand;
    }

    /**
       Returns a command to cause elevation, if elevation is needed.
       If no elevation is needed, original command is returned.

       \returns Command to execute for elevation of input command.
    */
    wstring SystemInfo::GetElevatedCommand(const wstring& command) const
    {
        // If we're already privileged, no need to elevate
        if (0 == m_deps->geteuid())
        {
            return command;
        }

        // Determine (and return) the complete elevated command
        wstring elevatedCommand = GetDefaultSudoPath();
        elevatedCommand.append(L" ");
        elevatedCommand.append(command);

        return elevatedCommand;
    }

#if defined(aix)
    /**
       Determines if this host is within a WPAR (Workload Partition) on an
       AIX system.  Always FALSE on AIX 5.3 (WPARs not supported there).

       Note: For compatibility, the caller can just look at the output parameter
       without checking the return value of the function.

       \param[out]  TRUE of FALSE if in WPAR (always FALSE on AIX 5.3)
       \returns     TRUE if actually supported, FALSE otherwise
    */
    bool SystemInfo::GetAIX_IsInWPAR(bool& isInWPAR) const
    {
        isInWPAR = m_isInWPAR;
#if PF_MAJOR >= 6
        return true;
#else
        return false;
#endif // PF_MAJOR >= 6
    }
#endif // defined(aix)


#if defined(sun)
    /**
       Determines if we're in the global zone or not on a Solaris system.
       On 5.9 and earlier, we always return TRUE (zones not supported there).
       ON 5.10, we'll actually make the check and return the truth.

       Note: For compatibility, the caller can just look at the output parameter
       without checking the return value of the function.

       \param[out]  VER <= 5.9: TRUE; VER >= 5.10: True if in global zone, false otherwise
       \returns TRUE or FALSE if the system actually supports global zones
    */
    bool SystemInfo::GetSUN_IsInGlobalZone(bool& isInGlobalZone) const
    {
        isInGlobalZone = m_isInGlobalZone;
#if defined(_SUN_HAS_ZONE_SUPPORT_)
        return true;
#else
        return false;
#endif // defined(_SUN_HAS_ZONE_SUPPORT_)
    }
#endif // defined(sun)
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
