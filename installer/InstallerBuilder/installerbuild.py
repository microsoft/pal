#coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Contains main program to build installer packages.
#
# Date:   2007-09-25 14:33:04
#

from optparse import OptionParser
from os import path
import sys
import scxutil
from hpuxpackage import HPUXPackageFile
from sunospkg import SunOSPKGFile
from linuxrpm import LinuxRPMFile
from linuxdeb import LinuxDebFile
from aixlpp import AIXLPPFile
from macospackage import MacOSPackageFile
import scxexceptions
from product import Product
from platforminformation import PlatformInformation
from opsmgr import OpsMgr
from configmgr import ConfigMgr
from opsmgrbuildinfo import OpsMgrBuildInformation
from configmgrbuildinfo import ConfigMgrBuildInformation
from opsmgrstagingdir import OpsMgrStagingDir
from configmgrstagingdir import ConfigMgrStagingDir

##
# Main program class
#
class InstallerBuild:
    ##
    # Handle program parameters.
    #
    def ParseParameters(self):
        # Get all command line parameters and arguments.
        usage = "usage: %prog [options]"
        parser = OptionParser(usage)

        parser.add_option("--product",
                          type="string",
                          dest="product",
                          help="Product identifier for the product to build")

        parser.add_option("--basedir",
                          type="string",
                          dest="basedir",
                          help="Path to the scx_core root directory.")
        parser.add_option("--pf",
                          type="choice",
                          choices=["Windows", "Linux", "SunOS", "HPUX", "AIX", "MacOS"],
                          dest="pf",
                          help="Specifies what platform to build installer for.")
        parser.add_option("--pfmajor",
                          type="int",
                          dest="pfmajor",
                          help="Specifies the platform major version")
        parser.add_option("--pfminor",
                          type="string",
                          dest="pfminor",
                          help="Specifies the platform minor version")
        parser.add_option("--pfdistro",
                          type="choice",
                          choices=["", "SUSE", "REDHAT", "UBUNTU"],
                          dest="pfdistro",
                          help="Specifies the distribution, or none")
        parser.add_option("--pfarch",
                          type="choice",
                          choices=["sparc", "pa-risc", "x86", "x64", "ia64", "ppc"],
                          dest="pfarch",
                          help="Specifies the platform architecture")
        parser.add_option("--pfwidth",
                          type="choice",
                          choices=["32", "64"],
                          dest="pfwidth",
                          help="Specifies the platform width")
        parser.add_option("--bt",
                          type="choice",
                          choices=["Debug", "Release", "Bullseye"],
                          dest="bt",
                          help="Debug, Release, or Bullseye build")
        parser.add_option("--cc",
                          action="store_true",
                          dest="cc",
                          help="Signals this is a code coverage build")
        parser.add_option("--major",
                          type="string",
                          dest="major",
                          help="Build major version number.")
        parser.add_option("--minor",
                          type="string",
                          dest="minor",
                          help="Build minor version number.")
        parser.add_option("--patch",
                          type="string",
                          dest="patch",
                          help="Build version patch number.")
        parser.add_option("--buildnr",
                          type="string",
                          dest="buildnr",
                          help="Build version build number.")
        
        (options, args) = parser.parse_args()

        if options.product == None: options.product = 'OpsMgr'
        if options.basedir == None: parser.error("You must specify a base directory")
        if options.pf      == None: parser.error("You must specify a platform")
        if options.pfmajor == None: parser.error("You must specify a platform major")
        if options.pfminor == None: parser.error("You must specify a platform minor")
        if options.pfdistro == None: parser.error("You must specify a distribution")
        if options.pfarch  == None: parser.error("You must specify a platform architecture")
        if options.pfwidth == None: parser.error("You must specify a platform width")
        if options.bt      == None: parser.error("You must specify a build type")
        if options.major == None: parser.error("You must specify a major version number")
        if options.minor == None: parser.error("You must specify a minor version number")
        if options.patch == None: parser.error("You must specify a version patch number")
        if options.buildnr == None: parser.error("You must specify a build number")

        return options

    def GetPlatform(self, options):
        platform = PlatformInformation()
        platform.OS = options.pf
        platform.MajorVersion = options.pfmajor
        platform.MinorVersion = int(options.pfminor)
        platform.MinorVersionString = options.pfminor
        platform.Distribution = options.pfdistro
        platform.Architecture = options.pfarch
        platform.Width = options.pfwidth
        return platform

    def GetBuildInformation(self, options, platform):
        return OpsMgrBuildInformation(options.basedir, platform, options.bt, options.cc)
    
    def GetOpsMgr(self, options, platform, buildInfo):
        version = options.major + '.' + options.minor + '.' + options.patch
        buildNr = options.buildnr
        
        return OpsMgr(version, buildNr, platform, buildInfo)

    def GetConfigMgr(self, options, platform, buildInfo):
        version = options.major + '.' + options.minor + '.' + options.patch
        buildNr = options.buildnr
        
        return ConfigMgr(version, buildNr, platform, buildInfo)

    def GetConfigMgrBuildInformation(self, options, platform):
        return ConfigMgrBuildInformation(options.basedir)
    

    ##
    # Main program
    #
    def main(self):
        # Get command line parameters.
        options = self.ParseParameters()
        platform = self.GetPlatform(options)

        if options.product == 'OpsMgr':
            buildInfo = self.GetBuildInformation(options, platform)
            stagingDir = OpsMgrStagingDir(buildInfo, platform)
            product = self.GetOpsMgr(options, platform, buildInfo)
        elif options.product == 'ConfigMgr':
            buildInfo = self.GetConfigMgrBuildInformation(options, platform)
            stagingDir = ConfigMgrStagingDir(buildInfo, platform)
            product = self.GetConfigMgr(options,platform, buildInfo)
            

        if platform.OS == "Windows":
            raise PlatformNotImplementedError(self.platform.OS)
        elif platform.OS == "Linux":
            if platform.Distribution == "UBUNTU":
                installerFile = LinuxDebFile(stagingDir,
                                             product,
                                             platform)
            else:
                installerFile = LinuxRPMFile(stagingDir,
                                             product,
                                             platform)
        elif platform.OS == "SunOS":
            installerFile = SunOSPKGFile(stagingDir,
                                         product,
                                         platform,
                                         buildInfo.GetBuildType())
        elif platform.OS == "HPUX":
            installerFile = HPUXPackageFile(stagingDir,
                                            product,
                                            platform)
        elif platform.OS == "AIX":
            installerFile = AIXLPPFile(stagingDir,
                                       product,
                                       platform)
        elif platform.OS == "MacOS":
            installerFile = MacOSPackageFile(stagingDir,
                                             product,
                                             platform)
        else:
            raise PlatformNotImplementedError(platform.OS)

        installerFile.Generate()

if __name__ == '__main__': InstallerBuild().main()

