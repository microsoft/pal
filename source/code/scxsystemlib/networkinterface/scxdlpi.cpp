/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        scxdlpi.cpp

    \brief       DLPI interface routines

    \date        04-13-12 12:12:12

*/
/*----------------------------------------------------------------------------*/
#ifdef hpux
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <sys/poll.h>
#include <sys/dlpi.h>
#include <sys/mib.h>
#include <scxdlpi.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/logsuppressor.h>
#include <scxcorelib/stringaid.h>

#define ERROR 128 

// initialize the static ms_bufsize
int SCXdlpi::ms_bufsize = 4096;
    
/**
    Sequentially attaches and binds to each PPA, collecting stats for the devices that
    allow so, then unbinds and detaches.  Filters out those that cannot attach+bind. 
    
    \returns     vector containing the stats of all valid DLPI LAN devices
*/
std::vector<DLPIStatsEntry> SCXdlpi::GetAllLANStats()
{
    std::vector<DLPIStatsEntry> validStats;
    
    // open DLPI driver
    int fd = openDLPI("/dev/dlpi");
    
    if (fd == -1)
    {
        SCX_LOGERROR(m_log, std::wstring(L"Unable to open /dev/dlpi with O_RDWR flag passed, errno=").append(SCXCoreLib::StrFrom(errno)));
        return validStats;
    }
    
    // get vector of valid PPAs
    PPAInfoList ppaInfoList = getPPAInfoList(fd);
    
    // for each PPA, get stats (i.e. mib_ifEntry)
    // if failed on any of these steps, make sure everything is clear and mark this PPA as invalid
    PPAInfoList::iterator i = ppaInfoList.begin();
    while (i != ppaInfoList.end())
    {
        DLPIStatsEntry newEntry;
        mib_ifEntry iStats;
        unsigned int collisions;
        // attach and bind, and if not able to, then skip this PPA
        if (attach(fd, i->dl_ppa) || 
            bind(fd))
        {
            // this happens commonly on some machines; thus, don't log an error
            i = ppaInfoList.erase(i);
            continue;
        }
        
        // if able to bind, but cannot get stats, add to vector with no values besides ppa/name
        if (getstats(fd, iStats, collisions))
        {
            SCX_LOGINFO(m_log, std::wstring(L"Able to attach and bind, but mot able to get stats for PPA=").append(SCXCoreLib::StrFrom(i->dl_ppa)).append(std::wstring(L" and errno=")).append(SCXCoreLib::StrFrom(errno)));
            memset(&iStats, 0, sizeof(iStats));
        }
        // if cannot unbind/detach, something has gone horribly wrong with this interface, so
        // remove it from the vector
        if (unbind(fd)  || 
            detach(fd))
        {
            SCX_LOGERROR(m_log, std::wstring(L"Unable to unbind and detach for PPA=").append(SCXCoreLib::StrFrom(i->dl_ppa)));
            i = ppaInfoList.erase(i);
            continue;
        }
        
        // once here, add the data we've acquired to the stats vector
        newEntry.ppa = i->dl_ppa;
        newEntry.name = std::string((char *)i->dl_module_id_1);
        newEntry.stats = iStats;
        newEntry.collisions = collisions;
        validStats.push_back(newEntry);
        i++;
    }
    
    closeDLPI(fd);
    
    return validStats;
}
 
