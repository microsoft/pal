#coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#

from os import path
from buildinfo import BuildInformation

##
# OpsMgr build information (like directories and such)
#
class OpsMgrBuildInformation(BuildInformation):
    def __init__(self, baseDirectory, platform, buildType, codeCoverage):
        self.baseDirectory = baseDirectory
        self.buildType = buildType
        self.codeCoverage = codeCoverage
        self.platform = platform
        self.platformString = self.CreatePlatformString()

    ##
    # Directory where installer can put temporary files.
    #
    def GetTempDir(self):
        return path.join(self.baseDirectory, 'installer/intermediate')

    ##
    # Directory where installer sould put the finished package.
    #
    def GetTargetDir(self):
        targetBaseDir = path.join(self.baseDirectory, 'target')
        return path.join(targetBaseDir, self.platformString)

    ##
    # Directory where files are staged for packaging.
    #
    def GetStagingDir(self):
        return path.join(self.GetTargetDir(), 'staging')

    ##
    # Build base directory
    #
    def GetBaseDir(self):
        return self.baseDirectory

    def GetBuildType(self):
        return self.buildType

    ##
    # Create the platform string
    #
    def CreatePlatformString(self):
        if self.platform.OS == "Linux":
            distroStr = "_%s" % (self.platform.Distribution)
        else:
            distroStr = ""

        platformString = "%s%s_%s.%s_%s_%s_%s" % (self.platform.OS,
                                                  distroStr,
                                                  self.platform.MajorVersion,
                                                  self.platform.MinorVersionString,
                                                  self.platform.Architecture,
                                                  self.platform.Width,
                                                  self.buildType)
        if self.codeCoverage:
            platformString = platformString + "_cc"

        return platformString
