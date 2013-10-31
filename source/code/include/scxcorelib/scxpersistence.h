/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Public interface of the persistence framework
    
    \date        2008-08-21 12:52:11

    Design of small and easy to implement persistence framework.
    Minimal design that should be easily extendable when we get more
    detailed use cases, but that should be sufficient for what we see as the
    current needs: 
    saving the state of providers before they get unloaded.
    Uses a stream-like interface for writing and reading from some media.
    Influenced by XML in the interface, but not restricted to XML in the
    persistence format. Only handles std::wstring data, since we already have
    lots of convenience methods for converting to/from strings.
    Since the media creates both the reader and the writer, data could be
    read/written directly from/to the media without need for caching in
    memory.
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXPERSISTENCE_H
#define SCXPERSISTENCE_H

#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxexception.h>

namespace SCXCoreLib {

    /*----------------------------------------------------------------------------*/
    /**
        Interface for Persistence Data Reader.
        Contains a set of pure virtual methods that any data reader must
        implement.
        Used to parse a stream of persitance data previously written by a
        persistence writer.
    */
    class SCXPersistDataReader
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Default constructor.
        */
        SCXPersistDataReader() {};

        /*----------------------------------------------------------------------------*/
        /**
            Destructor
        */
        virtual ~SCXPersistDataReader() {};

        /*----------------------------------------------------------------------------*/
        /**
            Get persistence data version

            \returns   persistence data version
            Retrive version stored by data writer.
        */
        virtual unsigned int GetVersion() = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Check if current item is a "start group" tag with the given name and if so
            consumes that item.

            \param[in] name     Name of start group tag to check for.
            \param[in] dothrow  Should this function throw an exception if current
                                item is not a start group tag with the given name.

            \returns   true if current item is a start group tag with the given name,
                       else false.
        */
        virtual bool ConsumeStartGroup(const std::wstring& name, bool dothrow = false) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Check if current item is an "end group" tag with the given name and if so
            consumes that item.

            \param[in] dothrow  Should this function throw an exception if current
                                item is not an end group tag with the given name.

            \returns   true if current item is an end group tag with the given name,
                       else false.
        */
        virtual bool ConsumeEndGroup(bool dothrow = false) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Check if current item is a "value" tag with the given name and if so
            consumes that item and retrive the value.

            \param[in]  name     Name of value tag to check for.
            \param[out] value    Return value.
            \param[in]  dothrow  Should this function throw an exception if current
                                 item is not a value tag with the given name.

            \returns   true if current item is a value tag with the given name, else
                       false.
        */
        virtual bool ConsumeValue(const std::wstring& name, std::wstring& value, bool dothrow = false) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Check if current item is a "value" tag with the given name and if so
            consumes that item and returns the value. Throws an exception if current 
            item is not a value tag with the given name.

