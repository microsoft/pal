/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        staticdiskpartitioninstance.cpp

    \brief       Implements the disk partition instance pal for static information.

    \date        2011-08-31 11:42:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <scxcorelib/scxdumpstring.h>
#include <scxcorelib/scxregex.h>
#include <scxcorelib/scxfile.h>
#include <scxcorelib/scxfilepath.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/scxmath.h>
#include <scxcorelib/logsuppressor.h>

// For running processes and retrieving output
#include <scxcorelib/scxprocess.h>
#include <scxsystemlib/staticdiskpartitioninstance.h>

/* System specific includes - generally for Update() method */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>

#if defined(linux)
#include <sstream>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/types.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>

#elif defined(sun)

#include <sys/dkio.h>
#include <sys/vtoc.h>

#endif


using namespace SCXCoreLib;
using namespace std;

static SCXCoreLib::LogSuppressor suppressor(SCXCoreLib::eWarning, SCXCoreLib::eTrace);

namespace SCXSystemLib
{

/*----------------------------------------------------------------------------*/
/**
   Constructor.

   \param       None.

*/
StaticDiskPartitionInstance::StaticDiskPartitionInstance(SCXCoreLib::SCXHandle<StaticDiskPartitionInstanceDeps> deps)
        :  m_log(SCXCoreLib::SCXLogHandleFactory::GetLogHandle(L"scx.core.common.pal.system.disk.staticdiskpartitioninstance"))
        ,m_blockSize(0)
        ,m_bootPartition(false)
        ,m_deviceID(L"")
        ,m_index(0)
        ,m_numberOfBlocks(0)
        ,m_partitionSize(0)
        ,m_startingOffset(0)
#if defined(linux)
        ,m_fdiskResult("")
        ,c_cmdFdiskStr(L"/bin/sh -c \"/sbin/fdisk -u -l\" ")
        ,c_cmdBlockDevSizeStr(L"/sbin/blockdev --getsize ")
        ,c_cmdBlockDevBszStr(L"/sbin/blockdev --getbsz ")
        ,c_RedHSectorSizePattern(L"^Units =[^=]*=[ ]*([0-9]+)")
        ,c_RedHBootPDetailPattern(L"(^/dev/[^ ]*) [ ]*(\\*) [ ]*([0-9]+) [ ]*([0-9]+) [ ]*([0-9]+)")
        ,c_RedHDetailPattern(L"(^/dev/[^ ]*) [ ]*([0-9]+) [ ]*([0-9]+) [ ]*([0-9]+)")
#endif
#if defined(sun) && defined(sparc)
        ,c_SolPrtconfPattern(L"bootpath:[ ]+'([^ ]*)'")
#elif defined(sun) && !defined(sparc)
        ,c_SolPrtconfPattern(L"bootpath[ ]+([^ ]*)")
#endif
#if defined(sun)  //Both SPARC and x86
        ,c_SolLsPatternBeg(L"(c[0-9]t?[0-9]?d[0-9]s[0-9]+).*")
        ,m_isZfsPartition(false)
        ,c_SolDfPattern(L"([^ ]*)[^(]*\\((/dev/dsk/[^ )]*)[ ]*):[ ]*([0-9]*)")
        ,c_SolPrtvtocBpSPattern(L"^\\*[ ]*([0-9]+) bytes/sector")
        ,c_SolprtvtocDetailPattern(L"^[^\\*]{1}[ ]*([0-9])[ ]*[0-9][ ]*[0-9]+[ ]*([0-9]+)[ ]*([0-9]+)[ ]+[0-9]+[ ]*(/[^ ]*)")
#endif
        ,MATCHCOUNT(0x20)
        ,m_deps(deps)
{
}

/*----------------------------------------------------------------------------*/
/**
   Virtual destructor.
*/
StaticDiskPartitionInstance::~StaticDiskPartitionInstance()
{
}


/*----------------------------------------------------------------------------*/
/**
   Dump the object as a string for logging purposes.
    
   \returns     String representation of object, suitable for logging.
*/
const wstring StaticDiskPartitionInstance::DumpString() const
{
        return SCXDumpStringBuilder("StaticDiskPartitionInstance")
            .Text("Name", GetId())
            .Scalar("Blocksize", m_blockSize)
            .Text("BootPartition", (m_bootPartition? L"TRUE" : L"FALSE") )
            .Text("DeviceID", m_deviceID)
            .Scalar("DiskIndex", m_index)
            .Scalar("NumberOfBlocks",m_numberOfBlocks)
            .Scalar("Size", m_partitionSize)
            .Scalar("StartingOffset", m_startingOffset);
}


#if defined(linux)
/*----------------------------------------------------------------------------*/
/**
   Check that this is a valid instance by seeing whether it's found by fdisk

   \returns    True if this instance is found in the fdisk output, False otherwise
*/
bool StaticDiskPartitionInstance::CheckFdiskLinux()
{
  SCX_LOGTRACE(m_log, L"DiskPartition::CheckFdiskLinux() Entering, DeviceID is:" + m_deviceID);


  bool foundIt = false;

  if (GetFdiskResult())
  {
    //We have the valid output from fdisk to use:
        SCXRegexPtr bootDetRegExPtr(NULL);
        SCXRegexPtr detailRegExPtr(NULL);

        std::vector<wstring> matchingVector;

        //Let's build our RegEx:
        try
        {
            bootDetRegExPtr = new SCXCoreLib::SCXRegex(c_RedHBootPDetailPattern);
            detailRegExPtr = new SCXCoreLib::SCXRegex(c_RedHDetailPattern);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            SCX_LOGERROR(m_log, L"Exception caught in compiling regex: " + e.What());
            return false;
        }

        std::istringstream fdskStringStrm(m_fdiskResult);

        vector<wstring>  allLines;                       // all lines read from fdisk output
        allLines.clear();

        SCXStream::NLFs nlfs;
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(fdskStringStrm, allLines, nlfs);
 
        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end() && !foundIt; it++)
        {
             wstring curLine(*it);
             SCX_LOGTRACE(m_log, SCXCoreLib::StrAppend(L"DiskPartition::CheckFdiskLinux() Top of FOR: We have a line= ", (*it)));
             matchingVector.clear();

             // Look for the detail line for this partition
             if (((detailRegExPtr->ReturnMatch(curLine, matchingVector, 0)) ||
                  (bootDetRegExPtr->ReturnMatch(curLine, matchingVector, 0))) &&
                 (m_deviceID == matchingVector[1]))
             {
                 //This is our record in the fdisk output
                 foundIt = true;
             }
             else if (matchingVector.size() > 0)
             {
                 //Have an error message
                 SCX_LOGINFO(m_log, L"No match found! Message: " + matchingVector[0]);
             }

        } //EndFor

  }
  else
  {
     SCX_LOGERROR(m_log, StrAppend(wstring(L"Error from fdisk for Device: "),m_deviceID));
  }

