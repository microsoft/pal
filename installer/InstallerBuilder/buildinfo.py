#coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#

from os import path

##
# Build information interface
#
class BuildInformation:
    def __init__(self):
        pass

    ##
    # Directory where installer can put temporary files.
    #
    def GetTempDir(self):
        pass

    ##
    # Directory where installer sould put the finished package.
    #
    def GetTargetDir(self):
        pass

    ##
    # Directory where files are staged for packaging.
    #
    def GetStagingDir(self):
        pass