            \param[in]  name     Name of value tag to check for.
            \returns   value of the value tag.
            \throws PersistUnexpectedDataException if current item is not a value tag with the
                    given name.
        */
        virtual std::wstring ConsumeValue(const std::wstring& name) = 0;
    };

    /*----------------------------------------------------------------------------*/
    /**
        Interface for Persistence Data Writer.
        Contains a set of pure virtual methods that any data writer must implement.
        Used to write a stream of data to be stored and later retrieved by a
        persistence reader.
    */
    class SCXPersistDataWriter
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor

            \param[in]  version     Version of data stored.
        */
        SCXPersistDataWriter(unsigned int version) : m_Version(version) {};

        /*----------------------------------------------------------------------------*/
        /**
            Destructor
        */
        virtual ~SCXPersistDataWriter() { DoneWriting(); };

        /*----------------------------------------------------------------------------*/
        /**
            Mark the start of a new group.

            \param[in]  name     Name of group.
        */
        virtual void WriteStartGroup(const std::wstring& name) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Mark the end of the last started group.
            \throws SCXInvalidStateException if no group is currently started.
        */
        virtual void WriteEndGroup() = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Write a new name/value pair.

            \param[in]  name    Name of value.
            \param[in] value    Value.
        */
        virtual void WriteValue(const std::wstring& name, const std::wstring& value) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Mark the end of writing data.
            Will also be called from destructor if not called explicitly
        */
        virtual void DoneWriting() {};
    
        /*----------------------------------------------------------------------------*/
        /**
            Get version number given at construction of writer.

            \returns   Version of data stored.
        */
        unsigned int GetVersion() { return m_Version; };

    private:
        unsigned int m_Version; //!< Version of persisted data.
    };

    /*----------------------------------------------------------------------------*/
    /**
        Interface for Persistence Media.
        Contains a set of pure virtual methods that any data media must implement.

        Used to implement the storage medium for the persistence data and as a way
        to get named readers and writers for that media.
    */
    class SCXPersistMedia
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Default constructor.
        */
        SCXPersistMedia() {};

        /*----------------------------------------------------------------------------*/
        /**
            Destructor.
        */
        virtual ~SCXPersistMedia() {};
        
        /*----------------------------------------------------------------------------*/
        /**
            Create a new data reader and populate it with the data previously written
            with the given name.

            \param[in]  name     Name of data to populate reader with.
            \returns   The new reader.
            \throws  PersistDataNotFoundException if no data previously written with the given name.
        */
        virtual SCXHandle<SCXPersistDataReader> CreateReader(const std::wstring& name) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Create a new data writer to write data with the given name.
            If data has previously been written with the same name, that data will be
            overwriten.

            \param[in]  name     Name of data to write.
            \param[in]  version  Version of data to write.
            \returns   The new writer.
        */
        virtual SCXHandle<SCXPersistDataWriter> CreateWriter(const std::wstring& name, unsigned int version = 0) = 0;

        /*----------------------------------------------------------------------------*/
        /**
            Remove persisted data with the given name.

            \param[in]  name     Name of data to remove.
            \throws  PersistDataNotFoundException if no data previously written with the given name.
        */
        virtual void UnPersist(const std::wstring& name) = 0;
    };

    /*----------------------------------------------------------------------------*/
    /**
        Exception thrown when data cannot be persisted since media is unavailable.
    */ 
    class PersistMediaNotAvailable : public SCXException
    {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
            Ctor 
            \param[in] reason Reason why persistence media is unavailable.
            \param[in] l    Source code location object
        */
        PersistMediaNotAvailable(const std::wstring& reason, 
                                 const SCXCodeLocation& l) :
            SCXException(l), 
            m_Reason(reason)
        {};

        std::wstring What() const
        {
            std::wstringstream s;
            s << L"Persistence media unavailable: " << m_Reason;
            return s.str();
        }

    protected:
        std::wstring   m_Reason; //!< Reason why persistence media is unavailable.
    };        

    /*----------------------------------------------------------------------------*/
    /**
        Exception thrown when requested persisted data was not found.
    */ 
    class PersistDataNotFoundException : public SCXException
    {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
            Ctor 
            \param[in] name Name of the persisted data requested.
            \param[in] l    Source code location object
        */
        PersistDataNotFoundException(const std::wstring& name, 
                                     const SCXCodeLocation& l) :
            SCXException(l), 
            m_Name(name)
        {};

        std::wstring What() const
        {
            std::wstringstream s;
            s << L"Could not find persisted data: " << m_Name;
            return s.str();
        }

    protected:
        std::wstring   m_Name; //!< Name of persisted data requested.
    };        

    /*----------------------------------------------------------------------------*/
    /**
        Exception thrown when persisted data is not what is expected.
    */ 
    class PersistUnexpectedDataException : public SCXException
    {
    public: 
        /*----------------------------------------------------------------------------*/
        /**
            Ctor 
            \param[in] expected Expected data
            \param[in] pos      Stream position where error was encountered
            \param[in] l        Source code location object
        */
        PersistUnexpectedDataException(const std::wstring& expected, 
                                       std::wstreampos pos,
                                       const SCXCodeLocation& l) :
            SCXException(l),
            m_Expected(expected),
            m_Pos(pos)
        {};

        std::wstring What() const
        {
            std::wstringstream s;
            s << L"Expected data: " << m_Expected << L", not found at pos: " << m_Pos;
            return s.str();
        }

    protected:
        std::wstring m_Expected; //!< Expected data
        std::wstreampos  m_Pos;  //!< Position in stream
    };        

    /*----------------------------------------------------------------------------*/
    /**
        Factory method to get persist media.
        \return Handle to an SCXPersistMedia object.
    */ 
    SCXHandle<SCXPersistMedia> GetPersistMedia();
}

#endif /* SCXPERSISTENCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
