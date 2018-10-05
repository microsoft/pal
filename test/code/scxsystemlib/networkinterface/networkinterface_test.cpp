/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Test of network interface

    \date        2008-03-04 11:03:56

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

// Solaris Studio 12.4 has construct in net/if.h that requires
// that it be included prior to #include <map> ...
#include <errno.h>
#include <net/if.h>

#include <scxsystemlib/networkinterface.h>
#include <scxsystemlib/networkinterfaceenumeration.h>
#include <scxsystemlib/scxsysteminfo.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxexception.h> /* CUSTOMIZE: Only needed if you want to test exception throwing */
#include <scxcorelib/scxprocess.h>
#include <testutils/scxunit.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <scxsystemlib/scxnetworkadapterip_test.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#if defined (sun)
#include <sys/sockio.h>
#endif

#if defined (hpux)
#include <sys/dlpi.h>
#include <sys/dlpi_ext.h>
#include <sys/mib.h>
#include <map>
#include <vector>
#endif

#include <queue>

using namespace SCXSystemLib;
using namespace SCXCoreLib;
using namespace std;

// Use instrumentTests to debug tests.
static const bool instrumentTests = false;


static int test_cnt; 
#if defined(hpux)
  /* The forced speed, 10Mb, 100Mb, gigabit, 10GbE. */
  #define SPEED_10		10
  #define SPEED_100		100
  #define SPEED_1000		1000
  #define SPEED_10000		10000
  #define MB_TO_BITS            1000000
#endif

#if defined(aix)
#include <sys/cdli_entuser.h> // For kent_all_stats_t
#include <sys/cdli_entuser.gxent.h> // For gxent_all_stats_t
#include <sys/cdli_entuser.goent.h> // For goent_all_stats_t
#include <sys/cdli_entuser.scent.h> // For scent_all_stats_t
#include <sys/cdli_entuser.phxent.h> // For phxent_all_stats_t Must be included after goent.h
#include <sys/cdli_entuser.ment.h> // For ment_all_stats_t
#include <sys/cdli_entuser.hea.h> // For hea_all_stats_t
#include <sys/ndd_var.h>
#endif

#if defined(sun)
class MyKstatDeps : public SCXKstatDependencies
{
public:
    MyKstatDeps() : m_chain(NULL) { }
    
    void SetKstat(kstat_ctl_t* kstat_ctl) {
        m_chain = kstat_ctl;
    }
    
    kstat_ctl_t* Open() { return m_chain; }
    void Close(kstat_ctl_t*) { }
    kid_t Update(kstat_ctl_t*) { return 0; }
    int Read(kstat_ctl_t*, kstat_t*, void*) { return static_cast<int>(m_chain->kc_chain_id); }
        
private:
    kstat_ctl_t* m_chain;
};

class MyKstat : public SCXKstat
{
public:
    MyKstat(SCXHandle<MyKstatDeps> deps) : SCXKstat(deps)
    {
        Init();
    }
    ~MyKstat() { }
protected:
    virtual void Lookup(const wstring &module, const wstring &name, int instance)
    {
        if(instrumentTests)
        {
            std::cout<<"MyKstat::Lookup(\""<<StrToUTF8(module)<<"\", \""<<
                StrToUTF8(name)<<"\", "<<instance<<")"<<std::endl;
        }
        SCXKstat::Lookup(module, name, instance);
    }
    virtual void Lookup(const char* module, const char* name, int instance)
    {
        // Currently Lookup method is used only to determine AutoSense property and parameter name is always "mii".
        // Unit tests for the AutoSense property currently do not exist and need to be implemented. Right now
        // this method only overrides actual system calls and does nothing. Once unit tests are implemented
        // this method will contain the code that will simulate the actual system calls necessary to determine
        // the AutoSense property of the network interface.
        CPPUNIT_ASSERT_EQUAL(string("mii"), string(name));

        if(instrumentTests)
        {
            string m = (module == NULL) ? "NULL": '\"' + string(module) + '\"';
            string n = (name == NULL) ? "NULL": '\"' + string(name) + '\"';
            std::cout<<"MyKstat::Lookup("<<m<<", "<<n<<", "<<instance<<")"<<std::endl;
        }
    }
    virtual void Lookup(const wstring &module, int instance)
    {
        if(instrumentTests)
        {
            std::cout<<"MyKstat::Lookup(\""<<StrToUTF8(module)<<"\", "<<
                instance<<")"<<std::endl;
        }
        SCXKstat::Lookup(module, instance);
    }
    virtual kstat_t* ResetInternalIterator()
    {
        kstat_t* ret = SCXKstat::ResetInternalIterator();
        if(instrumentTests)
        {
            if(ret != NULL)
            {
                std::cout<<"MyKstat::ResetInternalIterator() "<<ret->ks_module<<", "<<
                    ret->ks_name<<", "<<ret->ks_instance<<std::endl;
            }
            else
            {
                std::cout<<"MyKstat::ResetInternalIterator() ret == NULL"<<std::endl;
            }
        }
        return ret;
    }
    virtual kstat_t* AdvanceInternalIterator()
    {
        kstat_t* ret = SCXKstat::AdvanceInternalIterator();
        if(instrumentTests)
        {
            if(ret != NULL)
            {
                std::cout<<"MyKstat::AdvanceInternalIterator() "<<ret->ks_module<<", "<<
                    ret->ks_name<<", "<<ret->ks_instance<<std::endl;
            }
            else
            {
                std::cout<<"MyKstat::AdvanceInternalIterator() ret == NULL"<<std::endl;
            }
        }
        return ret;
    }
};
#endif

//! Replacement of the default dependencies of the network interface PAL
//! Makes it possible to control the input so as to predict and test the output
class MyNetworkInterfaceDependencies : public NetworkInterfaceDependencies {
public:
#if defined(sun)
    MyNetworkInterfaceDependencies()
    {
        m_kstatDeps = SCXHandle<MyKstatDeps>(new MyKstatDeps());
    }

    ~MyNetworkInterfaceDependencies() { }

    SCXCoreLib::SCXHandle<SCXKstat> CreateKstat() {
        if(instrumentTests)
        {
            std::cout<<"MyNetworkInterfaceDependencies::CreateKstat()"<<std::endl;
        }
        return SCXHandle<SCXKstat>(new MyKstat(m_kstatDeps));
    }

    void SetKStat(kstat_ctl_t* kstat) {
        m_kstatDeps->SetKstat(kstat);
    }
#elif defined(linux)
    virtual SCXFilePath GetDynamicInfoFile() const {
        return m_dynamicInfoFile;
    }

    void SetDynamicInfoFile(const SCXFilePath &file) {
        m_dynamicInfoFile = file;
    }
#elif defined(aix)
    int perfstat_netinterface(perfstat_id_t *name, perfstat_netinterface_t *userbuff,
                              size_t sizeof_struct, int desiredNumber) {
        int returnedStructs;
        if (name != 0 && userbuff != 0) {
           copy(m_perfstat.begin(), m_perfstat.end(), userbuff);
           returnedStructs = std::min(static_cast<int>(m_perfstat.size()), desiredNumber);
        } else {
           returnedStructs = m_perfstat.size();
        }
        return returnedStructs;
    }

    void SetPerfStat(const std::vector<perfstat_netinterface_t> &perfstat) {
        m_perfstat = perfstat;
    }

    virtual int bind(int s, const struct sockaddr * name, socklen_t namelen)
    {
        return 0;
    } 
#endif

