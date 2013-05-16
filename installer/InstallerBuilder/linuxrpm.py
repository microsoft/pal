# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to create a Linux RPM file
#
# Date:   2007-10-08 10:42:48
#

import os
import scxutil
from installer import Installer
from scriptgenerator import Script
from platforminformation import PlatformInformation
from cStringIO import StringIO

class LinuxRPMSpecFile:
    def __init__(self, product, platform, stagingDir):
        self.product = product
        self.platform = platform
        self.stagingDir = stagingDir

    def GetDependencies(self):
        return ', '.join(self.product.GetPrerequisites())
        
    # List all files in staging directory
    def WriteFilesListTo(self, output):
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'sysdir':
                pass
            else:
                output.write('%defattr(' + str(stagingObject.GetPermissions()) + \
                               ',' + stagingObject.GetOwner() + \
                               ',' + stagingObject.GetGroup() + ')\n')
                if stagingObject.GetFileType() == 'dir':
                    output.write('%dir /' + stagingObject.GetPath() + '\n')
                elif stagingObject.GetFileType() == 'conffile':
                    output.write('%config /' + stagingObject.GetPath() + '\n')
                else:
                    output.write('/' + stagingObject.GetPath() + '\n')

    def WritePreInstallScriptTo(self, output):
        script = Script(self.platform)
        script.WriteLn('if [ $1 -eq 2 ]; then')
        # This is an upgrade.
        self.product.WritePreUpgradeToScript(script)
        script.WriteLn(':')
        script.WriteLn('else')
        # If this a clean install.
        self.product.WritePreInstallToScript(script)
        script.WriteLn(':')
        script.WriteLn('fi')
        script.WriteTo(output)

    def WritePostInstallScriptTo(self, output):
        script = Script(self.platform)
        self.product.WritePostInstallToScript(script)
        script.WriteLn('exit 0')
        script.WriteTo(output)        

    def WritePreRemoveScriptTo(self, output):
        script = Script(self.platform)
        script.WriteLn('if [ $1 -eq 0 ]; then')
        # If this is a clean uninstall and not part of an upgrade
        self.product.WritePreRemoveToScript(script)
        script.WriteLn(':')
        script.WriteLn('fi')
        script.WriteLn('exit 0')
        script.WriteTo(output)

    def WritePostRemoveScriptTo(self, output):
        script = Script(self.platform)
        self.product.WritePostRemoveToScript(script)
        script.WriteLn('exit 0')
        script.WriteTo(output)

    def WriteTo(self, output):
        output.write('%define __find_requires %{nil}\n')
        output.write('%define _use_internal_dependency_generator 0\n\n')

        output.write('Name: ' + self.product.ShortName + '\n')
        output.write('Version: ' + self.product.Version + '\n')
        output.write('Release: ' + self.product.BuildNr + '\n')
        output.write('Summary: ' + self.product.LongName + '\n')
        output.write('Group: Applications/System\n')
        output.write('License: ' + self.product.License + '\n')
        output.write('Vendor: ' + self.product.Vendor + '\n')

        output.write('Requires: ' + self.GetDependencies() + '\n')

        output.write('Provides: cim-server\n')
        output.write('Conflicts: %{name} < %{version}-%{release}\n')
        output.write('Obsoletes: %{name} < %{version}-%{release}\n')
        output.write('%description\n')
        output.write(self.product.Description + '\n')
        
        output.write('%files\n')
        self.WriteFilesListTo(output)
        
        output.write('%pre\n')
        self.WritePreInstallScriptTo(output)
        output.write('%post\n')
        self.WritePostInstallScriptTo(output)
        output.write('%preun\n')
        self.WritePreRemoveScriptTo(output)
        output.write('%postun\n')
        self.WritePostRemoveScriptTo(output)

