/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.


*/
/**
\file        

\brief      Enumeration of Software Instances.

\date       2011-01-18 14:56:20

*/
/*----------------------------------------------------------------------------*/
#ifndef INSTALLEDSOFTWAREENUMERATION_H
#define INSTALLEDSOFTWAREENUMERATION_H

#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/installedsoftwaredepend.h>
#include <scxsystemlib/installedsoftwareinstance.h>
#include <scxcorelib/scxlog.h>

namespace SCXSystemLib
{
    class InstalledSoftwareDependencies;
    /*----------------------------------------------------------------------------*/
    /**
    Class that represents a colletion of Software instances.
    PAL Holding the Software instance.
    */
    class InstalledSoftwareEnumeration : public EntityEnumeration<InstalledSoftwareInstance>
    {
    public:
        InstalledSoftwareEnumeration(SCXCoreLib::SCXHandle<InstalledSoftwareDependencies> deps = SCXCoreLib::SCXHandle<InstalledSoftwareDependencies>(new InstalledSoftwareDependencies()) );
        virtual ~InstalledSoftwareEnumeration();

        virtual void Init();
        virtual void Update(bool updateInstances=true);
        virtual void CleanUp();

    private:
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle.
        SCXCoreLib::SCXHandle<InstalledSoftwareDependencies> m_deps; //!< Dependencies to rely on
    };

}

#endif /* INSTALLEDSOFTWAREENUMERATION_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