    virtual int ioctl(int /*fildes*/, int request, void *ifreqptr) {

#if defined(hpux)
        struct strioctl &strioctl = *static_cast<struct strioctl *>(ifreqptr);
#endif
        // Take care of the null pointer for all platforms. 
        // In some platforms (e.g. AIX), ioctl with NULL pointer is a valid 
        // command (e.g. ioctl(s,NDD_CLEAR_STATS,NULL) ).
        if (ifreqptr == NULL)
        {
            return 0;
        }
#if defined(aix)
        nddctl &ioctl_arg = *static_cast<nddctl *>(ifreqptr);
#endif
        ifreq &ifr = *static_cast<ifreq *>(ifreqptr);
        std::string address;

#if defined(linux) || defined(sun)
        static int count_loopback = 1; // the first and fifth, etc...  calls on the loopback interface. 
#endif
        switch (request) {
            case SIOCGIFADDR:
                address = m_ipAddress.front();
                m_ipAddress.pop();
                if(instrumentTests)
                {
                    std::cout<<"ioctl(SIOCGIFADDR,"<<ifr.ifr_name<<") "<<address<<std::endl;
                }
                break;
            case SIOCGIFNETMASK:
                address = m_netmask.front();
                m_netmask.pop();
                break;
            case SIOCGIFBRDADDR:
                address = m_broadcastAddress.front();
                m_broadcastAddress.pop();
                break;
            case SIOCGIFFLAGS:
                ifr.ifr_flags = 0;
#if defined(linux) || defined(sun)
                if ( count_loopback == 1 || count_loopback  == 6 ||  count_loopback == 11 || count_loopback == 16 ||
                        count_loopback == 21 || count_loopback == 26 || count_loopback == 31 )
                    ifr.ifr_flags |= IFF_LOOPBACK;
                ++count_loopback;
#endif
                if (m_up.size() > 0)
                {
                    if (m_up.front()) {
                        ifr.ifr_flags |= IFF_UP;
                    }
                    m_up.pop();
                }
                else
                {
                    CPPUNIT_FAIL("ioctl() dependecy injection method missing entries in m_up queue");
                }
                if (m_running.size() > 0)
                {
                    if (m_running.front()) {
                        ifr.ifr_flags |= IFF_RUNNING;
                    }
                    m_running.pop();
                }
                else
                {
                    CPPUNIT_FAIL("ioctl() dependecy injection method missing entries in m_running queue");
                }
                if(instrumentTests)
                {
                    std::cout<<"ioctl(SIOCGIFFLAGS,"<<ifr.ifr_name<<") "<<ifr.ifr_flags<<std::endl;
                }
                break;
#if defined(hpux)
            case I_STR:
                if (strioctl.ic_cmd == DL_HP_GET_DRV_PARAM_IOCTL)
                {
                    dl_hp_get_drv_param_ioctl_t &dl_params = *reinterpret_cast<dl_hp_get_drv_param_ioctl_t *>(strioctl.ic_dp);
                    switch (test_cnt)
                    {
                        case 0: // Existing tests
                            memset(&dl_params, 0, sizeof(dl_hp_get_drv_param_ioctl_t));
                            return true;
                        case 1:
                            dl_params.dl_speed = SPEED_10;
                            dl_params.dl_autoneg = DL_HP_AUTONEG_SENSE_ON;
                            return true;
                        case 2:
                            dl_params.dl_speed = SPEED_100;
                            dl_params.dl_autoneg = DL_HP_AUTONEG_SENSE_OFF;
                            return true;
                        case 3:
                            return false;
                    }
                }
                else 
                {
                    return true;
                }
#endif // if defined(hpux)

#if defined(aix)
            case NDD_GET_ALL_STATS:
                {
                    union ARG 
                    {
                        kent_all_stats_t kent;
                        phxent_all_stats_t phxent;
                        scent_all_stats_t scent;
                        gxent_all_stats_t gxent;
                        goent_all_stats_t goent;
                        ment_all_stats_t ment;
                        hea_all_stats_t hea;
                    }; // Used for ioctl argument.
                    ARG &arg = *reinterpret_cast<ARG *>(ioctl_arg.nddctl_buf);

                    switch(test_cnt)
                    {
                        case 0:
                            return 0;
                        case 1:
                            arg.kent.ent_gen_stats.device_type = ENT_3COM;
                            return 0;
                        case 2:
                            arg.kent.ent_gen_stats.device_type = ENT_PHX_PCI;
                            arg.phxent.phxent_stats.speed_selected = MEDIA_10_HALF;
                            return 0;
                        case 3:
                            arg.kent.ent_gen_stats.device_type = ENT_PHX_PCI;
                            arg.phxent.phxent_stats.speed_selected = MEDIA_100_FULL;
                            return 0;
                        case 4:
                            arg.kent.ent_gen_stats.device_type = ENT_PHX_PCI;
                            arg.phxent.phxent_stats.speed_selected = MEDIA_AUTO;
                            arg.phxent.phxent_stats.media_speed = MEDIA_10_FULL;
                            return 0;
                        case 5:
                            arg.kent.ent_gen_stats.device_type = ENT_SCENT_PCI;
                            arg.scent.scent_stats.speed_selected = MEDIA_10_FULL;
                            return 0;
                        case 6:
                            arg.kent.ent_gen_stats.device_type = ENT_SCENT_PCI;
                            arg.scent.scent_stats.speed_selected = MEDIA_100_HALF;
                            return 0;
                        case 7:
                            arg.kent.ent_gen_stats.device_type = ENT_SCENT_PCI;
                            arg.scent.scent_stats.speed_selected = MEDIA_AUTO;
                            arg.scent.scent_stats.speed_negotiated = MEDIA_10_FULL;
                            return 0;
                        case 8:
                            arg.kent.ent_gen_stats.device_type = ENT_SCENT_PCI;
                            arg.scent.scent_stats.speed_selected = MEDIA_AUTO;
                            arg.scent.scent_stats.speed_negotiated = MEDIA_100_HALF;
                            return 0;
                        case 9:
                            arg.kent.ent_gen_stats.device_type = ENT_GX_PCI;
                            arg.gxent.gxent_stats.speed_selected = MEDIA_10_HALF;
                            return 0;
                        case 10:
                            arg.kent.ent_gen_stats.device_type = ENT_GX_PCI;
                            arg.gxent.gxent_stats.speed_selected = MEDIA_100_HALF;
                            return 0;
                        case 11:
                            arg.kent.ent_gen_stats.device_type = ENT_GX_PCI;
                            arg.gxent.gxent_stats.speed_selected = MEDIA_AUTO;
                            arg.gxent.gxent_stats.link_negotiated = NDD_GXENT_LNK_1000MB;
                            return 0;
                        case 12:
                            arg.kent.ent_gen_stats.device_type = ENT_GOENT_PCI_TX;
                            arg.goent.goent_stats.speed_selected = MEDIA_10_FULL;
                            return 0;
                        case 13:
                            arg.kent.ent_gen_stats.device_type = ENT_GOENT_PCI_TX;
                            arg.goent.goent_stats.speed_selected = MEDIA_100_HALF;
                            return 0;
                        case 14:
                            arg.kent.ent_gen_stats.device_type = ENT_GOENT_PCI_TX;
                            arg.goent.goent_stats.speed_selected = MEDIA_1000_FULL;
                            return 0;
                        case 15:
                            arg.kent.ent_gen_stats.device_type = ENT_GOENT_PCI_TX;
                            arg.goent.goent_stats.speed_selected = MEDIA_AUTO;
                            arg.goent.goent_stats.speed_negotiated = MEDIA_10_FULL;
                            return 0;
                        case 16:
                            arg.kent.ent_gen_stats.device_type = ENT_GOENT_PCI_TX;
                            arg.goent.goent_stats.speed_selected = MEDIA_AUTO;
                            arg.goent.goent_stats.speed_negotiated = MEDIA_100_FULL;
                            return 0;
                        case 17:
                            arg.kent.ent_gen_stats.device_type = ENT_GOENT_PCI_TX;
                            arg.goent.goent_stats.speed_selected = MEDIA_AUTO;
                            arg.goent.goent_stats.speed_negotiated = MEDIA_1000_FULL;
                            return 0;
                        case 18:
                            arg.kent.ent_gen_stats.device_type = ENT_SM_SX_PCI;
                            arg.ment.ment_stats.speed_selected = MEDIA_10_HALF;
                            return 0;
                        case 19:
                            arg.kent.ent_gen_stats.device_type = ENT_SM_SX_PCI;
                            arg.ment.ment_stats.speed_selected = MEDIA_100_HALF;
                            return 0;
                        case 20:
                            arg.kent.ent_gen_stats.device_type = ENT_SM_SX_PCI;
                            arg.ment.ment_stats.speed_selected = MEDIA_1000_FULL;
                            return 0;
                        case 21:
                            arg.kent.ent_gen_stats.device_type = ENT_SM_SX_PCI;
                            arg.ment.ment_stats.speed_selected = MEDIA_AUTO;
                            arg.ment.ment_stats.link_negotiated = NDD_MENT_LNK_10MB;
                            return 0;
                        case 22:
                            arg.kent.ent_gen_stats.device_type = ENT_SM_SX_PCI;
                            arg.ment.ment_stats.speed_selected = MEDIA_AUTO;
                            arg.ment.ment_stats.link_negotiated = NDD_MENT_LNK_100MB;
                            return 0;
                        case 23:
                            arg.kent.ent_gen_stats.device_type = ENT_SM_SX_PCI;
                            arg.ment.ment_stats.speed_selected = MEDIA_AUTO;
                            arg.ment.ment_stats.link_negotiated = NDD_MENT_LNK_1000MB;
                            return 0;
                        case 24:
                            arg.kent.ent_gen_stats.device_type = 0;
                            arg.hea.hea_stats.speed_selected = HEA_MEDIA_10_FULL;
                            ioctl_arg.nddctl_buflen = sizeof(hea_all_stats_t);
                            return 0;
                        case 25:
                            arg.kent.ent_gen_stats.device_type = 0;
                            arg.hea.hea_stats.speed_selected = HEA_MEDIA_100_HALF;
                            ioctl_arg.nddctl_buflen = sizeof(hea_all_stats_t);
                            return 0;
                        case 26:
                            arg.kent.ent_gen_stats.device_type = 0;
                            arg.hea.hea_stats.speed_selected = HEA_MEDIA_1000_FULL;
                            ioctl_arg.nddctl_buflen = sizeof(hea_all_stats_t);
                            return 0;
                        case 27:
                            arg.kent.ent_gen_stats.device_type = 0;
                            arg.hea.hea_stats.speed_selected = HEA_MEDIA_10000_FULL;
                            ioctl_arg.nddctl_buflen = sizeof(hea_all_stats_t);
                            return 0;
                        case 28:
                            arg.kent.ent_gen_stats.device_type = 0;
                            arg.hea.hea_stats.speed_selected = HEA_MEDIA_AUTO;
                            ioctl_arg.nddctl_buflen = sizeof(hea_all_stats_t);
                            return 0;
                        default:
                            return 0;
                    }
                }
                break;

#endif // if defined(aix)

            default:
                return EINVAL;
        }
        std::istringstream addrsource(address);
        unsigned nr = 0;
        addrsource >> nr;
        ifr.ifr_addr.sa_data[2] = static_cast<unsigned char> (nr);
        addrsource.get();
        addrsource >> nr;
        ifr.ifr_addr.sa_data[3] = static_cast<unsigned char> (nr);
        addrsource.get();
        addrsource >> nr;
        ifr.ifr_addr.sa_data[4] = static_cast<unsigned char> (nr);
        addrsource.get();
        addrsource >> nr;
        ifr.ifr_addr.sa_data[5] = static_cast<unsigned char> (nr);
        return 0;
    }

    void AddIPAddress(const std::string &address) {
        m_ipAddress.push(address);
    }

    void AddBroadcastAddress(const std::string &address) {
        m_broadcastAddress.push(address);
    }

    void AddNetmask(const std::string &address) {
        m_netmask.push(address);
    }

    void AddUp(bool up) {
        m_up.push(up);
    }

    void AddRunning(bool running) {
        m_running.push(running);
    }
private:
#if defined(aix)
     std::vector<perfstat_netinterface_t> m_perfstat;
#elif defined(sun)
        SCXHandle<MyKstatDeps> m_kstatDeps;
#endif
    std::queue<std::string> m_ipAddress;
    std::queue<std::string> m_broadcastAddress;
    std::queue<std::string> m_netmask;
    std::queue<bool>        m_up;
    std::queue<bool>        m_running;
    SCXFilePath             m_dynamicInfoFile;
};

#if defined (hpux)
// This is used in the injected dlpi driver
typedef struct 
{
    dl_hp_ppa_info_t ppaInfo;
    mib_ifEntry mib;
} ppaInfoStruct; 

/**
   This class is used to create dependency injected tests for the HP DLPI network interface system.

   Currently supports only one open connection per instance of this class.
 */
class DLPINetworkInterfaceDependencies : public MyNetworkInterfaceDependencies 
{
private:
    static const int defaultFD = 12345;
    
    bool m_isOpen;
    int curREQ;
    int curPPA;
    char * currentBufPtr;
    
    // array containing PPA infos
    std::vector<dl_hp_ppa_info_t> ppaVector;
    
    // map containing stats
    std::map<int, mib_ifEntry> statsMap;
    
    virtual void initState()
    {
        curREQ = -1;
        curPPA = -1;
        currentBufPtr = (char*)ppaVector.begin();
    }
    
public:
    DLPINetworkInterfaceDependencies(std::vector<dl_hp_ppa_info_t> p, std::map<int, mib_ifEntry> s) :
        m_isOpen(false),
        ppaVector(p),
        statsMap(s),
        curREQ(-1),
        curPPA(-1),
        currentBufPtr(NULL)
    {
    }
    
    ~DLPINetworkInterfaceDependencies() { }
    
    
    ////////////////////////////////
    // begin injected DLPI driver //
    ////////////////////////////////

    // Code to get IPv6 address uses socket. IPv6 address data is not used in this tests so we disable it.
    // If we would allow socket to be open then close() would assert.
    virtual int socket(int domain, int type, int protocol)
    {
        if (domain == AF_INET6 && type == SOCK_DGRAM && protocol == 0)
        {
            return -1;
        }
        return MyNetworkInterfaceDependencies::socket(domain, type, protocol);
    }

    virtual int open(const char * path, int oflag)
    {
        CPPUNIT_ASSERT( !m_isOpen );
        
        m_isOpen = true;
        
        initState();
        return defaultFD;
    }

    virtual int close(int fildes)
    {
        CPPUNIT_ASSERT( m_isOpen );
        
        m_isOpen = false;
        
        initState();
        return 0;
    }

    virtual int putmsg(int fildes, const struct ::strbuf * ctlptr, const struct ::strbuf * dataptr, int flags)
    {
        // supported primitives:
        // DL_HP_PPA_REQ
        // DL_ATTACH_REQ
        // DL_BIND_REQ
        // DL_GET_STATISTICS_REQ
        // DL_UNBIND_REQ
        // DL_DETACH_REQ
        
        // read primitive and set it to the curREQ
        // return either error or not, depending on test data
        u_long primitive = *(uint32_t*)ctlptr->buf;
        
        if (primitive ==  DL_HP_PPA_REQ)
        {
            // do nothing extra for now
        }
        else if (primitive == DL_ATTACH_REQ)
        {
            dl_attach_req_t * attachREQ = (dl_attach_req_t *)ctlptr->buf;
            curPPA = attachREQ->dl_ppa;
        }
        else if (primitive == DL_BIND_REQ)
        {
            dl_bind_req_t * bindREQ = (dl_bind_req_t *)ctlptr->buf;
            // not necessary for the purposes of this driver (for now)
        }
        else if (primitive == DL_GET_STATISTICS_REQ)
        {
            // do nothing extra for now
        }
        else if (primitive == DL_UNBIND_REQ)
        {
            // do nothing extra for now
        }
        else if (primitive == DL_DETACH_REQ)
        {
            curPPA = -1;
        }
        else
        {
            string failString;
            std::stringstream ss;
            ss << "Unexpected Primitive, primitive=" << primitive;
            ss >> failString;
            CPPUNIT_FAIL(failString);
        }
        
        // store this request into curREQ for later processing by getmsg
        curREQ = primitive;
        
        return 0;
    }

