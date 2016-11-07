#include <Base64Helper.h>

using util::Base64Helper;

void Base64Helper::Encode(const std::vector<unsigned char>& input, std::string& encodedString)
{
    static char base64Table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static unsigned int maskArray[4] = {0x00FC0000, 0x0003F000, 0x00000FC0, 0x0000003F};
    static unsigned int shiftArray[4] = {18, 12, 6, 0};

    encodedString.clear();
    if (!input.empty())
    {
        std::vector<unsigned char>::const_iterator it = input.begin();
        std::vector<unsigned char>::const_iterator itEnd = input.end();
        size_t inputSize = input.size();
        size_t outputSize = ((inputSize / 3) + ((inputSize % 3) ? 1 : 0)) * 4;
        encodedString.reserve(outputSize);

        size_t paddingSize = 0;

        unsigned int base64Word = 0;
        for (; it != itEnd; ++it)
        {
            base64Word = 0;
            paddingSize = 0;

            // Add the first byte
            base64Word |= (*it << 16);
            ++it;

            if (it != itEnd)
            {
                // Add second byte
                base64Word |= (*it << 8);
                ++it;

                if (it != itEnd)
                {
                    // Add third byte
                    base64Word |= *it;
                }
                else
                {
                    paddingSize = 1;
                }
            }
            else
            {
                paddingSize = 2;
            }

            for (size_t i = 0; i < (4-paddingSize); ++i)
            {
                unsigned int j = (base64Word & maskArray[i]) >> shiftArray[i];
                encodedString.append(1, base64Table[j]);
            }

            // add padding if needed
            if (paddingSize != 0)
            {
                encodedString.append(paddingSize, '=');
                break;
            }
        }
    }
}

bool Base64Helper::Decode(const std::string& encodedInput, std::vector<unsigned char>& decodedOutput)
{
    static unsigned char base64ReverseTable[256] = {
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
         52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255, 255,  254, 255, 255,
        255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
        15,   16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
        255,  26,  27,  28,  29,  30,  31 , 32,  33,  34,  35,  36,  37,  38,  39,  40,
        41,   42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
    };

    static unsigned int shiftArray[4] = {18, 12, 6, 0};

    decodedOutput.clear();
    if (!encodedInput.empty())
    {
        size_t inputSize = encodedInput.size();
        if ((inputSize % 4) != 0)
        {
            return false;
        }

        size_t outputSize = (inputSize / 4) * 3;
        decodedOutput.reserve(outputSize);

        unsigned int base64Word = 0;
        unsigned int paddingSize = 0;
        // Process except the last four bytes
        for (size_t i = 0; i < inputSize ; i+=4)
        {
            base64Word = 0;
            for (size_t j = 0; ((j < 4) && ((i + j) < inputSize)); ++j)
            {
                unsigned char encodedByte = encodedInput[i + j];
                unsigned char decodedChar = base64ReverseTable[encodedByte];

                if (decodedChar == 255)
                {
                    // Invalid Character encountered
                    return false;
                }
                else if (decodedChar == 254)
                {
                    // Found an padding character.
                    // Verify the location
                    if ((i + j) == (inputSize - 2))
                    {
                        paddingSize = 2;
                        break;
                    }
                    else if ((i + j) == (inputSize - 1))
                    {
                        paddingSize = 1;
                        break;
                    }
                    else
                    {
                        // Bad Padding
                        return false;
                    }
                }
                else
                {
                    base64Word |= (decodedChar << shiftArray[j]);
                }
            }

            decodedOutput.push_back(static_cast<unsigned char>((base64Word & 0x00FF0000) >> 16));

            if (paddingSize <= 1)
            {
                decodedOutput.push_back(static_cast<unsigned char>((base64Word & 0x0000FF00) >> 8));
            }

            if (paddingSize <= 2)
            {
                decodedOutput.push_back(static_cast<unsigned char>(base64Word & 0x000000FF));
            }

            if (paddingSize != 0)
            {
                decodedOutput.resize(outputSize - paddingSize);
            }
        }
    }
    return true;
}
