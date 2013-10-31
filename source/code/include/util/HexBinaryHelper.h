/*--------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
   \file      HexBinaryHelper.h  

   \brief     Provides the definition for the HexBinaryHelper class

   \date      02-09-11 09:16:22

*/
/*----------------------------------------------------------------------------*/

#ifndef HEXBINARYHELPER_H
#define HEXBINARYHELPER_H

#include <string>
#include <vector>
namespace SCX
{
    namespace Util
    {
        namespace Xml
        {
                /*----------------------------------------------------------------------------*/
                /**
                   This class defines the logic for encoding and decoding hex binary vectors

                   \date        02-09-11 09:17:30
                */
                class HexBinaryHelper
                {
                public:
                    /*----------------------------------------------------------------------------*/
                    /**
                       Encode the input vector to Hex Encoded character vector

                       \param [in]      input Input vector to be encoded
                       \param [out]     encodedOutput Encoded Hex Binary vector

                    */
                    static void Encode(const std::vector<unsigned char>& input, std::vector<unsigned char>& encodedOutput);

                    /*----------------------------------------------------------------------------*/
                    /**
                       Encode the input vector into a hex encoded string

                       \param [in]      input Input vector to be encoded
                       \param [out]     encodedOutput Encoded hex string

                    */
                    static void Encode(const std::vector<unsigned char>& input, std::string& encodedOutput);

                    /*----------------------------------------------------------------------------*/
                    /**
                       Decode a hex encoded input string into a binary vector

                       \param [in]      inputStr Encoded Hex Binary string
                       \param [out]     decodedOutput Decoded output character vector

                       \throws          asserts if the input string has an odd number of characters
                    */
                    static void Decode(const std::string& inputStr, std::vector<unsigned char>& decodedOutput);

                    /**
                       Decode a hex encoded input string into a binary vector

                       \param [in]      inputStr - encoded hex binary string
                       \param [out]     decodedOutput - Decoded output vector of bytes (unsigned char)

                       \return          true if the string was a valid hexadecimal string; false if not
                    */
                    static bool DecodeIgnoringWhiteSpace(const std::string& inputStr, std::vector<unsigned char>& decodedOutput);
                };
        }
    }
}
#endif /* HEXBINARYHELPER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
