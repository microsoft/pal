# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing base classes to create an installer
#
# Date:   2007-09-26 15:55:44
#

import scxutil

##
# Class containing generic logic for creating a package file
#
class Installer:
    ##
    # Ctor.
    # \param[in] tempDir Absolute path to directory where temporary files can be placed.
    # \param[in] stagingDir StagingDirectory with files to be installed.
    #
    def __init__(self, tempDir, stagingDir):
        self.tempDir = tempDir
        self.stagingDir = stagingDir

    ##
    # Go through all steps to generate the installer package.
    #
    def Generate(self):
        self.ClearTempDir()
        self.GenerateStagingDir()
        self.GenerateScripts()
        self.GeneratePackageDescriptionFiles()
        self.BuildPackage()

    ##
    # Generate and populate the staging directory.
    #
    def GenerateStagingDir(self):
        self.stagingDir.Generate()
    ##
    # Clears the temp directory.
    #
    def ClearTempDir(self):
        scxutil.RmTree(self.tempDir)
        scxutil.MkAllDirs(self.tempDir)

    ##
    # Generate the package description files (e.g. prototype file)
    #
    def GeneratePackageDescriptionFiles(self):
        raise PlatformNotImplementedError("")

    ##
    # Create the actuall installer package.
    #
    def BuildPackage(self):
        raise PlatformNotImplementedError("")
        