  return foundIt;
}

/*----------------------------------------------------------------------------*/
/**
   Get the results of the fdisk command for later parsing

   \returns    True if fdisk is successfully executed, False otherwise
*/
bool StaticDiskPartitionInstance::GetFdiskResult()
{
    SCX_LOGTRACE(m_log, StrAppend(L"DiskPartition::GetFdiskResult() Entering, DeviceID is:", m_deviceID));

   std::istringstream processInput;
   std::ostringstream processOutput;
   std::ostringstream processErr;

   try
   {
        SCXCoreLib::SCXProcess::Run(c_cmdFdiskStr, processInput, processOutput, processErr, 15000);
        m_fdiskResult = processOutput.str();
        SCX_LOGTRACE(m_log, StrAppend(L"  Got this output: ", StrFromUTF8(m_fdiskResult)));

        std::string errOut = processErr.str();
        if (errOut.size() > 0)
        {
            SCX_LOGWARNING(m_log, StrAppend(L"Got this error string from fdisk command: ", StrFromUTF8(errOut)));
        }
   }
   catch (SCXCoreLib::SCXException &e) 
   {
        SCX_LOGERROR(m_log, L"attempt to execute fdisk command for the purpose of retrieving partition information failed.");
        m_fdiskResult.clear();
        return false;
   }

   if (m_fdiskResult.size() == 0)
   {
       SCX_LOGERROR(m_log, L"Unable to retrieve partition information from OS...");
       return false;
   }

   return true;
}
#endif