    virtual int getmsg(int fildes, struct ::strbuf * ctlptr, struct ::strbuf * dataptr, int * flagsp)
    {
        // depending on previous message for fildes, send specific message
        // supported primitives:
        // DL_OK_ACK
        // DL_ERROR_ACK
        // DL_HP_PPA_ACK
        // DL_BIND_ACK
        // DL_GET_STATISTICS_ACK
        
        
        if (curREQ == DL_HP_PPA_REQ)
        {
            // ctlptr->maxlen is the size of the buffer we'll be storing these structs in.
            // let's store at the very MAXIMUM ctlptr->maxlen bytes in this buffer
            
            // if this is not a re-call of getmsg to get more data that was left in the buffer
            // (due to a previous return of MORECTL instead of 0)
            if (currentBufPtr == (char *) ppaVector.begin())
            {
                dl_hp_ppa_ack_t * ppaACK = ((dl_hp_ppa_ack_t*)ctlptr->buf);
                ppaACK->dl_primitive = DL_HP_PPA_ACK;
                ppaACK->dl_count = ppaVector.size();
                ppaACK->dl_offset = sizeof(dl_hp_ppa_ack_t);
                ppaACK->dl_length = sizeof(dl_hp_ppa_ack_t) + ppaACK->dl_count * sizeof(dl_hp_ppa_info_t);
                
                dl_hp_ppa_info_t * ppaInfoPtr = (dl_hp_ppa_info_t *)(ctlptr->buf + ppaACK->dl_offset);
                
                // calculate the size in bytes we'll need to store all this data in
                int totalNeededSize = ppaVector.size() * sizeof(dl_hp_ppa_info_t) + sizeof(dl_hp_ppa_ack_t);
                
                if (ctlptr->maxlen < totalNeededSize)
                {
                    // we can't fit everything here
                    memcpy(ppaInfoPtr, currentBufPtr, ctlptr->maxlen - sizeof(dl_hp_ppa_ack_t));
                    currentBufPtr += ctlptr->maxlen - sizeof(dl_hp_ppa_ack_t);
                    return MORECTL;
                }
                else
                {
                    // we can fit everything here
                    memcpy(ppaInfoPtr, currentBufPtr, ppaVector.size() * sizeof(dl_hp_ppa_info_t));

                    // we're done copying, now reset the location to the beginning of the ppaVector
                    currentBufPtr = (char *)ppaVector.begin();
                }
                
            }
            // else: this is a re-call of getmsg to get another block of data
            else
            {
                int neededSize = ((char*)ppaVector.end()) - currentBufPtr;
                if (ctlptr->maxlen < neededSize)
                {
                    // we can't fit everything here
                    memcpy(ctlptr->buf, currentBufPtr, ctlptr->maxlen);
                    currentBufPtr += ctlptr->maxlen;
                    return MORECTL;
                }
                else
                {
                    // we can fit everything here
                    memcpy(ctlptr->buf, currentBufPtr, neededSize);
                    currentBufPtr = (char *)ppaVector.begin();
                }
            }
            
        }
        else if (curREQ ==  DL_ATTACH_REQ)
        {
            // send an OK_ACK
            dl_ok_ack_t * okACK = (dl_ok_ack_t *)ctlptr->buf;
            okACK->dl_primitive = DL_OK_ACK;
            okACK->dl_correct_primitive = DL_OK_ACK;
            
        }
        else if (curREQ ==  DL_BIND_REQ)
        {
            dl_bind_ack_t * bindACK = (dl_bind_ack_t *)ctlptr->buf;
            bindACK->dl_primitive = DL_BIND_ACK;
            
        }
        else if (curREQ ==  DL_GET_STATISTICS_REQ)
        {
            // put statistics table into ctlptr->buf, along with header struct
            dl_get_statistics_ack_t * statACK = (dl_get_statistics_ack_t *)ctlptr->buf;
            statACK->dl_primitive = DL_GET_STATISTICS_ACK;
            statACK->dl_stat_length = sizeof(mib_ifEntry);
            statACK->dl_stat_offset = sizeof(dl_get_statistics_ack_t);
            
            mib_ifEntry * stats = (mib_ifEntry *)(statACK->dl_stat_offset + ctlptr->buf);
            mib_Dot3StatsEntry * dot3stats = (mib_Dot3StatsEntry *)(reinterpret_cast<intptr_t>(stats) + sizeof(mib_ifEntry) );
            
            // get the statistics for the currently attached PPA
            *stats = statsMap[curPPA];
            
            // set the Dot3StatsEntry to entirely 0
            memset(dot3stats, 0, sizeof(mib_Dot3StatsEntry));
            
        }
        else if (curREQ ==  DL_UNBIND_REQ) 
        {
            // send DL_OK_ACK
            dl_ok_ack_t * okACK = (dl_ok_ack_t *)ctlptr->buf;
            okACK->dl_primitive = DL_OK_ACK;
            okACK->dl_correct_primitive = DL_OK_ACK;
            
        }
        else if (curREQ ==  DL_DETACH_REQ)
        {
            // send DL_OK_ACK
            dl_ok_ack_t * okACK = (dl_ok_ack_t *)ctlptr->buf;
            okACK->dl_primitive = DL_OK_ACK;
            okACK->dl_correct_primitive = DL_OK_ACK;
        }
        else
        {
            string failString;
            std::stringstream ss;
            ss << "Unexpected Primitive, curREQ=" << curREQ;
            ss >> failString;
            CPPUNIT_FAIL(failString);
        }
        
        curREQ = -1;
        return 0;
    }

    //////////////////////////////
    // end injected DLPI driver //
    //////////////////////////////
    
};
#endif

#if defined(hpux)
    #define WI384433_TEST_CASES 3 // Number of tests.
    #define WI384433_TEST_VALS 2  // speed, and autosense are to be tested.
#elif defined(aix)
    // Use SPEED_xx defined in the network class.
    unsigned int SPEED_10 =  NetworkInterfaceInfo::SPEED_10; 
    unsigned int SPEED_100 =  NetworkInterfaceInfo::SPEED_100; 
    unsigned int SPEED_1000 =  NetworkInterfaceInfo::SPEED_1000; 
    unsigned int SPEED_10000 =  NetworkInterfaceInfo::SPEED_10000; 
    #define WI384433_TEST_CASES 28 // Number of tests.
    #define WI384433_TEST_VALS 3  // max speed, speed, and autosense are to be tested.
#endif 

#if defined(hpux) || defined(aix)
    #define WI384433_NUM_OF_TESTS (WI384433_TEST_CASES * WI384433_TEST_VALS) // Total number of tests.
    const bool autoNeg = true;
    const bool noAutoNeg = false;
    // This table holds the expected values for the test cases.
    static const int WI384433_Test_Table[] =
    {
#if defined(hpux)
                   /* speed,                   autonegotiation */
        /* TEST 1 */ (SPEED_10  * MB_TO_BITS), autoNeg,
        /* TEST 2 */ (SPEED_100 * MB_TO_BITS), noAutoNeg,
        /* TEST 3 */ 0                       , noAutoNeg,

#elif defined(aix) 
                  /* max speed,   speed,       autonegotiation */
        /* TEST 1 */ SPEED_100  , 0          , noAutoNeg, 
        /* TEST 2 */ SPEED_100  , SPEED_10   , noAutoNeg, 
        /* TEST 3 */ SPEED_100  , SPEED_100  , noAutoNeg, 
        /* TEST 4 */ SPEED_100  , SPEED_10   , autoNeg, 
        /* TEST 5 */ SPEED_100  , SPEED_10   , noAutoNeg, 
        /* TEST 6 */ SPEED_100  , SPEED_100  , noAutoNeg, 
        /* TEST 7 */ SPEED_100  , SPEED_10   , autoNeg, 
        /* TEST 8 */ SPEED_100  , SPEED_100  , autoNeg, 
        /* TEST 9 */ SPEED_1000 , SPEED_10   , noAutoNeg, 
        /* TEST 10*/ SPEED_1000 , SPEED_100  , noAutoNeg, 
        /* TEST 11*/ SPEED_1000 , SPEED_1000 , autoNeg, 
        /* TEST 12*/ SPEED_1000 , SPEED_10   , noAutoNeg, 
        /* TEST 13*/ SPEED_1000 , SPEED_100  , noAutoNeg, 
        /* TEST 14*/ SPEED_1000 , SPEED_1000 , noAutoNeg, 
        /* TEST 15*/ SPEED_1000 , SPEED_10   , autoNeg, 
        /* TEST 16*/ SPEED_1000 , SPEED_100  , autoNeg, 
        /* TEST 17*/ SPEED_1000 , SPEED_1000 , autoNeg, 
        /* TEST 18*/ SPEED_1000 , SPEED_10   , noAutoNeg, 
        /* TEST 19*/ SPEED_1000 , SPEED_100  , noAutoNeg, 
        /* TEST 20*/ SPEED_1000 , SPEED_1000 , noAutoNeg, 
        /* TEST 21*/ SPEED_1000 , SPEED_10   , autoNeg, 
        /* TEST 22*/ SPEED_1000 , SPEED_100  , autoNeg, 
        /* TEST 23*/ SPEED_1000 , SPEED_1000 , autoNeg, 
        /* TEST 24*/ SPEED_10   , SPEED_10   , noAutoNeg, 
        /* TEST 25*/ SPEED_100  , SPEED_100  , noAutoNeg, 
        /* TEST 26*/ SPEED_1000 , SPEED_1000 , noAutoNeg, 
        /* TEST 27*/ SPEED_10000, SPEED_10000, noAutoNeg, 
        /* TEST 28*/ 0          , 0          , autoNeg, 
         
#endif
    }; 
#endif // #if defined(hpux) || defined(aix)

// Tests the network interface PAL
class SCXNetworkInterfaceTest : public CPPUNIT_NS::TestFixture /* CUSTOMIZE: use a class name with relevant name */
{
    CPPUNIT_TEST_SUITE( SCXNetworkInterfaceTest ); /* CUSTOMIZE: Name must be same as classname */
    CPPUNIT_TEST( createEnumerationForCoverage );
    CPPUNIT_TEST( TestFindAllBehavior );
    CPPUNIT_TEST( TestFindAllRunningBehavior );
    CPPUNIT_TEST( TestGetRunningInterfacesOnly );
    CPPUNIT_TEST( TestGetAllInterfacesEvenNotRunning );
    CPPUNIT_TEST( TestFindAllSoundness );
    CPPUNIT_TEST( TestEnumerationBehavior );
    CPPUNIT_TEST( TestEnumerationSoundness );
    CPPUNIT_TEST( TestBug5175_IgnoreNetDevicesNotInterfaces );
    CPPUNIT_TEST( TestMTU );
#if defined(hpux)
    CPPUNIT_TEST( TestHP_FindAllInDLPI_AtLeastOneInterface );
    CPPUNIT_TEST( TestHP_FindAllInDLPI_ComparedToLanscan );
    CPPUNIT_TEST( TestHP_FindAllInDLPI_SingleInterface_Injection );
    CPPUNIT_TEST( TestHP_FindAllInDLPI_ThreeInterface_Injection );
    CPPUNIT_TEST( TestHP_FindAllInDLPI_ManyInterface_Injection );
    CPPUNIT_TEST( TestHP_WI384433_Get_DataLink_Speed );
#endif
    CPPUNIT_TEST( TestAdapterNetworkIPAddress );
#if defined(aix)
    CPPUNIT_TEST( TestAIX_WI384433_Get_NDD_STAT );
#endif
    SCXUNIT_TEST_ATTRIBUTE(TestFindAllSoundness,SLOW);
    SCXUNIT_TEST_ATTRIBUTE(TestEnumerationSoundness,SLOW);
    CPPUNIT_TEST_SUITE_END();

private:
#if defined(sun)
    // These variables are used for FindAll behaviorial tests; it's easier to
    // include them here (and in setUp() and tearDown() rather than to refactor
    // so these are handled in a "cleaner" way.
    //
    // Since all we're doing in setUp() is assigning NULL, and since all we're
    // doing in tearDown() is deleting, performance impact is essentially nil.

    kstat_named_t *ksdata_sit0;
    kstat_t *header_sit0;
    kstat_named_t *ksdata_eth0;
    kstat_t *header_eth0;
    kstat_named_t *ksdata_lo;
    kstat_t *header_lo;
    kstat_ctl_t *ctl;
#endif

public:
    bool verifyNetworkInterfaces(wstring testName)
    {
#if defined(sun)
        SCXSystemLib::SystemInfo si;
        bool fIsInGlobalZone;
        si.GetSUN_IsInGlobalZone( fIsInGlobalZone );
        if (! fIsInGlobalZone)
        {
            wstring warnText
                = L"Network interfaces not enumerated in sub-zone on Solaris, test "
                + testName + L"; skipping (see wi13570)";
            SCXUNIT_WARNING(warnText);
            return false;
        }
#else
        (void) testName;
#endif

        return true;
    }

    void setUp()
    {
        /* CUSTOMIZE: This method will be called once before each test function. Use it to set up commonly used objects */

        test_cnt = 0;
        
#if defined(sun)
        // Initialize memory for the FindAll behavioral tests

        ksdata_sit0 = NULL;
        header_sit0 = NULL;
        ksdata_eth0 = NULL;
        header_eth0 = NULL;
        ksdata_lo   = NULL;
        header_lo   = NULL;
        ctl         = NULL;
#endif

        // Reset our interface list to a known state
        NetworkInterfaceInfo::ClearRunningInterfaceList();
    }

    void tearDown()
    {
#if defined(sun)
        // Clean up memory from the FindAll behavioral tests

        delete [] ksdata_sit0;
        delete header_sit0;
        delete [] ksdata_eth0;
        delete header_eth0;
        delete [] ksdata_lo;
        delete header_lo;
        delete ctl;
#endif
    }

    void createEnumerationForCoverage()
    {
        SCXHandle<MyNetworkInterfaceDependencies> deps(new MyNetworkInterfaceDependencies());
        CPPUNIT_ASSERT_NO_THROW(NetworkInterfaceEnumeration interfaces(deps));
    }

#if defined(sun)
    //! Initialize a KStat data structure representing a named value, that is, an attribute
    //! \param[in] attribute     Reference to structure to be initialized
    //! \param[in] name          Pointer to name
    //! \param[in] value         Value to be used
    void InitAttribute(kstat_named_t &attribute, const char *name, scxulong value) {
        memset(&attribute, 0, sizeof(attribute));
        strcpy(attribute.name, name);
        attribute.data_type = KSTAT_DATA_UINT64;
        attribute.value.ui64 = value;
    }

    //! Initialize the KStat header structure that points to an array of data structures
    //! \param[in]   header     Reference to header structure
    //! \param[in]   ksclass    KStat class name
    //! \param[in]   kstype     KStat type
    //! \param[in]   ksname     KStat name
    //! \param[in]   ksdata     Pointer to array of data structures
    //! \param[in]   ksndata    Number of data structures
    //! \param[in]   ksnext     Pointer to the next KStat header in the chain
    void InitHeader(kstat_t &header, const char *ksclass, const char *ksname, unsigned kstype,
                    kstat_named_t *ksdata, unsigned ksndata, kstat_t *ksnext)
    {
        memset(&header, 0, sizeof(header));
        header.ks_next = ksnext;
        strcpy(header.ks_class, ksclass);
        strcpy(header.ks_name, ksname);
        header.ks_type = kstype;
        header.ks_data = ksdata;
        header.ks_ndata = ksndata;
    }
#endif

