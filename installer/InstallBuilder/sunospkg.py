import os
from time import strftime

import scxutil

class SunOSPKGFile:
    def __init__(self, intermediateDir, targetDir, stagingDir, variables, sections):
        self.intermediateDir = intermediateDir
        self.targetDir = targetDir
        self.stagingDir = stagingDir
        self.variables = variables
        self.sections = sections
        self.tempDir = os.path.join(self.intermediateDir, "pkg-tmp")
        scxutil.MkAllDirs(self.tempDir)
        self.prototypeFileName = os.path.join(self.tempDir, 'prototype')
        self.pkginfoFile = PKGInfoFile(self.tempDir, self.variables)
        self.depFileName = os.path.join(self.tempDir, 'depend')
        self.preInstallPath = os.path.join(self.tempDir, "preinstall.sh")
        self.postInstallPath = os.path.join(self.tempDir, "postinstall.sh")
        self.preUninstallPath = os.path.join(self.tempDir, "preuninstall.sh")
        self.postUninstallPath = os.path.join(self.tempDir, "postuninstall.sh")
        self.iConfigFileName = os.path.join(self.tempDir, 'i.config')
        self.rConfigFileName = os.path.join(self.tempDir, 'r.config')
        self.fullversion_dashed = self.fullversion = self.variables["VERSION"]
        if "RELEASE" in self.variables:
            self.fullversion =  self.variables["VERSION"] + "." + self.variables["RELEASE"]
            self.fullversion_dashed =  self.variables["VERSION"] + "-" + self.variables["RELEASE"]

    def GeneratePackageDescriptionFiles(self):
        self.GeneratePrototypeFile()
        self.pkginfoFile.Generate()
        self.GenerateDepFile()
        self.GenerateScriptFiles()

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
        if self.variables['PFMAJOR'] >= 6 or self.variables['PFMINOR'] >= 10:
            prototype.write('i postremove=' + self.postUninstallPath + '\n')
        prototype.write('i i.config=' + self.iConfigFileName + '\n')
        prototype.write('i r.config=' + self.rConfigFileName + '\n')

        for d in self.sections["Directories"]:
            if d.type != "sysdir":
                prototype.write('d none')
                prototype.write(' ' + d.stagedLocation)
                prototype.write(' ' + str(d.permissions) + \
                                ' ' + d.owner + \
                                ' ' + d.group + '\n')

        for f in self.sections["Files"]:
            if f.type == "conffile":
                prototype.write('f config')
            else:
                prototype.write('f none')
            prototype.write(' ' + f.stagedLocation)
            prototype.write(' ' + str(f.permissions) + \
                            ' ' + f.owner + \
                            ' ' + f.group + '\n')

        for l in self.sections["Links"]:
            prototype.write("s none")
            prototype.write(' ' + l.stagedLocation)
            prototype.write('=' + l.baseLocation + '\n')
               
    def GenerateDepFile(self):
        depfile = open(self.depFileName, 'w')

        # The format of the dependency file is as follows:
        #
        # <type> <pkg>  <name>
        #       [(<arch>)<version>]
        #
        for d in self.sections["Dependencies"]:
            depfile.write(d)
            depfile.write("\n")

        # Solaris 11 uses a new package manager (Image Packaging System), and all of our dependencies 
        # are installed in it.  For now, do our dependency checks in the preinstall script, but in the
        # long term, we will create an IPS package for Solaris 11 that references IPS dependencies.

    def WriteScriptFile(self, filePath, section):
       scriptFile = open(filePath, 'w')
       script = ""
       for line in self.sections[section]:
           script += line + "\n"
       script += "exit 0\n"
       scriptFile.write(script)
       scriptFile.close()

    def GenerateScriptFiles(self):
        self.WriteScriptFile(self.preInstallPath, "Preinstall")
        self.WriteScriptFile(self.postInstallPath, "Postinstall")
        self.WriteScriptFile(self.preUninstallPath, "Preuninstall")
        self.WriteScriptFile(self.postUninstallPath, "Postuninstall")
        self.WriteScriptFile(self.iConfigFileName, "iConfig")
        self.WriteScriptFile(self.rConfigFileName, "rConfig")

    def BuildPackage(self):
	
        retval = os.system('pkgmk -o' + \
                  ' -r ' + self.stagingDir + \
                  ' -f ' + self.prototypeFileName + \
                  ' -d ' + self.tempDir)
        if retval != 0:
            print("Error: pkgmk failed")
            exit(1)

        if 'OUTPUTFILE' in self.variables:
            basepkgfilename = self.variables['OUTPUTFILE'] + '.pkg'
        else:
            basepkgfilename = self.variables['SHORT_NAME'] + '-' + \
                self.fullversion_dashed + \
                '.solaris.' + str(self.variables['PFMINOR']) + '.' + \
                self.variables['PFARCH'] + '.pkg'

        pkgfilename = os.path.join(self.targetDir, basepkgfilename)

        short_name_prefix = ""
        if "SHORT_NAME_PREFIX" in self.variables.keys():
            short_name_prefix = self.variables["SHORT_NAME_PREFIX"]

        if "SKIP_BUILDING_PACKAGE" in self.variables:
            return
            
        retval = os.system('pkgtrans -s ' + self.tempDir + ' ' + pkgfilename + ' ' +  short_name_prefix + self.variables['SHORT_NAME'])
        if retval != 0:
            print("Error: pkgtrans failed.")
            exit(1)

        package_filename = open(self.targetDir + "/" + "package_filename", 'w')
        package_filename.write("%s\n" % basepkgfilename)
        package_filename.close()

class PKGInfoFile:
    
    def __init__(self, directory, variables):
        self.filename = os.path.join(directory, 'pkginfo')
        self.properties = []

        self.fullversion_dashed = self.fullversion = variables["VERSION"]
        if "RELEASE" in variables:
            self.fullversion =  variables["VERSION"] + "." + variables["RELEASE"]
            self.fullversion_dashed =  variables["VERSION"] + "-" + variables["RELEASE"]

        short_name_prefix = ""
        if "SHORT_NAME_PREFIX" in variables.keys():
            short_name_prefix = variables["SHORT_NAME_PREFIX"]

        # Required entries
        self.AddProperty('PKG', short_name_prefix + variables['SHORT_NAME'])
        self.AddProperty('ARCH', variables['PFARCH'])
        self.AddProperty('CLASSES', 'none config')
        self.AddProperty('PSTAMP', strftime("%Y%m%d-%H%M"))
        self.AddProperty('NAME', variables['LONG_NAME'])
        self.AddProperty('VERSION', self.fullversion_dashed)
        self.AddProperty('CATEGORY', 'system')
        
        # Optional entries:
        self.AddProperty('DESC', variables['DESCRIPTION'])
        self.AddProperty('VENDOR', variables['VENDOR'])
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
