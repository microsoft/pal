# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to create a Linux Debian file (Ubuntu support)
#
# Date:   2009-03-30
#

import os
import scxutil
from installer import Installer
from scriptgenerator import Script
from executablefile import ExecutableFile

##
# Class containing logic for creating a Linux DEB file
#
class LinuxDebFile(Installer):
    ##
    # Ctor.
    # \param[in] stagingDir StagingDirectory with files to be installed.
    # \param[in] product Information about the product being built.
    # \param[in] platform Information about the current target platform.
    #
    def __init__(self, stagingDir, product, platform):
        Installer.__init__(self, product.GetBuildInfo().GetTempDir(), stagingDir)

        self.product = product
        self.platform = platform

        self.controlDir = os.path.join(self.stagingDir.GetRootPath(), 'DEBIAN')
        self.controlFileName = os.path.join(self.controlDir, 'control')
        self.configFileName = os.path.join(self.controlDir, 'conffiles')
        self.preInstallPath = os.path.join(self.controlDir, 'preinst')
        self.postInstallPath = os.path.join(self.controlDir, 'postinst')
        self.preUninstallPath = os.path.join(self.controlDir, 'prerm')
        self.postUninstallPath = os.path.join(self.controlDir, 'postrm')

    ##
    # Generate the package description files (e.g. prototype file)
    #
    def GeneratePackageDescriptionFiles(self):
        self.GenerateControlFile()

    def CreatePreInstallFile(self):
        preInstall = Script(self.platform)
        preInstall.WriteLn('if [ "$1" -eq "upgrade" ]; then')
        # This is an upgrade. PreUpgrade is handled by the prerm script.
        preInstall.WriteLn(':')
        preInstall.WriteLn('else')
        # If this an install.
        self.product.WritePreInstallToScript(preInstall)
        preInstall.WriteLn(':')
        preInstall.WriteLn('fi')
        preInstall.WriteLn('exit 0')
        ExecutableFile(self.preInstallPath, preInstall)

    def CreatePostInstallFile(self):
        postInstall = Script(self.platform)
        self.product.WritePostInstallToScript(postInstall)
        postInstall.WriteLn('exit 0')
        ExecutableFile(self.postInstallPath, postInstall)

    def CreatePreRemoveFile(self):
        preUninstall = Script(self.platform)
        preUninstall.WriteLn('if [ "$1" -eq "upgrade" ]; then')
        # This is an upgrade.
        self.product.WritePreUpgradeToScript(preUninstall)
        preUninstall.WriteLn(':')
        preUninstall.WriteLn('else')
        # If this an uninstall.
        self.product.WritePreRemoveToScript(preUninstall)
        preUninstall.WriteLn(':')
        preUninstall.WriteLn('fi')
        preUninstall.WriteLn('exit 0')
        ExecutableFile(self.preUninstallPath, preUninstall)

    def CreatePostRemoveFile(self):
        # Configuration file 'postrm' is a placeholder only
        postUninstall = Script(self.platform)
        self.product.WritePostRemoveToScript(postUninstall)
        postUninstall.WriteLn('exit 0')
        ExecutableFile(self.postUninstallPath, postUninstall)
        
    ##
    # Generate pre-, post-install scripts and friends
    #
    def GenerateScripts(self):
        #
        # On Ubuntu, we have four control scripts:
        #   preinst
        #   postinst
        #   prerm
        #   postrm
        #
        # On installation, preinst and postinst are called;
        #   Parameters: "configure"
        #
        # On removal, prerm and postrm are called;
        #   Parameters: "remove"
        # Note that postrm is called an additional time with "purge" argument if removing with --purge
        # (remove configuration files as well as the package).
        #
        # On upgrade, the calling sequence is as follows:
        #   prerm:    "upgrade" "<version>"
        #   preinst:  "upgrade" "<version>"
        #   postrm:   "upgrade" "<version>"
        #   postinst: "configure" "<version>"
        #

        # Create the directory (under staging directory) for control files
        scxutil.MkDir(self.controlDir)

        self.CreatePreInstallFile()

        self.CreatePostInstallFile()

        self.CreatePreRemoveFile()

        self.CreatePostRemoveFile()

    ##
    # Issues a du -s command to retrieve the total size of our package
    #
    def GetSizeInformation(self):
        pipe = os.popen("du -s " + self.stagingRootDir)

        sizeinfo = 0
        for line in pipe:
            [size, directory] = line.split()
            sizeinfo += int(size)

        return sizeinfo

    ##
    # Creates the control file used for packing.
    #
    def GenerateControlFile(self):
        # Build architecture string:
        #   For x86, the architecture should be: i386
        #   For x64, the architecture should be: amd64

        archType = self.platform.Architecture
        if archType == 'x86':
            archType = 'i386'
        elif archType == 'x64':
            archType = 'amd64'

        # Open and write the control file

        controlfile = open(self.controlFileName, 'w')

        controlfile.write('Package:      ' + self.product.ShortName + '\n')
        controlfile.write('Source:       ' + self.product.ShortName + '\n')
        controlfile.write('Version:      ' + self.product.Version + '_' + self.product.BuildNr + '\n')
        controlfile.write('Architecture: %s\n' % archType)
        controlfile.write('Maintainer:   ' + self.product.Vendor + '\n')
        controlfile.write('Installed-Size: %d\n' % self.GetSizeInformation())
        controlfile.write('Depends:      ' + ', '.join(self.product.GetPrerequisites()) + '\n')
        controlfile.write('Provides:     ' + self.product.ShortName + '\n')
        controlfile.write('Section:      utils\n')
        controlfile.write('Priority:     optional\n')
        controlfile.write('Description:  ' + self.product.LongName + '\n')
        controlfile.write(' %s\n' % self.product.Description)
        controlfile.write('\n')

        conffile = open(self.configFileName, 'w')

        # Now list all configuration files in staging directory
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'conffile':
                conffile.write(stagingObject.GetPath() + '\n')

    # Fix up owner/permissions in staging directory
    # (Files are installed on destination as staged)
    def SetOwnerAndPermissionOnStagedFiles(self):
        for obj in self.stagingDir.GetStagingObjectList():
            if obj.GetFileType() in ['conffile', 'file', 'dir', 'sysdir']:
                filePath = os.path.join(self.stagingDir.GetRootPath(), obj.GetPath())

                # Don't change the staging directory itself
                if obj.GetPath() != '':
                    scxutil.ChOwn(filePath, obj.GetOwner(), obj.GetGroup())
                    scxutil.ChMod(filePath, obj.GetPermissions())
            elif obj.GetFileType() == 'link':
                filePath = os.path.join(self.stagingDir.GetRootPath(), obj.GetPath())
                os.system('sudo chown --no-dereference %s:%s %s' \
                              % (obj.GetOwner(), obj.GetGroup(), filePath))
            else:
                print "Unrecognized type (%s) for object %s" % (obj.GetFileType(), obj.GetPath())

    ##
    # Actually creates the finished package file.
    #
    def BuildPackage(self):
        self.SetOwnerAndPermissionOnStagedFiles()

        pkgName = self.product.ShortName + '-' + \
            self.product.Version + '-' + \
            self.product.BuildNr + '.ubuntu.' + \
            str(self.platform.MajorVersion)  + '.' + self.platform.Architecture + '.deb'

        # Build the package - 'cd' to the directory where we want to store result
        os.system('cd ' + self.product.GetBuildInfo().GetTargetDir() + '; dpkg -b ' + self.stagingDir.GetRootPath() + ' ' + pkgName)

        # Undo the permissions settings so we can delete properly
        os.system('sudo chown -R $USER:admin %s' % self.stagingDir.GetRootPath())
