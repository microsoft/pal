#coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#

from product import Product
from scriptgenerator import Script

##
# ConfigMgr product definition
#
class ConfigMgr(Product):
    def __init__(self, version, buildNr, platform, buildInfo):
        self.ShortName = 'ConfigMgr'
        self.LongName = 'Microsoft ConfigMgr Client.'
        self.Description = 'Provides monitoring capabilities to the heterogenous platform'
        self.License = 'none'
        self.Vendor = 'http://www.microsoft.com'
        self.Copyright = 'For copyright and license information please refer to\n/opt/microsoft/scx/COPYRIGHT and /opt/microsoft/scx/LICENSE.\n'
        self.Version = version
        self.BuildNr = buildNr
        self.platform = platform
        self.buildInfo = buildInfo

    def GetBuildInfo(self):
        return self.buildInfo
    
    ##
    # Returns the path to a file that can be used to test if the product is installed
    # on the system. If the file is there, then the product is considered installed.
    # This is needed in order to determine if in upgrade or install mode for certain
    # installer formats.
    #
    def GetInstallationMarkerFile(self):
        return ''

    ##
    # The returned object must have a method WriteTo which takes an opened file.
    #
    def WritePreInstallToScript(self, script):
        pass

    ##
    # This method will be called both after a clean install
    # AND after an upgrade. Make sure that the script does not
    # disturb any user configuration.
    #
    def WritePostInstallToScript(self, script):
        pass

    ##
    # Note that this script will NOT be called by the solaris installer since
    # this installer doesn't have any notion of upgrade. On solaris there will
    # be an uninstall and then an install.
    #
    def WritePreUpgradeToScript(self, script):
        pass

    def WritePreRemoveToScript(self, script):
        pass

    def WritePostRemoveToScript(self, script):
        pass

    def GetDependencies(self):
        return []
