/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/

#include <string>
#include <vector>
#include <list>
#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/mib.h>
#include <sys/dlpi_ext.h>

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxassert.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxlog.h>
#include <scxsystemlib/networkinterface.h>


#undef DLPI_DEV

// The interface between scxdlpi and networkinterface
typedef struct {
    u_long ppa;                   //!< ppa number
    std::string name;             //!< this ppa's id_module_1 string, which we use as its name
    mib_ifEntry stats;            //!< the stats for the ppa
    unsigned int collisions;      //!< the number of collisions collected from the mib Dot3Stats struct
} DLPIStatsEntry;

typedef std::list<dl_hp_ppa_info_t> PPAInfoList;

class SCXdlpi
{

private:
    static int ms_bufsize; //!< High water mark used in the creation of the dynamic buffer

    SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceDependencies> m_deps; //!< dependency injection handle
    struct strbuf m_control; //!< used to interact with getmsg/putmsg

    SCXCoreLib::SCXLogHandle m_log; //!< log handle for error reporting

    // private helper functions below that exist to simplify algorithm of GetAllLANStats    
    int openDLPI(const char* devPath);
    int closeDLPI(int fd);
    int attach(int fd, int ppa);
    int bind(int fd);
    int getstats(int fd, mib_ifEntry & iStats, unsigned int & collisions);
    int unbind(int fd);
    int detach(int fd);
    int checkNamePPAInIoctl(std::vector<DLPIStatsEntry>::iterator j);
    PPAInfoList getPPAInfoList(int fd);
    int getMessage(int fd);
    int testControlPrimitive(int prim);
    int putControlMessage(int fd, int len, int pri);
    
public:
    
    /**
       Constructor for the SCXdlpi class.
       Initializes the dynamic buffer, and its struct strbuf
       Initializes log

       \param deps - the dependency injection object
     */
    SCXdlpi(SCXCoreLib::SCXHandle<SCXSystemLib::NetworkInterfaceDependencies> deps) : m_deps(deps)
    {
        // let m_control.buf be a dynamically allocated array that can be resized 
        // in getMessage if necessary
        m_control.maxlen = ms_bufsize * sizeof(u_long);
        m_control.len = 0;
        m_control.buf = (char *)calloc(ms_bufsize, sizeof(u_long));

        // initialize log
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.networkinterface");
    }

    /**
       Destructor for the SCXdlpi class.  Frees the allocated buffer.
       
     */
    ~SCXdlpi()
    {
        // free our dynamic buffer
        if (m_control.buf)
            free(m_control.buf);
    }

    std::vector<DLPIStatsEntry> GetAllLANStats();

    //! Description: This function issues the DLPI command passed through 
    //! parameters on an intended network interface.
    //! Reference: http://h10032.www1.hp.com/ctg/Manual/c02011471.pdf
    //! autonegotiation.
    //! (Ref: http://publib.boulder.ibm.com/infocenter/pseries/v5r3/index.jsp?topic=/com.ibm.aix.kernelext/doc/kernextc/ethernet_dd.htm
    //!      ( Kernel Extensions and Device Dupport Programming Concepts: Ethernet Device Drivers)
    //!            (Appendix A)
    //!
    //! \param Inputs: interface_name - the name of the network interface.
    //!                cmd_info - contains the DLPI command related info.
    //!
    //! \param Output:  cmd_info - returned values by the DLPI driver.
    //!
    //! \param Return: true if OK, false otherwise. 
    //!
    bool get_cur_link_speed(std::wstring& interface_name, 
                            dl_hp_get_drv_param_ioctl_t& cmd_info);
};