##
# Class containing logic for creating a Linux RPM file
#
class LinuxRPMFile(Installer):
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
        self.specFileName = os.path.join(self.tempDir, product.ShortName + '.spec')
        
    ##
    # Generate the package description files (e.g. prototype file)
    #
    def GeneratePackageDescriptionFiles(self):
        self.GenerateSpecFile()

    ##
    # Generate pre-, post-install scripts and friends
    # The scripts are baked into the spec file for the RPM format.
    #
    def GenerateScripts(self):
        pass
        
    ##
    # Creates the specification file used for packing.
    #
    def GenerateSpecFile(self):
        specFile = LinuxRPMSpecFile(self.product, self.platform, self.stagingDir)
        file = open(self.specFileName, 'w')
        specFile.WriteTo(file)

    ##
    # Create the RPM directive file (to refer to our own RPM directory tree)
    #
    def CreateRPMDirectiveFile(self):
        # Create the RPM directory tree

        targetDir = self.product.GetBuildInfo().GetTargetDir()
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/BUILD"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/RPMS/athlon"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/RPMS/i386"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/RPMS/i486"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/RPMS/i586"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/RPMS/i686"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/RPMS/noarch"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/SOURCES"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/SPECS"))
        scxutil.MkAllDirs(os.path.join(targetDir, "RPM-packages/SRPMS"))

        # Create the RPM directive file

        if os.path.exists(os.path.join(os.path.expanduser('~'), '.rpmmacros')):
            scxutil.Move(os.path.join(os.path.expanduser('~'), '.rpmmacros'),
                         os.path.join(os.path.expanduser('~'), '.rpmmacros.save'))

        rpmfile = open(os.path.join(os.path.expanduser('~'), '.rpmmacros'), 'w')
        rpmfile.write('%%_topdir\t%s\n' % os.path.join(targetDir, "RPM-packages"))
        rpmfile.close()

    ##
    # Cleanup RPM directive file
    #
    # (If a prior version of the file exists, retain that)
    #
    def DeleteRPMDirectiveFile(self):
        if os.path.exists(os.path.join(os.path.expanduser('~'), '.rpmmacros.save')):
            scxutil.Move(os.path.join(os.path.expanduser('~'), '.rpmmacros.save'),
                         os.path.join(os.path.expanduser('~'), '.rpmmacros'))
        else:
            os.unlink(os.path.join(os.path.expanduser('~'), '.rpmmacros'))

    ##
    # Actually creates the finished package file.
    #
    def BuildPackage(self):
        # Create the RPM working directory tree
        self.CreateRPMDirectiveFile()

        # Build the RPM. This puts the finished rpm in /usr/src/packages/RPMS/<arch>/
        os.system('rpmbuild --buildroot ' + self.stagingDir.GetRootPath() + ' -bb ' + self.specFileName)
        self.DeleteRPMDirectiveFile()

        # Now we try to find the file so we can copy it to the installer directory.
        # We need to find the build arch of the file:
        fin, fout = os.popen4('rpm -q --specfile --qf "%{arch}\n" ' + self.specFileName)
        arch = fout.read().strip()
        rpmpath = os.path.join(os.path.join(self.product.GetBuildInfo().GetTargetDir(), "RPM-packages/RPMS"), arch)
        if self.platform.Distribution == 'SUSE':
            shortPlatformName = 'sles'
        elif self.platform.Distribution == 'REDHAT':
            shortPlatformName = 'rhel'
        else:
            raise PlatformNotImplementedError(self.platform.Distribution)

        rpmNewFileName = self.product.ShortName + '-' + \
                         self.product.Version + '-' + \
                         self.product.BuildNr + '.' + shortPlatformName + '.' + \
                         str(self.platform.MajorVersion) + '.' + self.platform.Architecture + '.rpm'

        rpmfilename = self.product.ShortName + '-' + \
                      self.product.Version + '-' + \
                      self.product.BuildNr + '.' + arch + '.rpm'
                         
        
        scxutil.Move(os.path.join(rpmpath, rpmfilename), os.path.join(self.product.GetBuildInfo().GetTargetDir(), rpmNewFileName))
        print "Moved to: " + os.path.join(self.product.GetBuildInfo().GetTargetDir(), rpmNewFileName)

