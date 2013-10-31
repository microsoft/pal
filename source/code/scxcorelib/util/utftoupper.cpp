/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
// \file    UtfToUpper.cpp - UTF16-16/32 upcase function

#include <stddef.h>

// A table describing a range of characters that can be upcased by adding
// a fixed offset to the lower case value.
struct CaseTableEntry
{
    unsigned char Low;                  // the first character in the range of lower case characters
    unsigned char High;                 // the last character in the range of lower case characters
    unsigned short IncrOrValue;         // if Delta is zero, the new character; else the spacing between lower case characters in the range
    signed short Delta;                 // what to add to the lower case character to produce an upper case character
};

static CaseTableEntry Page00UpcaseTable[] =
{
    { 'a',  'z',    1,    -32 },                // US ASCII characters
    { 0xE0, 0xF6, 1,      -32 },        // ISO Latin-1 characters
    { 0xF8, 0xFE, 1,      -32 },
    { 0xFF, 0xFF, 0x0178, 0 }
};

static CaseTableEntry Page01UpcaseTable[] =
{
    { 0x01, 0x2F, 2,      -1 },         // Latin Extended-A characters
    { 0x33, 0x37, 2,      -1 },
        { 0x3A, 0x48, 2,      -1 },
    { 0x4B, 0x77, 2,      -1 },
    { 0x7A, 0x7E, 2,      -1 },
    { 0x80, 0x80, 0x0243, 0 },          // Latin Extended-B characters
    { 0x83, 0x85, 2,      -1 },
    { 0x88, 0x8C, 4,      -1 },
    { 0x92, 0x92, 0x0191, 0 },
    { 0x95, 0x95, 0x01F6, 0 },
    { 0x99, 0x99, 0x0198, 0 },
    { 0x9A, 0x9A, 0x023D, 0 },
    { 0x9E, 0x9E, 0x0220, 0 },
    { 0xA1, 0xA5, 2,      -1 },
    { 0xA8, 0xAD, 5,      -1 },
    { 0xB0, 0xB0, 0x01AF, 0 },
    { 0xB4, 0xB6, 2,      -1 },
    { 0xB9, 0xBD, 4,      -1 },
    { 0xBF, 0xBF, 0x01F7, 0 },
    { 0xC6, 0xCC, 3,      -2 },
    { 0xCE, 0xDC, 2,      -1 },
    { 0xDD, 0xDD, 0x018E, 0 },
    { 0xDF, 0xDF, 0x01DE, 0 },
    { 0xE1, 0xEF, 2,      -1 },
    { 0xF3, 0xF3, 0x01F1, 0 },
    { 0xF5, 0xF5, 0x01F4, 0 },
    { 0xF9, 0xFF, 2,      -1 }
};

static CaseTableEntry Page02UpcaseTable[] =
{
    { 0x01, 0x1F, 2,     -1 },          // Latin Extended-B characters
    { 0x23, 0x33, 2,     -1 },
    { 0x3C, 0x42, 6,     -1 },
    { 0x43, 0x43, 0x0180, 0 },
    { 0x47, 0x4F, 2,     -1 },
        { 0x50, 0x50, 0x2C6F, 0 },          // International Phonetic Alphabet extensions to Latin characters
        { 0x51, 0x51, 0x2C6D, 0 },
        { 0x53, 0x53, 0x0181, 0 },
        { 0x54, 0x54, 0x0186, 0 },
        { 0x56, 0x57, 1,      0x0189 - 0x0256 },
        { 0x59, 0x59, 0x018F, 0 },
        { 0x5B, 0x5B, 0x0190, 0 },
        { 0x60, 0x60, 0x0193, 0 },
        { 0x63, 0x63, 0x0194, 0 },
        { 0x68, 0x68, 0x0197, 0 },
        { 0x69, 0x69, 0x0196, 0 },
        { 0x6B, 0x6B, 0x2C62, 0 },
        { 0x6F, 0x6F, 0x019C, 0 },
        { 0x71, 0x71, 0x2C6E, 0 },
        { 0x72, 0x72, 0x019D, 0 },
        { 0x75, 0x75, 0x019F, 0 },
        { 0x7D, 0x7D, 0x2C64, 0 },
    { 0x80, 0x83, 3,      0x01A6 - 0x0280 },
    { 0x88, 0x88, 0x01AE, 0 },
    { 0x89, 0x89, 0x0244, 0 },
    { 0x8A, 0x8B, 1,      0x01B1 - 0x028A },
    { 0x8C, 0x8C, 0x0245, 0 },
    { 0x92, 0x92, 0x01B7, 0 }
};

