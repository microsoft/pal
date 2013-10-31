/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/

//compile:
//#g++ ioctl_scan.cpp -o ioctl_scan
//run:
//#sudo ./ioctl_scan /dev/hda

//#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cctype>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <linux/types.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <dirent.h>
#include <linux/cdrom.h>

using namespace std;

std::string GetCDIds()
{
    cout<<"GetCDIds()--------------->"<<endl;
    errno = 0;
    string CDIds;
    DIR *pd = NULL;
    pd = opendir("/dev");
    if (pd == NULL)
    {
        cout<<"opendir(\"/dev\") "<<pd<<" "<<errno<<endl;
        cout<<"GetCDIds()---------------<"<<endl;
        return CDIds;
    }
    struct dirent *pde = NULL;
    while(pde = readdir(pd))
    {
//cout<<"  --"<<pde->d_name<<"--"<<endl;
        int ret = 0;
        struct stat fileStatus;
        memset(&fileStatus, 0, sizeof(fileStatus));
        string fullName = "/dev/";
        fullName += pde->d_name;
        ret = lstat(fullName.c_str(), &fileStatus);
//cout<<"  ----"<<endl;
        if (ret != 0)
        {
            cout<<"stat() "<<ret<<" "<<errno<<endl;
            continue;
        }
        if (!(S_ISBLK(fileStatus.st_mode)))
        {
            continue;
        }
        int fd;
        fd = open(fullName.c_str(), O_RDONLY|O_NONBLOCK);
        if (fd == -1)
        {
            cout<<"open("<<fullName<<") "<<fd<<endl;
            continue;
        }
//        cout<<pde->d_name<<" "<<S_ISCHR(fileStatus.st_mode)<<" "<<S_ISBLK(fileStatus.st_mode)<<" "<<fileStatus.st_mode<<endl;
        ret=ioctl(fd, CDROM_GET_CAPABILITY, 0);
        cout<<"CDROM_GET_CAPABILITY "<<ret<<" "<<errno<<endl;
        if (ret != -1)
        {
            cout<<"####################################################################################### "<<endl;
            cout<<"Detected CD-ROM: "<<fullName<<endl;
        }
        
        close(fd);
    }
    cout<<"readdir() errno"<<errno<<endl;
    cout<<"GetCDIds()---------------<"<<endl;
}

// HDIO power mode codes:
static const unsigned char POWERMODE_UNSET    = (unsigned char)0xF0;
static const unsigned char POWERMODE_STANDBY  = (unsigned char)0x00;
static const unsigned char POWERMODE_SPINDOWN = (unsigned char)0x40;
static const unsigned char POWERMODE_SPINUP   = (unsigned char)0x41;
static const unsigned char POWERMODE_IDLE     = (unsigned char)0x80;
static const unsigned char POWERMODE_ACTIVE   = (unsigned char)0xFF;

__u8 DriveCmdATAPowerMode(int fd, __u8 modeCmd)
{
    __u8 args[4]   = {0, 0, 0, 0};
    __u8 powerMode = POWERMODE_UNSET;

    args[0] = modeCmd;

    // get power state of ata driver.
    if(ioctl(fd, HDIO_DRIVE_CMD, args) == 0)
    {
       powerMode = args[2];
    }
    else
    {
        if (errno == EIO && args[0] == 0 && args[1] == 0)
        {
            powerMode = POWERMODE_STANDBY;
        }
    }

    return powerMode;
}

bool CheckATAPowerMode(int fd, string &availability)
{
    availability = "Unknown";

    __u8 powerMode = POWERMODE_UNSET;

    // get power state of ata driver.
    if (POWERMODE_UNSET == (powerMode = DriveCmdATAPowerMode(fd, WIN_CHECKPOWERMODE1)))
    {
        if (POWERMODE_UNSET == (powerMode = DriveCmdATAPowerMode(fd, WIN_CHECKPOWERMODE2)))
        {
        }
    }

    switch(powerMode)
    {
        case POWERMODE_STANDBY:  // Device is in Standby mode.
            availability = "PowerSave_Standby";
            break;
        case POWERMODE_SPINDOWN: // Device is in NV Cache Power Mode and the spindle is spun own or spinning down.
        case POWERMODE_SPINUP:   // Device is in NV Cache Power Mode and the spindle is spun up or spinning up.
            availability = "PowerSave_LowPowerMode";
            break;
        case POWERMODE_IDLE:     // Device is in Idle mode.
        case POWERMODE_ACTIVE:   // Device is in Active mode or Idle mode.
            availability = "RunningOrFullPower";
            break;
        default: 
            availability = "Unknown";
    }
    if(availability!="Unknown")
    {
        return true;
    }
    return false;
}

