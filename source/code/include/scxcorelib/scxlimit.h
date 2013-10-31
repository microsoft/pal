/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        scxlimit.h

    \brief       Define limits used in SCX-project.
    
    \date        07-10-11 15:14:13

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLIMIT_H
#define SCXLIMIT_H

/** scxlong is always 64 bits. This is the largest signed 64 bit value SCXCore can handle. */
const scxlong cMaxScxLong = (static_cast<scxlong>(0x7FFFFFFF)) << 32 |  0xFFFFFFFF;

/** scxlong is always 64 bits. This is the smallest signed 64 bit value SCXCore can handle. */
const scxlong cMinScxLong = -cMaxScxLong - 1;

#endif /* SCXLIMIT_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