/*----------------------------------------------------------------------------*/
/**
   Update the instance.
*/
void StaticDiskPartitionInstance::Update()
{

        SCX_LOGTRACE(m_log, L"DiskPartition::Update():: Entering, DeviceID is:" + m_deviceID);

#if defined(linux)

        return Update_Linux();

#elif defined(sun)
        if(m_isZfsPartition == false)
        {
            Update_Solaris();
        }
        return;

#else

        return;

#endif
}



#if defined(sun)

/*----------------------------------------------------------------------------*/
/**
   Update the Solaris Sparc instance.
*/
void StaticDiskPartitionInstance::Update_Solaris()
{

        SCX_LOGTRACE(m_log, L"DiskPartition::Update_Solaris():: Entering, DeviceID is:" + m_deviceID);

        // Execute 'df -g' and retrieve result to determine filesystem that is mounted on
        // and block size.  Then, go through output of prtvtoc for this filesystem to
        // retrieve the remaining partition information.
#if PF_MAJOR == 5 && (PF_MINOR  == 9 || PF_MINOR == 10)
        wstring cmdStringDf = L"/usr/sbin/df -g";
#elif PF_MAJOR == 5 && PF_MINOR  == 11
        wstring cmdStringDf = L"/sbin/df -g";
#else
#error "Platform not supported"
#endif
        int status;
        std::istringstream processInputDf;
        std::ostringstream processOutputDf;
        std::ostringstream processErrDf;
        wstring curLine;
        curLine.clear();
        wstring mountedStr;
        wstring blockSizeStr;
        std::string dfResult;
        vector<wstring>  allLines;                       // all lines read from output
        allLines.clear();
        SCXStream::NLFs nlfs;
        bool foundIt = false;
        vector<wstring> matchingVector;

        SCXRegexPtr solDfPatternPtr(NULL);
        SCXRegexPtr solPrtvtocBpSPatternPtr(NULL);
        SCXRegexPtr solPrtvtocDetailPatternPtr(NULL);

        // Let's build our RegEx:
        try
        {
           solDfPatternPtr = new SCXCoreLib::SCXRegex(c_SolDfPattern);
           solPrtvtocBpSPatternPtr = new SCXCoreLib::SCXRegex(c_SolPrtvtocBpSPattern);
           solPrtvtocDetailPatternPtr = new SCXCoreLib::SCXRegex(c_SolprtvtocDetailPattern);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            SCX_LOGERROR(m_log, L"Exception caught in compiling regex: " + e.What());
            return;
        }

        try 
        {
            status = SCXCoreLib::SCXProcess::Run(cmdStringDf, processInputDf, processOutputDf, processErrDf, 15000);
            if (status != 0)
            {
                SCX_LOGERROR(m_log, StrAppend(L"Error on command " + cmdStringDf + L" - status ", status));
                SCX_LOGERROR(m_log, StrFromUTF8("Output - " + processOutputDf.str()));
                SCX_LOGERROR(m_log, StrFromUTF8("Error - " + processErrDf.str()));
                return;
            }
            dfResult = processOutputDf.str();
        }
        catch(SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, L"Unable to retrieve partition information from OS using 'df -g'..." + e.What());
        }

        std::istringstream stringStrmDf_g(dfResult);
        allLines.clear();
        size_t dfLineCt = 100;
        allLines.reserve(dfLineCt);
        foundIt = false;
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stringStrmDf_g, allLines, nlfs);

        for (vector<wstring>::iterator it = allLines.begin(); it != allLines.end() && !foundIt; it++)
        {
            curLine.assign(*it);
            matchingVector.clear();

            if ((solDfPatternPtr->ReturnMatch(curLine, matchingVector, 0)) && (matchingVector.size() >= 4) && 
                      (m_deviceID == matchingVector[2]))
            {
                mountedStr = matchingVector[1];
                blockSizeStr = matchingVector[3];
                foundIt = true; 
            }
            else if (matchingVector.size() > 0)
            {
                 //Have an error message
                 SCX_LOGINFO(m_log, L"No match found! Error: " + matchingVector[0]);
            }

        }

        //Check the results
        if (!foundIt || mountedStr .size() == 0)
        {
           SCX_LOGERROR(m_log, L"Failed to find this partition info with df -g: " + m_deviceID +
                         L"  And Regex Error Msg: " + matchingVector[0]);
           return;
        }     


        //The next (and last) step is to do a prtvtoc [dir] command to retrieve the rest of the partition info:
