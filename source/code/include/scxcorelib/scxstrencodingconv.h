/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file
 
    \brief     Defines the public interface for the string encoding conversion PAL.

    \date      07-Jan-2011 09:30 PST
 

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXSTRENCODINGCONV_H
#define SCXSTRENCODINGCONV_H

#include <string>
#include <vector>

namespace SCXCoreLib
{

    bool Utf8ToUtf16( const std::string& inUtf8Str,
                      std::vector< unsigned char >& outUtf16Bytes );
    bool Utf8ToUtf16le( const std::string& inUtf8Str,
                        std::vector< unsigned char >& outUtf16LEBytes );
    bool Utf16ToUtf8( const std::vector< unsigned char >& inUtf16Bytes,
                      std::string& outUtf8Str );
    bool Utf16leToUtf8( const std::vector< unsigned char >& inUtf16LEBytes,
                        std::string& outUtf8Str );

} /* namespace SCXCoreLib */

#endif /* SCXSTRENCODINGCONV_H */
/*------------------E-N-D---O-F---F-I-L-E-------------------------------*/
