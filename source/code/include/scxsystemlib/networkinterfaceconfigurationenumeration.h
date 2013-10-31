/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file        networkinterfaceconfigurationenumeration.h

    \brief       Implementation of network interface configuration enumeration PAL

    \date        01-20-12 12:12:02

*/
/*----------------------------------------------------------------------------*/
#ifndef NETWORKINTERFACECONFIGURATIONENUMERATION_H
#define NETWORKINTERFACECONFIGURATIONENUMERATION_H
#include <scxsystemlib/networkinterfaceconfigurationinstance.h>
#include <scxsystemlib/networkinterface.h>
#include <scxsystemlib/entityenumeration.h>
#include <scxsystemlib/processenumeration.h>

namespace SCXSystemLib
{
 /*----------------------------------------------------------------------------*/
 //   ! Encapsulates all external dependencies for unit-testing.
    class NetworkInstanceConfigurationEnumerationDeps 
    {
        public:
            // ! Constructor
            NetworkInstanceConfigurationEnumerationDeps () { }

            /**
             * For unit-testing.
             \param[in] name  Name of process that we're searching for
             \returns A vector with process instance pointer.
             */
            virtual std::vector<SCXCoreLib::SCXHandle<ProcessInstance> > Find(const std::wstring &name, ProcessEnumeration &procEnum);

            // ! Destructor
            virtual ~NetworkInstanceConfigurationEnumerationDeps () { }
        protected:
            // ! Prevent copying to avoid slicing
            NetworkInstanceConfigurationEnumerationDeps (const NetworkInstanceConfigurationEnumerationDeps &);
    };

    /*----------------------------------------------------------------------------*/
    /**
        Represents a collection of network interfaces
    */
    class NetworkInterfaceConfigurationEnumeration : public EntityEnumeration<NetworkInterfaceConfigurationInstance>
    {
    public:
        /*----------------------------------------------------------------------------*/
        /**
            Constructor, sets the internal dependencies handle.

            \param deps - Handle to the dependencies object to be used by this instance.
        */
        NetworkInterfaceConfigurationEnumeration(SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> deps = 
            SCXCoreLib::SCXHandle<NetworkInterfaceDependencies>(new NetworkInterfaceDependencies));
        virtual ~NetworkInterfaceConfigurationEnumeration();
        std::vector<NetworkInterfaceConfigurationInstance> FindAll();
        virtual void Init();
        virtual void Update(bool updateInstances=true);
        static bool GetDHCPEnabledFromConfigData(std::vector<std::wstring>configData, std::wstring interface);
        static bool GetDHCPEnabledFromProcessList(std::wstring& name,
                SCXCoreLib::SCXHandle<NetworkInstanceConfigurationEnumerationDeps> deps = SCXCoreLib::SCXHandle<NetworkInstanceConfigurationEnumerationDeps>(new NetworkInstanceConfigurationEnumerationDeps()));
    protected:
        virtual void UpdateInstances();
        virtual void UpdateEnumeration();
    private:
        SCXCoreLib::SCXHandle<NetworkInterfaceDependencies> m_deps;         //!< Dependency object handle.
        SCXCoreLib::SCXLogHandle m_log;         //!< Log handle.
    };

}
#endif