#if PF_MAJOR == 5 && (PF_MINOR  == 9 || PF_MINOR == 10)
        wstring cmdStringPrtvToc = L"/usr/sbin/prtvtoc " + m_deviceID;
#elif PF_MAJOR == 5 && PF_MINOR  == 11
        wstring cmdStringPrtvToc = L"/sbin/prtvtoc " + m_deviceID;
#else
#error "Platform not supported"
#endif
        std::istringstream processInputPrtvtoc;
        std::ostringstream processOutputPrtvtoc;
        std::ostringstream processErrPrtvtoc;
        curLine.clear();
        wstring firstSectorStr;
        wstring sectorCountStr;
        wstring bytesPerSectorStr;

        std::string prtResult("");

        try 
        {
            SCXCoreLib::SCXProcess::Run(cmdStringPrtvToc, processInputPrtvtoc, processOutputPrtvtoc, processErrPrtvtoc, 15000);
            prtResult = processOutputPrtvtoc.str();
            size_t lengthCaptured = prtResult.length();

            // Truncate trailing newline if there in captured output                  
            if (lengthCaptured > 0)
            {
              if (prtResult[lengthCaptured - 1] == '\n')
              {
                prtResult[lengthCaptured - 1] = '\0';
              }
            }

        }
        catch(SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, L"Unable to retrieve partition information from OS using 'df -g'..." + e.What());
        }

        std::istringstream stringStrmPrtvtoc(prtResult);
        allLines.clear();
        foundIt = false;
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stringStrmPrtvtoc, allLines, nlfs);;

        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end() && !foundIt; it++)
        {
            curLine.assign(*it);
            matchingVector.clear();

            // First, we match on a comment line that tells us the Sector Size
            if (solPrtvtocBpSPatternPtr->ReturnMatch(curLine, matchingVector, 0)) 
            {
                bytesPerSectorStr = matchingVector[1];
            } // Next, we look for a detail line that matches our index:
            else if ((solPrtvtocDetailPatternPtr->ReturnMatch(curLine, matchingVector, 0)) && 
                     (matchingVector.size() >= 5) && 
                     (m_index == SCXCoreLib::StrToUInt(matchingVector[1])))
            {
                // This is our row in the Partion info output
                firstSectorStr = matchingVector[2];
                sectorCountStr = matchingVector[3];
                foundIt = true;
               
            }
        }

        //Check the results
        if (!foundIt || bytesPerSectorStr.size() == 0)
        {
           SCX_LOGERROR(m_log, L"Failed to find this partition info with prtvtoc: " + m_deviceID +
                         L"  And Regex Error Msg: " + matchingVector[0]);
           return;
        }     

        // If we reached here we have everything we need
        //  just need to do a little arithmetic and fill in our Properties struct:
        m_blockSize = SCXCoreLib::StrToULong(blockSizeStr);

        char dummyChar;
        unsigned int sectorSz = StrToUInt(bytesPerSectorStr);
        unsigned long long totalSectors = StrToUInt(sectorCountStr);
        unsigned long long startingSector = StrToUInt(firstSectorStr);

        m_partitionSize = totalSectors * sectorSz;
        m_startingOffset = startingSector * sectorSz;
        m_numberOfBlocks = RoundToUnsignedInt(static_cast<double>(m_partitionSize) / 
                                              static_cast<double>(m_blockSize));

        return;
}
#endif




