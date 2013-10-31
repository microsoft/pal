/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/

#include <util/HexBinaryHelper.h>
#include <string>

#include <scxcorelib/scxassert.h>

using namespace SCX::Util::Xml;

static inline char ByteToHex(unsigned char c)
{
    static const char* HexArray = "0123456789ABCDEF";
    return HexArray[c];
}

/*
 Change a single ASCII hexit to a binary value between 0 and 15.

 \param[in]     hex - the hexit
 \param[out]    ok - set to false if a character was invalid; true otherwise

 \ return       the binary value of the hexit or 0 for a bad character
*/
static inline char HexToBinary(char hex, bool& ok)
{
    unsigned char returnChar = 0;

    ok = true;

    // HexToBinary uses a non-case-sensitve search for A-F & a-f
    if( (hex >= 'A') && (hex <= 'F') )
    {
        returnChar = static_cast<unsigned char>(hex - 'A' + 10);
    }
    else if( (hex >= 'a') && (hex  <= 'f') )
    {
        returnChar = static_cast<unsigned char>(hex - 'a' + 10);
    }
    else if ( (hex >= '0') && ( hex <= '9'))
    {
        returnChar = static_cast<unsigned char>(hex - '0');
    }
    else
    {
        // If there is an invalid character set the value to 0
        ok = false;
        returnChar = 0;
    }

    return returnChar;
}

void HexBinaryHelper::Decode(const std::string& inputStr, std::vector<unsigned char>& decodedOutput)
{
    std::string::size_type inputSize = inputStr.size();

    // By definition the input string size has to be even
    SCXASSERT(inputSize % 2 == 0);

    if (inputSize != 0)
    {
        bool ok;
        std::string::size_type outputSize = inputSize/2;
        decodedOutput.reserve(outputSize);

        unsigned char lowerNibble, upperNibble;

        for (unsigned int i = 0; i < inputSize; i+= 2)
        {
            upperNibble = HexToBinary(inputStr[i], ok);
            lowerNibble = HexToBinary(inputStr[i+1], ok);

            // Left shift upper nibble by 4 and get the top bits
            upperNibble = static_cast<unsigned char>(upperNibble << 4);
            upperNibble = static_cast<unsigned char>(upperNibble & 0xf0);

            // Get the bottom 4 bits from lowerNibble
            lowerNibble = static_cast<unsigned char>(lowerNibble & 0x0f);

            // Or Both and store
            decodedOutput.push_back(static_cast<unsigned char>(upperNibble | lowerNibble));
        }
    }
}

void HexBinaryHelper::Encode(const std::vector<unsigned char>& input, std::vector<unsigned char>& encodedOutput)
{
    // reserve twice the size of the input
    std::vector<unsigned char>::size_type outputSize = input.size() * 2;
    encodedOutput.reserve(outputSize);

    for (unsigned int i = 0; i != input.size(); ++i)
    {
        // take the upper nibble and right shift by 4
        encodedOutput.push_back((unsigned char)ByteToHex((input[i] & 0xf0) >> 4));

        // take the lower nibble
        encodedOutput.push_back(static_cast<unsigned char>(ByteToHex(static_cast<unsigned char>(input[i] & 0x0f))));
    }
}

void HexBinaryHelper::Encode(const std::vector<unsigned char>& input, std::string& encodedOutput)
{
    // reserve twice the size of the input
    encodedOutput.clear();
    std::string::size_type outputSize = input.size() * 2;
    encodedOutput.reserve(outputSize);

    for (unsigned int i = 0; i != input.size(); ++i)
    {
        // take the upper nibble and right shift by 4
        encodedOutput += ByteToHex((input[i] & 0xF0) >> 4);

        // take the lower nibble
        encodedOutput += static_cast<unsigned char>(ByteToHex(static_cast<unsigned char>(input[i] & 0x0F)));
    }
    return;
}

/*
 Decode a hex encoded input string into a binary array

 \param [in]      inputStr - encoded hex binary string
 \param [out]     decodedOutput - Decoded output vector of bytes (unsigned char)

 \return          true if the string was a valid hexadecimal string; false if not
*/
bool HexBinaryHelper::DecodeIgnoringWhiteSpace(const std::string& inputStr, std::vector<unsigned char>& decodedOutput)
{
    decodedOutput.reserve(inputStr.size() / 2);
    bool isUpperNibble = true;
    bool ok;
    unsigned char upperNibble = 0;
    unsigned char lowerNibble;

    for (size_t i = 0; i < inputStr.size(); i++)
    {
        if ((unsigned char)inputStr[i] > ' ')
        {                               // if not a space character, get a hexit
            if (isUpperNibble)
            {
                 upperNibble = HexToBinary(inputStr[i], ok);
                 if (!ok)
                 {                       // if invalid hexit, return error status
                     return false;
                 }
                 isUpperNibble = false;
            }
            else
            {
                 lowerNibble = HexToBinary((int)inputStr[i], ok);
                 if (!ok)
                 {                       // if invalid hexit, return error status
                     return false;
                 }
                 decodedOutput.push_back((unsigned char)((upperNibble << 4) | lowerNibble));
                 isUpperNibble = true;
            }
        }
    }

    if (!isUpperNibble)
    {                         // an odd number of hexits in the input string
        return false;
    }

    return true;
}