string HostName()
{
    string hostName;
    char hostNameBuff[256];
//    char hostNameBuff[HOST_NAME_MAX + 1];
    if(gethostname(hostNameBuff, sizeof(hostNameBuff)) == 0)
    {
        hostNameBuff[sizeof(hostNameBuff)-1] = 0;
        hostName = hostNameBuff;
    }
    else
    {
        hostName = "Failed to get hostname.";
    }
    return hostName;
}

string DataString(const void* dataVal, size_t dataSize)
{
    string dataString;
    const char *dataChar = reinterpret_cast<const char*>(dataVal);
    size_t i;
    for(i = 0; i < dataSize; i++)
    {
        char c = dataChar[i];
        if(c >= ' ' && c <= '~')
        {
            dataString += c;
        }
        else
        {
            dataString += '.';
        }
    }
    return dataString;
}

template<class tRet, class tErr> void WriteOneRecordS(
        const char* hdName, const char* valName, tRet retVal, tErr errVal,
        void* dataVal, size_t dataSize)
{
    cout<<"PHYS_HD_SCAN"<<';'<<HostName()<<';'<<hdName<<';'<<valName<<';'<<
            retVal<<';'<<errVal<<';'<<DataString(dataVal, dataSize)<<endl;
}        

template<class tRet, class tErr, class tData> void WriteOneRecord(
        const char* hdName, const char* valName, tRet retVal, tErr errVal, const tData &dataVal)
{
    cout<<"PHYS_HD_SCAN"<<';'<<HostName()<<';'<<hdName<<';'<<valName<<';'<<
            retVal<<';'<<errVal<<';'<<dataVal<<endl;
}