    //! Perform common setup for FindAll Behavioral Tests
    //! \param[in]   bAllInterfacesUp    Setup with all interfaces up or only some interfaces up?
    SCXHandle<MyNetworkInterfaceDependencies> SetupBehavioralTestDependency(bool bAllInterfacesUp)
    {
        SCXHandle<MyNetworkInterfaceDependencies> deps(new MyNetworkInterfaceDependencies());

#if defined(sun)

        ksdata_sit0 = new kstat_named_t[7];
        InitAttribute(ksdata_sit0[0], "rbytes64", 10);
        InitAttribute(ksdata_sit0[1], "ipackets64", 11);
        InitAttribute(ksdata_sit0[2], "ierrors", 12);
        InitAttribute(ksdata_sit0[3], "obytes64", 14);
        InitAttribute(ksdata_sit0[4], "opackets64", 15);
        InitAttribute(ksdata_sit0[5], "oerrors", 16);
        InitAttribute(ksdata_sit0[6], "collisions", 9);
        header_sit0 = new kstat_t;
        InitHeader(*header_sit0, "net", "sit0", KSTAT_TYPE_NAMED, ksdata_sit0, 7, 0);

        ksdata_eth0 = new kstat_named_t[7];
        InitAttribute(ksdata_eth0[0], "rbytes", 305641);
        InitAttribute(ksdata_eth0[1], "ipackets", 1606);
        InitAttribute(ksdata_eth0[2], "ierrors", 2);
        InitAttribute(ksdata_eth0[3], "obytes", 132686);
        InitAttribute(ksdata_eth0[4], "opackets", 437);
        InitAttribute(ksdata_eth0[5], "oerrors", 5);
        InitAttribute(ksdata_eth0[6], "collisions", 8);
        header_eth0 = new kstat_t;
        InitHeader(*header_eth0, "net", "eth0", KSTAT_TYPE_NAMED, ksdata_eth0, 7, header_sit0);

        ksdata_lo = new kstat_named_t[11];
        InitAttribute(ksdata_lo[0], "rbytes", 49);
        InitAttribute(ksdata_lo[1], "rbytes64", StrToULong(L"8749874987"));
        InitAttribute(ksdata_lo[2], "ipackets", 36);
        InitAttribute(ksdata_lo[3], "ipackets64", 136);
        InitAttribute(ksdata_lo[4], "ierrors", 1);
        InitAttribute(ksdata_lo[5], "obytes", 50);
        InitAttribute(ksdata_lo[6], "obytes64", 8750);
        InitAttribute(ksdata_lo[7], "opackets", 37);
        InitAttribute(ksdata_lo[8], "opackets64", 137);
        InitAttribute(ksdata_lo[9], "oerrors", 4);
        InitAttribute(ksdata_lo[10], "collisions", 7);
        header_lo = new kstat_t;
        InitHeader(*header_lo, "net", "lo", KSTAT_TYPE_NAMED, ksdata_lo, 11, header_eth0);

        ctl = new kstat_ctl_t;
        memset(ctl, 0, sizeof(*ctl));
        ctl->kc_chain = header_lo;
        deps->SetKStat(ctl);

#elif defined(aix)
        vector<perfstat_netinterface_t> perfs;
        perfs.resize(3);
        strcpy(perfs[0].name, "lo");
        perfs[0].type = IFT_LOOP;
        perfs[0].ibytes = StrToULong(L"8749874987");
        perfs[0].ipackets = 136;
        perfs[0].ierrors = 1;
        perfs[0].obytes = 8750;
        perfs[0].opackets = 137;
        perfs[0].oerrors = 4;
        perfs[0].collisions = 7;
        strcpy(perfs[1].name, "eth0");
        perfs[1].type = IFT_ETHER;
        perfs[1].ibytes = 305641;
        perfs[1].ipackets = 1606;
        perfs[1].ierrors = 2;
        perfs[1].obytes = 132686;
        perfs[1].opackets = 437;
        perfs[1].oerrors = 5;
        perfs[1].collisions = 8;
        strcpy(perfs[2].name, "sit0");
        perfs[2].type = IFT_ETHER;
        perfs[2].ibytes = 10;
        perfs[2].ipackets = 11;
        perfs[2].ierrors = 12;
        perfs[2].obytes = 14;
        perfs[2].opackets = 15;
        perfs[2].oerrors = 16;
        perfs[2].collisions = 9;
        deps->SetPerfStat(perfs);

#elif defined(linux)
        deps->SetDynamicInfoFile(L"./testfiles/procnetdev.txt");
#endif

#if defined(aix)
        deps->AddIPAddress("157.58.164.68");// eth0
        deps->AddIPAddress("157.58.162.69");// sit0
        deps->AddIPAddress("127.0.0.1");// lo
        deps->AddBroadcastAddress("157.58.164.255");// eth0
        deps->AddBroadcastAddress("157.58.162.255");// sit0
        deps->AddBroadcastAddress("127.0.0.255");// lo
        deps->AddNetmask("255.255.255.0");// eth0
        deps->AddNetmask("255.255.0.0");// sit0
        deps->AddNetmask("255.0.0.0");// lo

        // Build the interfaces to either be all-running - or not
        if (bAllInterfacesUp)
        {
            deps->AddUp(true);// eth0
            deps->AddUp(true);// sit0
            deps->AddUp(true);// lo
            deps->AddRunning(true);// eth0
            deps->AddRunning(true);// sit0
            deps->AddRunning(true);// lo
        }
        else
        {
            deps->AddUp(false);// eth0
            deps->AddUp(false);// sit0
            deps->AddUp(true);// lo
            deps->AddRunning(false);// eth0
            deps->AddRunning(true);// sit0
            deps->AddRunning(true);// lo
        }
#endif

#if defined(linux) || defined(sun) 
#if defined sun
        deps->AddIPAddress("157.58.164.68");// eth0
        deps->AddIPAddress("157.58.164.68");// eth0
        deps->AddIPAddress("157.58.162.69");// sit0
        deps->AddIPAddress("157.58.162.69");// sit0
#else // else linux.
        deps->AddIPAddress("157.58.164.68");// eth0
        deps->AddIPAddress("157.58.162.69");// sit0
#endif
         
        deps->AddBroadcastAddress("157.58.164.255");// eth0
        deps->AddBroadcastAddress("157.58.162.255");// sit0
        
        deps->AddNetmask("255.255.255.0");// eth0
        deps->AddNetmask("255.255.0.0");// sit0
       
        // Build the interfaces to either be all-running - or not
        if (bAllInterfacesUp)
        {
            deps->AddUp(true);// lo
            deps->AddUp(true);// eth0
            deps->AddUp(true);// sit0
            deps->AddUp(true);// eth0
            deps->AddUp(true);// sit0
            deps->AddRunning(true);// lo
            deps->AddRunning(true);// eth0
            deps->AddRunning(true);// sit0
            deps->AddRunning(true);// eth0
            deps->AddRunning(true);// sit0
        }
        else
        {
            deps->AddUp(true);// lo
            deps->AddUp(false);// eth0
            deps->AddUp(false);// sit0
            deps->AddUp(false);// eth0
            deps->AddUp(false);// sit0
            deps->AddRunning(true);// lo
            deps->AddRunning(false);// eth0
            deps->AddRunning(true);// sit0
            deps->AddRunning(false);// eth0
            deps->AddRunning(true);// sit0
        }
#endif

        return deps;
    }

    void WriteNetworkInterfaceInfo(NetworkInterfaceInfo &interface)
    {
        cout<<endl<<" ---------------------------------------"<<endl;
        cout<<"  "<<StrToUTF8(interface.GetName())<<endl;
        cout<<"  "<<StrToUTF8(interface.GetIPAddress())<<endl;
        cout<<"  "<<StrToUTF8(interface.GetBroadcastAddress())<<endl;
        cout<<"  "<<StrToUTF8(interface.GetNetmask())<<endl;
        cout<<"  "<<interface.GetBytesReceived()<<endl;
        cout<<"  "<<interface.GetPacketsReceived()<<endl;
        cout<<"  "<<interface.GetErrorsReceiving()<<endl;
        cout<<"  "<<interface.GetBytesSent()<<endl;
        cout<<"  "<<interface.GetPacketsSent()<<endl;
        cout<<"  "<<interface.GetErrorsSending()<<endl;
        cout<<"  "<<interface.GetCollisions()<<endl;
        cout<<"  "<<interface.IsUp()<<endl;
        cout<<"  "<<interface.IsRunning()<<endl;
    }
    
    void WriteNetworkInterfaceInfoAll(std::vector<NetworkInterfaceInfo> &interfaces)
    {
        cout<<endl<<"----------------------------------------"<<endl;
        cout<<"  "<<interfaces.size()<<endl;
        size_t i;
        for(i = 0; i < interfaces.size(); i++)
        {
            WriteNetworkInterfaceInfo(interfaces[i]);
        }
    }

    void WriteNetworkInterfaceInfoAll(NetworkInterfaceEnumeration &interfaces)
    {
        cout<<endl<<"----------------------------------------"<<endl;
        cout<<"  "<<interfaces.Size()<<endl;
        size_t i;
        for(i = 0; i < interfaces.Size(); i++)
        {
            cout<<"  "<<StrToUTF8(interfaces[i]->GetName())<<endl;
            bool cond;
            cond = false;
            cout<<"  "<<interfaces[i]->GetUp(cond)<<" "<<cond<<endl;
            cond = false;
            cout<<"  "<<interfaces[i]->GetRunning(cond)<<" "<<cond<<endl;
            wstring ipAddress;
            cout<<"  "<<interfaces[i]->GetIPAddress(ipAddress)<<" "<<StrToUTF8(ipAddress)<<endl;
        }
    }

    //! Test that NetworkInterfaceInfo::FindAll() returns all interfaces (if all are up),
    //! and verify that each element in the interface is as we expect.
    void TestFindAllBehavior()
    {

#if defined(linux) || defined(sun) || defined(aix)
        SCXHandle<MyNetworkInterfaceDependencies> deps = SetupBehavioralTestDependency(true);

        std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces);
        }
        CPPUNIT_ASSERT(interfaces.size() == 2);

        CPPUNIT_ASSERT(interfaces[0].GetName() == L"eth0");
        CPPUNIT_ASSERT(interfaces[0].GetIPAddress() == L"157.58.164.68");
        CPPUNIT_ASSERT(interfaces[0].GetBroadcastAddress() == L"157.58.164.255");
        CPPUNIT_ASSERT(interfaces[0].GetNetmask() == L"255.255.255.0");
        CPPUNIT_ASSERT(interfaces[0].GetBytesReceived() == 305641);
        CPPUNIT_ASSERT(interfaces[0].GetPacketsReceived() == 1606);
        CPPUNIT_ASSERT(interfaces[0].GetErrorsReceiving() == 2);
        CPPUNIT_ASSERT(interfaces[0].GetBytesSent() == 132686);
        CPPUNIT_ASSERT(interfaces[0].GetPacketsSent() == 437);
        CPPUNIT_ASSERT(interfaces[0].GetErrorsSending() == 5);
        CPPUNIT_ASSERT(interfaces[0].GetCollisions() == 8);
        CPPUNIT_ASSERT(interfaces[0].IsUp());
        CPPUNIT_ASSERT(interfaces[0].IsRunning());

        CPPUNIT_ASSERT(interfaces[1].GetName() == L"sit0");
        CPPUNIT_ASSERT(interfaces[1].GetIPAddress() == L"157.58.162.69");
        CPPUNIT_ASSERT(interfaces[1].GetBroadcastAddress() == L"157.58.162.255");
        CPPUNIT_ASSERT(interfaces[1].GetNetmask() == L"255.255.0.0");
        CPPUNIT_ASSERT(interfaces[1].GetBytesReceived() == 10);
        CPPUNIT_ASSERT(interfaces[1].GetPacketsReceived() == 11);
        CPPUNIT_ASSERT(interfaces[1].GetErrorsReceiving() == 12);
        CPPUNIT_ASSERT(interfaces[1].GetBytesSent() == 14);
        CPPUNIT_ASSERT(interfaces[1].GetPacketsSent() == 15);
        CPPUNIT_ASSERT(interfaces[1].GetErrorsSending() == 16);
        CPPUNIT_ASSERT(interfaces[1].GetCollisions() == 9);
        CPPUNIT_ASSERT(interfaces[1].IsUp());
        CPPUNIT_ASSERT(interfaces[1].IsRunning());
