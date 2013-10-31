/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       This file contains the abstraction of the smbios on Linux and Solaris x86. 
    
    \date        2011-03-21 16:51:51

    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXSMBIOS_H
#define SCXSMBIOS_H

#include <vector>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>


/** Make WORD or LONG Type by bytes.Two types of order of bytes: BIG ENDIAN or LITTLE ENDIAN*/
#ifdef BIGENDIAN
#define MAKEWORD(a,b) static_cast<unsigned short>(((a)[0] << 8) + ((b)[0])) 
#define MAKELONG(x,y) static_cast<unsigned int>(((x) << 16) + y)
#else
#define MAKEWORD(a,b) static_cast<unsigned short>(((a)[0]) + ((b)[0] <<8 )) 
#define MAKELONG(x,y) static_cast<unsigned int>(x + ((y) << 16))
#endif


using namespace SCXCoreLib;
using std::vector;

namespace SCXSystemLib
{

    /**Length of anchor-string "_DMI_"*/ 
    const int cDMIAnchorString = 5;
    /**Length of anchor-string "_SM_"*/ 
    const int cAnchorString= 4;
    /**Bytes length of searching paragraph for anchor-string "_SM_"*/ 
    const int cParagraphLength= 16;
    /**Start address of Smbios Table Entry Point on non-EFI system*/
    const size_t cStartAddress= 0xF0000;
    /**End address of Smbios Table Entry Point on non-EFI system*/
    const size_t cEndAddress= 0xFFFFF;
    /** offset where the length of the Entry Point Structure is*/ 
    const int cLengthEntry = 0x05;
    /** offset where the address of the SMBIOS Structure table is*/ 
    const int cAddressTable = 0x18;
    /** offset where the length of the SMBIOS Structure table is*/ 
    const int cLengthTable = 0x16;
    /** offset where the number of structures present in the SMBIOS Structure table is*/ 
    const int cNumberStructures = 0x1C;
    /** offset where the major version of this specification implemented is*/ 
    const int cMajorVersion = 0x06;
    /** offset where the minior version of this specification implemented is*/ 
    const int cMiniorVersion = 0x07;
    /** Length of each SMBIOS structure header*/
    const unsigned short cHeaderLength = 4;
    /** offset where the type of each SMBIOS structure is*/ 
    const int cTypeStructure = 0x00;
    /** offset where the length of each SMBIOS structure is*/ 
    const int cLengthStructure = 0x01;


    typedef vector<unsigned char> MiddleData;


    /*----------------------------------------------------------------------------*/
    /**
      Holds part fields of SMBIOS Structure Table Entry Point. 

     */
    struct SmbiosEntry
    {
        unsigned int tableAddress;
        unsigned short tableLength;
        unsigned short structureNumber;
        unsigned short majorVersion;
        unsigned short minorVersion;
        bool smbiosPresent;
        std::wstring name;
    };


    /*----------------------------------------------------------------------------*/
    /**
      Class representing all external dependencies from the SMBIOS PAL.

     */
    class SMBIOSPALDependencies
    {
        public:
            SMBIOSPALDependencies();
            virtual ~SMBIOSPALDependencies() {};
            /*----------------------------------------------------------------------------*/
            /**
              Read Smbios Table Entry Point on non-EFI system,from 0xF0000 to 0xFFFFF in device file. 
             */
            virtual bool ReadSpecialMemory(MiddleData &buf) const;
            /*----------------------------------------------------------------------------*/
            /**
              Read Smbios Table Entry Point on EFI system. 
             */
            virtual bool ReadSpecialmemoryEFI(MiddleData &buf) const;
            /*----------------------------------------------------------------------------*/
            /**
              Get SMBIOS Table content. 
             */
            virtual bool GetSmbiosTable(const struct SmbiosEntry& entryPoint,MiddleData &buf) const;

        private:
            std::wstring m_deviceName;  //!< BIOS system device file name 
            SCXCoreLib::SCXLogHandle m_log;  //!< Log handle
    };


    /*----------------------------------------------------------------------------*/
    /**
      Class encapsulating the SMBIOS on Linux and Solaris x86. 

     */
    class SCXSmbios 
    {
        public:
        explicit SCXSmbios(SCXCoreLib::SCXHandle<SMBIOSPALDependencies> deps = SCXCoreLib::SCXHandle<SMBIOSPALDependencies>(new SMBIOSPALDependencies()));
        virtual ~SCXSmbios(){};

        /*----------------------------------------------------------------------------*/
        /**
          Parse SMBIOS Structure Table Entry Point. 

          Parameter[out]:  smbiosEntry- Part fields value of SMBIOS Structure Table Entry Point. 
          Returns:     whether it's successful to parse it. 
         */
        bool ParseSmbiosEntryStructure(struct SmbiosEntry &smbiosEntry)const;
        /*----------------------------------------------------------------------------*/
        /**
          Get SMBIOS Table content. 

          Parameter[in]:  entryPoint- EntryPoint Structure. 
          Parameter[out]:  buf- Smbios Table. 
          Returns:     whether it's successful to get smbios table. 
         */
        bool GetSmbiosTable(const struct SmbiosEntry& entryPoint,MiddleData &buf) const;
        /*----------------------------------------------------------------------------*/
        /**
          Read specified index string.

          Parameter[in]:  buf- SMBIOS Structure Table.
          Parameter[in]:  length- offset to the start address of SMBIOS Structure Table.
          Parameter[in]:  index- index of string. 
          Returns:     The string which the program read. 
         */
        std::wstring ReadSpecifiedString(const MiddleData& buf,const size_t& length,const size_t& index)const;

        private:
        /*----------------------------------------------------------------------------*/
        /**
          Check the checksum of the Entry Point Structure. 

          Parameter[in]:  pEntry- the address of Entry Point Structure.
          Parameter[in]:  length- the length of Entry Point Structure. 
          Returns:     true,the checksum is 0;otherwise,false. 
         */
        bool CheckSum(const unsigned char* pEntry,const size_t& length)const;

        private:
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle
        SCXCoreLib::SCXHandle<SMBIOSPALDependencies> m_deps; //!< Collects external dependencies of this class.  

    };




}

#endif /* SCXSMBIOS_H*/
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
