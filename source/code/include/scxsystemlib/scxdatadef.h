/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       This file contains the common enum definition for wmi classes
    
    \date        2011-05-10 11:39:54

    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXDATADEF_H
#define SCXDATADEF_H

namespace SCXSystemLib
{
    /*----------------------------------------------------------------------------*/
    //!
    //! Availability type enum
    //!
    enum AvailabilityType
    {
        eAvailabilityInvalid  = 0xFFFF,  //!< invalid value
        eAvailabilityInvalid0 = 0x0,     //!< another invalid value
        eAvailabilityOther    = 0x01,    //!< 1   (0x1  ) Other
        eAvailabilityUnknown,            //!< 2   (0x2  ) Unknown
        eAvailabilityRunningOrFullPower, //!< 3   (0x3  ) Running or Full Power
        eAvailabilityWarning,            //!< 4   (0x4  ) Warning
        eAvailabilityInTest,             //!< 5   (0x5  ) In Test
        eAvailabilityNotApplicable,      //!< 6   (0x6  ) Not Applicable
        eAvailabilityPowerOff,           //!< 7   (0x7  ) Power Off
        eAvailabilityOffLine,            //!< 8   (0x8  ) Off Line
        eAvailabilityOffDuty,            //!< 9   (0x9  ) Off Duty
        eAvailabilityDegraded,           //!< 10  (0xA  ) Degraded
        eAvailabilityNotInstalled,       //!< 11  (0xB  ) Not Installed
        eAvailabilityInstallError,       //!< 12  (0xC  ) Install Error
        eAvailabilityPowerSaveUnknown,   //!< 13  (0xD  ) Power Save - Unknown The device is known to be in a power save state, but its exact status is unknown.
        eAvailabilityPowerSaveLowPower,  //!< 14  (0xE  ) Power Save - Low Power Mode The device is in a power save state, but still functioning, and may exhibit degraded performance.
        eAvailabilityPowerSaveStandby,   //!< 15  (0xF  ) Power Save - Standby The device is not functioning, but could be brought to full power quickly.
        eAvailabilityPowerCycle,         //!< 16  (0x10 ) Power Cycle
        eAvailabilityPowerSaveWarning,   //!< 17  (0x11 ) Power Save - Warning The device is in a warning state, though also in a power save state.
        eAvailabilityCnt                 //!< type count
    };

    /**
       Power management capabilitiy enumeration supporting LogicalDevice derivatives.
    */
    enum PowerManagementCapabilities
    {
        eUnknown = 0,                          //!< 0 (0x0), Unknown
        eNotSupported,                         //!< 1 (0x1), Not Supported
        eDisabled,                             //!< 2 (0x2), Disabled
        eEnabled,                              //!< 3 (0x3), Enabled
        ePowerSavingModesEnteredAutomatically, //!< 4 (0x4), Power Saving Modes Entered Automatically
        ePowerStateSettable,                   //!< 5 (0x5), Power State Settable
        ePowerCyclingSupported,                //!< 6 (0x6), Power Cycling Supported
        eTimedPowerOnSupported                 //!< 7 (0x7), Timed Power-On Supported
    };
}
#endif 
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
