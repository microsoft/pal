/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implement a PAL to handle RAID configurations.
    
    \date        2008-03-28 09:50:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/scxraid.h>
#include <string>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/logsuppressor.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       Get the raid configuration path.

       \returns The path to raid configuration.
    */
    const SCXCoreLib::SCXFilePath& SCXRaidCfgParserDefault::GetConfPath() const
    {
        static SCXCoreLib::SCXFilePath path(L"/etc/lvm/md.cf");
        return path;
    }

    /*----------------------------------------------------------------------------*/
    /**
           Prepare configuration lines for parsing.

           \param lines A vector of lines to prepare. Lines will be changed and removed as needed.

           Actions taken to prepare lines:
           Comments (everything after # on a line) are removed.
           Lines ending with \ are merged with the following line.
           Empty lines are removed.
           Lines containing only one word (no spaces) are removed
    */
    void SCXRaidCfgParserDefault::PrepareLines(std::vector<std::wstring>& lines)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        std::vector<std::wstring>::iterator it = lines.begin();
        while (it != lines.end())
        {
            std::wstring line = SCXCoreLib::StrTrim(*it);
            if (std::wstring::npos != line.find(L"#"))
            {
                line = SCXCoreLib::StrTrim(line.substr(0, line.find(L"#")));
            }
            if (line.length() > 0)
            {
                if (std::wstring::npos == line.find(L" "))
                {
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(line));
                    SCX_LOG(m_log, severity, L"PrepareLines - Skipping line: " + line);
                    line = L"";
                }
                if (line[line.length()-1] == L'\\')
                { // Merge with next line.
                    if ((it+1) != lines.end())
                    {
                        line = line.substr(0, line.length()-1);
                        line.append(L" ");
                        line.append(*(it+1));
                        *(it+1) = line;
                        line = L"";
                    }
                    else
                    {
                        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(line));
                        SCX_LOG(m_log, severity, L"PrepareLines - Skipping last line that ends with \\: " + line);
                        line = L"";
                    }
                }
            }
            if (line != *it)
            {
                if (line.length() == 0)
                {
                    it = lines.erase(it);
                }
                else
                {
                    *it = line;
                }
                continue;
            }
            it++;
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
           Parse a configuration line.

           \param[in] line Line to parse.
           \param[out] md Meta device parsed.
           \param[out] devices A list of devices part of the meta device.
           \param[out] options A map of options parsed.
           \returns The type of configuration line parsed.

           Lines have the format:
           
           \verbatim
           md #stripes (#slices (dev)*slices)*stripes [-i size|-h hotspare]
           mirror -m md0 md1 ... mdn [number]
           raid5 -r dev1 .. devn [-i size]
           soft -p -e dev1 size
           soft -p [dev|md] [-o number -b size]+
           hotspare dev1 dev2 .. devn
           mddb -c #devs dev1 .. devn

           Examples:
           d1 1 2 dev1 dev2
           d2 2 1 dev1 1 dev2
           d3 2 1 dev1 2 dev2 dev3
           d4 -p -e dev4 size
           d5 -p d1 -o 47 -b 11
           d6 -m d2 d3
           d7 -r dev1 dev2 -i 20k
           d8 -c 1 dev1
           \endverbatim

           \note Lines must be prepared before this method is called.
    */
    SCXRaidCfgParser::ParsedLineType SCXRaidCfgParserDefault::ParseLine(const std::wstring& line, std::wstring& md, std::vector<std::wstring>& devices, std::map<std::wstring, std::wstring>& options)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        SCXRaidCfgParser::ParsedLineType res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_UNKNOWN;
        std::vector<std::wstring> words;
        SCXCoreLib::StrTokenize(line, words);
        if (words.size() > 1)
        {
            md = words[0];
            words.erase(words.begin());
            if (words[0] == L"-m")
            { 
                res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_MIRROR;
                words.erase(words.begin());
                devices.assign(words.begin(), words.end());
                words.clear();
            }
            else if (words[0] == L"-t")
            {
                res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_TRANS;
                words.erase(words.begin());
                devices.assign(words.begin(), words.end());
                words.clear();
            }
            else if (words[0] == L"-c")
            {
                res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_STATE_DB_REPLICA;
                words.erase(words.begin());
                scxulong disks = SCXCoreLib::StrToULong(words[0]);
                words.erase(words.begin());
                for (scxulong disk = 0; disk < disks; ++disk)
                {
                    if (0 == words.size())
                    {
                        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(line));
                        SCX_LOG(m_log, severity, L"ParseLine - Unable to parse line: " + line);
                        return SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_UNKNOWN;
                    }
                    devices.push_back(words[0]);
                    words.erase(words.begin());
                }
            }
            else if (words[0] == L"-r")
            {
                res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_RAID;
                words.erase(words.begin());
                while (words.size() > 0 && words[0][0] != L'-')
                {
                    devices.push_back(words[0]);
                    words.erase(words.begin());
                }
                ParseOptions(words, options);
            }
            else if (words[0] == L"-p")
            {
                res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_SOFT;
                words.erase(words.begin());
                bool optionE = false;
                if (words[0] == L"-e")
                {
                    words.erase(words.begin());
                    optionE = true;
                }
                if (words.size() == 0)
                {
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(line));
                    SCX_LOG(m_log, severity, L"ParseLine - Unable to parse line: " + line);
                    return SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_UNKNOWN;
                }
                devices.push_back(words[0]);
                words.erase(words.begin());
                if (optionE)
                {
                    words.erase(words.begin());
                }
                ParseOptions(words, options);
            }
            else if (IsHotSpare(md))
            {
                res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_HOT_SPARE;
                devices.assign(words.begin(), words.end());
                words.clear();
            }
            else
            {
                res = SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_STRIPE;
                scxulong stripes = SCXCoreLib::StrToULong(words[0]);
                words.erase(words.begin());
                for (scxulong stripe = 0; stripe < stripes; ++stripe)
                {
                    if (0 == words.size())
                    {
                        SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(line));
                        SCX_LOG(m_log, severity, L"ParseLine - Unable to parse line: " + line);
                        return SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_UNKNOWN;
                    }
                    scxulong slices = SCXCoreLib::StrToULong(words[0]);
                    words.erase(words.begin());
                    for (scxulong slice = 0; slice < slices; ++slice)
                    {
                        if (0 == words.size())
                        {
                            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(line));
                            SCX_LOG(m_log, severity, L"ParseLine - Unable to parse line: " + line);
                            return SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_UNKNOWN;
                        }
                        devices.push_back(words[0]);
                        words.erase(words.begin());
                    }
                    ParseOptions(words, options);
                }
            }
        }
        if (words.size() > 0)
        {
            SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(line));
            SCX_LOG(m_log, severity, L"ParseLine - Unable to parse line: " + line);
            return SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_UNKNOWN;
        }
        return res;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Check if a name is a known hot spare name.

       \param device name to test if it is a known hot spare name.
       \returns true if the given name is a known hot spare name.
    */
    bool SCXRaidCfgParserDefault::IsHotSpare(const std::wstring& device) const
    {
        for (std::vector<std::wstring>::const_iterator it = m_hotSpares.begin();
             it != m_hotSpares.end(); ++it)
        {
            if (*it == device)
            {
                return true;
            }
        }
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Parse options from words vector and put them in optrions map.

       \param words Words to parse. Words parsed are removed from vector.
       \param options Options map to add options to.
    */
    void SCXRaidCfgParserDefault::ParseOptions(std::vector<std::wstring>& words, std::map<std::wstring, std::wstring>& options)
    {
        while (words.size() > 1 && (words[0] == L"-b" || words[0] == L"-h" || words[0] == L"-i" || words[0] == L"-o"))
        {
            if (words[0] == L"-h")
            {
                m_hotSpares.push_back(words[1]); // Save this for future use.
            }
            options[words[0]] = words[1];
            words.erase(words.begin());
            words.erase(words.begin());
        }
    }
    
    /*----------------------------------------------------------------------------*/
    /**
       Constructor

       \param parser Raid configuration parser to use.
    */ 
    SCXRaid::SCXRaid(const SCXCoreLib::SCXHandle<SCXRaidCfgParser>& parser)
        : m_parser(0)
    {
        static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.raid");
        m_parser = parser;

        SCXCoreLib::SCXStream::NLFs nlfs;
        std::vector<std::wstring> lines;
        SCXCoreLib::SCXFile::ReadAllLines(m_parser->GetConfPath(), lines, nlfs);
        m_parser->PrepareLines(lines);
        for (std::vector<std::wstring>::const_iterator it = lines.begin();
             it != lines.end(); ++it)
        {
            std::wstring md;
            std::vector<std::wstring> devs;
            std::map<std::wstring, std::wstring> opts;
            try
            {
                SCXRaidCfgParser::ParsedLineType line_type = m_parser->ParseLine(*it, md, devs, opts);
                switch (line_type)
                {
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_RAID:
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_STRIPE:
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_STATE_DB_REPLICA:
                    m_devices[md].assign(devs.begin(), devs.end());
                    if (opts.size() > 0)
                    {
                        std::map<std::wstring, std::wstring>::const_iterator oit = opts.find(L"-h");
                        if (oit != opts.end())
                        { // Hot spare found
                            m_deviceHotSpare[md] = oit->second;
                        }
                    }
                    break;
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_TRANS:
                    m_trans[md].assign(devs.begin(), devs.end());
                    break;
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_MIRROR:
                    m_mirrors[md].assign(devs.begin(), devs.end());
                    break;
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_HOT_SPARE:
                    m_hotSpares[md].assign(devs.begin(), devs.end());
                    break;
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_SOFT:
                    m_soft[md].assign(devs.begin(), devs.end());
                    break;
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_UNKNOWN:
                case SCXRaidCfgParser::eSCX_RAID_CFG_LINE_TYPE_MAX:
                default:
                    // ignore line
                    SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(*it));
                    SCX_LOG(m_log, severity, L"SCXRaid - Ignoring line: " + *it);
                    break;
                }
            }
            catch (SCXCoreLib::SCXNotSupportedException&)
            {
                SCXCoreLib::SCXLogSeverity severity(suppressor.GetSeverity(*it));
                SCX_LOG(m_log, severity, L"SCXRaid - Failed to parse, ignoring line: " + *it);
            }

        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get a logable string representation of the SCXRaid object.

       \returns a loggable string.

       Intended for debugging only.
    */
    std::wstring SCXRaid::DumpString() const
    {
        std::wstring r = L"SCXRaid: mirrors = [";
        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_mirrors.begin();
             it != m_mirrors.end(); it++)
        {
            r.append(L" ");
            r.append(it->first);
            r.append(L" => (");
            for (std::vector<std::wstring>::const_iterator iit = it->second.begin();
                 iit != it->second.end(); ++iit)
            {
                r.append(L" ");
                r.append(*iit);
            }
            r.append(L")");
        }
        r.append(L"] trans = [");
        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_trans.begin();
             it != m_trans.end(); it++)
        {
            r.append(L" ");
            r.append(it->first);
            r.append(L" => (");
            for (std::vector<std::wstring>::const_iterator iit = it->second.begin();
                 iit != it->second.end(); ++iit)
            {
                r.append(L" ");
                r.append(*iit);
            }
            r.append(L")");
        }
        r.append(L"] soft = [");
        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_soft.begin();
             it != m_soft.end(); it++)
        {
            r.append(L" ");
            r.append(it->first);
            r.append(L" => (");
            for (std::vector<std::wstring>::const_iterator iit = it->second.begin();
                 iit != it->second.end(); ++iit)
            {
                r.append(L" ");
                r.append(*iit);
            }
            r.append(L")");
        }
        r.append(L"] hotspare = [");
        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_hotSpares.begin();
             it != m_hotSpares.end(); it++)
        {
            r.append(L" ");
            r.append(it->first);
            r.append(L" => (");
            for (std::vector<std::wstring>::const_iterator iit = it->second.begin();
                 iit != it->second.end(); ++iit)
            {
                r.append(L" ");
                r.append(*iit);
            }
            r.append(L")");
        }
        r.append(L"] devices = [");
        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_devices.begin();
             it != m_devices.end(); it++)
        {
            r.append(L" ");
            r.append(it->first);
            r.append(L" => (");
            for (std::vector<std::wstring>::const_iterator iit = it->second.begin();
                 iit != it->second.end(); ++iit)
            {
                r.append(L" ");
                r.append(*iit);
            }
            r.append(L")");
        }
        r.append(L"] dev2hs = (");
        for (std::map<std::wstring,std::wstring>::const_iterator it = m_deviceHotSpare.begin();
                 it != m_deviceHotSpare.end(); ++it)
        {
            r.append(L" ");
            r.append(it->first);
            r.append(L" => ");
            r.append(it->second);
        }
        r.append(L")");
        
        return r;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve the names of all meta devices.

       \param[out] devices All meta devices configured are added to this vector.

       First are trans devices returned, then the mirrors and last striped/raid devices.

       \note The devices vector is not cleared so any existing data will remain.
    */
    void SCXRaid::GetMetaDevices(std::vector<std::wstring>& devices) const
    {
        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_soft.begin();
             it != m_soft.end(); it++)
        {
            devices.push_back(it->first);
        }

        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_trans.begin();
             it != m_trans.end(); it++)
        {
            devices.push_back(it->first);
        }

        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_mirrors.begin();
             it != m_mirrors.end(); it++)
        {
            devices.push_back(it->first);
        }

        for (std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_devices.begin();
             it != m_devices.end(); it++)
        {
            devices.push_back(it->first);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Retrieve all devices part of a certain meta device.

       \param[in] md The meta device name (i.e. d0)
       \param[out] devices This vector is set to be exactly the devices part of the meta device.

       \note The devices vector is always changed (empty if md is not found). Also the device 
       names are what is returned, not paths (i.e. c0t0d0)
    */
    void SCXRaid::GetDevices(const std::wstring& md, std::vector<std::wstring>& devices) const
    {
        std::map<std::wstring, std::vector<std::wstring> >::const_iterator it = m_devices.find(md);

        devices.clear();
        if (it != m_devices.end())
        {
            devices.assign(it->second.begin(), it->second.end());
            std::map<std::wstring, std::wstring>::const_iterator hs = m_deviceHotSpare.find(md);
            if (hs != m_deviceHotSpare.end())
            {
                std::vector<std::wstring> devs;
                GetDevices(hs->second, devs);
                devices.insert(devices.end(), devs.begin(), devs.end());
            }
        }
        else
        {
            it = m_trans.find(md);
            if (it != m_trans.end())
            {
                std::vector<std::wstring> devs;
                for (std::vector<std::wstring>::const_iterator iit = it->second.begin();
                     iit != it->second.end(); ++iit)
                {
                    GetDevices(*iit, devs);
                    devices.insert(devices.end(), devs.begin(), devs.end());
                }
            }
            else
            {
                it = m_mirrors.find(md);
                if (it != m_mirrors.end())
                {
                    std::vector<std::wstring> devs;
                    for (std::vector<std::wstring>::const_iterator mit = it->second.begin();
                         mit != it->second.end(); ++mit)
                    {
                        GetDevices(*mit, devs);
                        devices.insert(devices.end(), devs.begin(), devs.end());
                    }
                }
                else
                {
                    it = m_soft.find(md);
                    if (it != m_soft.end())
                    {
                        std::vector<std::wstring> devs;
                        for (std::vector<std::wstring>::const_iterator mit = it->second.begin();
                             mit != it->second.end(); ++mit)
                        {
                            GetDevices(*mit, devs);
                            if (devs.size() == 0)
                            {
                                devices.insert(devices.end(), *mit);
                            }
                            else
                            {
                                devices.insert(devices.end(), devs.begin(), devs.end());
                            }
                        }
                    }
                    else
                    {
                        it = m_hotSpares.find(md);
                        if (it != m_hotSpares.end())
                        {
                            devices.assign(it->second.begin(), it->second.end());
                        }
                    }
                }
            }
        }
    }

} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