#if defined(sun)
/**
   Since the process of determing the boot path on Sun is complex, no use doing this over
   again for each partition and get the same answer every time. Make this a separate method
   so Enumerate can call this for the first Instance, and just reuse the results for the 
   other instances.

       input parameter: String that will be set to the boot drive path (e.g. /dev/dsk/c1t0d0s0)
       returns T when set, F when it fails.
   
*/

bool StaticDiskPartitionInstance::GetBootDrivePath(wstring& bootpathStr)
{
        SCX_LOGTRACE(m_log, L"DiskPartition::GetBootDrivePath():: Entering . . .");

        bootpathStr.clear();

        // buffer to store lines read from process output
        wstring curLine;
        wstring bootInterfacePath;

        // Determine Solaris boot disk using 'prtconf' and 'ls /dev/dsk'
        // cmdString stores the current process we are running via SCXProcess::Run()
#if defined(sparc)
#if PF_MAJOR == 5 && (PF_MINOR  == 9 || PF_MINOR == 10)
        wstring cmdPrtString = L"/usr/sbin/prtconf -pv"; 
#elif PF_MAJOR == 5 && PF_MINOR  == 11
        wstring cmdPrtString = L"/sbin/prtconf -pv"; 
#else
#error "Platform not supported"
#endif
#else// sparc
        wstring cmdPrtString = L"/usr/bin/grep bootpath /boot/solaris/bootenv.rc"; 
#endif

        std::string prtconfResult;
        std::string finalResult;
        std::istringstream processInputPrt;
        std::ostringstream processOutputPrt;
        std::ostringstream processErrPrt;

        try 
        {
            m_deps->Run(cmdPrtString, processInputPrt, processOutputPrt, processErrPrt, 15000);
            prtconfResult = processOutputPrt.str();
            SCX_LOGTRACE(m_log, L"  Got this output from " + cmdPrtString + L" : " + StrFromUTF8(prtconfResult) );
            size_t lengthCaptured = prtconfResult.length();

            // Truncate trailing newline if there in captured output                  
            if (lengthCaptured > 0)
            {
              if (prtconfResult[lengthCaptured - 1] == '\n')
              {
                  prtconfResult[lengthCaptured - 1] = '\0';
              }
            }

        }
        catch(SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, L"Unable to determine boot partition using prtconf ..." + e.What());
            return false;
        }

        SCXRegexPtr solPrtconfPatternPtr(NULL);

        std::vector<wstring> matchingVector;

        // Let's build our RegEx:
        try
        {
            SCX_LOGTRACE(m_log, L"  Using this regex on PrtConf output: " + c_SolPrtconfPattern );
            solPrtconfPatternPtr = new SCXCoreLib::SCXRegex(c_SolPrtconfPattern);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            SCX_LOGERROR(m_log, L"Exception caught in compiling regex: " + e.What());
            return false;
        }

        std::istringstream stringStrmPrtconf(prtconfResult);
        vector<wstring>  allLines;                       // all lines read from prtconf output
        allLines.clear();
        SCXStream::NLFs nlfs;
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stringStrmPrtconf, allLines, nlfs);

        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end(); it++)
        {
            curLine.assign(*it);
            matchingVector.clear();

            // Let's get the Boot partition interface and drive letter from prtconf
            if (solPrtconfPatternPtr->ReturnMatch(curLine, matchingVector, 0))
            {
                bootInterfacePath = matchingVector[1];
                SCX_LOGTRACE(m_log, L"Found match of PrtConfPattern : " + bootInterfacePath);
                break;
            }
        }

        if (bootInterfacePath.size() == 0)
        {
            std::wstringstream warningMsg;
            if (matchingVector.size() > 0)
            {
                warningMsg << L"Couldn't find Boot Partition, regular expression error message was: " << matchingVector[0];
            }
            else 
            {
                warningMsg << L"Couldn't find Boot Partition.";
            }
            SCX_LOG(m_log, suppressor.GetSeverity(warningMsg.str()), warningMsg.str());
            return false;
        }     


        // Now we need to build up our pattern to find the bootdisk, using our results from above:
        SCXRegexPtr solLsPatternPtr(NULL);

        wstring solLsPattern(c_SolLsPatternBeg);
