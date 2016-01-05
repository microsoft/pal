# coding: utf-8
import os
import scxutil

class AIXLPPFile:
    def __init__(self, intermediateDir, targetDir, stagingDir, variables, sections):
        self.intermediateDir = intermediateDir
        self.targetDir = targetDir
        self.stagingDir = stagingDir
        self.variables = variables
        self.sections = sections
        self.tempDir = os.path.join(self.intermediateDir, "lpp-tmp")
        scxutil.MkAllDirs(self.tempDir)
        self.filesetName = self.variables["SHORT_NAME"] + '.rte'
        self.lppNameFileName = os.path.join(self.stagingDir, 'lpp_name')
        self.alFileName = os.path.join(self.tempDir, self.filesetName + '.al')
        self.cfgfilesFileName = os.path.join(self.tempDir, self.filesetName + '.cfgfiles')
        self.copyrightFileName = os.path.join(self.tempDir, self.filesetName + '.copyright')
        self.inventoryFileName = os.path.join(self.tempDir, self.filesetName + '.inventory')
        self.sizeFileName = os.path.join(self.tempDir, self.filesetName + '.size')
        self.productidFileName = os.path.join(self.tempDir, 'productid')
        self.liblppFileName = os.path.join(self.stagingDir, 'usr/lpp/' + self.filesetName +'/liblpp.a')
        # Need to specify new file names for scripts.
        self.preInstallPath = os.path.join(self.tempDir, self.filesetName + '.pre_i')
        self.postInstallPath = os.path.join(self.tempDir, self.filesetName + '.config')
        self.preUninstallPath = os.path.join(self.tempDir, self.filesetName + '.unconfig')
        self.postUninstallPath = os.path.join(self.tempDir, self.filesetName + '.unpost_i')
        self.preUpgradePath = os.path.join(self.tempDir, self.filesetName + '.pre_rm')
        self.fullversion_dashed = self.fullversion = self.variables["VERSION"]
        if "RELEASE" in self.variables:
            self.fullversion = self.variables["VERSION"] + "." + self.variables["RELEASE"]
            self.fullversion_dashed = self.variables["VERSION"] + "-" + self.variables["RELEASE"]

    def WriteScriptFile(self, filePath, section):
        scriptfile = open(filePath, 'w')
        script = ""
        for line in self.sections[section]:
            script += line + "\n"
        script += "exit 0\n"
        scriptfile.write(script)
        scriptfile.close()

    def GenerateScripts(self):
        self.WriteScriptFile(self.preInstallPath, "Preinstall")
        self.WriteScriptFile(self.preUpgradePath, "Preupgrade")
        self.WriteScriptFile(self.postInstallPath, "Postinstall")
        self.WriteScriptFile(self.preUninstallPath, "Preuninstall")
        self.WriteScriptFile(self.postUninstallPath, "Postuninstall")

    def GeneratePackageDescriptionFiles(self):
        self.GenerateScripts()
        self.GenerateLppNameFile()
        self.GenerateLiblppFile()
 
    def GenerateLppNameFile(self):
        specfile = open(self.lppNameFileName, 'w')

        specfile.write('4 R I ' + self.filesetName + ' {\n')
        specfile.write(self.filesetName + ' ' + self.fullversion + ' 1 N U en_US ' + self.variables["LONG_NAME"] + '\n')
        specfile.write('[\n')
        # Requisite information would go here.

        for dep in self.sections["Dependencies"]:
            specfile.write(dep + '\n')

        specfile.write('%\n')
        # Now we write the space requirements.
        sizeInfo = self.GetSizeInformation()
        for [directory, size] in sizeInfo:
            specfile.write('/' + directory + ' ' + size + '\n')
        specfile.write('%\n')
        specfile.write('%\n')
        specfile.write('%\n')
        specfile.write(']\n')
        specfile.write('}\n')

    def GetSizeInformation(self):
        pipe = os.popen("du -s " + os.path.join(self.stagingDir, '*'))

        sizeinfo = []
        for line in pipe:
            [size, directory] = line.split()
            directory = os.path.basename(directory)
            sizeinfo.append([directory, size])

        return sizeinfo

    def GenerateLiblppFile(self):
        self.GenerateALFile()
        self.GenerateCfgfilesFile()
        self.GenerateCopyrightFile()
        self.GenerateInventoryFile()
        self.GenerateSizeFile()
        self.GenerateProductidFile()

        # Now create a .a archive package
        retval = os.system('ar -vqg ' + self.liblppFileName + ' ' + os.path.join(self.tempDir, '*'))
        if retval != 0:
            print("Error: Unable to create lpp package.")
            exit(1)

    def GenerateALFile(self):
        alfile = open(self.alFileName, 'w')
        # The lpp_name file should be first.
        alfile.write('./lpp_name\n')
        # Now list everything in the stagingdir except sysdirs.
        for d in self.sections["Directories"]:
            if d.type == "":
                alfile.write('.' + d.stagedLocation + '\n')

        for f in self.sections["Files"]:
            alfile.write('.' + f.stagedLocation + '\n')

        for l in self.sections["Links"]:
            alfile.write('.' + l.stagedLocation + '\n')

    def GenerateCfgfilesFile(self):
        cfgfilesfile = open(self.cfgfilesFileName, 'w')

        # Now list all conffiles in the stagingdir.
        for f in self.sections["Files"]:
            if f.type == 'conffile':
                cfgfilesfile.write('.' + f.stagedLocation + ' preserve\n')

    def GenerateCopyrightFile(self):
        copyrightfile = open(self.copyrightFileName, 'w')
        copyright_string = self.variables["COPYRIGHT_FILE"]
        copyright_string = copyright_string.replace("\\n", "\n").replace("\\t", "\t")
        copyrightfile.write(copyright_string)

    def GenerateInventoryFile(self):
        inventoryfile = open(self.inventoryFileName, 'w')

        for d in self.sections["Directories"]:
            if d.type == 'sysdir':
                pass

            inventoryfile.write(d.stagedLocation + ':\n')
            inventoryfile.write('   owner = ' + d.owner + '\n')
            inventoryfile.write('   group = ' + d.group + '\n')
            inventoryfile.write('   mode = ' + str(d.permissions) + '\n')
            inventoryfile.write('   class = apply,inventory,' + self.filesetName + '\n')   
            inventoryfile.write('   type = DIRECTORY\n')
            inventoryfile.write('\n')

        for f in self.sections["Files"]:
            inventoryfile.write(f.stagedLocation + ':\n')
            inventoryfile.write('   owner = ' + f.owner + '\n')
            inventoryfile.write('   group = ' + f.group + '\n')
            inventoryfile.write('   mode = ' + str(f.permissions) + '\n')
            inventoryfile.write('   type = FILE\n')
            inventoryfile.write('   size = \n')
            inventoryfile.write('   checksum = \n')
            inventoryfile.write('\n')

        for l in self.sections["Links"]:
            inventoryfile.write(l.stagedLocation + ':\n')
            inventoryfile.write('   owner = ' + l.owner + '\n')
            inventoryfile.write('   group = ' + l.group + '\n')
            inventoryfile.write('   mode = ' + str(l.permissions) + '\n')
            inventoryfile.write('   type = SYMLINK\n')
            inventoryfile.write('   target = ' +l.baseLocation + '\n')
            inventoryfile.write('\n')

    def GenerateSizeFile(self):
        sizefile = open(self.sizeFileName, 'w')

        sizeInfo = self.GetSizeInformation()
        for [directory, size] in sizeInfo:
            sizefile.write('/' + directory + ' ' + size + '\n')

    def GenerateProductidFile(self):
        productidfile = open(self.productidFileName, 'w')

        productidfile.write(self.variables["SHORT_NAME"] + ',' + self.fullversion)


    def BuildPackage(self):
        if 'OUTPUTFILE' in self.variables:
            lppbasefilename = self.variables['OUTPUTFILE'] + '.lpp'
        else:
            lppbasefilename = self.variables["SHORT_NAME"] + '-' + \
                self.fullversion_dashed + \
                '.aix.' + str(self.variables["PFMAJOR"]) + '.' + \
                self.variables['PFARCH'] + '.lpp'

        lppfilename = os.path.join(self.targetDir, lppbasefilename)

        if "SKIP_BUILDING_PACKAGE" in self.variables:
            return
        
        retval = os.system('cd ' + self.stagingDir + \
                  ' && find . | grep -v \"^\\.$\" | backup -ivqf ' + lppfilename)

        if retval != 0:
            print("Internal error: Unable to find lpp file.")
            exit(1)

        package_filename = open(self.targetDir + "/" + "package_filename", 'w')
        package_filename.write("%s\n" % lppbasefilename)
        package_filename.close()
