/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       This file contains the abstraction of the smbios on Linux and Solaris x86. 
    
    \date        2011-03-21 16:51:51

    
*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxfile.h>
#include <scxsystemlib/scxsmbios.h>


namespace SCXSystemLib
{


    /*----------------------------------------------------------------------------*/
    /**
      Default constructor. 

     */
    SMBIOSPALDependencies::SMBIOSPALDependencies()
    { 
#if (defined(sun) && !defined(sparc))
        m_deviceName = L"/dev/xsvc";
#elif defined(linux)
        m_deviceName = L"/dev/mem";
#endif

        m_log = SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.common.scxsmbios"));
    }

    /*----------------------------------------------------------------------------*/
    /**
      Read Smbios Table Entry Point on non-EFI system,from 0xF0000 to 0xFFFFF in device file. 
     */
    bool SMBIOSPALDependencies::ReadSpecialMemory(MiddleData &buf) const
    {
        if(1 > buf.size()) return false;

        SCXFilePath devicePath(m_deviceName); 
        size_t length = cEndAddress - cStartAddress + 1;
        size_t offsetStart = cStartAddress;
        SCX_LOGTRACE(m_log, StrAppend(L"SMBIOSPALDependencies ReadSpecialMemory() - device name: ", m_deviceName));
        SCX_LOGTRACE(m_log, StrAppend(L"SMBIOSPALDependencies ReadSpecialMemory() - length: ", length));
        SCX_LOGTRACE(m_log, StrAppend(L"SMBIOSPALDependencies ReadSpecialMemory() - offsetStart: ", offsetStart));

        int readReturnCode = SCXFile::ReadAvailableBytesAsUnsigned(devicePath,&(buf[0]),length,offsetStart);
        if(readReturnCode == 0)
        {
            SCX_LOGTRACE(m_log, L"ReadSpecialMemory() - status of reading is: success");
        }
        else
        {
            SCX_LOGTRACE(m_log, L"ReadSpecialMemory() - status of reading is: failure");
            SCX_LOGTRACE(m_log, StrAppend(L"ReadSpecialMemory() - reason for read failure: ", readReturnCode));
            return false;
        }

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Read Smbios Table Entry Point on EFI system. 
      
      We will implement this function on EFI system later.
     */
    bool SMBIOSPALDependencies::ReadSpecialmemoryEFI(MiddleData &buf) const
    {
        //Just to avoid warning here
        bool bRet = true;

        buf.size();

        return bRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get SMBIOS Table content. 
     */
    bool SMBIOSPALDependencies::GetSmbiosTable(const struct SmbiosEntry& entryPoint,
            MiddleData &buf) const
    {
        if(1 > buf.size()) return false;

        //
        //Read Smbios Table from device system file according to info inside SmbiosEntry.
        //
        SCXFilePath devicePath(m_deviceName); 
        int readReturnCode = SCXFile::ReadAvailableBytesAsUnsigned(devicePath,&(buf[0]),
                entryPoint.tableLength,entryPoint.tableAddress);
        if(readReturnCode == 0)
        {
            SCX_LOGTRACE(m_log, L"GetSmbiosTable() -the status of reading is : success");
        }
        else
        {
            SCX_LOGTRACE(m_log, L"GetSmbiosTable() -the status of reading is : failure");
            SCX_LOGTRACE(m_log, StrAppend(L"GetSmbiosTable() - reason for read failure: ", readReturnCode));
            return false;
        }

        return true;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Default constructor.

     */
    SCXSmbios::SCXSmbios(SCXCoreLib::SCXHandle<SMBIOSPALDependencies> deps):
         m_deps(deps)
    {
        m_log = SCXLogHandleFactory::GetLogHandle(std::wstring(L"scx.core.common.pal.system.common.scxsmbios"));
    }

    /*----------------------------------------------------------------------------*/
    /**
      Parse SMBIOS Structure Table Entry Point. 

      Parameter[out]:  smbiosEntry- Part fields value of SMBIOS Structure Table Entry Point. 
      Returns:     whether it's successful to parse it. 
     */
    bool SCXSmbios::ParseSmbiosEntryStructure(struct SmbiosEntry &smbiosEntry)const
    {
        bool fRet = false;
        try
        {
            //
            //Read Smbios Table Entry Point on non-EFI system,from 0xF0000 to 0xFFFFF in device file.
            //
            size_t ilength = cEndAddress - cStartAddress + 1;
            MiddleData entryPoint(ilength);
            fRet = m_deps->ReadSpecialMemory(entryPoint);
            if (!fRet)
            {
                std::wstring sMsg(L"ParseSmbiosEntryStructure - Failed to read special memory.");
                SCX_LOGINFO(m_log, sMsg);
                smbiosEntry.smbiosPresent = false;
            }

            //
            //Searching for the anchor-string "_SM_" on paragraph (16-byte) boundaries mentioned in doc DSP0134_2.7.0.pdf
            //
            unsigned char* pbuf = &(entryPoint[0]);
            for(size_t i = 0; fRet && ((i + cParagraphLength) <= ilength); i += cParagraphLength)
            {
                if (memcmp(pbuf+i,"_SM_", cAnchorString) == 0)
                {
                    unsigned char *pcurBuf = pbuf+i;
                    // Before proceeding, verify that the SMBIOS is present.
                    // (Reference: (dmidecode.c ver 2.1: http://download.savannah.gnu.org/releases/dmidecode/)
                    if ( CheckSum(pcurBuf, pcurBuf[0x05]) && 
                         memcmp(pcurBuf+0x10, "_DMI_", cDMIAnchorString) == 0 &&
                         CheckSum(pcurBuf+0x10, 15))
                    {
                        SCX_LOGTRACE(m_log, std::wstring(L"SMBIOS is present."));
                        SCX_LOGTRACE(m_log, std::wstring(L"ParseSmbiosEntryStructure -anchor: _SM_"));
                        //
                        //Length of the Entry Point Structure.
                        //
                        size_t tmpLength = pcurBuf[cLengthEntry]; 
                        if(!CheckSum(pcurBuf,tmpLength))
                        {
                            throw SCXCoreLib::SCXInternalErrorException(L"Failed to CheckSum in ParseSmbiosEntryStructure().", SCXSRCLOCATION);
                        }

                        //
                        //Read the address,length and SMBIOS structure number of SMBIOS Structure Table.
                        //
                        unsigned int address = MAKELONG(MAKEWORD(pcurBuf+cAddressTable,pcurBuf+cAddressTable+1),MAKEWORD(pcurBuf+cAddressTable+2,pcurBuf+cAddressTable+3)); 
                        unsigned short length = MAKEWORD(pcurBuf+cLengthTable,pcurBuf+cLengthTable+1); 
                        unsigned short number = MAKEWORD(pcurBuf+cNumberStructures,pcurBuf+cNumberStructures+1); 

                        unsigned short majorVersion = pcurBuf[cMajorVersion];
                        unsigned short minorVersion = pcurBuf[cMiniorVersion];
                        smbiosEntry.majorVersion = majorVersion;
                        smbiosEntry.minorVersion = minorVersion;
                        smbiosEntry.smbiosPresent = true;
                        smbiosEntry.tableAddress = address;
                        SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosEntryStructure - address: ", address));
                        smbiosEntry.tableLength = length;
                        SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosEntryStructure - length: ", length));
                        smbiosEntry.structureNumber = number;
                        SCX_LOGTRACE(m_log, StrAppend(L"ParseSmbiosEntryStructure - number: ", number));
                    }

                    break;
                }
                else if (memcmp(pbuf+i, "_DMI_", cDMIAnchorString) == 0)
                {
                    SCX_LOGTRACE(m_log, std::wstring(L"Legacy DMI is present."));
                }
            }
            
        }
        catch(const SCXException& e)
        {
            throw SCXCoreLib::SCXInternalErrorException(L"Failed to parse Smbios Entry Structure." + e.What(), SCXSRCLOCATION);
        }
        return fRet;
    }


    /*----------------------------------------------------------------------------*/
    /**
      Check the checksum of the Entry Point Structure. 
      Checksum of the Entry Point Structure,added to all other bytes in the Entry Point Structure,
      results in the value 00h(using 8-bit addition calculations).
      Checksum is the value of the 4th byte, so the above algorithm is equal to add all the bytes in the Entry Point Structure.

      Parameter[in]:  pEntry- the address of Entry Point Structure.
      Parameter[in]:  length- the length of Entry Point Structure. 
      Returns:     true,the checksum is 0;otherwise,false. 
     */
    bool SCXSmbios::CheckSum(const unsigned char* pEntry,const size_t& length)const
    {
        unsigned char sum = 0;
        for(size_t i=0;i<length;++i)
        {
            sum = static_cast<unsigned char>(sum + pEntry[i]);
        }

        return (0 == sum);
    }


    /*----------------------------------------------------------------------------*/
    /**
      Read specified index string.

      Parameter[in]:  buf- SMBIOS Structure Table.
      Parameter[in]:  length- offset to the start address of SMBIOS Structure Table.
      Parameter[in]:  index- index of string. 
      Returns:     The string which the program read. 
     */
    std::wstring SCXSmbios::ReadSpecifiedString(const MiddleData& buf,const size_t& length,const size_t& index)const
    {
        std::wstring strRet = L"";
        if(1 > buf.size()) return strRet;

        unsigned char* ptablebuf = const_cast<unsigned char*>(&(buf[0])); 
        size_t curLength = length;

        size_t numstr = 1;
        while(numstr < index)
        {
            char *pcurstr = reinterpret_cast<char*>(ptablebuf+curLength);
            curLength = curLength + strlen(pcurstr);
            //
            // At last add 1 for the terminal char '\0'
            //
            curLength += 1;
            numstr++;
        }

        std::string curString = reinterpret_cast<char*>(ptablebuf + curLength); 
        strRet = StrFromUTF8(curString);
        SCX_LOGTRACE(m_log, StrAppend(L"ReadSpecifiedString() - ParsedStr is : ", strRet));

        return strRet;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Get SMBIOS Table content. 

      Parameter[in]:  entryPoint- EntryPoint Structure. 
      Parameter[out]:  buf- Smbios Table. 
      Returns:     whether it's successful to get smbios table. 
     */
    bool SCXSmbios::GetSmbiosTable(const struct SmbiosEntry& entryPoint,MiddleData &buf) const
    {
       return  m_deps->GetSmbiosTable(entryPoint,buf);
    }

    




}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