static CaseTableEntry Page03UpcaseTable[] =
{
    { 0x71, 0x73, 2,      -1 },         // Greek characters
        { 0x77, 0x77, 0x0376, 0 },
    { 0x7B, 0x7D, 1,      0x03FD - 0x037B },
    { 0xAC, 0xAC, 0x0386, 0 },
    { 0xAD, 0xAF, 1,      0x0388 - 0x03AD },
    { 0xB1, 0xC1, 1,      -32 },
    { 0xC3, 0xC4, 1,      -32 },
    { 0xC5, 0xCB, 1,      -32 },
    { 0xCC, 0xCC, 0x038C, 0 },
    { 0xCD, 0xCE, 1,      -63 },
    { 0xD7, 0xD7, 0x03CF, 0 },
    { 0xD9, 0xEF, 2,      -1 },
    { 0xF2, 0xF2, 0x03F9, 0 },
    { 0xF8, 0xFB, 3,      -1 }
};

static CaseTableEntry Page04UpcaseTable[] =
{
    { 0x30, 0x4F, 1,      -32 },        // Cyrillic characters
    { 0x50, 0x5F, 1,      -80 },
    { 0x61, 0x81, 2,      -1 },
    { 0x8B, 0x8B, 0x048A, 0 },
    { 0x8D, 0xBF, 2,      -1 },
    { 0xC2, 0xCE, 2,      -1 },
    { 0xCF, 0xCF, 0x04C0, 0 },
    { 0xD1, 0xFF, 2,      -1 }
};

static CaseTableEntry Page05UpcaseTable[] =
{
    { 0x01, 0x23, 2,      -1 },         // Cyrillic Supplement characters
    { 0x61, 0x86, 1,      -48 }
};

static CaseTableEntry Page1DUpcaseTable[] =
{
    { 0x79, 0x79, 0xA77D, 0 },          // old-style 'g'
    { 0x7D, 0x7D, 0x2C63, 0 }           // crossed 'p'
};

static CaseTableEntry Page1EUpcaseTable[] =
{
    { 0x01, 0x95, 2,      -1 },         // Latin Extended Additional characters
    { 0xA1, 0xFF, 2,      -1 }
};

static CaseTableEntry Page1FUpcaseTable[] =
{
    { 0x00, 0x07, 1,      8 },          // Greek Extended characters (Greek characters with diacritical marks)
    { 0x10, 0x15, 1,      8 },
    { 0x20, 0x27, 1,      8 },
    { 0x30, 0x37, 1,      8 },
    { 0x40, 0x45, 1,      8 },
    { 0x51, 0x57, 2,      8 },
    { 0x60, 0x67, 1,      8 },
    { 0x70, 0x71, 1,      0x1FBA - 0x1F70 },
        { 0x72, 0x75, 1,      0x1FC8 - 0x1F72 },
        { 0x76, 0x77, 1,      0x1FDA - 0x1F76 },
        { 0x78, 0x79, 1,      0x1FF8 - 0x1F78 },
        { 0x7A, 0x7B, 1,      0x1FEA - 0x1F7A },
        { 0x7C, 0x7D, 1,      0x1FFA - 0x1F7C },
        { 0x80, 0x87, 1,      8 },
        { 0x90, 0x97, 1,      8 },
        { 0xA0, 0xA7, 1,      8 },
        { 0xB0, 0xB1, 1,      8 },
        { 0xB3, 0xC3, 16,     9 },
        { 0xD0, 0xD1, 1,      8 },
        { 0xE0, 0xE1, 1,      8 },
        { 0xE5, 0xE5, 0x1FEC, 0 },
        { 0xF3, 0xF3, 0x1FFC, 0 }
};

static CaseTableEntry Page21UpcaseTable[] =
{
    { 0x4E, 0x4E, 0x2132, 0 },          // upside down 'F'
    { 0x70, 0x7F, 1,      -16 },        // Roman numeral alphabetic characters
    { 0x84, 0x84, 0x2183, 0 }           // upside down 'C'
};

static CaseTableEntry Page24UpcaseTable[] =
{
    { 0xD0, 0xE9, 1,      0x24B6 - 0x24D0 } // Circled Latin characters
};

