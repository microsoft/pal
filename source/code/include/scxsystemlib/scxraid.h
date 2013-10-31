/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Declares a PAL to handle RAID configurations.
    
    \date        2008-03-28 09:50:00

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXRAID_H
#define SCXRAID_H

#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxlog.h>

#include <string>
#include <vector>
#include <map>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    An interface for Raid configuration parsers.
*/
    class SCXRaidCfgParser
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
           Enumeration to indicate what kind of configuration line was parsed.
        */
        enum ParsedLineType
        {
            eSCX_RAID_CFG_LINE_TYPE_UNKNOWN = 0,      //!< unknown device type 
            eSCX_RAID_CFG_LINE_TYPE_STRIPE,           //!< striped (RAID0) and/or concat
            eSCX_RAID_CFG_LINE_TYPE_MIRROR,           //!< mirror (RAID1)
            eSCX_RAID_CFG_LINE_TYPE_TRANS,            //!< trans (master and logging)
            eSCX_RAID_CFG_LINE_TYPE_HOT_SPARE,        //!< hot spare pool
            eSCX_RAID_CFG_LINE_TYPE_RAID,             //!< raid (RAID5)
            eSCX_RAID_CFG_LINE_TYPE_SOFT,             //!< soft partition
            eSCX_RAID_CFG_LINE_TYPE_STATE_DB_REPLICA, //!< State database replica
            eSCX_RAID_CFG_LINE_TYPE_MAX               //!< enum max marker
        };

        /** Default constructor */
        SCXRaidCfgParser() 
        { 
            m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.raidcfgparser");
        }

        /** Default virtual destructor */
        virtual ~SCXRaidCfgParser() { }
        
        /*----------------------------------------------------------------------------*/
        /**
           Get the raid configuration path.

           \returns The path to raid configuration.
        */
        virtual const SCXCoreLib::SCXFilePath& GetConfPath() const = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Prepare configuration lines for parsing.

           \param lines A vector of lines to prepare. Lines will be changed and removed as needed.

           Typicaly this method removes comments, empty lines and merges lines when needed.
        */
        virtual void PrepareLines(std::vector<std::wstring>& lines) = 0;

        /*----------------------------------------------------------------------------*/
        /**
           Parse a configuration line.

           \param[in] line Line to parse.
           \param[out] md Meta device parsed.
           \param[out] devices A list of devices part of the meta device.
           \param[out] options A map of options parsed.
           \returns The type of configuration line parsed.
        */
        virtual ParsedLineType ParseLine(const std::wstring& line, std::wstring& md, std::vector<std::wstring>& devices, std::map<std::wstring, std::wstring>& options) = 0;

    protected:
        //! Handle to log file 
        SCXCoreLib::SCXLogHandle m_log;
    };

/*----------------------------------------------------------------------------*/
/**
    Default raid configuration parser

    \note Only tested for solaris.
*/
    class SCXRaidCfgParserDefault : public SCXRaidCfgParser
    {
    public:
        virtual const SCXCoreLib::SCXFilePath& GetConfPath() const;
        virtual void PrepareLines(std::vector<std::wstring>& lines);
        virtual ParsedLineType ParseLine(const std::wstring& line, std::wstring& md, std::vector<std::wstring>& devices, std::map<std::wstring, std::wstring>& options);

    protected:
        bool IsHotSpare(const std::wstring& device) const;
        void ParseOptions(std::vector<std::wstring>& words, std::map<std::wstring, std::wstring>& options);
        std::vector<std::wstring> m_hotSpares; //!< A list of seen hot spare names.
    };

/*----------------------------------------------------------------------------*/
/**
    RAID representation.
*/
    class SCXRaid
    {
    public:
        SCXRaid(const SCXCoreLib::SCXHandle<SCXRaidCfgParser>& parser);
        
        void GetMetaDevices(std::vector<std::wstring>& devices) const;
        void GetDevices(const std::wstring& md, std::vector<std::wstring>& devices) const;

        std::wstring DumpString() const;
    private:
        /** Default constructor should not be used - made private */
        SCXRaid() : m_parser(0) { }

        SCXCoreLib::SCXHandle<SCXRaidCfgParser> m_parser; //!< The config parser helper.
        //! A map with meta device to devices mappings.
        std::map<std::wstring, std::vector<std::wstring> > m_devices; 
        //! A map with mirror mappings
        std::map<std::wstring, std::vector<std::wstring> > m_mirrors;
        //! A map of trans devices
        std::map<std::wstring, std::vector<std::wstring> > m_trans;
        //! A map of soft partitions
        std::map<std::wstring, std::vector<std::wstring> > m_soft;
        //! A map with hot spare mappings (what devices is part of a hot spare)
        std::map<std::wstring, std::vector<std::wstring> > m_hotSpares;
        //! A map with device to hot spare mapping (i.e. what hotspare is part of meta device)
        std::map<std::wstring, std::wstring> m_deviceHotSpare;
        //! Handle to log file 
        SCXCoreLib::SCXLogHandle m_log;
    };

} /* namespace SCXSystemLib */
#endif /* SCXRAID_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
