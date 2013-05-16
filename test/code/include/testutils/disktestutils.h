/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Provides utilities for disk testing purposes

    \date        2009-05-29 10:17:00

*/
/*----------------------------------------------------------------------------*/
#ifndef DISKTESTUTILS_H
#define DISKTESTUTILS_H

#include <scxsystemlib/scxsysteminfo.h>

namespace
{
    bool HasPhysicalDisks(std::wstring testName, bool noWarn = false)
    {
#if defined(aix) || defined(sun)
        // On AIX, WPARs don't make the physical disks visible inside of a WPAR
        // On Solaris, non-global zones don't make the physical disks visible

        SCXSystemLib::SystemInfo si;
#if defined(aix)
        bool fIsInWPAR;
        si.GetAIX_IsInWPAR( fIsInWPAR );
        if ( ! fIsInWPAR )
#elif defined(sun)
        bool fIsInGlobalZone;
        si.GetSUN_IsInGlobalZone( fIsInGlobalZone );
        if ( fIsInGlobalZone )
#endif
        {
            return true;
        }

        if ( !noWarn )
        {
            std::wstring warnText
                = L"Platform must have physical disks to run SCXStatisticalDiskPalSanityTest::"
                + testName + L" test (for AIX, see wi10570)";
            SCXUNIT_WARNING(warnText);
        }
        return false;
#else // defined(aix) || defined(sun)
        (void) testName;
        (void) noWarn;
        return true;
#endif
    }
}

#endif /* DISKTESTUTILS_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