static CaseTableEntry Page2CUpcaseTable[] =
{
    { 0x30, 0x5E, 1,      -48 },        // Glogolitic (old Slavic) characters
    { 0x61, 0x61, 0x2C60, 0 },
    { 0x65, 0x65, 0x023A, 0 },
    { 0x66, 0x66, 0x023E, 0 },
    { 0x68, 0x6C, 2,      -1 },
    { 0x73, 0x76, 3,      -1 },
    { 0x81, 0xE3, 2,      -1 }
};

static CaseTableEntry Page2DUpcaseTable[] =
{
    { 0x00, 0x25, 1,      0x10A0 - 0x2D00 } // Georgian script
};

static CaseTableEntry PageA6UpcaseTable[] =
{
    { 0x41, 0x5F, 2,      -1 },         // Cyrillic Extended-B characters
    { 0x63, 0x6D, 2,      -1 },
    { 0x81, 0x97, 2,      -1 }
};

static CaseTableEntry PageA7UpcaseTable[] =
{
    { 0x23, 0x2F, 2,      -1 },         // Latin Extended-D characters
    { 0x33, 0x6F, 2,      -1 },
    { 0x7A, 0x7C, 2,      -1 },
    { 0x7F, 0x87, 2,      -1 },
    { 0x8C, 0x8C, 0xA78B, 0 }
};

static CaseTableEntry PageFFUpcaseTable[] =
{
    { 0x41, 0x5A, 1,      -32 }         // full-width US ASCII characters
};

/*
 Check a table to see if a character is in a given set of ranges, and,
 if the character is in a range, add a given offset to it.

 \param[in]     UpcaseTable - the section of the table for this character's Unicode page
 \param[in]     TableSize - the size of *UpcaseTable
 \param[in]     c - the character to be checked

  \returns      the upper case or lower case equivalent of c, if it is a lower case
                character, or c, if not
*/
static unsigned int UtfOffsetInRange(
    CaseTableEntry* UpcaseTable,
    size_t TableSize,
    unsigned int c)                     // the character to be converted to upper case
{
    size_t n;
    unsigned int LowRange;
        unsigned char Lsb = (unsigned char)c;

    for (n = 0; n < TableSize; n++, UpcaseTable++)
    {
        LowRange = UpcaseTable->Low;
        if (UpcaseTable->Delta == 0 && Lsb == LowRange)
                {
                        return (unsigned int)UpcaseTable->IncrOrValue;
                }
        else if (UpcaseTable->Delta != 0 &&
                             Lsb >= LowRange &&
                                 Lsb <= UpcaseTable->High &&
                                 (Lsb  - LowRange) % UpcaseTable->IncrOrValue == 0)
        {
            return c + (signed int)UpcaseTable->Delta;
        }
    }
        return c;
}