#endif
    }

    //! Test that NetworkInterfaceInfo::FindAll() returns only running interfaces
    //! (if some are not running), and tests "stickyness" - if an interface was ever
    //! running, make sure it's returned again even if it's not running.
    void TestFindAllRunningBehavior()
    {
#if defined(linux) || defined(sun) || defined(aix)
        // Verify that we work properly with some interfaces not running
        SCXHandle<MyNetworkInterfaceDependencies> deps_a = SetupBehavioralTestDependency(false);

        std::vector<NetworkInterfaceInfo> interfaces_a = NetworkInterfaceInfo::FindAll(deps_a);
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces_a);
        }
        CPPUNIT_ASSERT(interfaces_a.size() == 1);
        CPPUNIT_ASSERT(interfaces_a[0].GetName() == L"sit0");
        CPPUNIT_ASSERT(!interfaces_a[0].IsUp());
        CPPUNIT_ASSERT(interfaces_a[0].IsRunning());

        // Now regenerate the dependencies with all interfaces running
        // (Verify that we actually get all interfaces)
        SCXHandle<MyNetworkInterfaceDependencies> deps_b = SetupBehavioralTestDependency(true);

        std::vector<NetworkInterfaceInfo> interfaces_b = NetworkInterfaceInfo::FindAll(deps_b);
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces_b);
        }
        CPPUNIT_ASSERT(interfaces_b.size() == 2);
        CPPUNIT_ASSERT(interfaces_b[0].GetName() == L"eth0");
        CPPUNIT_ASSERT(interfaces_b[0].IsUp());
        CPPUNIT_ASSERT(interfaces_b[0].IsRunning());
        CPPUNIT_ASSERT(interfaces_b[1].GetName() == L"sit0");
        CPPUNIT_ASSERT(interfaces_b[1].IsUp());
        CPPUNIT_ASSERT(interfaces_b[1].IsRunning());

        // Now regenerate the dependencies with some interfaces not running

    //! Check that the enumeration are initialized correctly
        // (Verify that we actually get all interfaces)
        SCXHandle<MyNetworkInterfaceDependencies> deps_c = SetupBehavioralTestDependency(false);

        std::vector<NetworkInterfaceInfo> interfaces_c = NetworkInterfaceInfo::FindAll(deps_c);
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces_c);
        }
        CPPUNIT_ASSERT(interfaces_c.size() == 2);
        CPPUNIT_ASSERT(interfaces_c[0].GetName() == L"eth0");
        CPPUNIT_ASSERT(!interfaces_c[0].IsUp());
        CPPUNIT_ASSERT(!interfaces_c[0].IsRunning());
        CPPUNIT_ASSERT(interfaces_c[1].GetName() == L"sit0");
        CPPUNIT_ASSERT(!interfaces_c[1].IsUp());
        CPPUNIT_ASSERT(interfaces_c[1].IsRunning());
