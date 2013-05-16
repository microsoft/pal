# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to create a Solaris PKG file
#
# Date:   2007-09-26 11:16:44
#

import os
from time import strftime

import scxutil
from installer import Installer
from scriptgenerator import Script
from platforminformation import PlatformInformation
from executablefile import ExecutableFile

class ConfigureScriptBuilder:
    def __init__(self):
        pass

    def InstallConfigFile(self, script):
        script.WriteLn('error=no')
        script.WriteLn('while read src dest; do')
        script.CallFunction(script.InstallConfigurationFile(), '$src $dest')
        script.WriteLn('if [ $? -ne 0 ]; then')
        script.WriteLn('  error=yes')
        script.WriteLn('fi')
        script.WriteLn('done')
        script.WriteLn('[ \"$error\" = yes ] && exit 2')
        script.WriteLn('exit 0')
        return script

    def RemoveConfigFile(self, script):
        script.WriteLn('error=no')
        script.WriteLn('while read dest; do')
        script.CallFunction(script.UninstallConfigurationFile(), '$dest')
        script.WriteLn('if [ $? -ne 0 ]; then')
        script.WriteLn('  error=yes')
        script.WriteLn('fi')
        script.WriteLn('done')
        script.WriteLn('[ \"$error\" = yes ] && exit 2')
        script.WriteLn('exit 0')
        return script
