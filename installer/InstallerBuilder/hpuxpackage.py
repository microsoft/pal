# coding: utf-8

##
# Module containing classes to create a HP-UX package file
#
# Date:   2007-10-12 11:16:44
#

import os

import scxutil
from installer import Installer
from scriptgenerator import Script
from executablefile import ExecutableFile

##
# Class containing logic for creating a HP-UX package file
#
class HPUXPackageFile(Installer):
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
        self.specificationFileName = os.path.join(self.tempDir, 'product_specification')
        self.configurePath = os.path.join(self.tempDir, "configure.sh")
        self.unconfigurePath = os.path.join(self.tempDir, "unconfigure.sh")
        self.preinstallPath = os.path.join(self.tempDir, "preinstall.sh")
        self.postremovePath = os.path.join(self.tempDir, "postremove.sh")

    ##
    # Generate the package description files (e.g. prototype file)
    #
    def GeneratePackageDescriptionFiles(self):
        self.GenerateSpecificationFile()

    def GeneratePreInstallScript(self):
        # Stop services if scx is installed
        script = Script(self.platform)
        script.WriteLn('if [ -f ' + self.product.GetInstallationMarkerFile() + ' ]; then')
        # This is an upgrade
        self.product.WritePreUpgradeToScript(script)
        script.WriteLn(':')
        script.WriteLn('else')
        # This is a clean install
        self.product.WritePreInstallToScript(script)
        script.WriteLn(':')
        script.WriteLn('fi')
        # Back up any configuration files that are present at install time.
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'conffile':
                script.CallFunction(script.BackupConfigurationFile(),
                                    '/' + stagingObject.GetPath())
        script.WriteLn('exit 0')
        ExecutableFile(self.preinstallPath, script)
        
    def GenerateConfigureScript(self):
        configure = Script(self.platform)
        # Restore any configuration files that were present at install time.
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'conffile':
                configure.CallFunction(configure.RestoreConfigurationFile(),
                                       '/' + stagingObject.GetPath())
        self.product.WritePostInstallToScript(configure)
        configure.WriteLn('exit 0')
        ExecutableFile(self.configurePath, configure)

    def GenerateUnconfigureScript(self):
        unconfigure = Script(self.platform)
        self.product.WritePreRemoveToScript(unconfigure)
        ExecutableFile(self.unconfigurePath, unconfigure)

    def GeneratePostRemoveScript(self):
        postremove = Script(self.platform)
        self.product.WritePostRemoveToScript(postremove)
        postremove.WriteLn('exit 0')
        ExecutableFile(self.postremovePath, postremove)

    ##
    # Generate pre-, post-install scripts and friends
    #
    def GenerateScripts(self):
        self.GeneratePreInstallScript()
        self.GenerateConfigureScript()
        self.GenerateUnconfigureScript()
        self.GeneratePostRemoveScript()

    ##
    # Writes the content of the prototype file used for packing.
    #
    def WriteSpecTo(self, output):
        output.write('depot\n')
        output.write('  layout_version   1.0\n')
        output.write('\n')
        output.write('# Vendor definition:\n')
        output.write('vendor\n')
        output.write('  tag           MSFT\n')
        output.write('  title         ' + self.product.Vendor + '\n')
        output.write('category\n')
        output.write('  tag           ' + self.product.ShortName + '\n')
        output.write('  revision      ' + self.product.GetVersionString() + '\n')
        output.write('end\n')
        output.write('\n')
        output.write('# Product definition:\n')
        output.write('product\n')
        output.write('  tag            ' + self.product.ShortName + '\n')
        output.write('  revision       ' + self.product.GetVersionString() + '\n')
        output.write('  architecture   HP-UX_B.11.00_32/64\n')
        output.write('  vendor_tag     MSFT\n')
        output.write('\n')
        output.write('  title          ' + self.product.ShortName + '\n')
        output.write('  number         ' + self.product.BuildNr + '\n')
        output.write('  category_tag   ' + self.product.ShortName + '\n')
        output.write('\n')
        output.write('  description    ' + self.product.Description + '\n')
        output.write('  copyright      ' + self.product.Copyright + '\n')
        if self.platform.Architecture == 'ia64':
            output.write('  machine_type   ia64*\n')
        else:
            output.write('  machine_type   9000*\n')
        output.write('  os_name        HP-UX\n')
        output.write('  os_release     ?.11.*\n')
        output.write('  os_version     ?\n')
        output.write('\n')
        output.write('  directory      /\n')
        output.write('  is_locatable   false\n')
        output.write('\n')
        output.write('  # Fileset definitions:\n')
        output.write('  fileset\n')
        output.write('    tag          core\n')
        output.write('    title        ' + self.product.ShortName + ' Core\n')
        output.write('    revision     ' + self.product.GetVersionString() + '\n')
        output.write('\n')
        output.write('    # Dependencies\n')

        output.write('    prerequisites ' + ' & '.join(self.product.GetPrerequisites()) + '\n')

        output.write('\n')
        output.write('    # Control files:\n')
        output.write('    configure     ' + self.configurePath + '\n')
        output.write('    unconfigure   ' + self.unconfigurePath + '\n')
        output.write('    preinstall    ' + self.preinstallPath + '\n')
        output.write('    postremove    ' + self.postremovePath + '\n')
        output.write('\n')
        output.write('    # Files:\n')

        # Now list all files in staging directory
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'file' or \
                   stagingObject.GetFileType() == 'conffile' or \
                   stagingObject.GetFileType() == 'link' or \
                   stagingObject.GetFileType() == 'dir':
                output.write('    file -m ' + str(stagingObject.GetPermissions()) + \
                               ' -o ' + stagingObject.GetOwner() + \
                               ' -g ' + stagingObject.GetGroup() + \
                               ' ' + os.path.join(stagingObject.rootDir, stagingObject.GetPath()) + \
                               ' /' + stagingObject.GetPath() + '\n')


        output.write('\n')
        output.write('  end # core\n')
        output.write('\n')
        output.write('end  # SD\n')
        
    ##
    # Creates the prototype file used for packing.
    #
    def GenerateSpecificationFile(self):
        specfile = open(self.specificationFileName, 'w')
        self.WriteSpecTo(specfile)

    ##
    # Actually creates the finished package file.
    #
    def BuildPackage(self):
        if self.platform.MinorVersion < 30:
            osversion = '11iv2'
        else:
            osversion = '11iv3'
        if self.platform.Architecture == 'pa-risc':
            arch = 'parisc'
        else:
            arch = self.platform.Architecture

        depotfilename = os.path.join(self.product.GetBuildInfo().GetTargetDir(),
                                     self.product.ShortName + '-' + \
                                     self.product.Version + '-' + \
                                     self.product.BuildNr + \
                                     '.hpux.' + osversion + '.' + arch + '.depot')
        os.system('/usr/sbin/swpackage -s ' + os.path.join(self.tempDir, self.specificationFileName) + \
                  ' -x run_as_superuser=false -x admin_directory=' + self.tempDir + \
                  ' -x media_type=tape @ ' + depotfilename)
        os.system('compress -f ' + depotfilename)
        

