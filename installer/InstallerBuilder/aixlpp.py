# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to create an AIX package file (lpp)
#
# Date:   2008-03-24 03:15:03
#

import os

import scxutil
from installer import Installer
from scriptgenerator import Script
from executablefile import ExecutableFile

##
# Class containing logic for creating an AIX package file
#
class AIXLPPFile(Installer):
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
        self.filesetName = self.product.ShortName + '.rte'
        self.lppNameFileName = os.path.join(self.stagingDir.GetRootPath(), 'lpp_name')
        self.alFileName = os.path.join(self.tempDir, self.filesetName + '.al')
        self.cfgfilesFileName = os.path.join(self.tempDir, self.filesetName + '.cfgfiles')
        self.copyrightFileName = os.path.join(self.tempDir, self.filesetName + '.copyright')
        self.inventoryFileName = os.path.join(self.tempDir, self.filesetName + '.inventory')
        self.sizeFileName = os.path.join(self.tempDir, self.filesetName + '.size')
        self.productidFileName = os.path.join(self.tempDir, 'productid')
        self.liblppFileName = os.path.join(self.stagingDir.GetRootPath(), 'usr/lpp/' + self.filesetName +'/liblpp.a')
        # Need to specify new file names for scripts.
        self.preInstallPath = os.path.join(self.tempDir, self.filesetName + '.pre_i')
        self.postInstallPath = os.path.join(self.tempDir, self.filesetName + '.config')
        self.preUninstallPath = os.path.join(self.tempDir, self.filesetName + '.unconfig')
        self.preUpgradePath = os.path.join(self.tempDir, self.filesetName + '.pre_rm')

    ##
    # Generate the package description files (e.g. prototype file)
    # In the AIX case we generate an lpp_name and a liblpp.a file.
    #
    def GeneratePackageDescriptionFiles(self):
        self.GenerateLppNameFile()
        self.GenerateLiblppFile()
 
    ##
    # Generate pre-, post-install scripts and friends
    #
    def GenerateScripts(self):
        preInstall = Script(self.platform)
        self.product.WritePreInstallToScript(preInstall)
        ExecutableFile(self.preInstallPath, preInstall)

        preUpgrade = Script(self.platform)
        self.product.WritePreUpgradeToScript(preUpgrade)
        ExecutableFile(self.preUpgradePath, preUpgrade)

        postInstall = Script(self.platform)
        self.product.WritePostInstallToScript(postInstall)
        postInstall.WriteLn('exit 0')
        ExecutableFile(self.postInstallPath, postInstall)

        preUninstall = Script(self.platform)
        self.product.WritePreRemoveToScript(preUninstall)
        ExecutableFile(self.preUninstallPath, preUninstall)
        
    def WriteLppNameContentTo(self, output):
        output.write('4 R I ' + self.filesetName + ' {\n')
        output.write(self.filesetName + ' ' + self.product.Version + '.' + self.product.BuildNr + ' 1 N U en_US ' + self.product.LongName + '\n')
        output.write('[\n')
        # Requisite information would go here.
        for prereq in self.product.GetPrerequisites():
            output.write('*prereq ' + prereq + '\n')
        output.write('%\n')
        # Now we write the space requirements.
        sizeInfo = self.GetSizeInformation()
        for [directory, size] in sizeInfo:
            output.write('/' + directory + ' ' + size + '\n')
        output.write('%\n')
        output.write('%\n')
        output.write('%\n')
        output.write(']\n')
        output.write('}\n')

    ##
    # Generate the lpp_name file
    #
    def GenerateLppNameFile(self):
        specfile = open(self.lppNameFileName, 'w')
        self.WriteLppNameContentTo(specfile)

    ##
    # Issues a du -s command to retrieve a list of [directory, size] pairs
    #
    def GetSizeInformation(self):
        pipe = os.popen("du -s " + os.path.join(self.stagingDir.GetRootPath(), '*'))

        sizeinfo = []
        for line in pipe:
            [size, directory] = line.split()
            directory = os.path.basename(directory)
            sizeinfo.append([directory, size])

        return sizeinfo

    ##
    # Generates the liblpp.a file.
    #
    def GenerateLiblppFile(self):
        self.GenerateALFile()
        self.GenerateCfgfilesFile()
        self.GenerateCopyrightFile()
        self.GenerateInventoryFile()
        self.GenerateSizeFile()
        self.GenerateProductidFile()

        # Now create a .a archive package
        os.system('ar -vqg ' + self.liblppFileName + ' ' + os.path.join(self.tempDir, '*')) 

    ##
    # Generates the applied list file.
    #
    def GenerateALFile(self):
        alfile = open(self.alFileName, 'w')
        # The lpp_name file should be first.
        alfile.write('./lpp_name\n')
        # Now list everything in the stagingdir except sysdirs.
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'file' or \
                   stagingObject.GetFileType() == 'conffile' or \
                   stagingObject.GetFileType() == 'link' or \
                   stagingObject.GetFileType() == 'dir':
                alfile.write('./' + stagingObject.GetPath() + '\n')

    ##
    # Generates the cfgfiles file.
    #
    def GenerateCfgfilesFile(self):
        cfgfilesfile = open(self.cfgfilesFileName, 'w')

        # Now list all conffiles in the stagingdir.
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'conffile':
                cfgfilesfile.write('./' + stagingObject.GetPath() + ' preserve\n')

    ##
    # Generates the copyright file.
    #
    def GenerateCopyrightFile(self):
        copyrightfile = open(self.copyrightFileName, 'w')

        copyrightfile.write(self.product.Copyright)
                
    ##
    # Generates the inventory file.
    #
    def GenerateInventoryFile(self):
        inventoryfile = open(self.inventoryFileName, 'w')

        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'sysdir':
                pass
            else:
                inventoryfile.write('/' + stagingObject.GetPath() + ':\n')
                inventoryfile.write('   owner = ' + stagingObject.GetOwner() + '\n')
                inventoryfile.write('   group = ' + stagingObject.GetGroup() + '\n')
                inventoryfile.write('   mode = ' + str(stagingObject.GetPermissions()) + '\n')
                inventoryfile.write('   class = apply,inventory,' + self.filesetName + '\n')
                
                if stagingObject.GetFileType() == 'file' or stagingObject.GetFileType() == 'conffile':
                    inventoryfile.write('   type = FILE\n')
                    inventoryfile.write('   size = \n')
                    inventoryfile.write('   checksum = \n')

                if stagingObject.GetFileType() == 'link':
                    inventoryfile.write('   type = SYMLINK\n')
                    inventoryfile.write('   target = ' + stagingObject.GetTarget() + '\n')

                if stagingObject.GetFileType() == 'dir':
                    inventoryfile.write('   type = DIRECTORY\n')

                inventoryfile.write('\n')

    ##
    # Generate the size file
    #
    def GenerateSizeFile(self):
        sizefile = open(self.sizeFileName, 'w')

        sizeInfo = self.GetSizeInformation()
        for [directory, size] in sizeInfo:
            sizefile.write('/' + directory + ' ' + size + '\n')

    ##
    # Generates the productid file.
    #
    def GenerateProductidFile(self):
        productidfile = open(self.productidFileName, 'w')

        productidfile.write(self.product.ShortName + ',' + self.product.Version + '-' + self.product.BuildNr)


    ##
    # Actually creates the finished package file.
    #
    def BuildPackage(self):
        lppfilename = os.path.join(self.product.GetBuildInfo().GetTargetDir(),
                                   self.product.ShortName + '-' + \
                                   self.product.Version + '-' + \
                                   self.product.BuildNr + \
                                   '.aix.' + str(self.platform.MajorVersion) + '.' + \
                                   self.platform.Architecture + '.lpp')
        os.system('cd ' + self.stagingDir.GetRootPath() + \
                  ' && find . | grep -v \"^\\.$\" | backup -ivqf ' + lppfilename)
        os.system('gzip -f ' + lppfilename)


