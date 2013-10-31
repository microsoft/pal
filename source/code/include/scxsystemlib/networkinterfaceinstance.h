/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       Specification of network interface instance PAL 
    
    \date        08-03-03 11:19:02
    
*/
/*----------------------------------------------------------------------------*/
#ifndef NETWORKINTERFACEINSTANCE_H
#define NETWORKINTERFACEINSTANCE_H

#include <scxsystemlib/entityinstance.h>
#include <scxcorelib/scxlog.h>
#include <scxcorelib/scxhandle.h>

namespace SCXSystemLib  
{

    class NetworkInterfaceInfo;
    
    /*----------------------------------------------------------------------------*/
    /**
        Represent a network interface
    */
    class NetworkInterfaceInstance : public EntityInstance
    {
    public:
        NetworkInterfaceInstance(const NetworkInterfaceInfo &info);
        virtual ~NetworkInterfaceInstance();

        std::wstring GetName() const;

        virtual void Update();

        void Update(const NetworkInterfaceInfo &info);
        
        bool GetIPAddress(std::wstring &value) const;
        bool GetIPAddress(std::vector<std::wstring> &value) const;
        bool GetNetmask(std::wstring &value) const;
        bool GetBroadcastAddress(std::wstring &value) const;
        bool GetBytesReceived(scxulong &value) const;
        bool GetBytesSent(scxulong &value) const;
        bool GetPacketsReceived(scxulong &value) const;
        bool GetPacketsSent(scxulong &value) const;
        bool GetErrorsReceiving(scxulong &value) const;        
        bool GetErrorsSending(scxulong &value) const;
        bool GetCollisions(scxulong &value) const;
        bool GetUp(bool &value) const;
        bool GetRunning(bool &value) const;
        
        /*----------------------------------------------------------------------------*/
        /**
           Retrieve availability and status of the device.

           \param       value - output parameter where the availability of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAvailability(unsigned short& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve network medium in use of the device.

           \param       value - output parameter where the AdapterType of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAdapterType(std::wstring& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve network medium ID in use of the device.

           \param       value - output parameter where the AdapterTypeID of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAdapterTypeID(unsigned short& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve if the network adapter can automatically determine the speed of the attached or network media.

           \param       value - output parameter where the AutoSense of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetAutoSense(bool& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve index value that uniquely identifies the local network interface of the device.

           \param       value - output parameter where the InterfaceIndex of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetInterfaceIndex(unsigned int& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve the raw form of the media access control address for this network 
           adapter of the device where the case is preserved and there is no delimiter.

           \param value - output parameter where the MACAddress of the device is stored.
           \returns true if value is supported on platform, false otherwise.
        */
        bool GetMACAddressRAW(std::wstring& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve media access control address for this network adapter of the device.

           \param value - output parameter where the MACAddress of the device is stored.
           \param sepChar - the delimiter.
           \param upperCase - true for uppercase and false to preserve the case.
           \returns true if value is supported on platform, false otherwise.
        */
        bool GetMACAddress(std::wstring& value, const char sepChar=':', 
                           bool upperCase=false) const;
                
        /*----------------------------------------------------------------------------*/
        /**
           Retrieve name of the network adapter's manufacturer.

           \param       value - output parameter where the Manufacturer of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetManufacturer(std::wstring& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve maximum speed, in bits per second, for the network adapter.

           \param       value - output parameter where the MaxSpeed of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetMaxSpeed(scxulong& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve state of the network adapter connection to the network.

           \param       value - output parameter where the NetConnectionStatus of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetNetConnectionStatus(unsigned short& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve whether the adapter is a physical or a logical adapter.

           \param       value - output parameter where the PhysicalAdapter of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetPhysicalAdapter(bool& value) const;

        /*----------------------------------------------------------------------------*/
        /**
           Retrieve estimate of the current bandwidth in bits per second.

           \param       value - output parameter where the Speed of the device is stored.
           \returns     true if value is supported on platform, false otherwise.
        */
        bool GetSpeed(scxulong& value) const;


    private:
        SCXCoreLib::SCXLogHandle m_log;  //!< Log handle

        SCXCoreLib::SCXHandle<NetworkInterfaceInfo> m_info; //!< Source of data
    };

}


#endif /* NETWORKINTERFACE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
