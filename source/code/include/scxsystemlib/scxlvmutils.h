/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file

   \brief       Provides helper methods for working with LVM and devicemapper (dm)
                devices.

   \date        2010-12-14 18:11:00
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLVMUTILS_H
#define SCXLVMUTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>

#include <scxcorelib/logsuppressor.h>
#include <scxcorelib/scxdirectoryinfo.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/scxfilesystem.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxstream.h>


#if !defined(linux)
#error "Plattform not supported"
#endif


namespace SCXSystemLib {
    /**
       This exception class is used to indicate that a given LVM partition
       could not be mapped to the associated dm device.
     */
    class SCXBadLVMDeviceException : public SCXCoreLib::SCXFileSystemException
    {
    public:
        /**
          Constructor
          \param[in]    path      an LVM device related path.
          \param[in]    path      an LVM device related path.
          \param[in]    location  quasi-stack trace information.
        */
        SCXBadLVMDeviceException(const SCXCoreLib::SCXFilePath     & path,
                                 const std::wstring                & message,
                                 const SCXCoreLib::SCXCodeLocation & location)
                : SCXCoreLib::SCXFileSystemException(path, location), m_message(message)
        {
        };


        std::wstring What() const { return m_message; };


    private:
        std::wstring m_message;
    };


    /**
       This class defines the interface for the external API necessary to support
       the LVM utilities class on Linux.
     */
    class SCXLVMUtilsDepends
    {
    public:
        /**
           Finds the paths of file system objects in a specified directory.

           \param[in] path     Path to directory.
           \param[in] options  Search options that can be used to filter the result. Default: eDirSearchOptionDir

           \returns A vector of SCXFilePaths.

           \throws SCXFilePathNotFoundException if path is not found.
         */
        virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
            const SCXCoreLib::SCXFilePath               & path,
            const SCXCoreLib::SCXDirectorySearchOptions   options = SCXCoreLib::eDirSearchOptionDir) = 0;


        /**
           Fill in a stat structure.

           \param[in]  path   path to file/directory to perform stat on.
           \param[out] pStat  Pointer to a stat structure that should be filled.

           \throws SCXInvalidArgumentException If pStat is zero.
           \throws SCXFilePathNotFoundException if path is not found.
           \throws SCXUnauthorizedFileSystemAccessException
         */
        virtual void Stat(
            const SCXCoreLib::SCXFilePath            & path,
            SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat) = 0;


        /**
           Reads as many lines of the UT8 encoded file at the specified position
           in the file system as possible.

           Handles newline symbols in a platform independent way, that is, may
           be used to read a stream originating on one platform on another platform.
           If the lines is to be written back to the originating system
           the same nlf should in general be used, if we do not have other information.

           \param[in]  source  Position of the file in the filesystem
           \param[in]  lines   Lines that was read
           \param[out] nlfs   Newline symbols found used in the file

           \throws SCXUnauthorizedFileSystemAccessException
           \throws SCXLineStreamPartialReadException         Line to long to be stored in a wstring
           \throws SCXLineStreamContentException             Invalid byte sequence according to UTF8

           \note Will return no lines if file not found.
         */
        virtual void ReadAllLinesAsUTF8(
            const SCXCoreLib::SCXFilePath & source,
            std::vector< std::wstring >   & lines,
            SCXCoreLib::SCXStream::NLFs   & nlfs) = 0;


        /** Obligatory virtual destructor */
        virtual ~SCXLVMUtilsDepends() { }
    };


    /**
       This class implements the default interface for the external API necessary
       to support the LVM utilities class on Linux.
     */
    class SCXLVMUtilsDependsDefault
        : public SCXLVMUtilsDepends
    {
    public:
        virtual std::vector< SCXCoreLib::SCXFilePath > GetFileSystemEntries(
            const SCXCoreLib::SCXFilePath               & path,
            const SCXCoreLib::SCXDirectorySearchOptions  options)
        {
            return (SCXCoreLib::SCXDirectory::GetFileSystemEntries(path, options));
        }


        virtual void Stat(
            const SCXCoreLib::SCXFilePath            & path,
            SCXCoreLib::SCXFileSystem::SCXStatStruct * pStat)
        {
            SCXCoreLib::SCXFileSystem::Stat(path, pStat);
        }


        virtual void ReadAllLinesAsUTF8(
            const SCXCoreLib::SCXFilePath & source,
            std::vector< std::wstring >   & lines,
            SCXCoreLib::SCXStream::NLFs   & nlfs)
        {
            SCXCoreLib::SCXFile::ReadAllLinesAsUTF8(source, lines, nlfs);
        }


        virtual ~SCXLVMUtilsDependsDefault() { }
    };


    /**
       This class contains static utility methods to map an LVM partition name
       to a dm device and to enumerate the devices that contain a given dm device.
     */
    class SCXLVMUtils
    {
    public:
        SCXLVMUtils()
            : m_extDepends(new SCXLVMUtilsDependsDefault())
        {
        }


        SCXLVMUtils(SCXCoreLib::SCXHandle< SCXLVMUtilsDepends > extDepends)
            : m_extDepends(extDepends)
        {
        }


        bool IsDMDevice(const std::wstring & device);
        std::wstring GetDMDevice(const std::wstring & lvmDevice);
        std::vector< std::wstring > GetDMSlaves(const std::wstring & dmDevice);


    private:
        bool StatPathId(const std::wstring & path, unsigned int & major, unsigned int & minor);
        bool MatchIdInFile(const SCXCoreLib::SCXFilePath & path, unsigned int major, unsigned int minor);

        static SCXCoreLib::LogSuppressor m_errorSuppressor;
        static SCXCoreLib::LogSuppressor m_warningSuppressor;
        static SCXCoreLib::LogSuppressor m_infoSuppressor;

        SCXCoreLib::SCXHandle< SCXLVMUtilsDepends > m_extDepends;
    };
} /* namespace SCXSystemLib */
#endif /* SCXLVMUTILS_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
