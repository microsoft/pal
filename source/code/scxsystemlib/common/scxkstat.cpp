/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Contains implementation of the SCXKstat class.
    
    \date        2007-08-22 11:53:26

    
*/
/*----------------------------------------------------------------------------*/

// Only for solaris!

#if defined(sun)

#include <scxcorelib/scxcmn.h>

#include <scxsystemlib/scxkstat.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxlog.h>
#include <errno.h>

using namespace SCXCoreLib;


namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    
        \throws      SCXKstatErrorException if kstat internal error.
    
        Creates a SCXKstat object. Runs kstat_open and kstat_lookup.
    
    */
    SCXKstat::SCXKstat()
        : m_ChainControlStructure(0),
          m_KstatPointer(0),
          m_deps(0)
    {
        m_deps = new SCXKstatDependencies();

        Init();
    }


    void SCXKstat::Init()
    {
        if (NULL == m_ChainControlStructure)
        {
            m_ChainControlStructure = m_deps->Open();
            if (0 == m_ChainControlStructure)
            {
                SCXASSERT( ! "kstat_open() failed");
                throw SCXKstatErrorException(L"kstat_open() failed", errno, SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    
        Calls kstat_close.
    
    */
    SCXKstat::~SCXKstat()
    {
        // Frees all items associated with m_ChainControlStructure. Including m_KstatPointer.
        if (m_ChainControlStructure != 0) 
        {
            m_deps->Close(m_ChainControlStructure);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Refresh the kstat data
    
       \param       statistic - name of named statistic to get.
       \throws      SCXKstatErrorException if update fails and errno != EAGAIN.

       If we get an error and errno is EAGAIN, we will just ignore that for now.
       The result is that we will get stale data until next time we do update. We could
       do some "smart" loop where we try a couple of times with sleeps between but
       as of now we don't know if that would even give us anything.
    */
    void SCXKstat::Update()
    {
        if (-1 == m_deps->Update(m_ChainControlStructure))
        {
            if (errno != EAGAIN)
            {
                throw SCXKstatErrorException(L"kstat_chain_update() failed", errno, SCXSRCLOCATION);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Look up (find by name) and read a section of the kstat data
    
       \param       module    Module to read from
       \param       name      KStat name to read
       \param       instance  Instance number to read
       \throws      SCXKstatNotFoundException if kstat data could not be found by name
       \throws      SCXKstatErrorException if unable to read KStat data
    */
    void SCXKstat::Lookup(const std::wstring& module, const std::wstring& name, int instance /* = -1 */)
    {
        Lookup(SCXCoreLib::StrToUTF8(module).c_str(),
               SCXCoreLib::StrToUTF8(name).c_str(),
               instance);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Look up (find by name) and read a section of the kstat data
    
       \param       module    Module to read from
       \param       instance  Instance number to read
       \throws      SCXKstatNotFoundException if kstat data could not be found by name
       \throws      SCXKstatErrorException if unable to read KStat data
    */
    void SCXKstat::Lookup(const std::wstring& module, int instance /* = -1 */)
    {
        Lookup(SCXCoreLib::StrToUTF8(module).c_str(),
               NULL,
               instance);
    }

    /*----------------------------------------------------------------------------*/
    /**
       Look up (find by name) and read a section of the kstat data
    
       \param       module    Module to read from
       \param       name      KStat name to read
       \param       instance  Instance number to read
       \throws      SCXKstatNotFoundException if kstat data could not be found by name
       \throws      SCXKstatErrorException if unable to read KStat data
    */
    void SCXKstat::Lookup(const char *module, const char *name, int instance /* = -1 */)
    {
        m_KstatPointer = m_deps->Lookup(m_ChainControlStructure,
                                        const_cast<char*>(module),
                                        instance,
                                        const_cast<char*>(name));
        if (0 == m_KstatPointer)
        {
            throw SCXKstatNotFoundException(L"kstat_lookup() could not find kstat",
                                            errno,
                                            SCXCoreLib::StrFromUTF8(module),
                                            instance,
                                            ( name != NULL ? SCXCoreLib::StrFromUTF8(name) : L""),
                                            SCXSRCLOCATION);
        }

        // WI 17733 / 24844: We can get transient errors (certainly EAGAIN, unsure of
        // exactly when ENXIO is raised).  So try to read (and if fail, update) a few
        // times in case the error clears up.

        const int retryCountMax = 3;
        int retryCount = 0;

        for ( ; ; )
        {
            retryCount++;

            if (-1 != m_deps->Read(m_ChainControlStructure, m_KstatPointer, 0))
            {
                // Successful, so we're done
                break;
            }

            // If no tries are left, then throw exception

            if (retryCount >= retryCountMax)
            {
                throw SCXKstatErrorException(L"kstat_read() failed",
                                             errno,
                                             SCXCoreLib::StrFromUTF8(module),
                                             instance,
                                             ( name != NULL ? SCXCoreLib::StrFromUTF8(name) : L""),
                                             SCXSRCLOCATION);
            }

            // If EAGAIN, then don't try the kstat_chain_update (just retry the read)
            // Otherwise, try to update our chain (hoping that will clear up the error)

            if (EAGAIN != errno)
            {
                // If success or fail, we don't care; retry kstat_read regardless
                m_deps->Update(m_ChainControlStructure);
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
       Get a named value as an scxulong
    
       \param       statistic - name of named statistic to get.
       \returns     value of named value in kstat.
       \throws      SCXNotSupportedException if kstat is not of a known type.
       \throws      SCXKstatStatisticNotFoundException if requested statistic is not found in kstat.
    
       Return a named value from kstat converted to an scxulong.
    */
    scxulong SCXKstat::GetValue(const std::wstring& statistic) const
    {
        scxulong results;
        
        switch (m_KstatPointer->ks_type)
        {
        case KSTAT_TYPE_RAW:
            SCXASSERT( ! "You can't use GetValue() to read kstat type \"raw\". Use GetValueRaw() instead");
            throw SCXCoreLib::SCXNotSupportedException(L"Unsupported kstat type \"raw\" in this method", SCXSRCLOCATION);
        case KSTAT_TYPE_NAMED:
            if (!TryGetValue(statistic, results))
            {
                SCXASSERT( ! "kstat_data_lookup() failed");
                throw SCXKstatStatisticNotFoundException(SCXCoreLib::StrAppend(L"kstat_data_lookup() failed: ", statistic), errno, SCXSRCLOCATION);
            }
            return results;
        case KSTAT_TYPE_INTR:
            SCXASSERT( ! "Unsupported kstat type \"intr\"");
            throw SCXCoreLib::SCXNotSupportedException(L"Unsupported kstat type \"intr\"", SCXSRCLOCATION);
        case KSTAT_TYPE_IO:
            if (!TryGetValue(statistic, results))
            {
                SCXASSERT( ! "Unknown statistic");
                throw SCXKstatStatisticNotFoundException(SCXCoreLib::StrAppend(L"Unknown statistic: ", statistic), 0, SCXSRCLOCATION);
            }   
            return results;
        case KSTAT_TYPE_TIMER:
            SCXASSERT( ! "Unsupported kstat type \"timer\"");
            throw SCXCoreLib::SCXNotSupportedException(L"Unsupported kstat type \"timer\"", SCXSRCLOCATION);
        default:
            std::wstring error(L"Unknown kstat type: ");
            error = SCXCoreLib::StrAppend(error, m_KstatPointer->ks_type);
            SCXASSERT( ! error.c_str());
            throw SCXCoreLib::SCXNotSupportedException(error.c_str(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get a named value as an scxulong
    
        \param       statistic - name of named statistic to get.
       \param      value - the extracted value if there is one
       \returns     true if a value was extracted, or false otherwise.
        \throws      SCXNotSupportedException if kstat is not of a known type.
        \throws      SCXKstatStatisticNotFoundException if requested statistic is not found in kstat.
        
        Return a named value from kstat converted to an scxulong.
    */
    bool SCXKstat::TryGetValue(const std::wstring& statistic, scxulong& value) const
    {
        switch (m_KstatPointer->ks_type)
        {
        case KSTAT_TYPE_RAW:
            SCXASSERTFAIL(SCXCoreLib::StrAppend(L"You can't use GetValue() to read kstat type \"raw\". Use GetValueRaw() instead. Offending statistic was: ", statistic).c_str());
            throw SCXCoreLib::SCXNotSupportedException(L"Unsupported kstat type \"raw\" in this method", SCXSRCLOCATION);
        case KSTAT_TYPE_NAMED:
            return TryGetStatisticFromNamed(statistic, value);
        case KSTAT_TYPE_INTR:
            SCXASSERTFAIL(SCXCoreLib::StrAppend(L"Unsupported kstat type \"intr\". Offending statistic was: ", statistic).c_str());
            throw SCXCoreLib::SCXNotSupportedException(L"Unsupported kstat type \"intr\"", SCXSRCLOCATION);
        case KSTAT_TYPE_IO:
            return TryGetStatisticFromIO(statistic, value);
        case KSTAT_TYPE_TIMER:
            SCXASSERTFAIL(SCXCoreLib::StrAppend(L"Unsupported kstat type \"timer\". Offending statistic was: ", statistic).c_str());
            throw SCXCoreLib::SCXNotSupportedException(L"Unsupported kstat type \"timer\"", SCXSRCLOCATION);
        default:
            std::wstring error(L"Unknown kstat type: ");
            error = SCXCoreLib::StrAppend(error, m_KstatPointer->ks_type);
            error = SCXCoreLib::StrAppend(error, L". Offending statistic was: ");
            error = SCXCoreLib::StrAppend(error, statistic);
            SCXASSERTFAIL(error.c_str());
            throw SCXCoreLib::SCXNotSupportedException(error.c_str(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the common file system metrics for the last kstat lookup.
        \returns     the common file system metrics for the last kstat lookup.
        \throws      SCXNotSupportedException if kstat is not of a known type.
        \throws      SCXKstatStatisticNotFoundException if requested statistic is not found in kstat.
    */
    SCXKstatFSSample SCXKstat::GetFSSample() const
    {
        switch (m_KstatPointer->ks_type)
        {
        case KSTAT_TYPE_NAMED:
            return GetFSSampleFromNamed();
        case KSTAT_TYPE_IO:
            return GetFSSampleFromIO();
        default:
            std::wstring error(L"The kstat(");
            error = SCXCoreLib::StrAppend(error, m_KstatPointer->ks_module);
            error = SCXCoreLib::StrAppend(error, L":");
            error = SCXCoreLib::StrAppend(error, m_KstatPointer->ks_instance);
            error = SCXCoreLib::StrAppend(error, L":");
            error = SCXCoreLib::StrAppend(error, m_KstatPointer->ks_name);
            error = SCXCoreLib::StrAppend(error, L") does not support file system samples.  Offending type was: ");
            error = SCXCoreLib::StrAppend(error, m_KstatPointer->ks_type);
            SCXASSERTFAIL(error.c_str());
            throw SCXCoreLib::SCXNotSupportedException(error.c_str(), SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the value of the statistic from a kstat named data and return as an scxulong.
       
        \param        statistic - name of named statistic to get.
        \param        value - the value of the statistic
        \returns      true if a value was extracted, false otherwise.
        \throws       SCXNotSupportedException if named data is of unsupported type.

    */
    bool SCXKstat::TryGetStatisticFromNamed(const std::wstring& statistic, scxulong& value) const
    {
        SCXASSERT(KSTAT_TYPE_NAMED == m_KstatPointer->ks_type);

        kstat_named_t* named = static_cast<kstat_named_t*>(m_deps->DataLookup(m_KstatPointer, statistic));
        if (0 == named)
        {
            return false;
        }

        switch (named->data_type) {
        case KSTAT_DATA_CHAR:
            value = 0;
            return true;
        case KSTAT_DATA_INT32:
            value = named->value.i32;
            return true;
        case KSTAT_DATA_UINT32:
            value = named->value.ui32;
            return true;
        case KSTAT_DATA_INT64:
            value = named->value.i64;
            return true;
        case KSTAT_DATA_UINT64:
            value = named->value.ui64;
            return true;
        }
        
        SCXASSERT( ! "kstat named data of unknown type");
        throw SCXCoreLib::SCXNotSupportedException(L"Named data of unknown type", SCXSRCLOCATION);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the value of the statistic from a kstat IO struct return as an scxulong.
       
        \param        statistic - name of named statistic to get.
        \param       value - the extracted value
        \returns      true if a value was extracted, false otherwise
        \throws       SCXKstatErrorException if kstat internal error.

    */
    bool SCXKstat::TryGetStatisticFromIO(const std::wstring& statistic, scxulong& value) const
    {
        SCXASSERT(KSTAT_TYPE_IO == m_KstatPointer->ks_type);
        
        if (m_KstatPointer->ks_data_size != sizeof(kstat_io_t))
        {
            SCXASSERT( ! "kstat data is of wrong size!");
            throw SCXKstatErrorException(SCXCoreLib::StrAppend(L"kstat data is of wrong size: ", statistic), 0, SCXSRCLOCATION);
        }

        kstat_io_t* iostruct = 
            static_cast<kstat_io_t*>(m_KstatPointer->ks_data);

        if (L"nread" == statistic)
        {
            value = iostruct->nread;
            return true;
        }
        else if (L"nwritten" == statistic)
        {
            value = iostruct->nwritten;
            return true;
        }
        else if (L"reads" == statistic)
        {
            value = iostruct->reads;
            return true;
        }
        else if (L"writes" == statistic)
        {
            value = iostruct->writes;
            return true;
        }
        else if (L"wtime" == statistic)
        {
            value = iostruct->wtime;
            return true;
        }
        else if (L"wlentime" == statistic)
        {
            value = iostruct->wlentime;
            return true;
        }
        else if (L"wlastupdate" == statistic)
        {
            value = iostruct->wlastupdate;
            return true;
        }
        else if (L"rtime" == statistic)
        {
            value = iostruct->rtime;
            return true;
        }
        else if (L"rlentime" == statistic)
        {
            value = iostruct->rlentime;
            return true;
        }
        else if (L"rlastupdate" == statistic)
        {
            value = iostruct->rlastupdate;
            return true;
        }
        else if (L"wcnt" == statistic)
        {
            value = iostruct->wcnt;
            return true;
        }
        else if (L"rcnt" == statistic)
        {
            value = iostruct->rcnt;
            return true;
        }

        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the common file system metrics for a kstat named data from the last kstat lookup.
        \returns     the common file system metrics for the last kstat lookup.
        \throws      SCXNotSupportedException if kstat is not of a known type.
        \throws      SCXKstatStatisticNotFoundException if requested statistic is not found in kstat.
    */
    SCXKstatFSSample SCXKstat::GetFSSampleFromNamed() const
    {
        scxulong numReadOps   = GetValue(L"nread");
        scxulong bytesRead    = GetValue(L"read_bytes");
        scxulong numWriteOps  = GetValue(L"nwrite");
        scxulong bytesWritten = GetValue(L"write_bytes");

        return SCXKstatFSSample(numReadOps, bytesRead, numWriteOps, bytesWritten);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the common file system metrics for a kstat IO struct from the last kstat lookup.
        \returns     the common file system metrics for the last kstat lookup.
        \throws      SCXNotSupportedException if kstat is not of a known type.
        \throws      SCXKstatStatisticNotFoundException if requested statistic is not found in kstat.
    */
    SCXKstatFSSample SCXKstat::GetFSSampleFromIO() const
    {
        scxulong numReadOps   = GetValue(L"reads");
        scxulong bytesRead    = GetValue(L"nread");
        scxulong numWriteOps  = GetValue(L"writes");
        scxulong bytesWritten = GetValue(L"nwritten");

        return SCXKstatFSSample(numReadOps, bytesRead, numWriteOps, bytesWritten);
    }

    /*----------------------------------------------------------------------------*/
    /**
         Dump object as string (for logging).
    
         Parameters:  None
         Retval:      String representation of object.
        
    */
    std::wstring SCXKstat::DumpString() const
    {
        return L"SCXKstat: <No data>";
    }


    /*----------------------------------------------------------------------------*/
    /**
      Reset the internal iterator
      */
    kstat_t* SCXKstat::ResetInternalIterator()
    {
        m_KstatPointer = m_ChainControlStructure->kc_chain;
        m_deps->Read(m_ChainControlStructure, m_KstatPointer, NULL);
        return m_KstatPointer;
    }

    /*----------------------------------------------------------------------------*/
    /**
      Advance the internal iterator
      */
    kstat_t* SCXKstat::AdvanceInternalIterator()
    {
        // It's not an error to advance the iterator at the end of the chain
        if (NULL != m_KstatPointer)
        {
            m_KstatPointer = m_KstatPointer->ks_next;
            if (NULL != m_KstatPointer)
            {
                m_deps->Read(m_ChainControlStructure, m_KstatPointer, NULL);
            }
        }

        return m_KstatPointer;
    }

    /*----------------------------------------------------------------------------*/
    /**
         Format details of violation
       
    */
    std::wstring SCXKstatException::What() const 
    { 
        std::wstring s = L"kstat error (";
        s.append(m_Path).append(L"): ");
        s.append(m_Reason).append(L" (");
        s.append(SCXCoreLib::StrFrom(GetErrno())).append(L")");
        return s;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get the value of the statistic from a kstat named data and return as an scxulong.
       
        \param        statistic - name of named statistic to get.
        \param        value - the string value of the statistic
        \returns      true if a value was extracted, false otherwise.
        \throws       SCXNotSupportedException if named data is of unsupported type.

    */
    bool SCXKstat::TryGetStringValue(const std::wstring& statistic, std::wstring& value) const
    {
        SCXASSERT(KSTAT_TYPE_NAMED == m_KstatPointer->ks_type);
        
        kstat_named_t* named = static_cast<kstat_named_t*>(m_deps->DataLookup(m_KstatPointer, statistic));

        if (0 == named)
        {
            return false;
        }
        else
        {

            switch (named->data_type) {
            case KSTAT_DATA_CHAR:
                value = StrFrom(named->value.c);
                return true;
            case KSTAT_DATA_INT32:
                value = StrFrom(named->value.i32);
                return true;
            case KSTAT_DATA_UINT32:
                value = StrFrom(named->value.ui32);
                return true;
            case KSTAT_DATA_INT64:
                value = StrFrom(named->value.i64);
                return true;
            case KSTAT_DATA_UINT64:
                value = StrFrom(named->value.ui64);
                return true;
            case KSTAT_DATA_STRING:
                value = StrFromMultibyte(std::string(KSTAT_NAMED_STR_PTR(named), KSTAT_NAMED_STR_BUFLEN(named)));
                return true;
            }
        }
        
        SCXASSERT( ! "kstat named data of unknown type");
        throw SCXCoreLib::SCXNotSupportedException(L"Named data of unknown type", SCXSRCLOCATION);
    }


} // SCXSystemLib

#endif //sun
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
