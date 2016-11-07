/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Enumeration of instances

    \date        07-05-21 12:00:00

*/
/*----------------------------------------------------------------------------*/
#ifndef ENTITYENUMERATION_H
#define ENTITYENUMERATION_H

#include <vector>
#include <algorithm>

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxexception.h>
#include <scxcorelib/scxhandle.h>
#include <scxcorelib/scxlog.h>

using namespace std;

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Class template that represents a colletion of objects subclassed from
        EntityInstance. The template parameter should thus be a class subclassed
        from EntityInstance.

        A concrete implementation of an enumeration would subclass this template
        with a concrete implementation of EntityInstance as template parameter.

        Base class template for PAL entity enumeration.

        There are three main ways the system data are updated:
        \li by the entity enumeration subclass (one infomration source is distributed to
            the instances from the Update() method,
        \li by the enity instances themselves (the enumeration collection calls
            the Update() method on one or more instances using UpdateInstance[s]()
        \li a mix thereof.

        Since instances of the enumeration class are unrelated (i.e. quite OK to
        have private instances of an enumeration), locking must be provided
        externally if really necessary, or by the concrete enumeration or concrete
        instance.

        For the same reason, when using one of the Get* methods it is up to the
        caller not to perform any Update on the collection since that might
        invalidate the pointer.

    */
    template <class Inst>
    class EntityEnumeration
    {
    public:
        /** Iterator of instances */
        typedef typename std::vector<SCXCoreLib::SCXHandle<Inst> >::iterator EntityIterator;

        /** Pure virtual init method. Can be used to start threads etc. */
        virtual void Init() = 0;
        virtual void Update(bool updateInstances=true);
        virtual void CleanUp();

        void         UpdateInstances();
        void         UpdateInstance(const EntityInstanceId& id);

        size_t       Size() const;

        EntityIterator Begin();
        EntityIterator End();
        SCXCoreLib::SCXHandle<Inst> GetTotalInstance() const;
        SCXCoreLib::SCXHandle<Inst> GetInstance(const EntityInstanceId& id) const;
        SCXCoreLib::SCXHandle<Inst> GetInstance(size_t pos) const;
        SCXCoreLib::SCXHandle<Inst> operator[](size_t pos) const;

        typename EntityEnumeration<Inst>::EntityIterator RemoveInstance(typename EntityEnumeration<Inst>::EntityIterator iter);
        bool RemoveInstanceById(const EntityInstanceId& id);

    protected:
        EntityEnumeration();
        virtual ~EntityEnumeration();

        virtual void AddInstance(SCXCoreLib::SCXHandle<Inst> instance);
        virtual void SetTotalInstance(SCXCoreLib::SCXHandle<Inst> instance);
        virtual void RemoveInstances();
        virtual void CleanUpInstances();
        virtual void Clear(bool clearTotal = false);

    private:
        std::vector<SCXCoreLib::SCXHandle<Inst> > m_instances; //!< Contains the entity instances.
        SCXCoreLib::SCXHandle<Inst> m_totalInstance; //!< Pointer to the total instance.
    };

    /*----------------------------------------------------------------------------*/
    /**
        Default constructor

    */
    template<class Inst>
    EntityEnumeration<Inst>::EntityEnumeration() : m_totalInstance(NULL)
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    */
    template<class Inst>
    EntityEnumeration<Inst>::~EntityEnumeration()
    {
        RemoveInstances();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Default implementation of Cleanup virtual method
    */
    template<class Inst>
    void EntityEnumeration<Inst>::CleanUp()
    {
        CleanUpInstances();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Default implementation of Update virtual method.

        \param updateInstances - indicates whether only the existing instances shall be updated.

        The method refreshes the set of known instances in the enumeration.

        Any newly created instances must have a well-defined state after execution,
        meaning that instances which update themselves have to init themselves upon
        creation.

    */
    template<class Inst>
    void EntityEnumeration<Inst>::Update(bool updateInstances)
    {
        if (updateInstances)
        {
            UpdateInstances();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Run the Update() method on all instances in the colletion, including the
        Total instance if any.

    */
    template<class Inst>
    void EntityEnumeration<Inst>::UpdateInstances()
    {
        for (size_t i=0; i<Size(); i++)
        {
            try {
                m_instances[i]->Update();
                m_instances[i]->ResetUnexpectedException();
            } catch ( SCXCoreLib::SCXException& e ){
                static scx_atomic_t s_ExceptionsCounter(0);

                if ( s_ExceptionsCounter < 10 ){
                    scx_atomic_increment( &s_ExceptionsCounter );
                    SCX_LOGERROR(
                        SCXCoreLib::SCXLogHandleFactory::GetLogHandle(
                            L"scx.core.common.pal.system.enumerationtemplate"),
                        std::wstring(L"Unexpected exception during instance-update; only first 10 errors are logged; ") +
                            e.What() + std::wstring(L"; ") + e.Where() );
                }
                m_instances[i]->SetUnexpectedException( e );
            }
        }

        if (m_totalInstance != 0)
        {
            try {
                m_totalInstance->Update();
                m_totalInstance->ResetUnexpectedException();
            } catch ( SCXCoreLib::SCXException& e ){
                static scx_atomic_t s_ExceptionsCounter(0);

                if ( s_ExceptionsCounter < 10 ){
                    scx_atomic_increment( &s_ExceptionsCounter );
                    SCX_LOGERROR(
                        SCXCoreLib::SCXLogHandleFactory::GetLogHandle(
                            L"scx.core.common.pal.system.enumerationtemplate"),
                        std::wstring(L"Unexpected exception during total-instance-update; only first 10 errors are logged; ") +
                            e.What() + std::wstring(L"; ") + e.Where() );
                }
                m_totalInstance->SetUnexpectedException( e );
            }
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Run the Update() method on the specified instance in the colletion

        \param  id Identity of the instance to use
    */
    template<class Inst>
    void EntityEnumeration<Inst>::UpdateInstance(const EntityInstanceId& id)
    {
        EntityInstance* inst = GetInstance(id);

        if (inst)
        {
            inst->Update();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get iterator to the first instance in the collection

        \returns   An iterator addressing the first instance in the collection
    */
    template<class Inst>
    typename EntityEnumeration<Inst>::EntityIterator EntityEnumeration<Inst>::Begin()
    {
        return m_instances.begin();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get iterator to beyond last instance in the collection

        \returns      An iterator to the end of the collection

    */
    template<class Inst>
    typename EntityEnumeration<Inst>::EntityIterator EntityEnumeration<Inst>::End()
    {
        return m_instances.end();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get instance by ID

        \param    id ID of instance to get
        \returns  Pointer to instance or NULL if pos is out of range

    */
    template<class Inst>
    SCXCoreLib::SCXHandle<Inst> EntityEnumeration<Inst>::GetInstance(const EntityInstanceId& id) const
    {
        for (size_t i=0; i<Size(); i++)
        {
            if (m_instances[i]->GetId() == id)
            {
                return m_instances[i];
            }
        }

        return SCXCoreLib::SCXHandle<Inst>(0);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get number of instances in collection

        \returns    Number of instances in collection - NOT including the Total instance

    */
    template<class Inst>
    size_t EntityEnumeration<Inst>::Size() const
    {
        return m_instances.size();
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get instance by position

        \param     pos Position of instance to get
        \returns   Pointer to instance

    */
    template<class Inst>
    SCXCoreLib::SCXHandle<Inst> EntityEnumeration<Inst>::GetInstance(size_t pos) const
    {
        if (pos < Size())
        {
            return m_instances[pos];
        }
        else
        {
            throw SCXCoreLib::SCXIllegalIndexException<size_t>(L"pos", pos, 0, true, Size(), true, SCXSRCLOCATION);
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get total instance

        \returns  Pointer to total instance or NULL if not set

    */
    template<class Inst>
    SCXCoreLib::SCXHandle<Inst> EntityEnumeration<Inst>::GetTotalInstance() const
    {
        return m_totalInstance;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get instance by position

        \param    pos Position of instance to get
        \throws   SCXIllegalIndexExceptionUInt No instance at given position

    */
    template<class Inst>
    SCXCoreLib::SCXHandle<Inst> EntityEnumeration<Inst>::operator[](size_t pos) const
    {
        return GetInstance(pos);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Add an instance

        \param  instance Instance to add

    */
    template<class Inst>
    void EntityEnumeration<Inst>::AddInstance(SCXCoreLib::SCXHandle<Inst> instance)
    {
        m_instances.push_back(instance);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Set total instance

        \param  instance Instance to set total instance to

    */
    template<class Inst>
    void EntityEnumeration<Inst>::SetTotalInstance(SCXCoreLib::SCXHandle<Inst> instance)
    {
        m_totalInstance = instance;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove all instances, including any Total instance

    */
    template<class Inst>
    void EntityEnumeration<Inst>::RemoveInstances()
    {
        m_instances.clear();
        m_totalInstance = NULL;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Call cleanup on all instances, including any Total instance

    */
    template<class Inst>
    void EntityEnumeration<Inst>::CleanUpInstances() // private
    {
        for (size_t i=0; i<Size(); i++)
        {
            m_instances[i]->CleanUp();
        }

        if (m_totalInstance != 0)
        {
            m_totalInstance->CleanUp();
        }
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove an instance

        \param iter Iterator pointing at item to remove.
        \returns An iterator pointing to new location of the instance that followed
                 the element that was removed.

        \note The removed instance is deleted.
    */
    template<class Inst>
    typename EntityEnumeration<Inst>::EntityIterator EntityEnumeration<Inst>::RemoveInstance(typename EntityEnumeration<Inst>::EntityIterator iter)
    {
        return m_instances.erase(iter);
    }

    /*----------------------------------------------------------------------------*/
    /**
        Remove an instance with given id

        \param id Id of instance to remove.
        \returns true if the Id was found, otherwise false

        \note The removed instance is deleted.
    */
    template<class Inst>
    bool EntityEnumeration<Inst>::RemoveInstanceById(const EntityInstanceId& id)
    {
        for (typename EntityEnumeration<Inst>::EntityIterator iter = Begin(); iter != End(); iter++)
        {
            if ((*iter)->GetId() == id)
            {
                RemoveInstance(iter);
                return true;
            }
        }
        return false;
    }

    /*----------------------------------------------------------------------------*/
    /**
       Remove links to all instances.

       \param clearTotal Should the total instance also be cleared

       This method empties the container that points to instances without actually
       deleting the instances themselves. You must retain a pointer to every instance
       and delete them explicitly, or elese there will be a memory leak.
    */
    template<class Inst>
    void EntityEnumeration<Inst>::Clear(bool clearTotal /* = false */)
    {
        m_instances.clear();

        if (clearTotal)
        {
            m_totalInstance = 0;
        }
    }

}

#endif /* ENTITYENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