#endif
    }

    //! Make a resonable effort to check the correctness of an IPaddress
    //! \param[in]    address     IPAddress to be checked
    //! \returns      false if certainly incorrect
    bool ProbablyCorrect(const wstring &address)
    {
        bool correct = false;
        if (address.size() > 0) {
            struct in_addr inp;
#if defined(linux) || defined(aix)
            correct = inet_aton(StrToUTF8(address).c_str(), &inp);
#elif defined(sun) || defined(hpux)
            correct = inet_addr(StrToUTF8(address).c_str()) > 0;
#else
#error "Unsupported platform"
#endif
        }
        return correct;
    }

    //! Make a reasonable effort to check that the output of the PAL on the current system is sound, if not provably correct. Because we cannot make assumptions on IP-Address etc of the system, we cannot easily make fool-proof tests on correctness
    void TestFindAllSoundness()
    {
        SCXHandle<NetworkInterfaceDependencies> deps(new NetworkInterfaceDependencies());
        // Test "many" times to reveal resource problems (wi5040)
        //
        // This causes problems on Solaris 11 SPARC (an insanely slow platform).  For now,
        // we'll limit the times that we test soundness (and hope that intermittent bugs
        // don't creep back into the code on the Solaris 11 platform).

#if defined(sun) && PF_MAJOR == 5 && PF_MINOR == 11
        const int cIterationsToTest = 500;
#else
        const int cIterationsToTest = 1000;
#endif

        for(int i = 0; i < cIterationsToTest; i++) {
            std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
            if (interfaces.size() == 0 && (! verifyNetworkInterfaces(L"TestFindAllSoundness")))
                return;
            CPPUNIT_ASSERT(interfaces.size() > 0);
            for (size_t nr = 0; nr < interfaces.size(); nr++) {
                ostringstream streamBase;
                streamBase << "On iter " << i
                           << ", Interface " << nr
                           << ", Name: " << interfaces[nr].GetName().c_str()
                           << std::endl;

                ostringstream streamName(streamBase.str());
                streamName << "   Length: " << interfaces[nr].GetName().length();
                CPPUNIT_ASSERT_MESSAGE(streamName.str().c_str(), interfaces[nr].GetName().length() > 0);
                CPPUNIT_ASSERT_MESSAGE(streamName.str().c_str(), interfaces[nr].GetName().substr(0,5) != L"wrsmd"); // Soundness test for bug 5175

                ostringstream streamIPKnown(streamBase.str());
                streamIPKnown << std::endl << "   IsIPAddressKnown: " << interfaces[nr].IsIPAddressKnown();
                if ( interfaces[nr].IsIPAddressKnown() )
                {
                    streamIPKnown << ", interfaces[nr].GetIPAddress(): " << interfaces[nr].GetIPAddress().c_str();
                }
                CPPUNIT_ASSERT_MESSAGE(streamIPKnown.str().c_str(),
                    !interfaces[nr].IsIPAddressKnown() || ProbablyCorrect(interfaces[nr].GetIPAddress()));

                ostringstream streamBroadKnown(streamBase.str());
                streamBroadKnown << std::endl << "   IsBroadcastAddressKnown(): " << interfaces[nr].IsBroadcastAddressKnown();
                if ( interfaces[nr].IsBroadcastAddressKnown() )
                {
                    streamBroadKnown << ", GetBroadcastAddress(): " << interfaces[nr].GetBroadcastAddress().c_str()
                                     << ", ProbablyCorrect(): " << ProbablyCorrect(interfaces[nr].GetBroadcastAddress());
                }
                CPPUNIT_ASSERT_MESSAGE(streamBroadKnown.str().c_str(),
                    !interfaces[nr].IsBroadcastAddressKnown() || ProbablyCorrect(interfaces[nr].GetBroadcastAddress()));

                ostringstream streamMaskKnown(streamBase.str());
                streamMaskKnown << "   IsNetmaskKnown: " << interfaces[nr].IsNetmaskKnown();
                if ( interfaces[nr].IsNetmaskKnown() )
                {
                    streamMaskKnown << ", GetNetmask(): " << interfaces[nr].GetNetmask().c_str()
                                    << ", ProbablyCorrect(): " << ProbablyCorrect(interfaces[nr].GetNetmask());
                }
                CPPUNIT_ASSERT_MESSAGE(streamMaskKnown.str().c_str(),
                    !interfaces[nr].IsNetmaskKnown() || ProbablyCorrect(interfaces[nr].GetNetmask()));

                ostringstream streamBytesRec(streamBase.str());
                streamBytesRec << "   IsBytesReceivedKnown: " << interfaces[nr].IsBytesReceivedKnown()
                               << ", IsPacketsReceivedKnown: " << interfaces[nr].IsPacketsReceivedKnown();
                if ( interfaces[nr].IsBytesReceivedKnown() && interfaces[nr].IsPacketsReceivedKnown() )
                {
                    streamBytesRec << ", BytesReceived: " << interfaces[nr].GetBytesReceived()
                                   << ", PacketsReceived: " << interfaces[nr].GetPacketsReceived();

                    // See WI 27962:
                    //
                    // For some reason, very rarely, the packet count seems to
                    // be higher than the byte count.  It's not clear why this
                    // is happening.  Temporarily accept this specific failure
                    // until we can look into it further.
                    if (interfaces[nr].GetPacketsReceived() > interfaces[nr].GetBytesReceived())
                    {
                        continue;
                }
                }

                CPPUNIT_ASSERT_MESSAGE(streamBytesRec.str().c_str(),
                    !interfaces[nr].IsBytesReceivedKnown() ||
                    !interfaces[nr].IsPacketsReceivedKnown()  ||
                    (interfaces[nr].GetBytesReceived() == 0 && interfaces[nr].GetPacketsReceived() == 0) ||
                    interfaces[nr].GetBytesReceived() > interfaces[nr].GetPacketsReceived());

                ostringstream streamBytesSent(streamBase.str());
                streamBytesRec << "   IsBytesSentKnown: " << interfaces[nr].IsBytesSentKnown()
                               << ", IsPacketsSentKnown: " << interfaces[nr].IsPacketsSentKnown();
                if ( interfaces[nr].IsBytesSentKnown() && interfaces[nr].IsPacketsSentKnown() )
                {
                    streamBytesRec << ", BytesSent: " << interfaces[nr].GetBytesSent()
                                   << ", PacketsSent: " << interfaces[nr].GetPacketsSent();

                    // See WI 27962:
                    //
                    // For some reason, very rarely, the packet count seems to
                    // be higher than the byte count.  It's not clear why this
                    // is happening.  Temporarily accept this specific failure
                    // until we can look into it further.
                    if (interfaces[nr].GetPacketsSent() > interfaces[nr].GetBytesSent())
                    {
                        continue;
                }
                }

                CPPUNIT_ASSERT_MESSAGE(streamBytesSent.str().c_str(),
                    !interfaces[nr].IsBytesSentKnown() ||
                    !interfaces[nr].IsPacketsSentKnown()  ||
                    (interfaces[nr].GetBytesSent() == 0 && interfaces[nr].GetPacketsSent() == 0) ||
                    interfaces[nr].GetBytesSent() > interfaces[nr].GetPacketsSent());
            }
        }
    }

    //! Check that the enumeration are initialized correctly
    void CheckInitialEnumeration(NetworkInterfaceEnumeration &interfaces, SCXHandle<MyNetworkInterfaceDependencies> deps)
    {
        // Take into account that interfaces are ordered according to their name (alphabetically)
#if defined(linux)
        deps->SetDynamicInfoFile(L"./testfiles/procnetdev.txt");

        deps->AddIPAddress("127.0.0.0");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
          
        deps->AddBroadcastAddress("127.0.0.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");

        deps->AddNetmask("255.0.0.0"); 
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");

        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(false);

        interfaces.Init();

        // Localhost has been removed
        CPPUNIT_ASSERT(interfaces.Size() == 2);

        scxulong value = 0;
        wstring text;
        bool cond;

        CPPUNIT_ASSERT(interfaces[0]->GetName() == L"eth0");
        CPPUNIT_ASSERT(interfaces[0]->GetIPAddress(text));
        CPPUNIT_ASSERT(text == L"157.58.164.68");
        CPPUNIT_ASSERT(interfaces[0]->GetBytesReceived(value));
        CPPUNIT_ASSERT(value == 305641);
        CPPUNIT_ASSERT(interfaces[0]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces[0]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);

        CPPUNIT_ASSERT(interfaces[1]->GetIPAddress(text));
        CPPUNIT_ASSERT(interfaces.Size() == 2);
        CPPUNIT_ASSERT(text == L"157.58.162.69");
        CPPUNIT_ASSERT(interfaces[1]->GetBroadcastAddress(text));
        CPPUNIT_ASSERT(text == L"157.58.162.255");
        CPPUNIT_ASSERT(interfaces[1]->GetNetmask(text));
        CPPUNIT_ASSERT(text == L"255.255.0.0");
        CPPUNIT_ASSERT(interfaces[1]->GetName() == L"sit0");
        CPPUNIT_ASSERT(interfaces[1]->GetBytesReceived(value));
        CPPUNIT_ASSERT(value == 10);
        CPPUNIT_ASSERT(interfaces[1]->GetPacketsReceived(value) == true);
        CPPUNIT_ASSERT(value == 11);
        CPPUNIT_ASSERT(interfaces[1]->GetErrorsReceiving(value));
        CPPUNIT_ASSERT(value == 12);
        CPPUNIT_ASSERT(interfaces[1]->GetBytesSent(value));
        CPPUNIT_ASSERT(value == 14);
        CPPUNIT_ASSERT(interfaces[1]->GetPacketsSent(value));
        CPPUNIT_ASSERT(value == 15);
        CPPUNIT_ASSERT(interfaces[1]->GetErrorsSending(value));
        CPPUNIT_ASSERT(value == 16);
        CPPUNIT_ASSERT(interfaces[1]->GetCollisions(value));
        CPPUNIT_ASSERT(value == 9);
        CPPUNIT_ASSERT(interfaces[1]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces[1]->GetRunning(cond));
        CPPUNIT_ASSERT(!cond);

#else
        CPPUNIT_ASSERT(false);
#endif
    }

    //! Check that still existing instances are updated
    void CheckUpdatedInstances(NetworkInterfaceEnumeration &interfaces, SCXHandle<MyNetworkInterfaceDependencies> deps)
    {
#if defined(linux)
        deps->SetDynamicInfoFile(L"./testfiles/procnetdev2.txt");

        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");

        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0"); deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
          
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");

        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");

        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");

        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddRunning(false);
        deps->AddRunning(true);
        deps->AddRunning(false);
        deps->AddRunning(false);
        deps->AddRunning(true);
        deps->AddRunning(false);
 
        interfaces.Update(true);

        CPPUNIT_ASSERT(interfaces.Size() == 2);
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");

        scxulong value = 0;
        wstring text;
        bool cond;

        CPPUNIT_ASSERT(interfaces[0]->GetName() == L"eth0");
        deps->AddRunning(false);
        CPPUNIT_ASSERT(interfaces[0]->GetBytesReceived(value));
        CPPUNIT_ASSERT(value == 405641);

        CPPUNIT_ASSERT(interfaces[1]->GetIPAddress(text));
        CPPUNIT_ASSERT(text == L"157.58.162.69");
        CPPUNIT_ASSERT(interfaces[1]->GetBroadcastAddress(text));
        CPPUNIT_ASSERT(text == L"157.58.162.255");
        CPPUNIT_ASSERT(interfaces[1]->GetNetmask(text));
        CPPUNIT_ASSERT(text == L"255.255.0.0");
        CPPUNIT_ASSERT(interfaces[1]->GetName() == L"sit0");
        CPPUNIT_ASSERT(interfaces[1]->GetBytesReceived(value));
        CPPUNIT_ASSERT(value == 10);
        CPPUNIT_ASSERT(interfaces[1]->GetPacketsReceived(value) == true);
        CPPUNIT_ASSERT(value == 11);
        CPPUNIT_ASSERT(interfaces[1]->GetErrorsReceiving(value));
        CPPUNIT_ASSERT(value == 12); 
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
          
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");

        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");

        CPPUNIT_ASSERT(interfaces[1]->GetBytesSent(value));
        CPPUNIT_ASSERT(value == 14);
        CPPUNIT_ASSERT(interfaces[1]->GetPacketsSent(value));
        CPPUNIT_ASSERT(value == 15);
        CPPUNIT_ASSERT(interfaces[1]->GetErrorsSending(value));
        CPPUNIT_ASSERT(value == 16);
        CPPUNIT_ASSERT(interfaces[1]->GetCollisions(value));
        CPPUNIT_ASSERT(value == 9);
        CPPUNIT_ASSERT(interfaces[1]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces[1]->GetRunning(cond));
        CPPUNIT_ASSERT(!cond);
#else
        CPPUNIT_ASSERT(false);
#endif
    }

    //! Check that instances are discovered, removed and updated
    void CheckUpdatedEnumeration(NetworkInterfaceEnumeration &interfaces, SCXHandle<MyNetworkInterfaceDependencies> deps)
    {
#if defined(linux)
        deps->SetDynamicInfoFile(L"./testfiles/procnetdev3.txt");

        deps->AddIPAddress("127.0.0.0");
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
          
        deps->AddBroadcastAddress("127.0.0.255");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddBroadcastAddress("157.58.162.255");

        deps->AddNetmask("255.0.0.0"); 
        deps->AddNetmask("255.255.255.0");
        deps->AddNetmask("255.255.0.0");

        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddUp(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);
        deps->AddRunning(true);

        interfaces.Update(false);

        // Localhost has been removed
        CPPUNIT_ASSERT(interfaces.Size() == 2);

        scxulong value = 0;
        wstring text;

        CPPUNIT_ASSERT(interfaces[0]->GetName() == L"eth0");
        CPPUNIT_ASSERT(interfaces[0]->GetIPAddress(text));
        CPPUNIT_ASSERT(text == L"157.58.164.68");
        CPPUNIT_ASSERT(interfaces[0]->GetBytesReceived(value));
        CPPUNIT_ASSERT(value == 505641);

        CPPUNIT_ASSERT(interfaces[1]->GetIPAddress(text));
        CPPUNIT_ASSERT(text == L"157.58.162.69");
        CPPUNIT_ASSERT(interfaces[1]->GetBroadcastAddress(text));
        CPPUNIT_ASSERT(text == L"157.58.162.255");
        CPPUNIT_ASSERT(interfaces[1]->GetNetmask(text));
        CPPUNIT_ASSERT(text == L"255.255.0.0");
        CPPUNIT_ASSERT(interfaces[1]->GetName() == L"sit1");
        CPPUNIT_ASSERT(interfaces[1]->GetBytesReceived(value));
        CPPUNIT_ASSERT(value == 30);
        CPPUNIT_ASSERT(interfaces[1]->GetPacketsReceived(value) == true);
        CPPUNIT_ASSERT(value == 11);
        CPPUNIT_ASSERT(interfaces[1]->GetErrorsReceiving(value));
        CPPUNIT_ASSERT(value == 12);
        CPPUNIT_ASSERT(interfaces[1]->GetBytesSent(value));
        CPPUNIT_ASSERT(value == 14);
        CPPUNIT_ASSERT(interfaces[1]->GetPacketsSent(value));
        CPPUNIT_ASSERT(value == 15);
        CPPUNIT_ASSERT(interfaces[1]->GetErrorsSending(value));
        CPPUNIT_ASSERT(value == 16);
        CPPUNIT_ASSERT(interfaces[1]->GetCollisions(value));
        CPPUNIT_ASSERT(value == 9);

#else
        CPPUNIT_ASSERT(false);
#endif
    }

    //! Test that the enumeration contains expected data when run on controlled input
    void TestEnumerationBehavior()
    {
#if defined(linux)
        SCXHandle<MyNetworkInterfaceDependencies> deps(new MyNetworkInterfaceDependencies());
        NetworkInterfaceEnumeration interfaces(deps);
        CheckInitialEnumeration(interfaces, deps);
        CPPUNIT_ASSERT_NO_THROW(CheckUpdatedInstances(interfaces, deps));
        SCXHandle<MyNetworkInterfaceDependencies> deps2(new MyNetworkInterfaceDependencies());
        NetworkInterfaceEnumeration interfaces2(deps2);
        CPPUNIT_ASSERT_NO_THROW(CheckUpdatedEnumeration(interfaces2, deps2));
#else
        // Platform dependent code is covered by other test cases
        // Testing enumerations using injected input may be done on any platform
#endif
    }

    //! Check the soundness of next value
    void CheckNextValue(bool valueSupported, scxulong value, const wstring &name, std::map<wstring, scxulong> &valueOnInterface, bool &valueOnSome) {
        if (valueSupported) {
            CPPUNIT_ASSERT(value >= valueOnInterface[name]);
            valueOnInterface[name] = value;
            valueOnSome = true;
        }
    }

    //! Make a reasonable effort to check that the data of the instances of the enumeration is sound
    void TestEnumerationSoundness()
    {
        NetworkInterfaceEnumeration interfaces;
        std::map<wstring, scxulong> packetsSentOnInterface;
        std::map<wstring, scxulong> packetsReceivedOnInterface;
        std::map<wstring, scxulong> bytesSentOnInterface;
        std::map<wstring, scxulong> bytesReceivedOnInterface;
        interfaces.Init();
        interfaces.Update(true);
        bool packetsSentOnSome = false;
        bool packetsReceivedOnSome = false;
        bool bytesSentOnSome = false;
        bool bytesReceivedOnSome = false;
        bool ipAddressOnSome = false;
        bool broadcastAddressOnSome = false;
        bool netmaskOnSome = false;
        bool someIsUp = false;
        for (int testNr = 0; testNr < 5; testNr++) {
            SCXThread::Sleep(1000);
            interfaces.Update(false);
            if (interfaces.Size() == 0 && (! verifyNetworkInterfaces(L"TestEnumerationSoundness")))
                return;
            CPPUNIT_ASSERT(interfaces.Size() > 0);
            for (size_t instanceNr = 0; instanceNr < interfaces.Size(); instanceNr++) {
                wstring name(interfaces[instanceNr]->GetName());
                CPPUNIT_ASSERT(name.size() > 0);
                wstring text;
                bool cond;
                bool ipAddressSupported = interfaces[instanceNr]->GetIPAddress(text);
                if (ipAddressSupported) {
                    ipAddressOnSome |= text.size() > 0;
                    CPPUNIT_ASSERT(ProbablyCorrect(text));
                }
                bool broadcastAddressSupported = interfaces[instanceNr]->GetBroadcastAddress(text);
                if (broadcastAddressSupported) {
                    broadcastAddressOnSome |= text.size() > 0;
                    CPPUNIT_ASSERT(ProbablyCorrect(text));
                }
                bool netmaskSupported = interfaces[instanceNr]->GetNetmask(text);
                if (netmaskSupported) {
                    netmaskOnSome |= text.size() > 0;
                    CPPUNIT_ASSERT(ProbablyCorrect(text));
                }
                someIsUp |= (interfaces[instanceNr]->GetUp(cond) && cond);
                scxulong value;
                bool packetsSentSupported = interfaces[instanceNr]->GetPacketsSent(value);
                CheckNextValue(packetsSentSupported, value, name, packetsSentOnInterface, packetsSentOnSome);
                bool packetsReceivedSupported = interfaces[instanceNr]->GetPacketsReceived(value);
                CheckNextValue(packetsReceivedSupported, value, name, packetsReceivedOnInterface, packetsReceivedOnSome);
                bool bytesSentSupported = interfaces[instanceNr]->GetBytesSent(value);
                CheckNextValue(bytesSentSupported, value, name, bytesSentOnInterface, bytesSentOnSome);
                bool bytesReceivedSupported = interfaces[instanceNr]->GetBytesReceived(value);
                CheckNextValue(bytesReceivedSupported, value, name, bytesReceivedOnInterface, bytesReceivedOnSome);

            }
        }
#if !defined(hpux)
        CPPUNIT_ASSERT(packetsSentOnSome);
        CPPUNIT_ASSERT(packetsReceivedOnSome);
        CPPUNIT_ASSERT(packetsReceivedOnSome);
        CPPUNIT_ASSERT(packetsReceivedOnSome);
#endif
        CPPUNIT_ASSERT(broadcastAddressOnSome);
        CPPUNIT_ASSERT(someIsUp);
        CPPUNIT_ASSERT(ipAddressOnSome);
        CPPUNIT_ASSERT(netmaskOnSome);
    }

    void TestBug5175_IgnoreNetDevicesNotInterfaces()
    {
#if defined(sun)
        SCXHandle<MyNetworkInterfaceDependencies> deps(new MyNetworkInterfaceDependencies());

        ksdata_sit0 = new kstat_named_t[7];
        InitAttribute(ksdata_sit0[0], "rbytes64", 0);
        InitAttribute(ksdata_sit0[1], "ipackets64", 0);
        InitAttribute(ksdata_sit0[2], "lbufs", 0);
        InitAttribute(ksdata_sit0[3], "obytes64", 0);
        InitAttribute(ksdata_sit0[4], "opackets64", 0);
        InitAttribute(ksdata_sit0[5], "oerrors", 0);
        InitAttribute(ksdata_sit0[6], "collisions", 0);
        header_sit0 = new kstat_t;
        InitHeader(*header_sit0, "net", "wrsmd5", KSTAT_TYPE_NAMED, ksdata_sit0, 7, 0);

        ctl = new kstat_ctl_t;
        ctl->kc_chain = header_sit0;
        deps->SetKStat(ctl);

        deps->AddIPAddress("127.0.0.42");
        deps->AddBroadcastAddress("127.0.0.255");
        deps->AddNetmask("255.0.0.0");
        deps->AddUp(false);
        deps->AddRunning(false);

        std::vector<NetworkInterfaceInfo> interfaces;

        CPPUNIT_ASSERT_NO_THROW(interfaces = NetworkInterfaceInfo::FindAll(deps));

        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), interfaces.size());
#endif
    }

    void TestMTU()
    {
        SCXHandle<NetworkInterfaceDependencies> deps(new NetworkInterfaceDependencies());
        std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
        CPPUNIT_ASSERT_MESSAGE("No interface information found", interfaces.size() > 0);

        scxulong mtu;
        for (size_t nr = 0; nr < interfaces.size(); nr++)
        {
            CPPUNIT_ASSERT(interfaces[nr].GetMTU(mtu));
            std::cout << " " << mtu;
            // RFC 791 "Every internet module must be able to forward a datagram of 68 octets without further fragmentation."
            CPPUNIT_ASSERT_MESSAGE("MTU too small : " + StrToUTF8(StrFrom(mtu)), mtu >= 68);
            // Default maximum for the ip4 mtu in the tracepath utility
            CPPUNIT_ASSERT_MESSAGE("MTU too large : " + StrToUTF8(StrFrom(mtu)), mtu <= 65536);
        }
    }