/**
   SCSI Generic(sg) inquery.
   Get the data retrieved from the SCSI INQUIRY command.

   \param[in] page vital product data page code
   \param[in] evpd if set the evpd bit of the command
   \param[in|out] dxferp INQUIRY data
   \param[in] dxfer_len length of the dxferp in bytes

   \return true if inquery succeeded, false if ioctl fail or error happened.
*/
bool SqInq(const char *dev, int fd, int page, int evpd, void * dxferp, unsigned short dxfer_len, bool SCSIPowerMode,
    string &availability)
{
    availability = "Unknown";
    /* SCSI INQUIRY command has 6 bytes,  OPERATION CODE(12h) */
    unsigned char inqCmdBlk[6] = {0x12, 0, 0, 0, 0, 0 };
    if(SCSIPowerMode)
        inqCmdBlk[0] = 0x03;
    unsigned char sense_b[32]; /* 32 bytes is enough for test result */
    sg_io_hdr_t   io_hdr;

    if (evpd)
        inqCmdBlk[1] |= 1;      /* enable evpd, at bit 0 byte 1 */
    inqCmdBlk[2] = (unsigned char) page;    /* page code in byte 2 */
    inqCmdBlk[3] = (unsigned char)((dxfer_len >> 8) & 0xff); /* allocation length, MSB */
    inqCmdBlk[4] = (unsigned char)(dxfer_len & 0xff);        /* allocation, LSB */

    memset(&io_hdr, 0, sizeof(io_hdr));
    memset(sense_b, 0, sizeof(sense_b));

    io_hdr.interface_id    = 'S';
    io_hdr.cmd_len         = sizeof(inqCmdBlk);
    io_hdr.mx_sb_len       = sizeof(sense_b);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len       = dxfer_len;
    io_hdr.dxferp          = dxferp;
    io_hdr.cmdp            = inqCmdBlk;
    io_hdr.sbp             = sense_b;
    io_hdr.timeout         = 30000; /* 30 seconds */

    int ret = ioctl(fd, SG_IO, &io_hdr);
    if (page == 0 && evpd == 0 && SCSIPowerMode == false)
    {
        WriteOneRecord(dev, "ioctl(SG_IO, 0, 0)", ret, (ret == 0)?0:errno, 0);
    }
    else if (page == 0x80 && evpd == 1 && SCSIPowerMode == false)
    {
        WriteOneRecord(dev, "ioctl(SG_IO, 0x80, 1)", ret, (ret == 0)?0:errno, 0);
    }
    else if(SCSIPowerMode)
    {
        WriteOneRecord(dev, "ioctl(SG_IO, 0, 0, powermode)", ret, (ret == 0)?0:errno, 0);
    }
    if (ret < 0)
    {
        cout<<"SG_IO "<<ret<<" "<<errno<<endl;
        cout<<"EINVAL = "<<EINVAL<<endl;
        return false;
    }
    cout<<"SG_IO "<<ret<<" "<<errno<<endl;
    if(!SCSIPowerMode)
    {
        if(io_hdr.status == 0 &&io_hdr.host_status == 0 && io_hdr.driver_status == 0)
        {
            return true;
        }
        else
        {
            /* refer spc-4 chapter 4.5.1 */
            unsigned char sense_key;
            if(sense_b[0] & 0x2) /* for response code 72h and 73h, sense key in byte 1*/
            {
                sense_key = (unsigned char)(sense_b[1] & 0xf);
            }
            else                /* for response code 72h and 73h, sense key in byte 2 */
            {
                sense_key = (unsigned char)(sense_b[2] & 0xf);
            }
            /* 1h RECOVERED ERROR: Indicates that the command completed
               successfully, with some recovery action performed by the
               device server. */
            if(sense_key == 0x01)
            {
                return true;
            }
            return false;
        }
    }
    else
    {
        if(!(io_hdr.masked_status == GOOD &&io_hdr.host_status == 0 && io_hdr.driver_status == 0))
        {
            return false;
        }
        unsigned char sense_key = (unsigned char)(((unsigned char*)dxferp)[2] & 0x0f);
        if(sense_key == 0)
        {
            availability = "RunningOrFullPower";
            return true;
        }
        unsigned char ASC = ((unsigned char*)dxferp)[12];
        unsigned char ASCQ = ((unsigned char*)dxferp)[13];
        switch (ASC)
        {
            case 0x04: // NOT READY 
                if ( ASCQ == 0x09 ) // SELF-TEST IN PROGRESS
                {
                    availability = "InTest";
                    return true;
                }
                else if ( ASCQ == 0x12 ) // OFFLINE
                {
                    availability = "OffLine";
                    return true;
                }
                break;

            case 0x0B: // WARNING
                availability = "Warning";
                return true;

            case 0x5E:  
                if ( ASCQ == 0x00 ) // LOW POWER CONDITION ON 
                {
                    availability = "PowerSave-LowPowerMode";
                    return true;
                }
                else if ( ASCQ == 0x41 ||
                          ASCQ == 0x42 ) // POWER STATE CHANGE TO ACTIVE OR IDLE
                {
                    availability =  "RunningOrFullPower";
                    return true;
                }
                else if ( ASCQ == 0x43 ) // POWER STATE CHANGE TO STANDBY 
                {
                    availability =  "PowerSave-Standby";
                    return true;
                }
                break;

            default:
                availability = "Unknown";
                break;
        }
        return false;
    }
}

void writeString(const char* mem, size_t sz)
{
    size_t i;
    for(i = 0; i < sz; i++)
    {
        if((i%64) == 0)
        {
            cout<<" ";
        }
        char c = mem[i];
        if(c >= ' ' && c <= '~')
        {
            cout << c;
        }
        else
        {
            cout << '.';
        }
        if((((i+1)%64) == 0) || ((i+1) == sz))
        {
            cout<<endl;
        }
    }
}

