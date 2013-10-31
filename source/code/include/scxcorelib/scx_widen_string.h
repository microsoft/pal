/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \date     07-06-05 00:23:50
    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCX_WIDEN_STRING_H
#define SCX_WIDEN_STRING_H

/** Widen a constant string to wstring */
#define SCXSTRINGTOWSTRING2(x) L ## x
/** Widen a constant string to wstring */
#define SCXSTRINGTOWSTRING(x) SCXSTRINGTOWSTRING2(x)


/**
  __SCXWFILE__ is a std::wstring correspondance to __FILE__ 
  Useful mostly in unit tests, otherwise the SCXSRCLOCATION macro should be used.
*/
#define __SCXWFILE__ SCXSTRINGTOWSTRING(__FILE__)

#endif /* SCX_WIDEN_STRING_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
