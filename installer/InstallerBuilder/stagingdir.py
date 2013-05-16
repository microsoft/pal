# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#

##
# A staging directory is a representation of how the files should be installed on the
# system but located in different directory. 
# The implementations of this interface gives the installer builder a means of
# generating the staging directory and a way to extract details for each staged object.
#
class StagingDir:
    def __init__(self):
        pass

    ##
    # Return the list of staging objects.
    # Staging object implement the interface of filegenerator.StagedObjet.
    # There are already a bunch of helpers in that same file for things like
    # file copy, empty file, soft link and directories.
    #
    # \returns The list of staging objects.
    #
    def GetStagingObjectList(self):
        pass

    ##
    # Return the root path of the staging directory.
    # \returns The root path of the staging directory.
    #
    def GetRootPath(self):
        pass

    ##
    # Generate the staging directory.
    # The implementations of this method must physically
    # create directories and files inside the staging directory.
    #
    def Generate(self):
        pass
    
