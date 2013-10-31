/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Defines a class to parse lvmtab files.
    
    \date        2008-02-21 13:29:02

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLVMTAB_H
#define SCXLVMTAB_H
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxlog.h>
#include <vector>

namespace SCXSystemLib
{
/*----------------------------------------------------------------------------*/
/**
    Representation of a parsed lvmtab file
*/
    class SCXLvmTab
    {
    public:
        SCXLvmTab(const SCXCoreLib::SCXFilePath& path);

        size_t GetVGCount() const;
        size_t GetPartCount(size_t vg_idx) const;
        const std::wstring& GetVG(size_t vg_idx) const;
        const std::wstring& GetPart(size_t vg_idx, size_t part_idx) const;

    protected:
        SCXLvmTab() { } //!< Default constructor delibetrly made private 
        SCXLvmTab(const SCXLvmTab&); //!< Copy constructor delibetrly made private 
    
        //! A private representation of volume group information.
        struct SCXVG
        {
            std::wstring m_name; //!< Name of the volume group.
            std::vector<std::wstring> m_part; //!< Array of parts in volume group.
        };

        std::vector<SCXVG> m_vg; //!< List of volume groups.
        SCXCoreLib::SCXLogHandle m_log; //!< Log handle.
    };

/*----------------------------------------------------------------------------*/
/**
   A generic exception for lvmtab format exceptions.
*/
    class SCXLvmTabFormatException: public SCXCoreLib::SCXException
    {
    public:
        /**
           Ctor
           \param[in] reason Description of format error
           \param[in] l      Source code location object

        */
        SCXLvmTabFormatException(const std::wstring& reason, const SCXCoreLib::SCXCodeLocation& l)
            : SCXCoreLib::SCXException(l), m_Reason(reason)
        { }
        
        std::wstring What() const {
            return L"Access violation exception was thrown because: " + m_Reason;
        }

    protected:
        //! Description of internal error
        std::wstring   m_Reason;

    };
} /* namespace SCXSystemLib */

#endif /* SCXLVMTAB_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
