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
#include <scxsystemlib/scxdlpi.h>

    #define INSAP 22 
    #define OUTSAP 24 
    #define CONTROL 1 
    #define DATA 2 
    #define BOTH 3 
    #define INTERRUPT 4 
    #define ERROR 128 
    #define MAX_PPA 10
    
/**
    Get a message from a stream; return type of message

    \param      fd - file descriptor
    \returns    0 if OK, -1 otherwise
*/
int dlpi::getMessage(int fd)
{
    int flags = 0;
    int result;
    int status;
    controlInfo[0] = 0;
    dataInfo[0] = 0;
    status = 0;
    result = getmsg(fd, &control, &data, &flags);
    if (result < 0)
    {
        if (errno == EINTR)
        {
            return(INTERRUPT);
        } else
        {
            return(ERROR);
        }
    }
    if (control.len > 0)
    {
        status |= CONTROL;
    }
    if (data.len > 0)
    {
        status |= DATA;
    }
    return (status);
}

/**
    Verify that dl_primitive in controlInfo = prim

    \param      prim - DLPI primitive (DL_INFO_REQ, DL_INFO_ACK ... etc.)
    \returns    0 if OK, -1 otherwise
*/
int dlpi::testControlPrimitive(int prim)
{
    dl_error_ack_t *errorAck = (dl_error_ack_t *)controlInfo;
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
    \returns    0 if OK, -1 otherwise
*/
int dlpi::putControlMessage(int fd, int len, int pri)
{
    control.len = len;
    if (putmsg(fd, &control, 0, pri) < 0)
    {
        return ERROR;
    }
    return  0;
}

/**
    Put a control + data message on a stream

    \param      fd - DLPI device file descriptor
    \param      controlLen - length of control message
    \param      dataLen - length of data message
    \param      pri - message type
    \returns    0 if OK, -1 otherwise
*/
int dlpi::putControlAndData(int fd, int controlLen, int dataLen, int pri)
{
    control.len = controlLen;
    data.len = dataLen;
    if (putmsg(fd, &control, &data, pri) < 0)
    {
        return ERROR;
    }
    return  0;
}

/**
    Open file descriptor and attach

    \param      device - DLPI device name
    \param      ppa - Physical Point of Attachment
    \param      fd - DLPI device file descriptor
    \returns    0 if OK, -1 otherwise
*/
int dlpi::openDLPI(const char *device, int ppa, int *fd)
{
    dl_attach_req_t *attachReq = (dl_attach_req_t *)controlInfo;
    if ((*fd = open(device, O_RDWR)) == -1)
    {
        return ERROR;
    }
    attachReq->dl_primitive = DL_ATTACH_REQ;
    attachReq->dl_ppa = ppa;
    putControlMessage(*fd, sizeof(dl_attach_req_t), 0);
    getMessage(*fd);
    return testControlPrimitive(DL_OK_ACK);
}

/**
    Send DL_BIND_REQ

    \param      fd - File Descriptor
    \param      sap - Service Access Point
    \param      bindAddr - Bind address
    \returns    0 if OK, -1 otherwise
*/
int dlpi::bindDLPI(int fd, int sap, u_char *bindAddr)
{
    dl_bind_req_t *bindReq = (dl_bind_req_t *)controlInfo;
    dl_bind_ack_t *bindAck = (dl_bind_ack_t *)controlInfo;
    bindReq->dl_primitive = DL_BIND_REQ;
    bindReq->dl_sap = sap;
    bindReq->dl_max_conind = 1;
    bindReq->dl_service_mode = DL_CLDLS;
    bindReq->dl_conn_mgmt = 0;
    bindReq->dl_xidtest_flg = 0;
    putControlMessage(fd, sizeof(dl_bind_req_t), 0);
    getMessage(fd);
    if (ERROR == testControlPrimitive(DL_BIND_ACK))
    {
        return ERROR;
    }
    memcpy
    (
        bindAddr,
        (u_char *)bindAck + bindAck->dl_addr_offset, 
        bindAck->dl_addr_length
    );
    return 0;
}

/**
    Get the mac address of matching interface

    \param       addr - an array of six bytes, has to be allocated by the caller
    \returns     0 if OK, -1 if the address could not be determined
*/
long dlpi::getMacAddress ( u_char  *addr)
{
    int fd;
    int ppa;
    u_char macAddress[25];
    int i;
    
    char **device;
    
    // Should work for AIX and SUN, but using their device files in place of /dev/dlpi
    if (ERROR != openDLPI("/dev/dlpi", 0, &fd))
    {
        if (ERROR != bindDLPI(fd, INSAP, macAddress))
        {
            memcpy( addr, macAddress, 6);
            close(fd);
            return 0;
        }
        close(fd);
    }
    return -1;
}
#endif