bool SCXdlpi::get_cur_link_speed(std::wstring& interface_name,
                                 dl_hp_get_drv_param_ioctl_t& cmd_info)
{
    int rtn = false; // return value.

    // get vector of valid PPAs
    std::vector<DLPIStatsEntry> validStats = GetAllLANStats();
    
    // open DLPI driver
    int fd = openDLPI("/dev/dlpi");
    if (fd == -1)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eError, SCXCoreLib::eTrace);
        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(SCXCoreLib::StrFrom(errno)));
        SCX_LOG(m_log, severity, std::wstring(L"Unable to open /dev/dlpi with O_RDWR flag passed, errno=").append(SCXCoreLib::StrFrom(errno)));
        return rtn;
    }
    
    // Search through the PPA list to find the interface in question.
    for (std::vector<DLPIStatsEntry>::iterator i = validStats.begin(); i != validStats.end(); ++i)
    {
        // The PPA name is name+number.
        std::string namePPA;
        std::stringstream ppaStream;
        ppaStream << i->name << i->ppa;
        ppaStream >> namePPA;

        if (strcmp(SCXCoreLib::StrToUTF8(interface_name).c_str(),namePPA.c_str()) == 0)
        {
           /* We have to attach to the PPA before issuing the command. */
            if (attach(fd, i->ppa))
            {
                // This is to log file name and line number along with the rest of error message.
                SCXCoreLib::SCXErrnoException e(std::wstring(L"Could not attach. PPA: ").
                                                        append(SCXCoreLib::StrFromUTF8(namePPA)).
                                                        append(std::wstring(L" errno: ")),
                                                        errno, 
                                                        SCXSRCLOCATION);
                SCX_LOGERROR(m_log, e.What());
            }
            /*
             * Zero out the cmd_info before filling it up with the IOCTL
             * information.
             * */
            memset(&cmd_info, 0, sizeof(dl_hp_get_drv_param_ioctl_t));
            /*
             * Set the strioctl element to appropriate values.
             * */
            struct strioctl strioctl;
            strioctl.ic_cmd = DL_HP_GET_DRV_PARAM_IOCTL;
            strioctl.ic_len = sizeof(cmd_info);
            strioctl.ic_timout = 0;
            strioctl.ic_dp = (char *)&cmd_info;
            cmd_info.dl_request = DL_HP_DRV_SPEED;
            if (m_deps->ioctl(fd, I_STR, &strioctl) >= 0) 
            {
                rtn = true;
            }
            else
            {
                SCXCoreLib::SCXErrnoException e(std::wstring(L"ioctl(DL_HP_DRV_SPEED) failed. errno= "),
                                                 errno, SCXSRCLOCATION);
                SCX_LOGERROR(m_log, e.What());
            }

            // Detach before closing.
            if (detach(fd))
            {
                SCXCoreLib::SCXErrnoException e(std::wstring(L"Unable to Detach for PPA= ").
                                                        append(SCXCoreLib::StrFromUTF8(namePPA)).
                                                        append(L" errno: "),
                                                        errno, SCXSRCLOCATION);
                SCX_LOGERROR(m_log, e.What());
            }
            break;
        }
    } // for loop()

    closeDLPI(fd);

    return rtn;

} // SCXdlpi::get_cur_link_speed()

/////////////////////////////////////////
// private helper functions begin here //
/////////////////////////////////////////

/**
    This is used for dependency injection
    
    \param      fd - file descriptor
    \returns    the value that m_deps->open returns
*/
int SCXdlpi::openDLPI(const char* devPath)
{
    return m_deps->open(devPath, O_RDWR);
}

/**
    This is used for dependency injection
    
    \param      fd - file descriptor
    \returns    the value that m_deps->close returns
*/
int SCXdlpi::closeDLPI(int fd)
{
    return m_deps->close(fd);
}

