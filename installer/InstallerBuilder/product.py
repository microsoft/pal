#coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#

class Product:
    ShortName = ''
    LongName = ''
    Version = ''
    BuildNr = ''
    Description = ''
    License = ''
    Vendor = ''
    Copyright = ''

    ##
    # Utility method
    #
    def GetVersionString(self):
        return self.Version + '-' + self.BuildNr

    ##
    # Returns an object implementing BuildInfo.
    #
    def GetBuildInfo(self):
        raise 'The product implementation needs to support getting a BuildInfo instance.'

    ##
    # Returns the path to a file that can be used to test if the product is installed
    # on the system. If the file is there, then the product is considered installed.
    # This is needed in order to determine if in upgrade or install mode for certain
    # installer formats.
    #
    def GetInstallationMarkerFile(self):
        raise 'The product implementation needs to support getting an installation marker file.'

    ##
    # This method gets passes a scriptgenerator.Script instance which it should
    # add any pre installation tasks to. Note that preinstallation could be only
    # part of the complete script.
    #
    def WritePreInstallToScript(self, script):
        raise 'The product implementation needs to support writing pre install to a script.'
        
    ##
    # This method gets passes a scriptgenerator.Script instance which it should
    # add any post installation tasks to. Note that post installation could be only
    # part of the complete script.
    #
    # This method will be called both after a clean install
    # AND after an upgrade. Make sure that the script does not
    # disturb any user configuration.
    #
    def WritePostInstallToScript(self, script):
        raise 'The product implementation needs to support writing post install to a script.'

    ##
    # This method gets passes a scriptgenerator.Script instance which it should
    # add any pre upgrade tasks to. Note that preupgrade could be only
    # part of the complete script.
    #
    # Note that this script will NOT be called by the solaris installer since
    # this installer doesn't have any notion of upgrade. On solaris there will
    # be an uninstall and then an install.
    #
    def WritePreUpgradeToScript(self, script):
        raise 'The product implementation needs to support writing pre upgrade to a script.'

    ##
    # This method gets passes a scriptgenerator.Script instance which it should
    # add any pre remove tasks to. Note that preremove could be only
    # part of the complete script.
    #
    def WritePreRemoveToScript(self, script):
        raise 'The product implementation needs to support writing pre remove to a script.'

    ##
    # This method gets passes a scriptgenerator.Script instance which it should
    # add any post remove tasks to. Note that postremove could be only
    # part of the complete script.
    #
    def WritePostRemoveToScript(self, script):
        raise 'The product implementation needs to support writing post remove to a script.'

    ##
    # This method needs to return a list of installation prerequisites (as strings).
    # The content of each of the strings in the list will be different for each supported platform
    # so the best template is to look at how it's defined in opsmgr.py
    #
    def GetPrerequisites(self):
        raise 'The product implementation needs to support getting the list of installation prerequisites.'
