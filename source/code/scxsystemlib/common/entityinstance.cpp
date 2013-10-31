/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Representation of an instance

    \date        07-05-21 12:00:00

*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>

#include <string>
#include <vector>

#include <math.h>

#include <scxcorelib/scxmath.h>
#include <scxsystemlib/entityinstance.h>

using namespace std;

namespace SCXSystemLib
{

    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    */
    EntityInstance::EntityInstance(const EntityInstanceId& id, bool isTotal) : 
        m_Id(id), 
        m_total(isTotal),
        m_exceptionCaught( false )
    {
    }


    /*----------------------------------------------------------------------------*/
    /**
        Constructor
    */
    EntityInstance::EntityInstance(bool isTotal) : 
        m_total(isTotal),
        m_exceptionCaught( false )
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Is this a composite total instance
        
        Retval:      name of processor instance
    */
    bool EntityInstance::IsTotal() const
    {
        return m_total;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Destructor
    */
    EntityInstance::~EntityInstance()
    {
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get instance id
        
        Retval:      Instance id
    */
    const EntityInstanceId& EntityInstance::GetId() const
    {
        return m_Id;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Get instance id
        
        Parameters:  id - Identity to use
    */
    void EntityInstance::SetId(const EntityInstanceId& id)
    {
        m_Id = id;
    }

    /*----------------------------------------------------------------------------*/
    /**
        Update the instance
    */
    void EntityInstance::Update()
    {
        // Empty default implementation
    }

    /*----------------------------------------------------------------------------*/
    /**
        Clean up the instance
    */
    void EntityInstance::CleanUp()
    {
        // Empty default implementation
    }

    /*----------------------------------------------------------------------------*/
    /**
        Fault-tolerance feature:
        should "UpdateInstances" catch any exception, it marks
        'bad' instance and continue updating the rest instead of interrupting the loop
        
        Parameters:  e - excpetion
    */
    void EntityInstance::SetUnexpectedException( const SCXCoreLib::SCXException& e )
    {
        m_exceptionCaught = true;
        m_exceptionText = e.What() + L"; " + e.Where();
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        clears previously set exception (if any)
    */
    void EntityInstance::ResetUnexpectedException()
    {
        m_exceptionCaught = false;
        m_exceptionText.clear();
    }
    
    /*----------------------------------------------------------------------------*/
    /**
        Returns true if last update on this instance had thrown exception
    */
    bool EntityInstance::IsUnexpectedExceptionSet() const
    {
        return m_exceptionCaught;
    }

}

/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