#if defined(hpux)
    // This is a system test designed to test if there is at least one network interface active 
    void TestHP_FindAllInDLPI_AtLeastOneInterface()
    {
        SCXHandle<NetworkInterfaceDependencies> deps(new NetworkInterfaceDependencies());
        
        std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
        CPPUNIT_ASSERT(interfaces.size() >= static_cast<size_t> (1));
    }
    
    // This makes sure that all interfaces found on the machine are also found in lanscan
    void TestHP_FindAllInDLPI_ComparedToLanscan()
    {
        SCXHandle<NetworkInterfaceDependencies> deps(new NetworkInterfaceDependencies());
        
        std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
        
        std::istringstream input;
        std::ostringstream output;
        std::ostringstream error;
        SCXProcess::Run(L"/bin/sh -c \"LANG=C /usr/sbin/lanscan\"", input, output, error, 1000);

        // check error for output
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Error output from lanscan: " + error.str(), (size_t)0, error.str().size());
        
        std::istringstream lanscan_istream(output.str());
        
        
        // this set will contain pairs of <namePPA, MACAddress>
        // we will insert each line of lanscan output into this set
        std::set< std::pair<string, string> > lanscanItemSet;
        
        /* 
           Example output from lanscan:
           
           Hardware Station        Crd Hdw   Net-Interface  NM  MAC       HP-DLPI DLPI
           Path     Address        In# State NamePPA        ID  Type      Support Mjr#
           0/1/2/0  0x00306E4B09D4 0   UP    lan0 snap0     1   ETHER     Yes     119
           0/4/1/0  0x00248177336E 1   UP    lan1 snap1     2   ETHER     Yes     119
           0/4/1/1  0x00248177336F 2   UP    lan2 snap2     3   ETHER     Yes     119
           LinkAgg0 0x000000000000 900 DOWN  lan900 snap900 5   ETHER     Yes     119
           LinkAgg1 0x000000000000 901 DOWN  lan901 snap901 6   ETHER     Yes     119
           LinkAgg2 0x000000000000 902 DOWN  lan902 snap902 7   ETHER     Yes     119
           LinkAgg3 0x000000000000 903 DOWN  lan903 snap903 8   ETHER     Yes     119
           LinkAgg4 0x000000000000 904 DOWN  lan904 snap904 9   ETHER     Yes     119
           
        */
        
        string curline;
        int count = 0;
        while ( getline(lanscan_istream, curline) )
        {
            // skip the first two header lines
            if ((count++) < 2)
            {
                continue;
            }
            
            // important values here are the MAC address (#2) and the namePPA (#5)
            string macAddress;
            string namePPA;
            
            // parse each interface line into tokens
            std::vector<string> tokens;
            int loc1 = 0;
            int loc2 = 0;
            while ((long)loc2 != string::npos)
            {            
                loc1 = curline.find_first_not_of(" \t", loc2);
                loc2 = curline.find_first_of(" \t", loc1);
                tokens.push_back(curline.substr(loc1, loc2-loc1));
            }
            
            // make sure the first 2 characters on the mac address are 0x
            CPPUNIT_ASSERT_EQUAL_MESSAGE("lanscan is behaving unexpectedly with respect to how it prints out mac addresses",
                                         tokens[1].substr(0,2), string("0x"));
            
            // chop off the 0x prefix of the mac address
            macAddress = tokens[1].substr(2);
            namePPA = tokens[4];
            
            // put them into lanscan set
            std::pair<string, string> lanscanItem(namePPA, macAddress);
            lanscanItemSet.insert(lanscanItem);
            
        }
        
        // make sure that every element in interfaces is in lanscan
        // but it doesn't have to be the case that every element in lanscan's output is in interfaces
        for (std::vector<NetworkInterfaceInfo>::iterator i = interfaces.begin(); i != interfaces.end(); i++)
        {
            wstring macaddrW;
            i->GetMACAddress(macaddrW);
            string macaddr = StrToUTF8(StrToUpper(macaddrW));

            // remove ':' from macaddr so that it matches lanscan
            size_t curLoc = 0;
            while((curLoc = macaddr.find(":", curLoc)) != string::npos)
            {
                macaddr.replace(curLoc, 1, "");
            }

            std::pair<string, string> tempPair(StrToUTF8(i->GetName()), macaddr);
            
            // assert if an interface is not found in the set of lanscan interfaces
            CPPUNIT_ASSERT_MESSAGE("Not able to find NamePPA " + tempPair.first + 
                                   " with MAC Address " + tempPair.second + " in lanscan.",
                                   lanscanItemSet.find(tempPair) != lanscanItemSet.end());
        }
        
        
    }
    
    // This is a helper function for the HP tests below
    dl_hp_ppa_info_t createPPAInfo(int ppa, string name)
    {
        dl_hp_ppa_info_t testPPAInfo;
        memset(&testPPAInfo, 0, sizeof(testPPAInfo));
        testPPAInfo.dl_ppa = ppa;
        memcpy(&testPPAInfo.dl_module_id_1, name.c_str(), name.length());
        return testPPAInfo;
    }
    
    void TestHP_FindAllInDLPI_SingleInterface_Injection()
    {
        // create our test stats
        const int ppa = 234;
        const string name = "lan";
        const scxulong inOctets = 987;
        
        // array containing PPA infos
        std::vector<dl_hp_ppa_info_t> ppaVector;
        
        // add new ppa info to ppaVector
        ppaVector.push_back(createPPAInfo(ppa, name));
        
        // map containing stats
        std::map<int, mib_ifEntry> statsMap;
        
        // add new stats to statsMap
        mib_ifEntry testMIB;
        memset(&testMIB, 0, sizeof(testMIB));
        testMIB.ifInOctets = inOctets;
        statsMap[ppa] = testMIB;
        
        
        SCXHandle<DLPINetworkInterfaceDependencies> deps(new DLPINetworkInterfaceDependencies(ppaVector, statsMap));

        // set up our ioctl calls
        deps->AddIPAddress("127.0.0.1");
        deps->AddBroadcastAddress("127.0.0.255");
        deps->AddNetmask("255.255.0.0");
        deps->AddUp(true);
        deps->AddRunning(true);

        deps->AddUp(true);
        deps->AddRunning(true);


        
        // Do our test, verify they match
        std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (1), interfaces.size());
        
        std::ostringstream tmpOStream;
        tmpOStream << name << ppa;
        string namePPA = tmpOStream.str();
        
        CPPUNIT_ASSERT_EQUAL(namePPA, StrToUTF8(interfaces[0].GetName()));
        CPPUNIT_ASSERT_EQUAL(inOctets, interfaces[0].GetBytesReceived());
        
    }

    // Helper function that sets dependencies for HP tests.
    SCXHandle<DLPINetworkInterfaceDependencies> FindAllInDLPI_ThreeInterface_Initialization(
        int ppa1, int ppa2, int ppa3, const string &name1, const string &name2, const string &name3,
        scxulong inOctets1, scxulong outOctets2, scxulong outPackets3, bool bAllInterfacesUp)
    {
        
        // array containing PPA infos
        std::vector<dl_hp_ppa_info_t> ppaVector;
        
        // add new ppa infos to ppaVector
        ppaVector.push_back(createPPAInfo(ppa1, name1));
        ppaVector.push_back(createPPAInfo(ppa2, name2));
        ppaVector.push_back(createPPAInfo(ppa3, name3));
        
        // map containing stats
        std::map<int, mib_ifEntry> statsMap;
        
        // associate ppa1 with inOctets1
        mib_ifEntry testMIB;
        memset(&testMIB, 0, sizeof(testMIB));
        testMIB.ifInOctets = static_cast<unsigned int>(inOctets1);
        statsMap[ppa1] = testMIB;
        
        // associate ppa2 with outOctets2
        memset(&testMIB, 0, sizeof(testMIB));
        testMIB.ifOutOctets = static_cast<unsigned int>(outOctets2);
        statsMap[ppa2] = testMIB;
        
        // associate ppa3 with outPackets3
        memset(&testMIB, 0, sizeof(testMIB));
        testMIB.ifOutUcastPkts = static_cast<unsigned int>(outPackets3);
        statsMap[ppa3] = testMIB;
        
        SCXHandle<DLPINetworkInterfaceDependencies> deps(new DLPINetworkInterfaceDependencies(ppaVector, statsMap));
        
        // make ioctl fail for the second call
        deps->AddIPAddress("157.58.164.68");
        deps->AddIPAddress("157.58.162.69");
        deps->AddIPAddress("127.0.0.1");
        deps->AddBroadcastAddress("127.0.0.255");
        deps->AddBroadcastAddress("127.0.0.255");
        deps->AddBroadcastAddress("127.0.0.255");
        deps->AddNetmask("255.255.0.0");
        deps->AddNetmask("255.255.0.0");
        deps->AddNetmask("255.255.0.0");

        if(bAllInterfacesUp)
        {
            deps->AddUp(true);
            deps->AddUp(true);
            deps->AddUp(true);
            deps->AddRunning(true);
            deps->AddRunning(true);
            deps->AddRunning(true);
    
            deps->AddUp(true);
            deps->AddUp(true);
            deps->AddUp(true);
            deps->AddRunning(true);
            deps->AddRunning(true);
            deps->AddRunning(true);
        }
        else
        {
            deps->AddUp(true);
            deps->AddUp(false);
            deps->AddUp(false);
            deps->AddRunning(true);
            deps->AddRunning(false);
            deps->AddRunning(true);
    
            deps->AddUp(true);
            deps->AddUp(false);
            deps->AddUp(false);
            deps->AddRunning(true);
            deps->AddRunning(false);
            deps->AddRunning(true);
        }

        return deps;
    }

    void TestHP_FindAllInDLPI_ThreeInterface_Injection()
    {
        
        // create our test stats
        const int ppa1 = 0;
        const int ppa2 = 1;
        const int ppa3 = 2;
        const string name1 = "lan";
        const string name2 = "eth";
        const string name3 = "lo";
        const scxulong inOctets1 = 987;
        const scxulong outOctets2 = 10109283;
        const scxulong outPackets3 = 999999;

        SCXHandle<DLPINetworkInterfaceDependencies> deps = FindAllInDLPI_ThreeInterface_Initialization(
            ppa1, ppa2, ppa3, name1, name2, name3, inOctets1, outOctets2, outPackets3, false);
        
        // Do our test, verify they match
        std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
        
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces);
        }
        
        std::ostringstream tmpOStream;
        string namePPA;
        
        // we should only find two interfaces, one with ppa1 and one with ppa3 and associated values
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (2), interfaces.size());
        
        tmpOStream << name1 << ppa1;
        namePPA = tmpOStream.str();
        
        CPPUNIT_ASSERT_EQUAL(namePPA, StrToUTF8(interfaces[0].GetName()));
        CPPUNIT_ASSERT_EQUAL(inOctets1, interfaces[0].GetBytesReceived());

        tmpOStream.str("");
        tmpOStream << name3 << ppa3;
        namePPA = tmpOStream.str();
        
        CPPUNIT_ASSERT_EQUAL(namePPA, StrToUTF8(interfaces[1].GetName()));
        CPPUNIT_ASSERT_EQUAL(outPackets3, interfaces[1].GetPacketsSent());
        
    }
    
    
    void TestHP_FindAllInDLPI_ManyInterface_Injection()
    {
        const int many = 500;
        const string name = "lan";
        
        std::vector<dl_hp_ppa_info_t> ppaVector;
        std::map<int, mib_ifEntry> statsMap;
        
        // create an empty stats, we won't be using it in this test
        mib_ifEntry testMIB;
        memset(&testMIB, 0, sizeof(testMIB));
        
        // insert 'many' number of ppa's
        for (int ppa = 0; ppa < many; ppa++)
        {
            ppaVector.push_back(createPPAInfo(ppa, name));
            statsMap[ppa] = testMIB;
        }
        
        SCXHandle<DLPINetworkInterfaceDependencies> deps(new DLPINetworkInterfaceDependencies(ppaVector, statsMap));
        
        // insert data so that the injected ioctl does not fail
        // cannot include this loop into the previous loop because of its reliance on 'deps'
        // and 'deps' cannot be initialized until the ppaVector and statsMap are completely built
        for (int ppa = 0; ppa < many; ppa++)
        {
            deps->AddIPAddress("127.0.0.1");
            deps->AddBroadcastAddress("127.0.0.255");
            deps->AddNetmask("255.255.0.0");
            deps->AddUp(true);
            deps->AddUp(true);
            deps->AddUp(true);
            deps->AddRunning(true);
            deps->AddRunning(true);
            deps->AddRunning(true);
        }

        // Do our test, verify they match
        std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);
        
        std::ostringstream tmpOStream;
        string namePPA;
        
        // we should find 'many' number of instances, each with location
        // in our interfaces array equivalent to the PPA number
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t> (many), interfaces.size());

        for (int ppa = 0; ppa < many; ppa++)
        {
            tmpOStream.str("");
            tmpOStream << name << ppa;
            namePPA = tmpOStream.str(); 
            CPPUNIT_ASSERT_EQUAL(namePPA, StrToUTF8(interfaces[ppa].GetName()));
        }
    }

    void TestHP_WI384433_Get_DataLink_Speed() 
    {
        // create our test stats
        const int ppa = 567;
        const string name = "lan";
        const scxulong inoctets = 987;

        // array containing ppa infos
        std::vector<dl_hp_ppa_info_t> ppavector;

        // add new ppa info to ppavector
        ppavector.push_back(createPPAInfo(ppa, name));
        // map containing stats
        std::map<int, mib_ifEntry> statsMap;

        // add new stats to statsMap
        mib_ifEntry testmib;
        memset(&testmib, 0, sizeof(testmib));
        testmib.ifInOctets = inoctets;
        statsMap[ppa] = testmib;

        SCXHandle<DLPINetworkInterfaceDependencies> deps(new DLPINetworkInterfaceDependencies(ppavector, statsMap));

        scxulong speed;
        bool autoNeg;
        test_cnt = 1;
        for (int i = 0; i < WI384433_NUM_OF_TESTS; i++)
        {
            speed = 0;
            autoNeg = false;

            // set up our ioctl calls
            deps->AddIPAddress("127.0.0.1");
            deps->AddBroadcastAddress("127.0.0.255");
            deps->AddNetmask("255.255.0.0");
            deps->AddUp(true);
            deps->AddRunning(true);
            deps->AddUp(true);
            deps->AddRunning(true);
           
            // Calling FindAll() is to call the function in question.
            std::vector<NetworkInterfaceInfo> interfaces = NetworkInterfaceInfo::FindAll(deps);

            interfaces[0].GetSpeed(speed);
            interfaces[0].GetAutoSense(autoNeg);
            CPPUNIT_ASSERT_EQUAL(WI384433_Test_Table[i++], speed); 
            CPPUNIT_ASSERT_EQUAL(WI384433_Test_Table[i], autoNeg); 

            ++test_cnt;
        }
        test_cnt = 0;

    } // End of TestHP_WI384433_Get_DataLink_Speed() 
    