##
# Class containing logic for creating a Solaris PKG file
#
class SunOSPKGFile(Installer):
    ##
    # Ctor.
    # \param[in] targetDir Absolute path to target directory.
    # \param[in] stagingDir StagingDirectory with files to be installed.
    # \param[in] product Information about the product being built.
    # \param[in] platform Information about the current target platform.
    # \param[in] buildType Debug, Release, Bullseye.
    #
    def __init__(self, stagingDir, product, platform, buildType):
        Installer.__init__(self, product.GetBuildInfo().GetTempDir(), stagingDir)
        self.product = product
        self.platform = platform
        self.buildType = buildType
        self.prototypeFileName = os.path.join(self.tempDir, 'prototype')
        self.pkginfoFile = PKGInfoFile(self.tempDir, product, platform)
        self.depFileName = os.path.join(self.tempDir, 'depend')
        self.preInstallPath = os.path.join(self.tempDir, "preinstall.sh")
        self.postInstallPath = os.path.join(self.tempDir, "postinstall.sh")
        self.preUninstallPath = os.path.join(self.tempDir, "preuninstall.sh")
        self.postUninstallPath = os.path.join(self.tempDir, "postuninstall.sh")
        self.iConfigFileName = os.path.join(self.tempDir, 'i.config')
        self.rConfigFileName = os.path.join(self.tempDir, 'r.config')

    ##
    # Generate the package description files (e.g. prototype file)
    #
    def GeneratePackageDescriptionFiles(self):
        self.GeneratePrototypeFile()
        self.pkginfoFile.Generate()
        self.GenerateDepFile()

    ##
    # Generate pre-, post-install scripts and friends
    #
    def GenerateScripts(self):
        preInstall = Script(self.platform)
        self.product.WritePreInstallToScript(preInstall)
        preInstall.WriteLn('exit 0')
        ExecutableFile(self.preInstallPath, preInstall)

        postInstall = Script(self.platform)
        self.product.WritePostInstallToScript(postInstall)
        postInstall.WriteLn('exit 0')
        ExecutableFile(self.postInstallPath, postInstall)

        preUninstall = Script(self.platform)
        self.product.WritePreRemoveToScript(preUninstall)
        preUninstall.WriteLn('exit 0')
        ExecutableFile(self.preUninstallPath, preUninstall)

        postUninstall = Script(self.platform)
        self.product.WritePostRemoveToScript(postUninstall)
        postUninstall.WriteLn('exit 0')
        ExecutableFile(self.postUninstallPath, postUninstall)

        scriptBuilder = ConfigureScriptBuilder()
        
        #
        # The i.config script is used to perform the installation
        # of all files tagged as configuration files.
        # A copy of each original file is kept in a special place
        # to make a good choice about what to do in case of a conflict
        # with an existing file
        #
        iConfig = scriptBuilder.InstallConfigFile(Script(self.platform))
        ExecutableFile(self.iConfigFileName, iConfig)
        
        #
        # The r.config script is used to perform the uninstallation
        # of all files tagged as configuration files.
        # A copy of each original file is kept in a special place
        # to make a good choice about what to do in case the user has
        # changed the file after installation
        #
        rConfig = scriptBuilder.RemoveConfigFile(Script(self.platform))
        ExecutableFile(self.rConfigFileName, rConfig)

    ##
    # Creates the prototype file used for packing.
    #
    def GeneratePrototypeFile(self):
        prototype = open(self.prototypeFileName, 'w')

        # include the info file
        prototype.write('i pkginfo=' + self.pkginfoFile.GetFileName() + '\n')
        # include depencency file
        prototype.write('i depend=' + self.depFileName + '\n')
        # include the install scripts
        prototype.write('i preinstall=' + self.preInstallPath + '\n')
        prototype.write('i postinstall=' + self.postInstallPath + '\n')
        prototype.write('i preremove=' + self.preUninstallPath + '\n')
        if self.platform.MajorVersion >= 6 or self.platform.MinorVersion >= 10:
            prototype.write('i postremove=' + self.postUninstallPath + '\n')
        prototype.write('i i.config=' + self.iConfigFileName + '\n')
        prototype.write('i r.config=' + self.rConfigFileName + '\n')

        # Now list all files in staging directory
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetPath() != '':
                if stagingObject.GetFileType() == 'dir':
                    prototype.write('d none')
                elif stagingObject.GetFileType() == 'sysdir':
                    # Ignore sysdir - see bug 4065
                    continue
                elif stagingObject.GetFileType() == 'file':
                    prototype.write('f none')
                elif stagingObject.GetFileType() == 'conffile':
                    prototype.write('f config')
                elif stagingObject.GetFileType() == 'link':
                    prototype.write('s none')
                else:
                    pass
                    # ToDo: Error
                prototype.write(' /' + stagingObject.GetPath())

                if stagingObject.GetFileType() == 'link':
                    prototype.write('=' + stagingObject.lnPath + '\n')
                else:
                    prototype.write(' ' + str(stagingObject.GetPermissions()) + \
                                    ' ' + stagingObject.GetOwner() + \
                                    ' ' + stagingObject.GetGroup() + '\n')

    ##
    # Creates the dependency file used for packing.
    #
    def GenerateDepFile(self):
        depfile = open(self.depFileName, 'w')

        # The format of the dependency file is as follows:
        #
        # <type> <pkg>  <name>
        #       [(<arch>)<version>]
        #
        for prereq in self.product.GetPrerequisites():
            depfile.write('P ' + prereq + '\n')

    ##
    # Actually creates the finished package file.
    #
    def BuildPackage(self):
        os.system('pkgmk -o' + \
                  ' -r ' + self.stagingDir.GetRootPath() + \
                  ' -f ' + self.prototypeFileName + \
                  ' -d ' + self.tempDir)
        pkgfilename = os.path.join(self.product.GetBuildInfo().GetTargetDir(),
                                   self.product.ShortName + '-' + \
                                   self.product.GetVersionString() + \
                                   '.solaris.' + str(self.platform.MinorVersion) + '.' + \
                                   self.platform.Architecture + '.pkg')
        os.system('pkgtrans -s ' + self.tempDir + ' ' + pkgfilename + ' ' + 'MSFT' + self.product.ShortName)

        # Note: On a "Core System Support" installation, gzip doesn't exist - use compress instead
        os.system('compress -f ' + pkgfilename)

##
# Represents pkg info file used for packing.
#
class PKGInfoFile:
    
    def __init__(self, directory, product, platform):
        self.filename = os.path.join(directory, 'pkginfo')
        self.properties = []

        # Required entries
        self.AddProperty('PKG', 'MSFT' + product.ShortName)
        self.AddProperty('ARCH', platform.Architecture)
        self.AddProperty('CLASSES', 'application config none')
        self.AddProperty('PSTAMP', strftime("%Y%m%d-%H%M"))
        self.AddProperty('NAME', product.LongName)
        self.AddProperty('VERSION', product.GetVersionString())
        self.AddProperty('CATEGORY', 'system')
        
        # Optional entries:
        self.AddProperty('DESC', product.Description)
        self.AddProperty('VENDOR', product.Vendor)
        self.AddProperty('SUNW_PKG_ALLZONES', 'false')
        self.AddProperty('SUNW_PKG_HOLLOW', 'false')
        self.AddProperty('SUNW_PKG_THISZONE', 'true')
        
    def AddProperty(self, key, value):
        self.properties.append((key, value))
        
    def Generate(self):
        pkginfo = open(self.filename, 'w')
        for (key, value) in self.properties:
            pkginfo.write(key + '=' + value + '\n')

    def GetFileName(self):
        return self.filename
