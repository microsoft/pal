/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Representation of an instance

    \date        07-05-21 12:00:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef ENTITYINSTANCE_H
#define ENTITYINSTANCE_H

#include <string>
#include <scxcorelib/scxexception.h>

namespace SCXSystemLib
{

    /** Type to identify an entity instance. */
    typedef std::wstring EntityInstanceId;

    /*----------------------------------------------------------------------------*/
    /**
        Class that represents an instances.
        
        Base class for an instance.

        Any GetXXX() methods on the concrete subclasses shall be const values and 
        cheap. Computing power should be localized to the Update() methods of the 
        EntityEnumeration subclass or of this class. 

    */
    class EntityInstance
    {
    public:
        virtual ~EntityInstance();

        const EntityInstanceId& GetId() const;
        virtual void            Update();
        virtual void            CleanUp();

        bool                    IsTotal() const;

        // unexpected exception handling
        // functions are declared virtual to give entity a chance to extend behaviour
        virtual void  SetUnexpectedException( const SCXCoreLib::SCXException& e );
        virtual void  ResetUnexpectedException();
        virtual bool  IsUnexpectedExceptionSet() const;

        //! Returns exception status flag
        //! \returns true if last update has thrown exception
        const std::wstring&  GetUnexpectedExceptionText() const { return m_exceptionText;}


    protected:

        EntityInstance(const EntityInstanceId& id, bool isTotal = false);
        EntityInstance(bool isTotal = false);
        void SetId(const EntityInstanceId& id);

    private:

        EntityInstanceId m_Id; //!< Instance id.
        bool m_total;          //!< True if this is the total instance.

        // exception handling additional variables
        // if unexpected exception caught in "UpdateInstances", it will be stored in
        // instance, so enumerate may take appropriate actions
        bool m_exceptionCaught; //!< boolean flag to track unexpected exception status
        std::wstring    m_exceptionText;    //!< text to identify exception
    };

}

#endif /* ENTITYINSTANCE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