namespace SCXCoreLib
{
/*
 Convert a UTF character represented as a code point to its upper case equivalent
 in a locale- and language-independent way and in a way that
 if (IsLower(c)) UtfToLower(UtfToUpper(c)) == c, that is, round trip conversions
 always result in the original character. This function also ignores combining
 diacritical characters, so a string of calls to this function will return a
 correct lower case string of exactly the same length as its input.

 The locale-independence means that languages where upcasing removes diacriticals,
 like Spanish (but not French), where (upcase(vowel') yields VOWEL, or modern
 montonic Greek, where the iota adscript is dropped when upper casing some vowels,
 are rendered incorrectly by this upcasing algorithm.

 If the character is not a lower case character, the input character is returned.

 This function works on characters represented as unsigned int representation of code
 points; it does not handle UTF-8 or UTF-16 encoding/decoding.

 This function is l

 \param[in]    c - the character to be upper cased

 \returns      the uppercase equivalent of c

 \note         an IsLower macro can be constructed by using the 
               expression "UtfToUpper(c) != c"
*/
unsigned int UtfToUpper(
   unsigned int c)
{
    unsigned int Page = c >> 8;

    switch (Page)
    {
    case 0x00000000:
        c = UtfOffsetInRange(Page00UpcaseTable, sizeof Page00UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000001:
        c = UtfOffsetInRange(Page01UpcaseTable, sizeof Page01UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000002:
        c = UtfOffsetInRange(Page02UpcaseTable, sizeof Page02UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000003:
        c = UtfOffsetInRange(Page03UpcaseTable, sizeof Page03UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000004:
        c = UtfOffsetInRange(Page04UpcaseTable, sizeof Page04UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000005:
        c = UtfOffsetInRange(Page05UpcaseTable, sizeof Page05UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000001D:
        c = UtfOffsetInRange(Page1DUpcaseTable, sizeof Page1DUpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000001E:
        c = UtfOffsetInRange(Page1EUpcaseTable, sizeof Page1EUpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000001F:
        c = UtfOffsetInRange(Page1FUpcaseTable, sizeof Page1FUpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000021:
        c = UtfOffsetInRange(Page21UpcaseTable, sizeof Page21UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000024:
        c = UtfOffsetInRange(Page24UpcaseTable, sizeof Page24UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000002C:
        c = UtfOffsetInRange(Page2CUpcaseTable, sizeof Page2CUpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000002D:
        c = UtfOffsetInRange(Page2DUpcaseTable, sizeof Page2DUpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x000000A6:
        c = UtfOffsetInRange(PageA6UpcaseTable, sizeof PageA6UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x000000A7:
        c = UtfOffsetInRange(PageA7UpcaseTable, sizeof PageA7UpcaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x000000FF:
        c = UtfOffsetInRange(PageFFUpcaseTable, sizeof PageFFUpcaseTable / sizeof (CaseTableEntry), c);
        break;
    }
    return c;
}

static CaseTableEntry Page00DncaseTable[] =
{
    { 'A',  'Z',  1,      32 },         // US ASCII characters
    { 0xC0, 0xD6, 1,      32 },         // ISO Latin-1 characters
    { 0xD8, 0xDE, 1,      32 },
};

static CaseTableEntry Page01DncaseTable[] =
{
    { 0x00, 0x2E, 2,      1 },          // Latin Extended-A characters
    { 0x32, 0x36, 2,      1 },
        { 0x39, 0x47, 2,      1 },
    { 0x4A, 0x76, 2,      1 },
    { 0x78, 0x78, 0x00FF, 0 },
    { 0x79, 0x7D, 2,      1 },
    { 0x80, 0x80, 0x0243, 0 },          // Latin Extended-B characters
        { 0x81, 0x81, 0x0253, 0 },
        { 0x82, 0x84, 2,      1 },
        { 0x86, 0x86, 0x0254, 0 },
    { 0x87, 0x8B, 4,      1 },
        { 0x89, 0x89, 1,      0x0256 - 0x0189 },
    { 0x8A, 0x8A, 0x0257, 0 },
    { 0x8E, 0x8E, 0x01DD, 0 },
        { 0x8F, 0x8F, 0x0259, 0 },
        { 0x90, 0x90, 0x025B, 0 },
    { 0x91, 0x91, 0x0192, 0 },   
        { 0x93, 0x93, 0x0260, 0 },
        { 0x94, 0x94, 0x0263, 0 },
        { 0x96, 0x96, 0x0269, 0 },
        { 0x97, 0x97, 0x0268, 0 },
        { 0x98, 0x98, 0x0199, 0 },
        { 0x9C, 0x9C, 0x026F, 0 },
        { 0x9D, 0x9D, 0x0272, 0 },
        { 0x9F, 0x9F, 0x0275, 0 },
    { 0xA0, 0xA4, 2,      1 },
    { 0xA6, 0xA9, 3,      0x0280 - 0x01A6 },
    { 0xAE, 0xAE, 0x0288, 0 },
    { 0xA7, 0xAC, 5,      1 },
    { 0xAF, 0xAF, 0x01B0, 0 },
        { 0xB1, 0xB2, 1,      0x28A - 0x01B1 },
    { 0xB3, 0xB5, 2,      1 },
    { 0xB7, 0xB7, 0x0292, 0 },
    { 0xB8, 0xBC, 4,      1 },
    { 0xC4, 0xCA, 3,      2 },
    { 0xCD, 0xDB, 2,      1 },
    { 0xDE, 0xDE, 0x01DF, 0 },
    { 0xE0, 0xEE, 2,      1 },
    { 0xF1, 0xF1, 0x01F3, 0 },
    { 0xF4, 0xF4, 0x01F5, 0 },
    { 0xF6, 0xF6, 0x0195, 0 },
    { 0xF7, 0xF7, 0x01BF, 0 },
    { 0xF8, 0xFE, 2,      1 }
};

static CaseTableEntry Page02DncaseTable[] =
{
    { 0x00, 0x1E, 2, 1 },                       // Latin Extended-B characters
    { 0x20, 0x20, 0x019E, 0 },
    { 0x22, 0x32, 2, 1 },
    { 0x3A, 0x3A, 0x2C65, 0 },
    { 0x3B, 0x41, 6, 1 },
    { 0x3D, 0x3D, 0x019A, 0 },
    { 0x3E, 0x3E, 0x2C66, 0 },
    { 0x43, 0x43, 0x0180, 0 },
    { 0x44, 0x44, 0x0289, 0 },
    { 0x45, 0x45, 0x028C, 0 },
    { 0x46, 0x4E, 2, 1 }
};

static CaseTableEntry Page03DncaseTable[] =
{
    { 0x70, 0x72, 2,      1 },          // Greek characters
        { 0x76, 0x76, 0x0377, 0 },
        { 0x86, 0x86, 0x03AC, 0 },
        { 0x88, 0x8A, 1,      0x03AD - 0x0388 },
    { 0x8C, 0x8C, 0x03CC, 0 },
    { 0x8E, 0x8F, 1,      63 },
    { 0x91, 0xA1, 1,      32 },
    { 0xA3, 0xA4, 1,      32 },
    { 0xA5, 0xAB, 1,      32 },
    { 0xCF, 0xCF, 0x03D7, 0 },
    { 0xD8, 0xEE, 2,      1 },
    { 0xF9, 0xF9, 0x03F2, 0 },
    { 0xF7, 0xFA, 3,      1 },
    { 0xFD, 0xFF, 1,      0x037B - 0x03FD }
};

static CaseTableEntry Page04DncaseTable[] =
{
    { 0x10, 0x2F, 1,      32 },         // Cyrillic characters
    { 0x00, 0x0F, 1,      80 },
    { 0x60, 0x80, 2,      1 },
    { 0x8A, 0x8A, 0x048B, 0 },
    { 0x8C, 0xBE, 2,      1 },
    { 0xC1, 0xCD, 2,      1 },
    { 0xC0, 0xC0, 0x04CF, 0 },
    { 0xD0, 0xFE, 2,      1 }
};

static CaseTableEntry Page05DncaseTable[] =
{
    { 0x00, 0x22, 2,      1 },          // Cyrillic Supplement characters
    { 0x31, 0x56, 1,      48 }
};

static CaseTableEntry Page10DncaseTable[] =
{
    { 0xA0, 0xC5, 1,      0x2D00 - 0x10A0 } // Georgian script
};

//static CaseTableEntry Page1DDncaseTable[] =
//{
//    { 0x79, 0x79, 0xA77D, 0 },          // old-style 'g'
//};

static CaseTableEntry Page1EDncaseTable[] =
{
    { 0x00, 0x94, 2,      1 },          // Latin Extended Additional characters
    { 0xA0, 0xFE, 2,      1 }
};

static CaseTableEntry Page1FDncaseTable[] =
{
    { 0x08, 0x0F, 1,      -8 },         // Greek Extended characters (Greek characters with diacritical marks)
    { 0x18, 0x1D, 1,      -8 },
    { 0x28, 0x2F, 1,      -8 },
    { 0x38, 0x3F, 1,      -8 },
    { 0x48, 0x4D, 1,      -8 },
    { 0x59, 0x5F, 2,      -8 },
    { 0x68, 0x6F, 1,      -8 },
    { 0xBA, 0xBB, 1,      0x1F70 - 0x1FBA },
        { 0xC8, 0xCB, 1,      0x1F72 - 0x1FC8 },
        { 0xDA, 0xDB, 1,      0x1F76 - 0x1FDA },
        { 0xF8, 0xF9, 1,      0x1F78 - 0x1FF8 },
        { 0xEA, 0xEB, 1,      0x1F7A - 0x1FEA },
        { 0xFA, 0xFB, 1,      0x1F7C - 0x1FFA },
        { 0x88, 0x8F, 1,      -8 },
        { 0x98, 0x9F, 1,      -8 },
        { 0xA8, 0xAF, 1,      -8 },
        { 0xB8, 0xB9, 1,      -8 },
        { 0xBB, 0xBB, 0x1F71, 0 },
        { 0xBC, 0xCC, 16,     0x1FB3 - 0x1FBC },
        { 0xD8, 0xD9, 1,      -8 },
        { 0xE8, 0xE9, 1,      -8 },
        { 0xEC, 0xEC, 0x1FE5, 0 },
        { 0xFC, 0xFC, 0x1FF3, 0 }
};

static CaseTableEntry Page21DncaseTable[] =
{
    { 0x32, 0x32, 0x214E, 0 },          // upside down 'F'
    { 0x60, 0x6F, 1,      16 },         // Roman numeral alphabetic characters
    { 0x83, 0x83, 0x2184, 0 }           // upside down 'C'
};

static CaseTableEntry Page24DncaseTable[] =
{
    { 0xB6, 0xCF, 1,      0x24D0 - 0x24B6 } // Circled Latin characters
};

static CaseTableEntry Page2CDncaseTable[] =
{
    { 0x00, 0x2E, 1,      48 },         // Glogolitic (old Slavic) characters
    { 0x60, 0x60, 0x2C61, 0 },
    { 0x67, 0x6B, 2,      1 },
    { 0x72, 0x75, 3,      1 },
    { 0x80, 0xE2, 2,      1 },
        { 0x62, 0x62, 0x026B, 0 },
        { 0x63, 0x63, 0x1D7D, 0 },
        { 0x64, 0x64, 0x027D, 0 },
        { 0x6D, 0x6D, 0x0251, 0 },
        { 0x6E, 0x6E, 0x0271, 0 },
        { 0x6F, 0x6F, 0x0250, 0 }
};

static CaseTableEntry PageA6DncaseTable[] =
{
    { 0x40, 0x5E, 2,      1 },          // Cyrillic Extended-B characters
    { 0x62, 0x6C, 2,      1 },
    { 0x80, 0x96, 2,      1 }
};

static CaseTableEntry PageA7DncaseTable[] =
{
    { 0x22, 0x2E, 2,      1 },          // Latin Extended-D characters
    { 0x32, 0x6E, 2,      1 },
    { 0x79, 0x7B, 2,      1 },
    { 0x7D, 0x7D, 0x1D79, 0 },
    { 0x7E, 0x86, 2,      1 },
    { 0x8B, 0x8B, 0xA78C, 0 }
};

static CaseTableEntry PageFFDncaseTable[] =
{
    { 0x21, 0x3A, 1,      32 }          // full-width US ASCII characters
};

/*
 Convert a UTF character represented as a code point to its lower case equivalent
 in a locale- and language-independent way and in a way that
 if (IsUpper(c)) UtfToUpper(UtfToLower(c)) == c, that is, round trip conversions
 always result in the original character. This function also ignores combining
 diacritical characters, so a string of calls to this function will return a correct
 lower case string of exactly the same length as its input.

 This function works on characters represented as unsigned int representation of code
 points; it does not handle UTF-8 or UTF-16 encoding/decoding.

 \param[in]     c - the character to be lower cased

 \returns       the lower case equivalent of c

 \note         an IsUpper macro can be constructed by using the 
               expression "UtfToLower(c) != c"
*/
unsigned int UtfToLower(
    unsigned int c)
{
    unsigned int Page = c >> 8;

    switch (Page)
    {
    case 0x00000000:
        c = UtfOffsetInRange(Page00DncaseTable, sizeof Page00DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000001:
        c = UtfOffsetInRange(Page01DncaseTable, sizeof Page01DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000002:
        c = UtfOffsetInRange(Page02DncaseTable, sizeof Page02DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000003:
        c = UtfOffsetInRange(Page03DncaseTable, sizeof Page03DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000004:
        c = UtfOffsetInRange(Page04DncaseTable, sizeof Page04DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000005:
        c = UtfOffsetInRange(Page05DncaseTable, sizeof Page05DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000010:
        c = UtfOffsetInRange(Page10DncaseTable, sizeof Page10DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000001E:
        c = UtfOffsetInRange(Page1EDncaseTable, sizeof Page1EDncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000001F:
        c = UtfOffsetInRange(Page1FDncaseTable, sizeof Page1FDncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000021:
        c = UtfOffsetInRange(Page21DncaseTable, sizeof Page21DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x00000024:
        c = UtfOffsetInRange(Page24DncaseTable, sizeof Page24DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x0000002C:
        c = UtfOffsetInRange(Page2CDncaseTable, sizeof Page2CDncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x000000A6:
        c = UtfOffsetInRange(PageA6DncaseTable, sizeof PageA6DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x000000A7:
        c = UtfOffsetInRange(PageA7DncaseTable, sizeof PageA7DncaseTable / sizeof (CaseTableEntry), c);
        break;
    case 0x000000FF:
        c = UtfOffsetInRange(PageFFDncaseTable, sizeof PageFFDncaseTable / sizeof (CaseTableEntry), c);
        break;
    }
    return c;
}

} // end namespace SCXCoreLib