#if defined(sparc) &&  PF_MAJOR == 5 && PF_MINOR  == 9
        // Replace "disk" by "sd"
        wstring from(L"disk");
        size_t start_pos = bootInterfacePath.find(from);
        if(start_pos != std::string::npos)
        {
            bootInterfacePath.replace(start_pos, from.length(), L"sd");
        }
#endif
        solLsPattern += bootInterfacePath;
        matchingVector.clear();  //clear the decks!

        //Let's build our RegEx:
        try
        {
            SCX_LOGTRACE(m_log, L"  Using this regex on ls -l /dev/dsk output: " + solLsPattern );
            solLsPatternPtr = new SCXCoreLib::SCXRegex(solLsPattern);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            SCX_LOGERROR(m_log, L"Exception caught in compiling LS Pattern regex: " + e.What());
            return false;
        }


        // Retrieve the bootdrive using the bootInterface and driveLetter
        wstring cmdStringLs = L"/usr/bin/ls -l /dev/dsk";
        std::string devDskResult;

        std::istringstream processInputLs;
        std::ostringstream processOutputLs;
        std::ostringstream processErrLs;
        curLine.clear();

        try 
        {
            SCXCoreLib::SCXProcess::Run(cmdStringLs, processInputLs, processOutputLs, processErrLs, 15000);
            devDskResult = processOutputLs.str();
            SCX_LOGTRACE(m_log, L"  Got this output from " + cmdStringLs + L" : " + StrFromUTF8(devDskResult) );

            size_t lengthCaptured = devDskResult.length();

            // Truncate trailing newline if there in captured output
            if (lengthCaptured > 0)
            {
              if (devDskResult[lengthCaptured - 1] == '\n')
              {
                devDskResult[lengthCaptured - 1] = '\0';
              }
            }

        }
        catch(SCXCoreLib::SCXException &e)
        {
            SCX_LOGERROR(m_log, L"Unable to determine boot partition..." + e.What());
            return false;
        }

        std::istringstream stringStrmDevDsk(devDskResult);
        allLines.clear();
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stringStrmDevDsk, allLines, nlfs);

        wstring bootDisk(L"");
        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end(); it++)
        {
            curLine.assign(*it);
            curLine.push_back('\n');
            matchingVector.clear();

            // Let's get the boot drive
            if (solLsPatternPtr->ReturnMatch(curLine, matchingVector, 0))
            {
                bootDisk = matchingVector[1];  //e.g. "c1t0d0s0"
                break;
            }
        }

        //Check the results
        if (bootDisk.size() == 0)
        {
            std::wstringstream warningMsg;
            if (matchingVector.size() > 0)
            {
                warningMsg << L"Couldn't find Boot Drive, regular expression error message was: " << matchingVector[0];
            }
            else
            {
                warningMsg << L"Couldn't find Boot Drive.";
            }
            SCX_LOG(m_log, suppressor.GetSeverity(warningMsg.str()), warningMsg.str());
            return false;
        }     

        bootpathStr = L"/dev/dsk/" + bootDisk; //e.g. "/dev/dsk/c1t0d0s0"

        return true;
}
#endif

#if defined(linux)