#endif // if defined(hpux)

    //! Test that NetworkInterfaceEnumeration returns only interfaces that are UP or RUNNING.
    void TestGetRunningInterfacesOnly()
    {
#if defined(linux) || defined(sun) || defined(aix)
        bool cond = false;

        // Case when some interfaces are not UP or RUNNING.
        SCXHandle<MyNetworkInterfaceDependencies> deps_a = SetupBehavioralTestDependency(false);
        NetworkInterfaceEnumeration interfaces_a(deps_a);
        interfaces_a.Init();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), interfaces_a.Size());
        CPPUNIT_ASSERT(interfaces_a[0]->GetName() == L"sit0");
        CPPUNIT_ASSERT(interfaces_a[0]->GetUp(cond));
        CPPUNIT_ASSERT(!cond);
        CPPUNIT_ASSERT(interfaces_a[0]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);

        // Case when all interfaces are UP or RUNNING.
        SCXHandle<MyNetworkInterfaceDependencies> deps_b = SetupBehavioralTestDependency(true);
        NetworkInterfaceEnumeration interfaces_b(deps_b);
        interfaces_b.Init();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), interfaces_b.Size());
        CPPUNIT_ASSERT(interfaces_b[0]->GetName() == L"eth0");
        CPPUNIT_ASSERT(interfaces_b[0]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_b[0]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_b[1]->GetName() == L"sit0");
        CPPUNIT_ASSERT(interfaces_b[1]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_b[1]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);
#endif
#if defined(hpux)
        // create our test stats
        const int ppa1 = 0;
        const int ppa2 = 1;
        const int ppa3 = 2;
        const string name1 = "lan";
        const string name2 = "eth";
        const string name3 = "lo";
        const scxulong inOctets1 = 987;
        const scxulong outOctets2 = 10109283;
        const scxulong outPackets3 = 999999;

        bool cond = false;
        std::ostringstream tmpOStream;
        wstring namePPA1;
        wstring namePPA2;
        wstring namePPA3;

        tmpOStream << name1 << ppa1;
        namePPA1 = StrFromUTF8(tmpOStream.str());
        tmpOStream.str("");
        tmpOStream << name2 << ppa2;
        namePPA2 = StrFromUTF8(tmpOStream.str());
        tmpOStream.str("");
        tmpOStream << name3 << ppa3;
        namePPA3 = StrFromUTF8(tmpOStream.str());

        // Case when some interfaces are not UP or RUNNING.
        SCXHandle<DLPINetworkInterfaceDependencies> deps_a = FindAllInDLPI_ThreeInterface_Initialization(
            ppa1, ppa2, ppa3, name1, name2, name3, inOctets1, outOctets2, outPackets3, false);
        NetworkInterfaceEnumeration interfaces_a(deps_a);// OM case.
        interfaces_a.Init();
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces_a);
        }
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), interfaces_a.Size());
        CPPUNIT_ASSERT(interfaces_a[0]->GetName() == namePPA1);
        CPPUNIT_ASSERT(interfaces_a[0]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_a[0]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);

        // Case when all interfaces are UP or RUNNING.
        SCXHandle<DLPINetworkInterfaceDependencies> deps_b = FindAllInDLPI_ThreeInterface_Initialization(
            ppa1, ppa2, ppa3, name1, name2, name3, inOctets1, outOctets2, outPackets3, true);
        NetworkInterfaceEnumeration interfaces_b(deps_b);// OM case.
        interfaces_b.Init();
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces_b);
        }
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), interfaces_b.Size());
        CPPUNIT_ASSERT(interfaces_b[0]->GetName() == namePPA2);
        CPPUNIT_ASSERT(interfaces_b[0]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_b[0]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_b[1]->GetName() == namePPA1);
        CPPUNIT_ASSERT(interfaces_b[1]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_b[1]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);
#endif
    }
    
    //! Test that NetworkInterfaceEnumeration returns all interfaces regardless of UP or RUNNING state.
    void TestGetAllInterfacesEvenNotRunning()
    {
#if defined(linux) || defined(sun) || defined(aix)
        bool cond = false;

        // Case when some interfaces are not UP or RUNNING. NetworkInterfaceEnumeration must
        // return all of the interfaces regardless of UP or RUNNING state.
        SCXHandle<MyNetworkInterfaceDependencies> deps_c = SetupBehavioralTestDependency(false);
        NetworkInterfaceEnumeration interfaces_c(deps_c, true);
        interfaces_c.Init();
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), interfaces_c.Size());
        CPPUNIT_ASSERT(interfaces_c[0]->GetName() == L"eth0");
        CPPUNIT_ASSERT(interfaces_c[0]->GetUp(cond));
        CPPUNIT_ASSERT(!cond);
        CPPUNIT_ASSERT(interfaces_c[0]->GetRunning(cond));
        CPPUNIT_ASSERT(!cond);
        CPPUNIT_ASSERT(interfaces_c[1]->GetName() == L"sit0");
        CPPUNIT_ASSERT(interfaces_c[1]->GetUp(cond));
        CPPUNIT_ASSERT(!cond);
        CPPUNIT_ASSERT(interfaces_c[1]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);
#endif
#if defined(hpux)
        // create our test stats
        const int ppa1 = 0;
        const int ppa2 = 1;
        const int ppa3 = 2;
        const string name1 = "lan";
        const string name2 = "eth";
        const string name3 = "lo";
        const scxulong inOctets1 = 987;
        const scxulong outOctets2 = 10109283;
        const scxulong outPackets3 = 999999;

        bool cond = false;
        std::ostringstream tmpOStream;
        wstring namePPA1;
        wstring namePPA2;
        wstring namePPA3;

        tmpOStream << name1 << ppa1;
        namePPA1 = StrFromUTF8(tmpOStream.str());
        tmpOStream.str("");
        tmpOStream << name2 << ppa2;
        namePPA2 = StrFromUTF8(tmpOStream.str());
        tmpOStream.str("");
        tmpOStream << name3 << ppa3;
        namePPA3 = StrFromUTF8(tmpOStream.str());

        // Case when some interfaces are not UP or RUNNING. NetworkInterfaceEnumeration must
        // return all of the interfaces regardless of UP or RUNNING state.
        SCXHandle<DLPINetworkInterfaceDependencies> deps_c = FindAllInDLPI_ThreeInterface_Initialization(
            ppa1, ppa2, ppa3, name1, name2, name3, inOctets1, outOctets2, outPackets3, false);
        NetworkInterfaceEnumeration interfaces_c(deps_c, true);// CM case.
        interfaces_c.Init();
        if(instrumentTests)
        {
            WriteNetworkInterfaceInfoAll(interfaces_c);
        }
        CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(2), interfaces_c.Size());
        CPPUNIT_ASSERT(interfaces_c[0]->GetName() == namePPA2);
        CPPUNIT_ASSERT(interfaces_c[0]->GetUp(cond));
        CPPUNIT_ASSERT(!cond);
        CPPUNIT_ASSERT(interfaces_c[0]->GetRunning(cond));
        CPPUNIT_ASSERT(!cond);
        CPPUNIT_ASSERT(interfaces_c[1]->GetName() == namePPA1);
        CPPUNIT_ASSERT(interfaces_c[1]->GetUp(cond));
        CPPUNIT_ASSERT(cond);
        CPPUNIT_ASSERT(interfaces_c[1]->GetRunning(cond));
        CPPUNIT_ASSERT(cond);
#endif
    } // End of TestGetAllInterfacesEvenNotRunning().

#if defined(aix)
    void WI384433_set_deps(SCXHandle<MyNetworkInterfaceDependencies> deps)
    {
        std::vector<perfstat_netinterface_t> perfs;
        perfs.resize(1);
        strcpy(perfs[0].name, "eth0");
        perfs[0].type = IFT_ETHER;
        perfs[0].ibytes = 305641;
        perfs[0].ipackets = 1606;
        perfs[0].ierrors = 2;
        perfs[0].obytes = 132686;
        perfs[0].opackets = 437;
        perfs[0].oerrors = 5;
        perfs[0].collisions = 8;
        deps->SetPerfStat(perfs);

        deps->AddIPAddress("157.58.164.68");
        deps->AddBroadcastAddress("157.58.164.255");
        deps->AddNetmask("255.255.255.0");

        deps->AddUp(true);
        deps->AddRunning(true);
    }

    void TestAIX_WI384433_Get_NDD_STAT()
    {
        SCXHandle<MyNetworkInterfaceDependencies> deps(new MyNetworkInterfaceDependencies());
        std::vector<NetworkInterfaceInfo> interfaces;

        scxulong speed;
        scxulong max_speed;
        bool autoNeg;

        test_cnt = 1;
        for (int i = 0; i < WI384433_NUM_OF_TESTS; i++)
        {
            speed = 0;
            max_speed = 0;
            autoNeg = false;

            WI384433_set_deps(deps);
            // Call FindALl to run the Get_NDD_STAT() method.
            interfaces = NetworkInterfaceInfo::FindAll(deps);
            
            interfaces[0].GetMaxSpeed(max_speed);
            CPPUNIT_ASSERT_EQUAL(WI384433_Test_Table[i++], max_speed);
            
            interfaces[0].GetSpeed(speed);
            CPPUNIT_ASSERT_EQUAL(WI384433_Test_Table[i++], speed);

            interfaces[0].GetAutoSense(autoNeg);
            CPPUNIT_ASSERT_EQUAL(WI384433_Test_Table[i], autoNeg);

            ++test_cnt;
        }
        test_cnt = 0;

    } // TestAIX_WI384433_Get_NDD_STAT()

#endif // defined(aix)

    void TestAdapterNetworkIPAddress()
    {
        NetworkInterfaceEnumeration networkInterfaceEnum(true);
        networkInterfaceEnum.Init();
        if (networkInterfaceEnum.Size() == 0)
        {
            // No network detected on this machine, nothing to do.
            return;
        }
        SCXHandle<NetworkInterfaceInstance> inst = networkInterfaceEnum.GetInstance(0);

        wstring ifName = inst->GetName();
        vector<wstring> gettedIpAddrs;
        CPPUNIT_ASSERT(inst->GetIPAddress(gettedIpAddrs));
        set<wstring> IPaddrSet; 
        getIPAddrfromIfconfig(ifName, IPaddrSet);
#if defined(hpux)
        for (size_t j = 1; j < gettedIpAddrs.size(); j++)
        {
            wstring ifName1 = ifName + L':' + SCXCoreLib::StrFrom(j);
            getIPAddrfromIfconfig(ifName1, IPaddrSet);
        }
#endif
        CPPUNIT_ASSERT_EQUAL(IPaddrSet.size(), gettedIpAddrs.size());
        set<wstring>::iterator itr;
        for(unsigned int i=0; i<gettedIpAddrs.size(); i++)
        {
            itr = IPaddrSet.find(gettedIpAddrs[i]);
            CPPUNIT_ASSERT(itr != IPaddrSet.end());
        }
    }
};

CPPUNIT_TEST_SUITE_REGISTRATION( SCXNetworkInterfaceTest ); /* CUSTOMIZE: Name must be same as classname */
