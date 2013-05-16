#coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#

from os import path
from buildinfo import BuildInformation

##
# ConfigMgr build information (like directories and such)
#
class ConfigMgrBuildInformation(BuildInformation):
    def __init__(self, baseDirectory):
        self.baseDirectory = baseDirectory

    ##
    # Directory where installer can put temporary files.
    #
    def GetTempDir(self):
        return path.join(self.baseDirectory, 'tmp')

    ##
    # Directory where installer sould put the finished package.
    #
    def GetTargetDir(self):
        return path.join(self.baseDirectory, 'bin')

    ##
    # Directory where files are staged for packaging.
    #
    def GetStagingDir(self):
        return path.join(self.GetTempDir(), 'staging')

    def GetBaseDir(self):
        return self.baseDirectory
