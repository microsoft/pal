import os
import scxutil

class LinuxRPMFile:
    def __init__(self, intermediateDir, targetDir, stagingDir, variables, sections):
        self.intermediateDir = intermediateDir
        self.targetDir = targetDir
        self.stagingDir = stagingDir
        self.variables = variables
        self.sections = sections
        self.specfileName = self.intermediateDir + "/" + "rpm.spec"
        self.fullversion_dashed = self.fullversion = self.variables["VERSION"]
        if "RELEASE" in self.variables:
            self.fullversion = self.variables["VERSION"] + "." + self.variables["RELEASE"]
            self.fullversion_dashed = self.variables["VERSION"] + "-" + self.variables["RELEASE"]
            
    def GeneratePackageDescriptionFiles(self):
        self.GenerateSpecFile()

    def CreateRPMDirectiveFile(self):
        # Create the RPM directory tree

        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/BUILD"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/RPMS/athlon"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/RPMS/i386"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/RPMS/i486"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/RPMS/i586"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/RPMS/i686"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/RPMS/ppc64le"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/RPMS/noarch"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/SOURCES"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/SPECS"))
        scxutil.MkAllDirs(os.path.join(self.intermediateDir, "RPM-packages/SRPMS"))

        # Create the RPM directive file

        if os.path.exists(os.path.join(os.path.expanduser('~'), '.rpmmacros')):
            scxutil.Move(os.path.join(os.path.expanduser('~'), '.rpmmacros'),
                         os.path.join(os.path.expanduser('~'), '.rpmmacros.save'))

        rpmfile = open(os.path.join(os.path.expanduser('~'), '.rpmmacros'), 'w')
        rpmfile.write('%%_topdir\t%s\n' % os.path.join(self.intermediateDir, "RPM-packages"))
        rpmfile.close()

    def DeleteRPMDirectiveFile(self):
        if os.path.exists(os.path.join(os.path.expanduser('~'), '.rpmmacros.save')):
            scxutil.Move(os.path.join(os.path.expanduser('~'), '.rpmmacros.save'),
                         os.path.join(os.path.expanduser('~'), '.rpmmacros'))
        else:
            os.unlink(os.path.join(os.path.expanduser('~'), '.rpmmacros'))

    def GetScriptAsString(self, section):
        script = ""
        for line in self.sections[section]:
            script += line
            script += "\n"
        script += "exit 0\n"
        return script

    def GenerateSpecFile(self):
        specfile = open(self.intermediateDir + "/" + "rpm.spec", 'w')

        specfile.write('%define __find_requires %{nil}\n')
        specfile.write('%define _use_internal_dependency_generator 0\n')

        if self.variables["PFDISTRO"] == "REDHAT":
            specfile.write('%%define dist el%(DISTNUM)d\n\n' % {'DISTNUM': int(self.variables["PFMAJOR"]) } )
        elif self.variables["PFDISTRO"] == "SUSE" or self.variables["PFDISTRO"] == "ULINUX":
            # SUSE doesn't appear to have a release standard in their RPMs
            # ULINUX doesn't need a release standard - there aren't multiple releases in same version #
            specfile.write("\n")
        else:
            print("Platform not implemented")
            exit(1)

        specfile.write('Name: ' + self.variables["SHORT_NAME"] + '\n')
        specfile.write('Version: ' + self.variables["VERSION"] + '\n')

        if "RELEASE" in self.variables:
            if self.variables["PFDISTRO"] == "REDHAT":
                specfile.write('Release: ' + self.variables["RELEASE"] + '.%{?dist}\n')
            else:
                specfile.write('Release: ' + self.variables["RELEASE"] + '\n')
        else:
            specfile.write('Release: 1\n')

        specfile.write('Summary: ' + self.variables["LONG_NAME"] + '\n')
        specfile.write('Group: ' + self.variables["GROUP"] + '\n')
        specfile.write('License: ' + self.variables["LICENSE"] + '\n')
        specfile.write('Vendor: '+ self.variables["VENDOR"] + '\n')

        if len(self.sections["Dependencies"]) > 0:
            specfile.write('Requires: ')
            for d in self.sections["Dependencies"]:
                specfile.write(d)
                if d != self.sections["Dependencies"][-1]:
                    specfile.write(", ")
        specfile.write("\n")

        specfile.write('Provides: ' + self.variables["PROVIDES"] + '\n')
        specfile.write('Conflicts: %{name} < %{version}-%{release}\n')
        specfile.write('Obsoletes: %{name} < %{version}-%{release}\n')
        specfile.write('%description\n')
        specfile.write(self.variables["DESCRIPTION"] + '\n')
        specfile.write('%files\n')

        # Now list all files in staging directory
        for d in self.sections["Directories"]:
            if d.type != "sysdir":
                specfile.write('%defattr(' + str(d.permissions) + "," + d.owner + "," + d.group + ")\n")
                specfile.write('%dir ' + d.stagedLocation + '\n')

        for f in self.sections["Files"]:
            specfile.write('%defattr(' + str(f.permissions) + "," + f.owner + "," + f.group + ")\n")
            if f.type == "conffile":
                specfile.write("%config ")
            specfile.write(f.stagedLocation + '\n')

        for l in self.sections["Links"]:
            specfile.write('%defattr(' + str(l.permissions) + "," + l.owner + "," + l.group + ")\n")
            specfile.write(l.stagedLocation + '\n')

        specfile.write("%pre\n")
        specfile.write(self.GetScriptAsString("Preinstall"))

        specfile.write('%post\n')
        specfile.write(self.GetScriptAsString("Postinstall"))

        specfile.write('%preun\n')
        specfile.write(self.GetScriptAsString("Preuninstall"))

        specfile.write('%postun\n')
        specfile.write(self.GetScriptAsString("Postuninstall"))

        specfile.close()

    def StageAndProperlyNameRPM(self):
        if 'OUTPUTFILE' in self.variables:
            rpmNewFileName = self.variables['OUTPUTFILE'] + '.rpm'
        elif self.variables['PFDISTRO'] == 'SUSE':
            rpmNewFileName = self.variables['SHORT_NAME'] + '-' + \
                self.fullversion_dashed + '.sles.' + \
                str(self.variables['PFMAJOR']) + '.' + self.variables['PFARCH'] + '.rpm'
        elif self.variables['PFDISTRO'] == 'REDHAT':
            rpmNewFileName = self.variables['SHORT_NAME'] + '-' + \
                self.fullversion_dashed + '.rhel.' + \
                str(self.variables['PFMAJOR'])  + '.' + self.variables['PFARCH'] + '.rpm'
        elif self.variables['PFDISTRO'] == 'ULINUX':
            rpmNewFileName = self.variables['SHORT_NAME'] + '-' + \
                self.fullversion_dashed + '.universalr.' + \
                str(self.variables['PFMAJOR'])  + '.' + self.variables['PFARCH'] + '.rpm'

        retval = os.system("mv `find %s/RPM-packages/ -name *.rpm` %s" % (self.intermediateDir, self.targetDir + '/' + rpmNewFileName))
        if retval != 0:
            print("Internal error: Unable to find generated rpm")
            exit(1)

        retval = os.system("rm -rf %s/RPM-packages" % self.intermediateDir)
        if retval != 0:
            print("Internal error: Unable to remove RPM-packages directory")
            exit(1)

        package_filename = open(self.targetDir + "/" + "package_filename", 'w')
        package_filename.write("%s\n" % rpmNewFileName)
        package_filename.close()

    def BuildPackage(self):
        self.CreateRPMDirectiveFile()
        self.specfileName = self.intermediateDir + "/" + "rpm.spec"

        if "SKIP_BUILDING_PACKAGE" in self.variables:
            return

        retval = os.system("rpmbuild --buildroot " + self.stagingDir + " -bb " + self.specfileName)
        if retval != 0:
            print("Error: rpmbuild failed")
            exit(1)

        self.DeleteRPMDirectiveFile()
        self.StageAndProperlyNameRPM()
