/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Implement a lvmtab file parser.
    
    \date        2008-02-21 13:31:02

    The file format used in lvmtab files is (reverse engineered):
    7 bytes
    1 byte = number of volume groups
    4 bytes
    For each volume group:
        1024 bytes = name of volume group
        17 bytes
        1 byte = number of parts in volume group
        12 bytes
        For each part in volume group:
            1024 bytes = name of part
            4 bytes
    
*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/scxlvmtab.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/stringaid.h>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Constructor parsing a given file as an lvmtab file.
    
    \param       path A file path to an lvmtab file.
    \throws      SCXLvmTabFormatException if the file parsed has the wrong format.
    \throws      SCXFilePathNotFoundException if the path could not be opened.
    \throws      SCXUnauthorizedFileSystemAccessException if the file may not be opened.
    
    \note The parser does not fail for files with wrong format. It does a best
    effort try to parse the file.
    
*/
    SCXLvmTab::SCXLvmTab(const SCXCoreLib::SCXFilePath& path)
    {
        m_log = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.lvmtab.parser");

        SCXCoreLib::SCXHandle<std::fstream> fs = SCXCoreLib::SCXFile::OpenFstream(path, std::ios_base::in);
        fs.SetOwner();
        std::fstream::pos_type file_pos = 0; // HPUX aCC forces us to keep track of this manually to be able to seekg
        // Skip 7 first bytes
        file_pos += 7;
        // Read number of VGs.
        fs->seekg(file_pos);
        unsigned char num_vg = 0;
        fs->read((char *)&num_vg, sizeof(num_vg));
        SCX_LOGHYSTERICAL(m_log, L"SCXLvmTab:   Read num_vg: " + SCXCoreLib::StrFrom((int) num_vg));
        file_pos += sizeof(num_vg);
        // Skip 4 bytes.
        file_pos += 4;
        for (unsigned char vg_idx=0; vg_idx < num_vg && SCXCoreLib::SCXStream::IsGood(*fs); ++vg_idx)
        {
            SCXVG vg;
            // Read volume group (1K)
            char volumeGroupRaw[1024+1];
            memset(volumeGroupRaw, 0, sizeof(volumeGroupRaw));
            fs->seekg(file_pos);
            fs->read(volumeGroupRaw, sizeof(volumeGroupRaw)-1);
            file_pos += sizeof(volumeGroupRaw)-1;
            vg.m_name = SCXCoreLib::StrFromUTF8(std::string(volumeGroupRaw));
            SCX_LOGHYSTERICAL(m_log, L"SCXLvmTab:  Volume group name" + vg.m_name);
            // Skip 17 bytes
            file_pos += 17;
            // Read number of parts
            unsigned char num_parts = 0;
            fs->seekg(file_pos);
            fs->read((char *) &num_parts, sizeof(num_parts));
            file_pos += sizeof(num_parts);
            SCX_LOGHYSTERICAL(m_log, L"SCXLvmTab:    Parts: " + SCXCoreLib::StrFrom((int) num_parts));
            // Skip 12 bytes
            file_pos += 12;
            for (unsigned char part_idx = 0; part_idx < num_parts && SCXCoreLib::SCXStream::IsGood(*fs); ++part_idx)
            {
                // Read partition (1K)
                char partitionRaw[1024+1];
                memset(partitionRaw, 0, sizeof(partitionRaw));
                fs->seekg(file_pos);
                fs->read(partitionRaw, sizeof(partitionRaw)-1);
                file_pos += sizeof(partitionRaw)-1;
                vg.m_part.push_back(SCXCoreLib::StrFromUTF8(std::string(partitionRaw)));
                SCX_LOGHYSTERICAL(m_log, L"SCXLvmTab:      Part: " + SCXCoreLib::StrFrom((int) (part_idx + 1)) + L": "
                                  + SCXCoreLib::StrFromUTF8(std::string(partitionRaw)));
                // Skip 4 bytes.
                file_pos += 4;
            }
            m_vg.push_back(vg);
        }
        
        fs->seekg(0, std::ios_base::end);
        SCX_LOGHYSTERICAL(m_log, L"SCXLvmTab: File_pos: " + SCXCoreLib::StrFrom((int) file_pos)
                          + L", fs_tellg(): " + SCXCoreLib::StrFrom(fs->tellg()));

        if (file_pos > fs->tellg())
        {
            throw SCXLvmTabFormatException(L"File to short", SCXSRCLOCATION);
        }
        if (file_pos < fs->tellg())
        {
            throw SCXLvmTabFormatException(L"File to long", SCXSRCLOCATION);
        }
        fs->close();
    }

/*----------------------------------------------------------------------------*/
/**
    Return the number of volume groups in the parsed file.
    
    \returns     Number of volume groups.
    
*/
    size_t SCXLvmTab::GetVGCount() const
    {
        return m_vg.size();
    }

/*----------------------------------------------------------------------------*/
/**
    Return the number of partitions in a particular volume group.
    
    \param       vg_idx Volume group index (zero being the first VG in the lvmtab file).
    \returns     Number of partitions in the given volume group.
    \throws      SCXIllegalIndexExceptionUInt if the volume group index is out of bounds.
    
*/
    size_t SCXLvmTab::GetPartCount(size_t vg_idx) const
    {
        if (vg_idx >= m_vg.size())
        {
            throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"vg_idx", vg_idx, 0, true, m_vg.size()-1, true, SCXSRCLOCATION);
        }
        return m_vg[vg_idx].m_part.size();
    }

/*----------------------------------------------------------------------------*/
/**
    Return the name of a volume group.
    
    \param       vg_idx Volume group index (zero being the first VG in the lvmtab file).
    \returns     Name of the given volume group.
    \throws      SCXIllegalIndexExceptionUInt if the volume group index is out of bounds.
    
*/
    const std::wstring& SCXLvmTab::GetVG(size_t vg_idx) const
    {
        if (vg_idx >= m_vg.size())
        {
            throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"vg_idx", vg_idx, 0, true, m_vg.size()-1, true, SCXSRCLOCATION);
        }
        return m_vg[vg_idx].m_name;
    }

/*----------------------------------------------------------------------------*/
/**
    Return the partition name of a given partition in a given volume group.
    
    \param       vg_idx Volume group index (zero being the first VG in the lvmtab file).
    \param       part_idx Partition index (zero being the first partition for the given VG).
    \returns     Name of given partition in given volume group.
    \throws      SCXIllegalIndexExceptionUInt if the volume group or partition index is out of bounds.
    
*/
    const std::wstring& SCXLvmTab::GetPart(size_t vg_idx, size_t part_idx) const
    {
        if (vg_idx >= m_vg.size())
        {
            throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"vg_idx", vg_idx, 0, true, m_vg.size()-1, true, SCXSRCLOCATION);
        }
        if (part_idx >= m_vg[vg_idx].m_part.size())
        {
            throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"part_idx", part_idx, 0, true, m_vg[vg_idx].m_part.size()-1, true, SCXSRCLOCATION);
        }
        return m_vg[vg_idx].m_part[part_idx];
    }

} /* namespace SCXSystemLib */

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