/*----------------------------------------------------------------------------*/
/**
   Update the Linux instance.
*/
void StaticDiskPartitionInstance::Update_Linux()
{
    SCX_LOGTRACE(m_log, L"DiskPartition::Update_Linux():: Entering, DeviceID is:" + m_deviceID);


    // ****************************** start linux ****************************** 

    if (m_fdiskResult.size() > 0)
    {
      //Good, we've already retrieved the fdisk info we need

        std::istringstream fdskStringStrm(m_fdiskResult);

        SCXRegexPtr sectSzRegExPtr(NULL);
        SCXRegexPtr bootDetRegExPtr(NULL);
        SCXRegexPtr detailRegExPtr(NULL);

        std::vector<wstring> matchingVector;

        //Let's build our RegEx:
        try
        {
            sectSzRegExPtr = new SCXCoreLib::SCXRegex(c_RedHSectorSizePattern);
            bootDetRegExPtr = new SCXCoreLib::SCXRegex(c_RedHBootPDetailPattern);
            detailRegExPtr = new SCXCoreLib::SCXRegex(c_RedHDetailPattern);
        }
        catch(SCXCoreLib::SCXInvalidRegexException &e)
        {
            SCX_LOGERROR(m_log, L"Exception caught in compiling regex: " + e.What());
            return;
        }

        vector<wstring>  allLines;                       // all lines read from fdisk output
        allLines.clear();

        unsigned int sectorSz = 0;
        unsigned int savedSectorSz = 0;
        wstring endOffsetStr;
        wstring numBlocksStr;
        bool bootpart = false;
        bool foundIt = false;
        scxulong startBlk = 0;

        SCXStream::NLFs nlfs;
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(fdskStringStrm, allLines, nlfs);
 
        for(vector<wstring>::iterator it = allLines.begin(); it != allLines.end() && !foundIt; it++)
        {
             wstring curLine(*it);
             SCX_LOGTRACE(m_log, L"DiskPartition::Update():: Top of FOR: We have a line= " + (*it));
             matchingVector.clear();

             // First let's get the value for Sector Size
             if (sectSzRegExPtr->ReturnMatch(curLine, matchingVector, 0))
             {
                 //First field [0] repeats the whole matching string
                 if (matchingVector.size() >= 1)
                 {
                     //Want to overwrite any existing values to keep with the latest
                     sectorSz = SCXCoreLib::StrToUInt(matchingVector[1]);
                 }
             }
             else if ((bootDetRegExPtr->ReturnMatch(curLine, matchingVector, 0)) &&
                      (m_deviceID == matchingVector[1]))
             {
                 //This is our record in the fdisk output
                 //First field repeats the whole matching string
                 if ((matchingVector.size() >= 5) && (sectorSz != 0))
                 {
                     savedSectorSz = sectorSz;  //Save off the last Sector size read in.
                     bootpart = true;  //matchingVector[2]
                     startBlk = SCXCoreLib::StrToULong(matchingVector[3]);
                     foundIt = true;

                 }
                 else
                 {
                     SCX_LOGERROR(m_log, L"Matched Boot Detail line, but not enough fields available. Matching Size= " 
                     + StrFrom(matchingVector.size()));
                 }

             }
             else if ((detailRegExPtr->ReturnMatch(curLine, matchingVector, 0)) &&
                      (m_deviceID == matchingVector[1]))
             {
                 //This is our record in the fdisk output
                 //First field repeats the whole matching string
                 if ((matchingVector.size() >= 4) && (sectorSz != 0))
                 {
                     savedSectorSz = sectorSz;  //Save off the last Sector size read in.
                     bootpart = false;
                     startBlk = SCXCoreLib::StrToULong(matchingVector[2]);
                     foundIt = true;
                 }
                 else
                 {
                     SCX_LOGERROR(m_log, L"Matched Boot Detail line, but not enough fields available. Matching Size= " 
                     + StrFrom(matchingVector.size()));
                 }
             }
             else if (matchingVector.size() > 0)
             {
                 //Have an error message
                 SCX_LOGINFO(m_log, L"No match found! Error: " + matchingVector[0]);
             }

        } //EndFor

          
        // Now let's use the 'blockdev' command to get the partition size, and the correct block size:
        std::istringstream processBdevSizeInput;
        std::ostringstream processBdevSizeOutput;
        std::ostringstream processBdevSizeErr;

        scxulong partitionSize = 0;
        scxulong blockDevBsz = 0;
        wstring bDevSizeCmd(c_cmdBlockDevSizeStr);
        bDevSizeCmd += m_deviceID;
        std::string blockDevResult;

        try
        {
            SCXCoreLib::SCXProcess::Run(bDevSizeCmd.c_str(), processBdevSizeInput, processBdevSizeOutput, processBdevSizeErr, 15000);
            blockDevResult = processBdevSizeOutput.str();
            SCX_LOGTRACE(m_log, StrAppend(wstring(L"  Got this output from BlockDev: "), StrFromUTF8(blockDevResult)));

            std::string errOutPSz = processBdevSizeErr.str();
            if (errOutPSz.size() > 0)
            {
                SCX_LOGERROR(m_log, StrAppend(L"Got this error string from blockdev GetSize command: ", StrFromUTF8(errOutPSz)));
            }
        }
        catch (SCXCoreLib::SCXException &e) 
        {
            SCX_LOGERROR(m_log, L"attempt to execute blockdev command for the purpose of retrieving partition information failed.");
            return;  //No use going on . . .
        }

        if (blockDevResult.size() == 0)
        {
            SCX_LOGERROR(m_log, L"Unable to retrieve partition Size information from OS...");
            return;
        }

        //So, everything checks out, lets save off the result:
        partitionSize = SCXCoreLib::StrToULong(SCXCoreLib::StrFromUTF8(blockDevResult));


        //Now we'll use blockdev to retrieve the blocksize for the file system (as opposed to the kernel buffer block size).
        std::istringstream processBlkSizeInput;
        std::ostringstream processBlkSizeOutput;
        std::ostringstream processBlkSizeErr;
        wstring bDevBlkSizeCmd(c_cmdBlockDevBszStr);
        bDevBlkSizeCmd += m_deviceID;
        blockDevResult.clear();

        try
        {
            SCXCoreLib::SCXProcess::Run(bDevBlkSizeCmd.c_str(), processBlkSizeInput, processBlkSizeOutput, processBlkSizeErr, 15000);
            blockDevResult = processBlkSizeOutput.str();
            SCX_LOGTRACE(m_log, StrAppend(L"  Got this output from BlockDev: ", StrFromUTF8(blockDevResult)));

            std::string errOutBsz = processBlkSizeErr.str();
            if (errOutBsz.size() > 0)
            {
                SCX_LOGERROR(m_log, StrAppend(L"Got this error string from blockdev GetSize command: ", StrFromUTF8(errOutBsz)));
            }
        }
        catch (SCXCoreLib::SCXException &e) 
        {
            SCX_LOGERROR(m_log, L"attempt to execute blockdev command for the purpose of retrieving partition information failed.");
            return;  //No use going on . . .
        }

        if (blockDevResult.size() == 0)
        {
            SCX_LOGERROR(m_log, L"Unable to retrieve partition Size information from OS...");
            return;
        }

        //So, everything checks out, lets save off the result:
        blockDevBsz = SCXCoreLib::StrToULong(SCXCoreLib::StrFromUTF8(blockDevResult));

        // Now, let's fill in our fields  
        if (foundIt && startBlk > 0)
        {      
           // fill in all the fields in the instance of PartitionInformation
           m_bootPartition = bootpart; 
           m_partitionSize = partitionSize * savedSectorSz;
           m_blockSize = blockDevBsz;
           m_startingOffset = startBlk;
           m_numberOfBlocks = RoundToUnsignedInt(
                                 static_cast<double>(m_partitionSize) / 
                                            static_cast<double>(m_blockSize)); ;
        }
        else
        {
           SCX_LOGERROR(m_log, L"Partition Record Not Found! Starting Block= " + StrFrom(startBlk) +
                        L"  And Regex Error Msg: " + matchingVector[0]);

        }

    }
    else
    {
      //The fdisk command failed earlier
      SCX_LOGERROR(m_log, L"No fdisk information available for this instance.");
    }

    return;

    // ****************************** end linux ******************************
}

#endif


    
} /* namespace SCXSystemLib */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
