# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to populate a staging directory
#
# Date:   2007-09-25 13:45:34
#

from filegenerator import FileCopy
from filegenerator import NewDirectory
import scxutil
from os import path
from stagingdir import StagingDir

##
# Class containing logic for creating the staging directory
# structure.
#
class ConfigMgrStagingDir(StagingDir):
    ##
    # Ctor.
    # \param[in] buildInfo Provides information about paths
    # \param[in] platform Platform to build for.
    #
    def __init__(self, buildInfo, platform):
        self.buildInfo = buildInfo
        self.platform = platform

        self.stagingRoot = buildInfo.GetStagingDir()

        stagingDirectories = [
            NewDirectory('',                            700, 'root', 'root', 'sysdir'),
            NewDirectory('opt',                         755, 'root', 'root'),
            NewDirectory('opt/microsoft',               755, 'root', 'root'),
            NewDirectory('opt/microsoft/ConfigMgr',     755, 'root', 'root')
            ]

        fileCopies = [
            FileCopy('opt/microsoft/ConfigMgr/CCMExec', 'bin/CCMExec', 755, 'root', 'root')
            ]
        for stagingObject in fileCopies:
            stagingObject.SetSrcRootDir(buildInfo.GetBaseDir())

        self.stagingObjectList = stagingDirectories + fileCopies
        for stagingObject in self.stagingObjectList:
            stagingObject.SetRootDir(self.stagingRoot)
    
    ##
    # Return the list of staging objects.
    # \returns The list of staging objects.
    #
    def GetStagingObjectList(self):
        return self.stagingObjectList

    ##
    # Return the root path of the staging directory.
    # \returns The root path of the staging directory.
    #
    def GetRootPath(self):
        return self.stagingRoot

    ##
    # Generate the staging directory.
    #
    def Generate(self):
        if not path.isdir(self.stagingRoot):
            scxutil.MkAllDirs(self.stagingRoot)

        scxutil.RmTree(self.stagingRoot)

        for stagingObject in self.stagingObjectList:
            stagingObject.DoCreate()
