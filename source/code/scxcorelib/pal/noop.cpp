/*------------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
   \file

   \brief     No-op helper (primarily for Solaris 11 platforms)

   \date      2012-01-19 14:40:00


*/
/*----------------------------------------------------------------------------*/

/*
 * The Solaris 11 linker has an issue (at least for Solaris Studio 12.2) where
 * it doesn't allow a link line with no input files at all.
 *
 * Normally, we link libscxcoreprovidermodule.so with no input files (but lots
 * of -l libraries that resolve to our lib*.a files).  The Solaris 11 compiler
 * doesn't care for this.
 *
 * Two ways to solve this:
 *   1. For Solaris 11, rather than use -l for inclusion of our .a files,
 *      include them as input files (ifdef in Makefile.components), or
 *   2. Include a dummy input file (a noop file, if you will), that contains
 *      no executable code, but exists solely to pacify Solaris 11.
 *
 * We decided to solve the issue using #2.  This is the noop file.  It contains
 * no executable code.
 */