int main(int argc, char* argv[])
{
    std::string cdIds = GetCDIds();

    if(argc < 2)
    {
        cout<<"Error, use with device path parameter."<<endl;
        return -1;
    }
    cout<<"---------------------------------------------------"<<endl;
    char *dev = argv[1];
    int fd;

    fd = open(dev, O_RDONLY|O_NONBLOCK);
//    fd = open(dev, 3);
    cout<<"open("<<dev<<") "<<fd<<" errno: "<<errno<<endl;
    if(fd == -1)return -2;
    WriteOneRecord(dev, "open()", fd, (fd >= 0)?0:errno, fd);

    int ret=0;

    int ver=0;
    ret=ioctl(fd, SG_GET_VERSION_NUM, &ver);
    cout<<"SG_GET_VERSION_NUM "<<ret<<" "<<errno<<" ver: "<<ver<<endl;
    WriteOneRecord(dev, "ioctl(SG_GET_VERSION_NUM)", ret, (ret == 0)?0:errno, ver);
    int emulated=0;
    ret=ioctl(fd, SG_EMULATED_HOST, &emulated);
    cout<<"SG_EMULATED_HOST "<<ret<<" "<<errno<<" emulated: "<<emulated<<endl;
    WriteOneRecord(dev, "ioctl(SG_EMULATED_HOST)", ret, (ret == 0)?0:errno, emulated);

    string availability;

    unsigned char rsp_buff[255];
    memset(rsp_buff, 0, sizeof(rsp_buff));
    bool r=SqInq(dev, fd, 0, 0, rsp_buff, sizeof(rsp_buff), false, availability);
    writeString(reinterpret_cast<char *>(rsp_buff), sizeof(rsp_buff));
    WriteOneRecordS(dev, "SqInq(0, 0)", r, 0, rsp_buff, sizeof(rsp_buff));
    string productRevisionLevel(reinterpret_cast<char*>(rsp_buff)+32, 4);
    WriteOneRecord(dev, "SqInq(0, 0).ProductRevLev", r, 0, productRevisionLevel.c_str());
    string manufacturer(reinterpret_cast<char*>(rsp_buff)+8, 8);
    WriteOneRecord(dev, "SqInq(0, 0).manufacturer", r, 0, manufacturer.c_str());
    string productIdentification(reinterpret_cast<char*>(rsp_buff)+16, 16);
    WriteOneRecord(dev, "SqInq(0, 0).ProductID", r, 0, productIdentification.c_str());
    WriteOneRecord(dev, "SqInq(0, 0).[1]", r, 0, static_cast<int>(rsp_buff[1]));
    WriteOneRecord(dev, "SqInq(0, 0).[1].bit7[removable]", r, 0, static_cast<int>(rsp_buff[1]&(1<<7)));
    
    memset(rsp_buff, 0, sizeof(rsp_buff));
    r=SqInq(dev, fd, 0x80, 1, rsp_buff, sizeof(rsp_buff), false, availability);
    cout<<"  rsp_buff[3]: "<<static_cast<unsigned int>(rsp_buff[3])<<endl;
    writeString(reinterpret_cast<char *>(rsp_buff), sizeof(rsp_buff));
    WriteOneRecordS(dev, "SqInq(0x80, 1)", r, 0, rsp_buff, sizeof(rsp_buff));
    string serialNumber(reinterpret_cast<char*>(rsp_buff)+4, rsp_buff[3]);
    WriteOneRecord(dev, "SqInq(0x80, 1).[3]", r, 0, static_cast<int>(rsp_buff[3]));
    WriteOneRecord(dev, "SqInq(0x80, 1).serialNumber", r, 0, serialNumber.c_str());

    unsigned char rsp_buff_pm[252];
    memset(rsp_buff_pm, 0, sizeof(rsp_buff_pm));
    r=SqInq(dev, fd, 0, 0, rsp_buff_pm, sizeof(rsp_buff_pm), true, availability);
    WriteOneRecordS(dev, "SqInq(0, 0, powermode)", r, 0, rsp_buff_pm, sizeof(rsp_buff_pm));
    WriteOneRecord(dev, "SqInq(0, 0, powermode)[2]", r, 0, static_cast<int>(rsp_buff_pm[2]));
    WriteOneRecord(dev, "SqInq(0, 0, powermode)[12]", r, 0, static_cast<int>(rsp_buff_pm[12]));
    WriteOneRecord(dev, "SqInq(0, 0, powermode)[13]", r, 0, static_cast<int>(rsp_buff_pm[13]));
    WriteOneRecord(dev, "Availability(SG_IO)", r, 0, availability);    

    int io32bit=0;
    ret=ioctl(fd, HDIO_GET_32BIT, &io32bit);
    cout<<"HDIO_GET_32BIT "<<ret<<" "<<errno<<" io32bit: "<<io32bit<<endl;
    WriteOneRecord(dev, "ioctl(HDIO_GET_32BIT)", ret, (ret == 0)?0:errno, io32bit);

    int ro=0;
    ret=ioctl(fd, BLKROGET, &ro);
    cout<<"BLKROGET "<<ret<<" "<<errno<<" ro: "<<ro<<endl;
    WriteOneRecord(dev, "ioctl(BLKROGET)", ret, (ret == 0)?0:errno, ro);

    const size_t MBR_LEN = 512;
    unsigned char mbrbuf[MBR_LEN];
    ssize_t readret = read(fd, mbrbuf, sizeof(mbrbuf));
    WriteOneRecord(dev, "read(512)[510] - 0x55aa - 43605 - MBR", readret, (readret >= 0)?0:errno, *reinterpret_cast<unsigned short *>(mbrbuf+510));
    
    __u8 args[4];
    memset(args, 0, sizeof(args));
    args[0] = WIN_CHECKPOWERMODE1;
    ret = ioctl(fd, HDIO_DRIVE_CMD, args);
    WriteOneRecord(dev, "ioctl(HDIO_DRIVE_CMD, WIN_CHECKPOWERMODE1)[0]", ret, (ret == 0)?0:errno, static_cast<int>(args[0]));
    WriteOneRecord(dev, "ioctl(HDIO_DRIVE_CMD, WIN_CHECKPOWERMODE1)[1]", ret, (ret == 0)?0:errno, static_cast<int>(args[1]));
    WriteOneRecord(dev, "ioctl(HDIO_DRIVE_CMD, WIN_CHECKPOWERMODE1)[2]", ret, (ret == 0)?0:errno, static_cast<int>(args[2]));
    
    memset(args, 0, sizeof(args));
    args[0] = WIN_CHECKPOWERMODE2;
    ret = ioctl(fd, HDIO_DRIVE_CMD, args);
    WriteOneRecord(dev, "ioctl(HDIO_DRIVE_CMD, WIN_CHECKPOWERMODE2)[0]", ret, (ret == 0)?0:errno, static_cast<int>(args[0]));
    WriteOneRecord(dev, "ioctl(HDIO_DRIVE_CMD, WIN_CHECKPOWERMODE2)[1]", ret, (ret == 0)?0:errno, static_cast<int>(args[1]));
    WriteOneRecord(dev, "ioctl(HDIO_DRIVE_CMD, WIN_CHECKPOWERMODE2)[2]", ret, (ret == 0)?0:errno, static_cast<int>(args[2]));

    r = CheckATAPowerMode(fd, availability);
    WriteOneRecord(dev, "Availability(HDIO_DRIVE_CMD)", r, 0, availability);    

    struct hd_driveid id;
    memset(&id,0,sizeof(id));
    ret=ioctl(fd, HDIO_GET_IDENTITY, &id);
    cout<<"HDIO_GET_IDENTITY "<<ret<<" "<<errno<<endl;
    writeString(reinterpret_cast<char *>(&id), sizeof(id));
    cout<<"  heads: "<<id.heads<<endl;
    cout<<"  sectors: "<<id.sectors<<endl;
    cout<<"  cyls: "<<id.cyls<<endl;
    cout<<"  cur_heads: "<<id.cur_heads<<endl;
    cout<<"  cur_sectors: "<<id.cur_sectors<<endl;
    cout<<"  cur_cyls: "<<id.cur_cyls<<endl;
    cout<<"  serial_no: \""<<string(reinterpret_cast<char*>(id.serial_no), sizeof(id.serial_no))<<"\""<<endl;
    cout<<"  fw_rev: \""<<string(reinterpret_cast<char*>(id.fw_rev), sizeof(id.fw_rev))<<"\""<<endl;
    cout<<"  model: \""<<string(reinterpret_cast<char*>(id.model), sizeof(id.model))<<"\""<<endl;
    WriteOneRecordS(dev, "ioctl(HDIO_GET_IDENTITY)", ret, (ret == 0)?0:errno, &id, sizeof(id));
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).heads", ret, (ret == 0)?0:errno, id.heads);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).sectors", ret, (ret == 0)?0:errno, id.sectors);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cyls", ret, (ret == 0)?0:errno, id.cyls);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cur_heads", ret, (ret == 0)?0:errno, id.cur_heads);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cur_sectors", ret, (ret == 0)?0:errno, id.cur_sectors);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cur_cyls", ret, (ret == 0)?0:errno, id.cur_cyls);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).serial_no", ret, (ret == 0)?0:errno, id.serial_no);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).fw_rev", ret, (ret == 0)?0:errno, id.fw_rev);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).model", ret, (ret == 0)?0:errno, id.model);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).config", ret, (ret == 0)?0:errno, id.config);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).config.bit7[removable]", ret, (ret == 0)?0:errno, id.config&(1<<7));
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).command_set_1", ret, (ret == 0)?0:errno, id.command_set_1);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).command_set_1.bit3[power management feature]", ret, (ret == 0)?0:errno, id.command_set_1&(1<<3));
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cfs_enable_1", ret, (ret == 0)?0:errno, id.cfs_enable_1);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cfs_enable_1.bit3[power management enabled]", ret, (ret == 0)?0:errno, id.cfs_enable_1&(1<<3));
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cfs_enable_2", ret, (ret == 0)?0:errno, id.cfs_enable_2);
    WriteOneRecord(dev, "ioctl(HDIO_GET_IDENTITY).cfs_enable_2.bit5[power up in standby]", ret, (ret == 0)?0:errno, id.cfs_enable_2&(1<<5));
    
    struct hd_geometry geo;
    memset(&geo,0,sizeof(geo));
    ret=ioctl(fd, HDIO_GETGEO, &geo);
    cout<<"HDIO_GETGEO "<<ret<<" "<<errno<<endl;
    cout<<"  heads: "<<(int)geo.heads<<endl;
    cout<<"  sectors: "<<(int)geo.sectors<<endl;
    cout<<"  cylinders: "<<geo.cylinders<<endl;
    cout<<"  start: "<<geo.start<<endl;
    WriteOneRecordS(dev, "ioctl(HDIO_GETGEO)", ret, (ret == 0)?0:errno, &geo, sizeof(geo));
    WriteOneRecord(dev, "ioctl(HDIO_GETGEO).heads", ret, (ret == 0)?0:errno, static_cast<int>(geo.heads));
    WriteOneRecord(dev, "ioctl(HDIO_GETGEO).sectors", ret, (ret == 0)?0:errno, static_cast<int>(geo.sectors));
    WriteOneRecord(dev, "ioctl(HDIO_GETGEO).cylinders", ret, (ret == 0)?0:errno, geo.cylinders);
    WriteOneRecord(dev, "ioctl(HDIO_GETGEO).start", ret, (ret == 0)?0:errno, geo.start);
    
    u_int64_t ssz=0;
    ret=ioctl(fd, BLKSSZGET, &ssz);
    cout<<"BLKSSZGET "<<ret<<" "<<errno<<" block size: "<<ssz<<endl;
    WriteOneRecord(dev, "ioctl(BLKSSZGET)", ret, (ret == 0)?0:errno, ssz);
    
    u_int64_t bsz=0;
    ret=ioctl(fd, BLKBSZGET, &bsz);
    cout<<"BLKBSZGET "<<ret<<" "<<errno<<" physical block size: "<<bsz<<endl;
    WriteOneRecord(dev, "ioctl(BLKBSZGET)", ret, (ret == 0)?0:errno, bsz);
    
    unsigned long bgs=0;
    ret=ioctl(fd, BLKGETSIZE, &bgs);
    cout<<"BLKGETSIZE "<<ret<<" "<<errno<<" size/blk: "<<bgs<<endl;
    WriteOneRecord(dev, "ioctl(BLKGETSIZE)", ret, (ret == 0)?0:errno, bgs);
    
    u_int64_t bgs64=0;
    ret=ioctl(fd, BLKGETSIZE64, &bgs64);
    cout<<"BLKGETSIZE64 "<<ret<<" "<<errno<<" size/blk64: "<<bgs64<<endl;
    WriteOneRecord(dev, "ioctl(BLKGETSIZE64)", ret, (ret == 0)?0:errno, bgs64);

    //struct sg_scsi_id sg_scsi;
    Sg_scsi_id sg_scsi;
    memset(&sg_scsi, 0, sizeof(sg_scsi));
    ret=ioctl(fd, SG_GET_SCSI_ID, &sg_scsi);
    WriteOneRecordS(dev, "ioctl(SG_GET_SCSI_ID)", ret, (ret == 0)?0:errno, &sg_scsi, sizeof(sg_scsi));
    WriteOneRecord(dev, "ioctl(SG_GET_SCSI_ID).host_no", ret, (ret == 0)?0:errno, sg_scsi.host_no);
    WriteOneRecord(dev, "ioctl(SG_GET_SCSI_ID).channel", ret, (ret == 0)?0:errno, sg_scsi.channel);
    WriteOneRecord(dev, "ioctl(SG_GET_SCSI_ID).scsi_id", ret, (ret == 0)?0:errno, sg_scsi.scsi_id);
    WriteOneRecord(dev, "ioctl(SG_GET_SCSI_ID).lun", ret, (ret == 0)?0:errno, sg_scsi.lun);
    WriteOneRecord(dev, "ioctl(SG_GET_SCSI_ID).scsi_type", ret, (ret == 0)?0:errno, sg_scsi.scsi_type);

    unsigned int SCSIBus = 0;
    ret=ioctl(fd, SCSI_IOCTL_GET_BUS_NUMBER, &SCSIBus);
    WriteOneRecord(dev, "ioctl(SCSI_IOCTL_GET_BUS_NUMBER)", ret, (ret == 0)?0:errno, SCSIBus);
   
    struct my_scsi_idlun
    {
        __u32 dev_id;
        __u32 host_unique_id;
    }idlun;
    memset(&idlun, 0, sizeof(idlun));
    ret=ioctl(fd, SCSI_IOCTL_GET_IDLUN, &idlun);
    WriteOneRecord(dev, "ioctl(SCSI_IOCTL_GET_IDLUN).dev_id", ret, (ret == 0)?0:errno, idlun.dev_id);
    WriteOneRecord(dev, "ioctl(SCSI_IOCTL_GET_IDLUN).host_unique_id", ret, (ret == 0)?0:errno, idlun.host_unique_id);
    WriteOneRecord(dev, "ioctl(SCSI_IOCTL_GET_IDLUN).SCSILogicalUnit", ret, (ret == 0)?0:errno, (idlun.dev_id >> 8) & 0x00ff);
    WriteOneRecord(dev, "ioctl(SCSI_IOCTL_GET_IDLUN).SCSITargetId", ret, (ret == 0)?0:errno, idlun.dev_id & 0x00ff);
    
    errno = 0;
    ret=ioctl(fd, CDROM_GET_CAPABILITY, 0);
    cout<<"CDROM_GET_CAPABILITY "<<ret<<" "<<errno<<endl;
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_CLOSE_TRAY", ret, (ret == 0)?0:errno, ret&CDC_CLOSE_TRAY);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_OPEN_TRAY", ret, (ret == 0)?0:errno, ret&CDC_OPEN_TRAY);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_LOCK", ret, (ret == 0)?0:errno, ret&CDC_LOCK);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_SELECT_SPEED", ret, (ret == 0)?0:errno, ret&CDC_SELECT_SPEED);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_SELECT_DISC", ret, (ret == 0)?0:errno, ret&CDC_SELECT_DISC);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_MULTI_SESSION", ret, (ret == 0)?0:errno, ret&CDC_MULTI_SESSION);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_MCN", ret, (ret == 0)?0:errno, ret&CDC_MCN);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_MEDIA_CHANGED", ret, (ret == 0)?0:errno, ret&CDC_MEDIA_CHANGED);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_PLAY_AUDIO", ret, (ret == 0)?0:errno, ret&CDC_PLAY_AUDIO);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_RESET", ret, (ret == 0)?0:errno, ret&CDC_RESET);
//    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_IOCTLS", ret, (ret == 0)?0:errno, ret&CDC_IOCTLS);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_DRIVE_STATUS", ret, (ret == 0)?0:errno, ret&CDC_DRIVE_STATUS);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_GENERIC_PACKET", ret, (ret == 0)?0:errno, ret&CDC_GENERIC_PACKET);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_CD_R", ret, (ret == 0)?0:errno, ret&CDC_CD_R);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_CD_RW", ret, (ret == 0)?0:errno, ret&CDC_CD_RW);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_DVD", ret, (ret == 0)?0:errno, ret&CDC_DVD);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_DVD_R", ret, (ret == 0)?0:errno, ret&CDC_DVD_R);
    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_DVD_RAM", ret, (ret == 0)?0:errno, ret&CDC_DVD_RAM);
//    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_MO_DRIVE", ret, (ret == 0)?0:errno, ret&CDC_MO_DRIVE);
//    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_MRW", ret, (ret == 0)?0:errno, ret&CDC_MRW);
//    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_MRW_W", ret, (ret == 0)?0:errno, ret&CDC_MRW_W);
//    WriteOneRecord(dev, "ioctl(CDROM_GET_CAPABILITY)|CDC_RAM", ret, (ret == 0)?0:errno, ret&CDC_RAM);

    close(fd);

    cout<<"---------------------------------------------------"<<endl;
    return 0;
}

