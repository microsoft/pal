/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Provide a reference counted pointer handle. 
    
    \date        07-08-06 09:51:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXHANDLE_H
#define SCXHANDLE_H

#include <scxcorelib/scxcmn.h>
#include <memory>
#include <scxcorelib/scxatomic.h> 

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Provide a reference counted pointer handle.
    
        Objects being reference counted MUST be allocated using new operator and
        may not be an array. Template class will deallocate object using delete
        once the reference counter reaches zero.

        \note This implementation is \b not thread safe.
    */

    template<class T>
    class SCXHandle
    {
    private:
        T* m_pData;          //!< The actual data.
        scx_atomic_t* m_pCounter; //!< The reference counter for the data.
        bool m_isOwner;      //!< Indicates if the SCXHandle instance is owner of data.

        /*----------------------------------------------------------------------------*/
        /**
            Helper method used to increment the reference counter.
    
        */
        void AddRef()
        {
            SCXASSERT(0 != m_pCounter);
            SCXASSERT((*m_pCounter) > 0);
            scx_atomic_increment(m_pCounter);
        }

        /*----------------------------------------------------------------------------*/
        /**
            Helper method to decrease the reference counter.
    
            Will deallocate object reference counted if the counter reaches zero.
    
        */
        void Release()
        {
            SCXASSERT(0 != m_pCounter);
            SCXASSERT((*m_pCounter) > 0);
            SCXASSERT(( ! m_isOwner) || (1 == (*m_pCounter)));
            if ( scx_atomic_decrement_test(m_pCounter) )
            {
                if (0 != m_pData)
                    delete m_pData;
                delete m_pCounter;

                m_pData = NULL;
                m_pCounter = NULL;
            }
        }


    public:
        /*----------------------------------------------------------------------------*/
        /**
            Default construcor, so you can create array of handles, 
            use them as members and use map [] operator
    
        */
        SCXHandle() 
            : m_pData(0)
            , m_pCounter(new scx_atomic_t(1))
            , m_isOwner(false)
        {
            
        }

        /*----------------------------------------------------------------------------*/
        /**
            Construcor to start reference counting an object. 
    
            \param       p - object to reference count.
    
        */
        explicit SCXHandle(T* p) 
            : m_pData(p)
            , m_pCounter(new scx_atomic_t(1))
            , m_isOwner(false)
        {
            
        }

        /*----------------------------------------------------------------------------*/
        /**
            Copy constructor.
    
            \param       h - handle to copy.
    
        */
        SCXHandle(const SCXHandle& h)
            : m_pData(h.m_pData)
            , m_pCounter(h.m_pCounter)
            , m_isOwner(false)
        {
            AddRef();
        }
 
        /*----------------------------------------------------------------------------*/
        /**
            Constructor
    
            \param    pData       Object to reference count.
            \param    pCounter    Reference counter 
        */
        SCXHandle(T* pData, scx_atomic_t* pCounter)
            : m_pData(pData)
            , m_pCounter(pCounter)
            , m_isOwner(false)
        {
            AddRef();
        }

        /*----------------------------------------------------------------------------*/
        /**
         * Allow the same kind of implicit type conversion that is allowed for pointers.
         * A handle to a subclass may be assigned to a handle to a superclass, in the
         * same way that a pointer to a subclass may be assigned to a pointer to superclass
         */
        template<class T2> operator SCXHandle<T2>()
        {
            return SCXHandle<T2>(m_pData, m_pCounter); 
        }

        /*----------------------------------------------------------------------------*/
        /**
         * Allow the same kind of implicit type conversion that is allowed for pointers.
         * A handle to a subclass may be assigned to a handle to a superclass, in the
         * same way that a pointer to a subclass may be assigned to a pointer to superclass
         */
        template<class T2> operator SCXHandle<T2>() const
        {
            return SCXHandle<T2>(m_pData, m_pCounter); 
        }

        /*----------------------------------------------------------------------------*/
        /**
            Virtual destructor.
    
        */
        virtual ~SCXHandle()
        {
            Release();
        }

        /*----------------------------------------------------------------------------*/
        /**
         * Dereferencing operator
         * \returns Reference to reference counted object
         */
        T& operator*() const {
            return *m_pData;
        }       
        
        /*----------------------------------------------------------------------------*/
        /**
            Member access operator to access reference counted object members.
    
            \returns     Pointer to reference counted object.
    
        */
        T* operator->() const
        {
            return m_pData;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Comparison with pointer.
    
            \returns     true if pointer is equal to m_pData.
    
        */
        bool operator == (const T* p ) const {
            return m_pData == p;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Comparison with pointer.
    
            \returns     true if pointer is not equal to m_pData.
    
        */
        bool operator != (const T* p ) const {
            return m_pData != p;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Comparison with other handle
    
            \returns     true if handles point to the same object.
    
        */
        bool operator == (const SCXHandle& other) const {
            return m_pData == other.m_pData;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Comparison with other handle
    
            \returns     false if handles point to the same object.
    
        */
        bool operator != (const SCXHandle& other) const {
            return m_pData != other.m_pData;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Assignment operator.
    
            \param       other - object to replace reference with.
            \returns     reference to this.
    
            Replaces the currently reference counted object with that of the argument.
    
        */
        SCXHandle& operator=(const SCXHandle& other)
        {
            if (m_pCounter == other.m_pCounter)
            { // Using counter to determine if handle is same object since data may be NULL.
                return *this;
            }
            Release();
            m_pData = other.m_pData;
            m_pCounter = other.m_pCounter;
            m_isOwner = false;
            AddRef();
            return *this;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Assignment operator.
    
            \param       p - pointer to the new object
            \returns     reference to this.
    
            Replaces the currently reference counted object with that of the argument.
    
        */
        SCXHandle& operator=(T* p)
        {
            SetData( p );
            return *this;
        }
        
        /*----------------------------------------------------------------------------*/
        /**
            Method to retrieve the object pointer.
    
            \returns     Pointer to reference counted object.
    
        */
        T* GetData() const
        {
            return m_pData;
        }

        /*----------------------------------------------------------------------------*/
        /**
            Replace the reference counted pointer with a new pointer.
    
            \param       p - new pointer to reference count.
    
        */
        void SetData(T* p)
        {
            if (p == m_pData)
            {
                return;
            }

            m_isOwner = false;
             
            if (scx_atomic_decrement_test(m_pCounter))
            {
                if (0 != m_pData)
                {
                    delete m_pData;
                }

                m_pData = p;
                *m_pCounter = 1;
                return;
            }

            m_pData = p;
            m_pCounter = new scx_atomic_t(1);
        }
    
        /*----------------------------------------------------------------------------*/
        /**
            Set ownership of data.

            This method is used to set a certain instance of SCXHandle as owner of the
            data. The purpose is to add the possibility to assert if ref counter is not
            zero when the owning SCXHandle goes out of scope. makes it easier to catch
            the scenario when a reference is kept at an unknown location.

        */
        void SetOwner()
        {
            m_isOwner = true;
        }
    }; /* SCXHandle */

} /* namespace SCXCoreLib */


/*----------------------------------------------------------------------------*/
/**
    Equality operator for ordinary pointer and SCXHandle
*/
template<class T> inline bool operator == ( const T* p, const SCXCoreLib::SCXHandle<T>& ptr ){
    return p == ptr.GetData();
}


/*----------------------------------------------------------------------------*/
/**
   Equality operator for NULL pointer and handle
*/
template<class T> inline bool operator == ( void* p, const SCXCoreLib::SCXHandle<T>& ptr ){
    // NULL comparison
    SCXASSERT( p == 0 );
    p = p;  // to disable warning
    return 0 == ptr.GetData();
}

/*----------------------------------------------------------------------------*/
/**
    Inequality operator for ordinary pointer and SCXHandle
*/
template<class T> inline bool operator != ( const T* p, const SCXCoreLib::SCXHandle<T>& ptr ){
    return p != ptr.GetData();
}

/*----------------------------------------------------------------------------*/
/**
   Inequality operator for NULL pointer and handle
*/
template<class T> inline bool operator != ( void* p, const SCXCoreLib::SCXHandle<T>& ptr ){
    // NULL comparison
    SCXASSERT( p == 0 );
    p = p;  // to disable warning
    return 0 != ptr.GetData();
}


#endif /* SCXHANDLE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