/**
    This produces a std::list, and adds to it every element in the dl_hp_ppa_info_t array
    that getMessage delivers
    
    \param      fd - file descriptor
    \returns    std::list containing all of the dl_hp_ppa_info_t data from getMessage
*/
PPAInfoList SCXdlpi::getPPAInfoList(int fd)
{
    PPAInfoList ppaList;
    
    dl_hp_ppa_req_t * ppaREQ = (dl_hp_ppa_req_t *)m_control.buf;
    dl_hp_ppa_ack_t * ppaACK = (dl_hp_ppa_ack_t *)m_control.buf;
    ppaREQ->dl_primitive = DL_HP_PPA_REQ;
    
    if (putControlMessage(fd, sizeof(dl_hp_ppa_req_t), 0) ||
        getMessage(fd) == ERROR ||
        testControlPrimitive(DL_HP_PPA_ACK))
    {
        SCX_LOGERROR(m_log, std::wstring(L"Unable to enumerate PPAs from DLPI, errno=").append(SCXCoreLib::StrFrom(errno))); 
        return ppaList;
    }
    
    dl_hp_ppa_info_t * ppaArray = (dl_hp_ppa_info_t *)(ppaACK->dl_offset + m_control.buf);
    
    // put the ppa infos into a vector
    int ppaArray_count = ppaACK->dl_count;
    for (int i=0; i < ppaArray_count; i++)
    {
        ppaList.push_back(ppaArray[i]);
    }
    
    return ppaList;
}

