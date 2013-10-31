/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file        

    \brief       C and C++ compaibility library for portability purposes.
    
    \date        2008-08-18 14:59:00
    
*/
/*----------------------------------------------------------------------------*/
#ifndef SCXCOMPAT_H
#define SCXCOMPAT_H

/* Define CPP_ICONV_CONST_CHARPTR if iconv() requires const char* pointers */
#undef CPP_ICONV_CONST_CHARPTR
#if defined(sun) && (PF_MAJOR == 5 && PF_MINOR <= 10)
#define CPP_ICONV_CONST_CHARPTR 1
#endif

/*
  Solaris 11 Sparc is problematic if 'isdigit' is called with signed character with 8-th bit set
  (Sign extension occurs, causing stack corruption).  On Solaris 11, isdigit (and friends) are all
  inline functions.  Define a macro to implement the inline function with a static cast to resolve.

  We used a test program to replicate this (compile on Solaris with: CC -o chartest chartest.cpp):

      #include <stdio.h>
      #include <ctype.h>

      int main()
      {
          char c = '\xC2';
          int i = -1;

          printf("%x\n\n", isdigit(c));
      }

  We got the macro substitutions below by compiling with CC -E chartest.cpp (look at expansions)
  to see how the inline functions were implemented.
*/

#if defined(sun) && defined(sparc) && PF_MAJOR == 5 && PF_MINOR == 11 && !defined(_SOLARIS_11_NO_MACROS_)

#include <string>
#include <istream>

  #define isalpha(c) ( ( __ctype + 1 ) [ (unsigned char) c ] & ( 0x00000001 | 0x00000002 ) )
  #define isdigit(c) ( ( __ctype + 1 ) [ (unsigned char) c ] & 0x00000004 )
  #define isalnum(c) ( ( __ctype + 1 ) [ (unsigned char) c ] & ( 0x00000001 | 0x00000002 | 0x00000004 ) )
  #define isspace(c) ( ( __ctype + 1 ) [ (unsigned char) c ] & 0x00000008 )
  #define isprint(c) ( ( __ctype + 1 ) [ (unsigned char) c ] & ( 0x00000010 | 0x00000001 | 0x00000002 | 0x00000004 | 0x00000040 ) )
  #define iscntrl(c) ( ( __ctype + 1 ) [ (unsigned char) c ] & 0x00000020 )
#endif

#endif /* SCXCOMPAT_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
