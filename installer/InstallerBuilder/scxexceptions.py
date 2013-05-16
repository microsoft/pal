# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Contains main program to build installer packages.
#
# Date:   2007-09-25 14:33:04
#

##
# Thrown when trying to build an installer for a platform that is not
# yet implemented.
#
class PlatformNotImplementedError(Exception):
    ##
    # Ctor.
    # param[in] name Name of unsupported platform.
    #
    def __init__(self, name):
        self.name = name
    ##
    # String representation.
    # \returns String representation of object.
    #
    def __str__(self):
        return repr(self.name)
        
        