/**
    Attaches a PPA to the current DLPI connection
    
    \param      fd - file descriptor
    \param      ppa - the ppa ID to attach to
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::attach(int fd, int ppa)
{
    dl_attach_req_t * attachREQ = (dl_attach_req_t *)m_control.buf;
    dl_ok_ack_t * okACK = (dl_ok_ack_t *)m_control.buf;
    
    attachREQ->dl_primitive = DL_ATTACH_REQ;
    attachREQ->dl_ppa = ppa;
    
    if (putControlMessage(fd, sizeof(dl_attach_req_t), 0) ||
        getMessage(fd) == ERROR ||
        testControlPrimitive(DL_OK_ACK))
    {
        return ERROR;
    }
    
    return 0;
}

/**
    Binds the DLPI connection to the currently attached PPA
    
    \param      fd - file descriptor
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::bind(int fd)
{
    
    dl_bind_req_t * bindREQ = (dl_bind_req_t *)m_control.buf;
    dl_bind_ack_t * bindACK = (dl_bind_ack_t *)m_control.buf;
    memset((char*)bindREQ, 0, sizeof(dl_bind_req_t));
    bindREQ->dl_primitive = DL_BIND_REQ;
    bindREQ->dl_sap = 22;
    bindREQ->dl_service_mode = DL_CODLS;
    bindREQ->dl_max_conind = 1;
    
    if (putControlMessage(fd, sizeof(dl_bind_req_t), 0) ||
        getMessage(fd) == ERROR    ||
        testControlPrimitive(DL_BIND_ACK))
    {
        return ERROR;
    }
    
    return 0;
}

/**
    Gets the stats for the currently attached PPA

    \param      fd - file descriptor
    \param      iStats - the mib statistics that is returned to the caller
    \param      collisions - the collision statistics that is returned to the caller
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::getstats(int fd, mib_ifEntry & iStats, unsigned int & collisions)
{
    dl_get_statistics_req_t * statREQ = (dl_get_statistics_req_t *)m_control.buf;
    dl_get_statistics_ack_t * statACK = (dl_get_statistics_ack_t *)m_control.buf;
    statREQ->dl_primitive = DL_GET_STATISTICS_REQ;
    
    if (putControlMessage(fd, sizeof(dl_get_statistics_req_t), 0) ||
        getMessage(fd) == ERROR              ||
        testControlPrimitive(DL_GET_STATISTICS_ACK))
    {
        return ERROR;
    }
    
    mib_ifEntry * stats = (mib_ifEntry *)(statACK->dl_stat_offset + m_control.buf);
    
    // This is slightly ugly, but it's the way that it's implemented on HPUX 11 machines.
    // go to the end of the mib_ifEntry, and that is where the mib_Dot3StatsEntry begins
    mib_Dot3StatsEntry * dot3stats = (mib_Dot3StatsEntry *) (reinterpret_cast<intptr_t>(stats) + sizeof(mib_ifEntry) );
    collisions = dot3stats->dot3StatsLateCollisions + 
                 dot3stats->dot3StatsExcessiveCollisions +
                 dot3stats->dot3StatsExcessCollisions;

    iStats = *stats;
    
    return 0;
}

/**
    Unbinds the dlpi connection

    \param      fd - file descriptor
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::unbind(int fd)
{
    dl_unbind_req_t * unbindREQ = (dl_unbind_req_t *)m_control.buf;
    dl_ok_ack_t * okACK = (dl_ok_ack_t *)m_control.buf;
    unbindREQ->dl_primitive = DL_UNBIND_REQ;
    
    if (putControlMessage(fd, sizeof(dl_unbind_req_t), 0) ||
        getMessage(fd) == ERROR      ||
        testControlPrimitive(DL_OK_ACK))
    {
        return ERROR;
    }
    
    return 0;
}

/**
    Detaches current PPA from the dlpi connection

    \param      fd - file descriptor
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::detach(int fd)
{          
    dl_detach_req_t * detachREQ = (dl_detach_req_t *)m_control.buf;
    dl_ok_ack_t * okACK = (dl_ok_ack_t *)m_control.buf;
    detachREQ->dl_primitive = DL_DETACH_REQ;
    
    if (putControlMessage(fd, sizeof(dl_detach_req_t), 0) ||
        getMessage(fd) == ERROR      ||
        testControlPrimitive(DL_OK_ACK))
    {
        return ERROR;
    }
    
    return 0;
}

/**
    Get a message from a stream; return type of message
    Allocate more space in m_control.buf if getmsg doesn't have enough space
    to put all of its return data

    \param      fd - file descriptor
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::getMessage(int fd)
{
    int flags = 0;
    int result;
    int status;
    m_control.buf[0] = 0;
    m_control.len = 0;
    status = 0;
    
    // use this tempStrBuf to hold the current location that we are writing to in m_control.buf
    // initialize it to be equal to m_control
    struct strbuf tempStrBuf;
    tempStrBuf.maxlen = m_control.maxlen;
    tempStrBuf.len = 0;
    tempStrBuf.buf = m_control.buf;
    
    while ( (result = m_deps->getmsg(fd, &tempStrBuf, 0, &flags)) == MORECTL )
    { 
        // keep doubling the size of the array until we get all data from getmsg
        
        // idea behind algorithm:
        // tempStrBuf.maxlen = size of additional space allocated
        // tempStrBuf.buf = m_control.buf's new location after realloc + the previous size of the array
        // m_control.buf grows like expected from a realloc, likewise m_control.maxlen
        tempStrBuf.maxlen = ms_bufsize * sizeof(u_long);

        ms_bufsize *= 2;
        
        // reallocate memory
        m_control.buf = (char *)realloc(m_control.buf, ms_bufsize * sizeof(u_long));
        m_control.maxlen = ms_bufsize * sizeof(u_long);

        // set the tempStrBuf position to be at the newly allocated memory location to write to
        tempStrBuf.buf = m_control.buf + tempStrBuf.maxlen;
    }
    
    if (result < 0)
    {
        return ERROR;
    }
    return 0;
}

/**
    Verify that dl_primitive in m_control.buf = prim

    \param      prim - DLPI primitive
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::testControlPrimitive(int prim)
{
    dl_error_ack_t *errorAck = (dl_error_ack_t *)m_control.buf;
    if (errorAck->dl_primitive != prim)
    {
        return ERROR;
    }
    return 0;
}

/**
    Put a control message on a stream

    \param      fd - DLPI device file descriptor
    \param      len - length of control message
    \param      pri - message type
    \returns    0 if OK, ERROR otherwise
*/
int SCXdlpi::putControlMessage(int fd, int len, int pri)
{
    m_control.len = len;
    if (m_deps->putmsg(fd, &m_control, 0, pri) < 0)
    {
        return ERROR;
    }
    return  0;
}

#endif
