/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       This file contains the abstraction of the kstat system on Solaris

    \date        2007-08-22 11:39:54


*/
/*----------------------------------------------------------------------------*/
#ifndef SCXKSTAT_H
#define SCXKSTAT_H

#if !defined(sun)
#error this file is only meaningful on the SunOS platform
#endif

#include <string>
#include <sstream>
#include <kstat.h>
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxthread.h>
#include <scxcorelib/scxthreadlock.h>

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    /**
       This class encapsulates dependencies for the kstat system on solaris.

       \date 2008-11-12 13:30
    */
    class SCXKstatDependencies
    {
    public:
        SCXKstatDependencies() {
            m_lock = SCXCoreLib::ThreadLockHandleGet(L"SCXSystemLib::SCXKstatDependencies");
        }

        /** Virtual destructor */
        virtual ~SCXKstatDependencies() {}

        /** Open kstat system.
            \returns a kstat_ctl_t pointer.
        */
        virtual kstat_ctl_t* Open() {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            return kstat_open();
        }

        /** Close the kstat system.
            \param pCCS A kstat_ctl_t pointer.
        */
        virtual void Close(kstat_ctl_t* pCCS) {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            kstat_close(pCCS);
        }

        /** Update the kstat header chain.
            \param pCCS A kstat_ctl_t pointer.
            \returns Upon successful completion, kstat_chain_update() returns the new
            KCID if the kstat chain has changed and 0 if it has not changed.
            Otherwise, it returns -1 and sets errno to indicate the error.
        */
        virtual kid_t Update(kstat_ctl_t* pCCS) {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            return kstat_chain_update(pCCS);
        }

        /** Lookup a kstat instance.
            \param pCCS A kstat_ctl_t pointer.
            \param m Module.
            \param i Instance.
            \param n Name.
            \returns A kstat_t pointer.
        */
        virtual kstat_t* Lookup(kstat_ctl_t* pCCS, char* m, int i, char* n) {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            return kstat_lookup(pCCS, m, i, n);
        }

        /** Read a kstat instance.
            \param pCCS A kstat_ctl_t pointer.
            \param pKS A kstat_t pointer.
            \param p A void pointer.
            \returns -1 on error.
        */
        virtual int Read(kstat_ctl_t* pCCS, kstat_t* pKS, void* p) {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            return kstat_read(pCCS, pKS, p);
        }

        /**
              Extract named data from a kstat instance.
              \param pKS        a kstat_t pointer
              \param statistic   the name of the statistic
              \returns a pointer to the data element, or NULL if it doesn't exist.
              */
        virtual void* DataLookup(kstat_t* pKS, const std::wstring& statistic) {
            SCXCoreLib::SCXThreadLock lock(m_lock);
            return kstat_data_lookup(pKS, const_cast<char*>(SCXCoreLib::StrToUTF8(statistic).c_str()));
        }

        private:

        //! Named lock for this instance to be shared across all threads
        SCXCoreLib::SCXThreadLockHandle m_lock;
    };

    /*----------------------------------------------------------------------------*/
    /**
        This class represents the file system metrics on Solaris that are available
        to both KSTAT_TYPE_NAMED and KSTAT_TYPE_IO.

        Prior to Solaris 10, the recommended method for getting FS statistics from
        kstat was to lookup the appropriate KSTAT_TYPE_IO structure for the device.
        Since Solaris 10, the recommended method is to use VOP statistics and lookup
        a KSTAT_TYPE_NAMED structure.  These two structures do not map 1-to-1, they
        do not even completely cover each other.  This class represents a mapping
        of the statistics that are common or at least seem to have a reasonable
        mapping.
    */
    class SCXKstatFSSample
    {
    private:
        scxulong m_numReadOps;
        scxulong m_bytesRead;
        scxulong m_numWriteOps;
        scxulong m_bytesWritten;

    public:
        SCXKstatFSSample(scxulong numReadOps, scxulong bytesRead, scxulong numWriteOps, scxulong bytesWritten)
            : m_numReadOps(numReadOps),
              m_bytesRead(bytesRead),
              m_numWriteOps(numWriteOps),
              m_bytesWritten(bytesWritten)
        {
        }
        SCXKstatFSSample(const SCXKstatFSSample & source)
            : m_numReadOps(source.m_numReadOps),
              m_bytesRead(source.m_bytesRead),
              m_numWriteOps(source.m_numWriteOps),
              m_bytesWritten(source.m_bytesWritten)
        {
        }

        SCXKstatFSSample & operator=(const SCXKstatFSSample & source)
        {
            if (&source != this)
            {
                m_numReadOps   = source.m_numReadOps;
                m_bytesRead    = source.m_bytesRead;
                m_numWriteOps  = source.m_numWriteOps;
                m_bytesWritten = source.m_bytesWritten;
            }

            return *this;
        }

        scxulong GetNumReadOps() const { return m_numReadOps; }
        scxulong GetBytesRead() const { return m_bytesRead; }
        scxulong GetNumWriteOps() const { return m_numWriteOps; }
        scxulong GetBytesWritten() const { return m_bytesWritten; }
    };

    /*----------------------------------------------------------------------------*/
    /**
        This class encapsulates the kstat system on Solaris.

        \date        2007-08-22 11:41:20

        Most uses of the kstat system follows the same pattern. By encapsulating
        this pattern in this class, writers of PALs do not have to worry
        to much about the inner workings of kstat.
    */
    class SCXKstat
    {
    private:
        kstat_ctl_t* m_ChainControlStructure;  //!< Pointer to kstat chain.
        kstat_t*     m_KstatPointer;           //!< Pointer to retrieved kstat.

        bool TryGetStatisticFromNamed(const std::wstring& statistic, scxulong& value) const;
        bool TryGetStatisticFromIO(const std::wstring& statistic, scxulong& value) const;

        SCXKstatFSSample GetFSSampleFromNamed() const;
        SCXKstatFSSample GetFSSampleFromIO() const;

    protected:
        /** Test constructor - used during tests for dependency injection
         */
        SCXKstat(SCXCoreLib::SCXHandle<SCXKstatDependencies> deps)
            : m_ChainControlStructure(0),
              m_KstatPointer(0),
              m_deps(deps)
        { }

        SCXCoreLib::SCXHandle<SCXKstatDependencies> m_deps; //!< Dependency object.

        /** Gets pointer to external data. This is a way for a mock-object to
            replace a pointer to RAW data with a pointer to a local area
            over which it has control.
            \returns NULL for the base class. Override in subclasses.
        */
        virtual void* GetExternalDataPointer() { return 0; }

        /** Initializes the kstat object for given instance.
            \throws      SCXKstatErrorException if kstat internal error.
            \throws      SCXKstatNotFoundException if requested kstat is not found in kstat system.
        */
        void Init();

    public:
        SCXKstat();
        virtual ~SCXKstat();

        virtual void Update();
        virtual void Lookup(const std::wstring& module, const std::wstring& name, int instance = -1);
        virtual void Lookup(const std::wstring& module, int instance = -1);
        virtual void Lookup(const char* module, const char *name, int instance = -1);
        virtual scxulong GetValue(const std::wstring& statistic) const;
        virtual bool TryGetValue(const std::wstring& statistic, scxulong& value) const;
        template<typename T> void GetValueRaw(const T*& dataArea); // Defined below
        virtual SCXKstatFSSample GetFSSample() const;

        virtual std::wstring DumpString() const;

        virtual kstat_t* ResetInternalIterator();
        virtual kstat_t* AdvanceInternalIterator();

        /** Read string value from the kstat object. 
            \throws       SCXNotSupportedException if named data is of unsupported type.
        */
        virtual bool TryGetStringValue(const std::wstring& statistic, std::wstring& value) const;
    };

    /** Exception for general kstat error */
    class SCXKstatException : public SCXCoreLib::SCXException
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] module Module name of related kstat object.
            \param[in] instance Instance number of related kstat object.
            \param[in] name Name of related kstat object.
            \param[in] l Source code location.
        */
        SCXKstatException(std::wstring reason, int eno,
                          const std::wstring& module, int instance, const std::wstring name,
                          const SCXCoreLib::SCXCodeLocation& l)
            : SCXException(l),
              m_Reason(reason),
              m_errno(eno)
        {
            m_Path = module;
            m_Path.append(L":").append(SCXCoreLib::StrFrom(instance)).append(L":").append(name);
        };

        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] l Source code location.
        */
        SCXKstatException(std::wstring reason, int eno, const SCXCoreLib::SCXCodeLocation& l)
            : SCXException(l),
              m_Reason(reason),
              m_errno(eno),
              m_Path(L"::")
        { }

        std::wstring What() const;
        /*----------------------------------------------------------------------------*/
        /**
           Return errno related to the error.
        */
        int GetErrno() const { return m_errno; }

    protected:
        //! Description of internal error
        std::wstring   m_Reason;
        int m_errno; //!< Errno related to the error.
        std::wstring m_Path; //!< Kstat path describing related kstat object.
    };

    /** Exception for kstat internal error */
    class SCXKstatErrorException : public SCXKstatException
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] module Module name of related kstat object.
            \param[in] instance Instance number of related kstat object.
            \param[in] name Name of related kstat object.
            \param[in] l Source code location.
        */
        SCXKstatErrorException(std::wstring reason, int eno,
            const std::wstring& module, int instance, const std::wstring name,
            const SCXCoreLib::SCXCodeLocation& l)
            : SCXKstatException(reason, eno, module, instance, name, l)
        {};

        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] l Source code location.
        */
        SCXKstatErrorException(std::wstring reason, int eno, const SCXCoreLib::SCXCodeLocation& l)
            : SCXKstatException(reason, eno, l)
        {};
    };

    /** Exception for when kstat was not found */
    class SCXKstatNotFoundException : public SCXKstatException
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] module Module name of related kstat object.
            \param[in] instance Instance number of related kstat object.
            \param[in] name Name of related kstat object.
            \param[in] l Source code location.
        */
        SCXKstatNotFoundException(std::wstring reason, int eno,
            const std::wstring& module, int instance, const std::wstring name,
            const SCXCoreLib::SCXCodeLocation& l)
            : SCXKstatException(reason, eno, module, instance, name, l)
        {};

        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] l Source code location.
        */
        SCXKstatNotFoundException(std::wstring reason, int eno,const SCXCoreLib::SCXCodeLocation& l)
            : SCXKstatException(reason, eno, l)
        {};
    };

    /** Exception for when a speciffic kstat speciffic was not found */
    class SCXKstatStatisticNotFoundException : public SCXKstatException
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] module Module name of related kstat object.
            \param[in] instance Instance number of related kstat object.
            \param[in] name Name of related kstat object.
            \param[in] l Source code location.
        */
        SCXKstatStatisticNotFoundException(std::wstring reason, int eno,
            const std::wstring& module, int instance, const std::wstring name,
            const SCXCoreLib::SCXCodeLocation& l)
            : SCXKstatException(reason, eno, module, instance, name, l)
        {};

        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in] reason Reason for exception.
            \param[in] eno Errno related to the error.
            \param[in] l Source code location.
        */
        SCXKstatStatisticNotFoundException(std::wstring reason, int eno,
                                           const SCXCoreLib::SCXCodeLocation& l)
            : SCXKstatException(reason, eno, l)
        {};
    };

    /**
       Retrieves raw data from the kstat interface. Raw data from Kstat is expected
       to map directly to a "known" datatype, represented by T in this template. This
       method will return a pointer to such raw data. The user is not expected to
       allocate a data area, but will instead receive a pointer directly onto the Kstat data.

       \param[out]      dataArea Will receive a pointer to raw data from kstat
       \throws          SCXNotSupportedException if trying to extract non-raw data, or
       if size of data area doesn't match size of retrieved data.
     */
    template<typename T> inline void SCXKstat::GetValueRaw(const T*& dataArea)
    {
        SCXASSERT(KSTAT_TYPE_RAW == m_KstatPointer->ks_type);
        SCXASSERT(sizeof(T) == m_KstatPointer->ks_data_size);

        if (KSTAT_TYPE_RAW != m_KstatPointer->ks_type) {
            throw SCXCoreLib::SCXNotSupportedException(L"kstat type must be \"raw\"",
                                                       SCXSRCLOCATION);
        }

        if (sizeof(T) != m_KstatPointer->ks_data_size) {
            std::ostringstream errmsg;
            errmsg << "Size of data for kstat module " << m_KstatPointer->ks_module
                   << " doesn't match datatype \"" << typeid(T).name() << "\":";
            throw SCXCoreLib::SCXNotSupportedException(SCXCoreLib::StrFromUTF8(
                errmsg.str()), SCXSRCLOCATION);
        }

        // Get mock data area, or real data area.
        if (0 == (dataArea = static_cast<T*>(GetExternalDataPointer()))) {
            dataArea = static_cast<T*>(m_KstatPointer->ks_data);
        }
    }

}

#endif /* SCXKSTAT_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
