# coding: utf-8

##
# Module containing classes to create a Mac OS package file
#
# Date:   2008-06-11 10:34:15
#

import copy
import os
import scxutil
from installer import Installer
from scriptgenerator import Script
import scxexceptions
from platforminformation import PlatformInformation
from executablefile import ExecutableFile

##
# Support to generate XML list of files in the staging directory
# (Necessary for Mac installer to set permissions properly)
#

class TreeItem:
    def __init__(self, node):
        self.node = node
        self.children = {}

    def Insert(self, path, node):
        parts = path.split('/')
        if len(parts) == 1:
            #print 'adding ' + node
            self.children[parts[0]] = TreeItem(node)
        else:
            if not self.children.has_key(parts[0]):
                self.children[parts[0]] = TreeItem(None)
            self.children[parts[0]].Insert('/'.join(parts[1:]), node)

    def AsXML(self, file, indent):
        if self.node is not None:
            # Regular files get 100000 (octal), directories get 40000 (octal)
            # The special system '/Library' directory needs 41000 (dir/sticky bit)
            terminalChar = ''
            if self.node.GetFileType() == 'sysdir':
                if self.node.GetPath() == 'Library':
                    fileType = '41000'
                else:
                    fileType = '40000'
            elif self.node.GetFileType() == 'dir':
                fileType = '40000'
            else:
                fileType = '100000'
                terminalChar = '/'
            # In the XML contents file, we need permissions to be in base 10 ...
            perm = int('%d' % self.node.GetPermissions(), 8) + int(fileType, 8)
            file.write('%s<f n="%s" o="%s" g="%s" p="%d"%s>\n' \
                           % (indent, self.node.GetPath(), self.node.GetOwner(), self.node.GetGroup(), perm, terminalChar))
        for n in self.children.itervalues():
            n.AsXML(file, indent + '  ')
        if self.node is not None:
            if self.node.GetFileType() == 'sysdir' or self.node.GetFileType() == 'dir':
                file.write('%s</f>\n' % (indent))

def ListToTree(inList):
    out = TreeItem(None)
    for ref in inList:
        # Make copy so we're not destructive
        l = copy.copy(ref)
        path = l.GetPath()
        parts = path.split('/')
        l.SetPath(parts[-1])
        # No empty nodes
        if l.GetPath() != '':
            out.Insert(path, l)
    return out

##
# Class containing logic for creating a Mac OS package file
#
class MacOSPackageFile(Installer):
    ##
    # Ctor.
    # \param[in] stagingDir StagingDirectory with files to be installed.
    # \param[in] product Information about the product being built.
    # \param[in] platform Information about the current target platform.
    #
    def __init__(self, stagingDir, product, platform):
        Installer.__init__(self, product.GetBuildInfo().GetTempDir(), stagingDir)
        self.targetDir = product.GetBuildInfo().GetTargetDir()
        self.product = product
        self.platform = platform
        self.scriptDir = os.path.join(self.tempDir, 'scripts')
        self.preinstallPath = os.path.join(self.scriptDir, "preinstall")
        self.postinstallPath = os.path.join(self.scriptDir, "postinstall")
        self.preupgradePath = os.path.join(self.scriptDir, "preupgrade")
        self.postupgradePath = os.path.join(self.scriptDir, "postupgrade")

        self.osverstring = str(self.platform.MajorVersion) + '.' + self.platform.MinorVersionString
        if self.platform.MajorVersion == 10:
            if self.platform.MinorVersion == 4:
                self.packageHelper = MacOS10_4PackageHelper(stagingDir.GetRootPath(),
                                                            self.tempDir,
                                                            self.product.Version,
                                                            self.product.BuildNr,
                                                            self.scriptDir)
            elif self.platform.MinorVersion == 5:
                self.packageHelper = MacOS10_5PackageHelper(stagingDir.GetRootPath(),
                                                            self.tempDir,
                                                            self.targetDir,
                                                            self.product.Version,
                                                            self.product.BuildNr,
                                                            self.preinstallPath,
                                                            self.postinstallPath,
                                                            self.preupgradePath,
                                                            self.postupgradePath)
            else:
                raise scxexceptions.PlatformNotImplementedError(self.osverstring)
        else:
            raise scxexceptions.PlatformNotImplementedError(self.osverstring)

    ##
    # Generate the package description files (e.g. prototype file)
    #
    def GeneratePackageDescriptionFiles(self):
        self.packageHelper.GeneratePackageDescriptionFiles(self.stagingDir)

    ##
    # Generate pre-, post-install scripts and friends
    #
    def GenerateScripts(self):
        os.system('mkdir ' + self.scriptDir + ' > /dev/null 2>&1')

        uninstallPath = '/usr/libexec/microsoft/scx/bin/scxUninstall.sh'

        preInstall = Script(self.platform)
        preInstall.WriteLn('if [ -f ' + uninstallPath + ' ]; then')
        # Run the uninstall script of the installed version.
        preInstall.WriteLn('  ' + uninstallPath + ' upgrade')
        preInstall.WriteLn('fi')

        self.product.WritePreInstallToScript(preInstall)
        # Back up any configuration files that are present at install time.
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'conffile':
                preInstall.CallFunction(preInstall.BackupConfigurationFile(),
                                    '/' + stagingObject.GetPath())

        ExecutableFile(self.preinstallPath, preInstall)

        scxutil.Copy(self.preinstallPath, self.preupgradePath)

        #
        # Postinstall will do final system configuration
        #
        postinstall = Script(self.platform)
        # Restore any configuration files that were present at install time.
        for stagingObject in self.stagingDir.GetStagingObjectList():
            if stagingObject.GetFileType() == 'conffile':
                postinstall.CallFunction(postinstall.RestoreConfigurationFile(),
                                       '/' + stagingObject.GetPath())
        self.product.WritePostInstallToScript(postinstall)
        postinstall.WriteLn('exit 0')
        ExecutableFile(self.postinstallPath, postinstall)
        scxutil.Copy(self.postinstallPath, self.postupgradePath)

        #
        # Create the uninstall script to clean up services and remove the product
        #

        uninstall = Script(self.platform)
        # Check for administrator privilege
        uninstall.WriteLn('echo "Microsoft SCX version ' + self.product.Version + '-' + self.product.BuildNr + ' uninstallation script\\n"')
        uninstall.WriteLn('')
        uninstall.WriteLn('# Check for administrator privilege')
        uninstall.WriteLn('if [ `id -u` -ne 0 ]')
        uninstall.WriteLn('then')
        uninstall.WriteLn('        echo \'Root privilege is required to uninstall Microsoft SCX for OSX\'')
        uninstall.WriteLn('        exit 2')
        uninstall.WriteLn('fi')
        uninstall.WriteLn('')
        uninstall.WriteLn('if [ "$1" -eq "upgrade" ]; then')
        # This is an upgrade.
        self.product.WritePreUpgradeToScript(uninstall)
        uninstall.WriteLn(':')
        uninstall.WriteLn('else')
        # If this an uninstall.
        self.product.WritePreRemoveToScript(uninstall)
        uninstall.WriteLn(':')
        uninstall.WriteLn('fi')

        # Now list all files to delete in staging directory - one by one
        uninstall.WriteLn('')
        uninstall.WriteLn('# Delete each of the files that we installed as part of the package')
        uninstall.WriteLn('echo "Deleting SCX installed files ..."')
        # Now loop through and delete all of the package files;
        # We do this in reverse order so directories all come last
        #
        # Note: Empty scxUninstall.sh script is created very early during staging
        # This means (due to reversed()) that the scxUninstall.sh script is
        # one of the last files deleted during uninstallation.  This is good
        # so that, if uninstall is interrupted, most of cleanup is done.
        for stagingObject in reversed(self.stagingDir.GetStagingObjectList()):
            if stagingObject.GetFileType() == 'conffile':
                # Only delete configuration files if zero length
                uninstall.WriteLn('[ ! -s \'/' + stagingObject.GetPath() + '\' ] && rm \'/' + stagingObject.GetPath() + '\'')
            elif stagingObject.GetFileType() == 'dir':
                # No warnings or errors - if directory isn't empty, leave it alone
                uninstall.WriteLn('rmdir \'/' + stagingObject.GetPath() + '\' > /dev/null 2>&1')
            elif stagingObject.GetFileType() == 'file' or stagingObject.GetFileType() == 'link':
                uninstall.WriteLn('rm \'/' + stagingObject.GetPath() + '\'')
            elif stagingObject.GetFileType() == 'sysdir':
                # Nothing to do for system directories - they're never deleted
                pass
            else:
                print 'File type ' + stagingObject.GetFileType() + ' for \'/' + stagingObject.GetPath() + '\' is not recognized'
                raise UnknownFileType
        uninstall.WriteLn('')
        uninstall.WriteLn('# Delete the package from the bill of materials')
        if self.platform.MinorVersion == 4:
            uninstall.WriteLn('rm -rf /Library/Receipts/scx-%s-%s.macos.%s.*.*pkg'
                              % (self.product.Version,
                                 self.product.BuildNr,
                                 self.osverstring))
        else:
            uninstall.WriteLn('pkgutil --forget com.microsoft.scx')

        self.product.WritePostRemoveToScript(uninstall)
        uninstall.WriteLn('exit 0')

        uninstallFile = os.path.join(self.stagingDir.GetRootPath(), uninstallPath)

        ExecutableFile(uninstallFile, uninstall)

    def GetPackageBaseFileName(self):
        return \
            'scx-' + \
            self.product.Version + '-' + \
            self.product.BuildNr + \
            '.macos.' + self.osverstring + '.' + \
            self.platform.Architecture

    def GetPackageFileName(self):
        return self.GetPackageBaseFileName() + self.packageHelper.GetPackageExtension()

    def GetPackageFilePath(self):
        return os.path.join(self.targetDir, self.GetPackageFileName());


    ##
    # Actually creates the finished package file.
    #
    def BuildPackage(self):
        os.system('rm -rf ' + self.GetPackageFilePath())

        self.packageHelper.BuildPackage(self.targetDir, self.GetPackageBaseFileName(), self.stagingDir)


##
# Several files (the info file and the description file) needed to generate 
# 10.4 packages are so called property list files.
# They are XML files with a dictionary representation inside.
#
class PropertyListFile:
    
    ##
    # Constructor.
    # \param[in] path Path to where the file should be generated.
    #
    def __init__(self, path):
        self.path = path
        self.xmlHeader = '<?xml version="1.0" encoding="UTF-8"?>\n'
        self.xmlDocTypeString = \
            '<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n'
        self.properties = []

    ##
    # Generates the file.
    #
    def Generate(self):
        file = open(self.path, 'w')
        file.write(self.xmlHeader)
        file.write(self.xmlDocTypeString)
        file.write('<plist version="1.0">\n')
        file.write('  <dict>\n')
        
        for (key, value, type) in self.properties:
            file.write(self.CreateXmlStringProperty(key, value, type))

        file.write('  </dict>\n')
        file.write('</plist>\n')

    ##
    # Adds a string property to the dictionary.
    # \param[in] key Name of property key.
    # \param[in] value Value of property.
    #
    def AddProperty(self, key, value):
        self.properties.append((key, value, 'string'))

    ##
    # Adds an integer property to the dictionary.
    # \param[in] key Name of property key.
    # \param[in] value Value of property.
    #
    def AddIntegerProperty(self, key, value):
        self.properties.append((key, value, 'integer'))

    ##
    # Internal method that creates an xml snippet out of a single property.
    # \param[in] key Name of property key.
    # \param[in] value Value of property.
    # \param[in] type Propety type (string or integer).
    #
    def CreateXmlStringProperty(self, key, value, type):
        return \
            '    <key>' + key + '</key>\n' + \
            '    <' + type + '>' + value + '</' + type + '>\n'


##
# Represents a MacOS 10.4 sub package.
# A sub package is a package that has all files under a certain directory on the target system.
#
class MacOS10_4SubPackage:

    ##
    # Constructor
    # \param[in] rootStagingDir Path to the staging directory of the main (meta-) package.
    # \param[in] subPackageRoot Path to the staging area of the sub package relative to the root staging directory.
    # \param[in] subPackageName Name of the sub package.
    # \param[in] tempDir Directory where temporary files are created.
    # \param[in] targetDir Directory where the sub package will be generated.
    # \param[in] versionString SCX version string excluding build nr. E.g. 1.0.700
    # \param[in] buildNrString SCX build number as string.
    #
    def __init__(self,
                 rootStagingDir,
                 subPackageRoot,
                 subPackageName,
                 tempDir,
                 targetDir,
                 versionString,
                 buildNrString):
        self.stagingDir = os.path.join(rootStagingDir, subPackageRoot)
        self.subPackageRoot = subPackageRoot
        self.subPackageName = subPackageName
        self.tempDir = tempDir
        self.versionString = versionString
        self.buildNrString = buildNrString
        self.packageFileName = os.path.join(targetDir, 'scx-' + versionString + '-' + buildNrString + '.macos.10.4.' + self.subPackageName + '.pkg')
        self.infoFileName = os.path.join(self.tempDir, 'Info-' + self.subPackageName + '.plist')
        self.descFileName = os.path.join(self.tempDir, 'Desc-' + self.subPackageName + '.plist')

    ##
    # Generates the installinfo file needed to build the sub package.
    #
    def GenerateInfoFile(self):
        infoFile = PropertyListFile(self.infoFileName)
        infoFile.AddProperty('CFBundleIdentifier', 'com.microsoft.scx.' + self.subPackageName)
        infoFile.AddProperty('CFBundleVersion', self.versionString + '.' + self.buildNrString)
        infoFile.AddProperty('CFBundleShortVersionString', self.versionString)
        infoFile.Generate()

    ##
    # Generates the description file needed to build the sub package.
    #
    def GenerateDescFile(self):
        descFile = PropertyListFile(self.descFileName)
        descFile.AddProperty('IFPkgDescriptionDescription',
                             'Installs all parts of the Microsoft SCX Agent stack for Apple Mac OS X that reside under /' + self.subPackageRoot + '.')
        descFile.AddProperty('IFPkgDescriptionTitle', 'SCX ' + self.subPackageName)
        descFile.Generate()

    ##
    # After the package has been generated, the info file needs to be changed to
    # set the correct path as the default location.
    #
    def ReplaceInfoFile(self):
        infoFileDirectory = os.path.join(self.packageFileName, 'Contents/Resources/English.lproj')
        infoFileName =  'scx-' + self.versionString + '-' + self.buildNrString + '.macos.10.4.' + self.subPackageName + '.info'
        infoFilePath = os.path.join(infoFileDirectory, infoFileName)
        newcontent = ''
        for line in open(infoFilePath, 'r').readlines():
            if line.find('DefaultLocation') == 0:
                newcontent = newcontent + 'DefaultLocation /' + self.subPackageRoot + '\n'
            else:
                newcontent = newcontent + line

        # This file is generated read only.
        # We need to change permissions before we can write to it.
        os.system('chmod u+w ' + infoFilePath)

        infoFile = open(infoFilePath, 'w')
        infoFile.write(newcontent)

    ##
    # Generates all files and builds the package.
    #
    def Generate(self):
        self.GenerateInfoFile()
        self.GenerateDescFile()
        os.system('rm -rf ' + self.packageFileName)
        os.system('/Developer/Tools/packagemaker -build -p ' + self.packageFileName + \
                      ' -f ' + self.stagingDir + \
                      ' -ds' + \
                      ' -i ' + self.infoFileName + ' -d ' + self.descFileName)

        self.ReplaceInfoFile()


##
# Contains 10.4 speciffic methods.
#
class MacOS10_4PackageHelper:

    ##
    # Initializes a MacOS X 10.4 speciffic package helper.
    # \param[in] stagingDir Path to staging directory root.
    # \param[in] tempDir Temporary directory to put generated files in.
    # \param[in] versionString SCX version string excluding build nr. E.g. 1.0.700
    # \param[in] buildNrString SCX build number as string.
    #
    def __init__(self, stagingDir, tempDir, versionString, buildNrString, scriptDir):
        self.tempDir = tempDir
        self.versionString = versionString
        self.buildNrString = buildNrString
        self.subPackageDir = os.path.join(tempDir, 'subpackges')
	self.infoFileName = os.path.join(self.tempDir, 'Info.plist')
        self.descFileName = os.path.join(self.tempDir, 'Desc.plist')
        self.scriptDir = scriptDir

        self.subPackages = [
            MacOS10_4SubPackage(stagingDir, 'private/etc', 'etc', tempDir, self.subPackageDir, versionString, buildNrString),
            MacOS10_4SubPackage(stagingDir, 'private/var', 'var', tempDir, self.subPackageDir, versionString, buildNrString),
            MacOS10_4SubPackage(stagingDir, 'usr/libexec', 'libexec', tempDir, self.subPackageDir, versionString, buildNrString),
            MacOS10_4SubPackage(stagingDir, 'usr/sbin', 'sbin', tempDir, self.subPackageDir, versionString, buildNrString),
            MacOS10_4SubPackage(stagingDir, 'Library/LaunchDaemons', 'LaunchDaemons', tempDir, self.subPackageDir, versionString, buildNrString)
            ]

    ##
    # Get the extension of the final package file.
    #
    def GetPackageExtension(self):
        return '.dmg'

    ##
    # Generates the files needed by the package maker.
    # \param[in] stagingDir Ignored for 10.4
    #
    def GeneratePackageDescriptionFiles(self, stagingDir):
        infoFile = PropertyListFile(self.infoFileName)
        infoFile.AddProperty('CFBundleIdentifier', 'com.microsoft.scx')
        infoFile.AddProperty('CFBundleShortVersionString', self.versionString)
        infoFile.AddProperty('CFBundleVersion', self.versionString + '.' + self.buildNrString)
        infoFile.AddIntegerProperty('IFPkgFlagInstalledSize', '0')
        infoFile.Generate()

        descFile = PropertyListFile(self.descFileName)
        descFile.AddProperty('IFPkgDescriptionDescription', 'Installs and configures the Microsoft SCX Agent stack for Apple Mac OS X.')
        descFile.AddProperty('IFPkgDescriptionTitle', 'SCX')
        descFile.Generate()

    ##
    # Builds the sub packages and the meta package.
    # \param[in] packageFileName Path to package to build.
    # \param[in] stagingDir Staging directory object.
    #
    def BuildPackage(self, targetDir, packageBaseFileName, stagingDir):

        # Fix up owner/permissions in staging directory
        # (Files are installed on destination as staged)

        for obj in stagingDir.GetStagingObjectList():
            if obj.GetFileType() in ['conffile', 'file', 'dir', 'sysdir']:
                filePath = os.path.join(stagingDir.GetRootPath(), obj.GetPath())

                # Don't change the staging directory itself
                if obj.GetPath() != '':
                    scxutil.ChOwn(filePath, obj.GetOwner(), obj.GetGroup())
                    scxutil.ChMod(filePath, obj.GetPermissions())
            elif obj.GetFileType() == 'link':
                os.system('sudo chown %s:%s %s' \
                              % (obj.GetOwner(), obj.GetGroup(), filePath))
            else:
                print "Unrecognized type (%s) for object %s" % (obj.GetFileType(), obj.GetPath())

        os.system('mkdir ' + self.subPackageDir)
        for subpackage in self.subPackages:
            subpackage.Generate()
            
        metaPackageFilePath = os.path.join(self.tempDir, packageBaseFileName + '.mpkg')
        packageFilePath = os.path.join(targetDir, packageBaseFileName + self.GetPackageExtension())

        os.system('/Developer/Tools/packagemaker -build' + \
                      ' -mi ' + self.subPackageDir + \
                      ' -p ' + metaPackageFilePath + \
                      ' -r ' + self.scriptDir + \
                      ' -i ' + self.infoFileName + \
                      ' -d ' + self.descFileName)

        # Undo the permissions settings so we can delete properly
        os.system('sudo chown -R $USER:admin %s' % stagingDir.GetRootPath())

        # Remove old disk image file.
        os.system('rm -f ' + packageFilePath)

        # Now build a disk image from the package (which is actually a folder at this time.
        os.system('hdiutil create -srcfolder ' + metaPackageFilePath + ' -volname scx ' + packageFilePath)

##
# Contains 10.5 speciffic methods.
#
class MacOS10_5PackageHelper:
    ##
    # Initializes a MacOS X 10.5 speciffic package helper.
    # \param[in] stagingRootDir Path to staging directory root.
    # \param[in] tempDir Temporary directory to put generated files in.
    # \param[in] targetDir Directory where generated package will be put.
    # \param[in] versionString SCX version string excluding build nr. E.g. 1.0.700
    # \param[in] buildNrString SCX build number as string.
    # \param[in] preinstallPath Path to the preinstall script.
    # \param[in] postinstallPath Path to the postinstall script.
    # \param[in] preupgradePath Path to the preupgrade script.
    # \param[in] postupgradePath Path to the postupgrade script.
    #
    def __init__(self,
                 stagingRootDir,
                 tempDir,
                 targetDir,
                 versionString,
                 buildNrString,
                 preinstallPath,
                 postinstallPath,
                 preupgradePath,
                 postupgradePath):

        self.stagingRootDir = stagingRootDir
        self.tempDir = tempDir
        self.targetDir = targetDir
        self.versionString = versionString
        self.buildNrString = buildNrString
        self.preinstallPath = preinstallPath
        self.postinstallPath = postinstallPath
        self.preupgradePath = preupgradePath
        self.postupgradePath = postupgradePath
        self.packageDir = os.path.join(self.targetDir, 'SCX.pmdoc')
        self.graphicalWelcomePath = os.path.join(self.tempDir, "welcome.txt")
        self.graphicalReadmePath = os.path.join(self.tempDir, "readme.txt")
        self.graphicalLicensePath = os.path.join(self.targetDir, 'staging/usr/libexec/microsoft/scx/LICENSE')
        self.graphicalConclusionPath = os.path.join(self.tempDir, "conclusion.txt")

    ##
    # Get the extension of the final package file.
    #
    def GetPackageExtension(self):
        return '.pkg'

    ##
    # Generates all package description files.
    #
    def GeneratePackageDescriptionFiles(self, stagingDir):
        self.GenerateGraphicalFiles()
        self.GeneratePackageMakerFiles(stagingDir)

    ##
    # Generates the GUI installation files.
    #
    # These files are displayed during package installation via the
    # Graphical User Interface (not via command line)
    #
    # The screens, in order of display, are:
    #	Welcome (Introduction)
    #	Read Me (Important Information)
    #	License
    #	Finish Up (Conclusion)
    #	
    def GenerateGraphicalFiles(self):
        welcomeFile = open(self.graphicalWelcomePath, 'w')
        welcomeFile.write('This package will install and configure the Microsoft System Center Cross-Platform (SCX) Agent stack for Apple Mac OS X to enable enterprise management.\n')

        readmeFile = open(self.graphicalReadmePath, 'w')
        readmeFile.write('At the end of this installation, it is recommended that the system be restarted to complete the configuraiton and start the services. Please save your work and close open applications before proceeding with this installation.\n')

        conclusionFile = open(self.graphicalConclusionPath, 'w')
        conclusionFile.write('If you wish to remove the Microsoft System Center Cross-Platform (SCX) Agent Stack from your system, you should execute file: /usr/libexec/microsoft/scx/bin/scxUninstall.sh\n')

    ##
    # Creates the prototype file used for packing.
    #
    def GeneratePackageMakerFiles(self, stagingDir):
        scxutil.RmTree(self.packageDir)
        scxutil.MkDir(self.packageDir)

        self.GenerateIndexXml()

        stagingFileName = os.path.join(self.packageDir, '01staging.xml')
        stagingfile = open(stagingFileName, 'w')
        stagingfile.write('<pkgref spec="1.12" uuid="82C32EDB-3856-4713-9854-7D803878E995">\n')
        stagingfile.write('  <config>\n')
        stagingfile.write('    <identifier>com.microsoft.scx</identifier>\n')
        stagingfile.write('    <version>' + self.versionString + '-' + self.buildNrString + '</version>\n')
        stagingfile.write('    <description></description>\n')
        stagingfile.write('    <post-install type="none"/>\n')
        stagingfile.write('    <requireAuthorization/>\n')
        stagingfile.write('    <installFrom>' + os.path.join(self.targetDir, 'staging') + '</installFrom>\n')
        stagingfile.write('    <installTo>/</installTo>\n')
        stagingfile.write('    <flags>\n')
        stagingfile.write('      <followSymbolicLinks/>\n')
        stagingfile.write('    </flags>\n')
        stagingfile.write('    <packageStore type="internal"></packageStore>\n')
        stagingfile.write('    <mod>parent</mod>\n')
        stagingfile.write('    <mod>scripts.postinstall.path</mod>\n')
        stagingfile.write('    <mod>scripts.preinstall.isAbsoluteType</mod>\n')
        stagingfile.write('    <mod>scripts.postinstall.isAbsoluteType</mod>\n')
        stagingfile.write('    <mod>scripts.preinstall.path</mod>\n')
        stagingfile.write('    <mod>version</mod>\n')
        stagingfile.write('    <mod>scripts.scriptsDirectoryPath.path</mod>\n')
        stagingfile.write('    <mod>identifier</mod>\n')
        stagingfile.write('    <mod>installTo</mod>\n')
        stagingfile.write('  </config>\n')
        stagingfile.write('  <scripts>\n')
        stagingfile.write('    <preinstall mod="true">' + self.preinstallPath + '</preinstall>\n')
        stagingfile.write('    <postinstall mod="true">' + self.postinstallPath + '</postinstall>\n')
        stagingfile.write('  </scripts>\n')
        stagingfile.write('  <contents>\n')
        stagingfile.write('    <file-list>01staging-contents.xml</file-list>\n')
        stagingfile.write('    <filter>/CVS$</filter>\n')
        stagingfile.write('    <filter>/\\.svn$</filter>\n')
        stagingfile.write('    <filter>/\\.cvsignore$</filter>\n')
        stagingfile.write('    <filter>/\\.cvspass$</filter>\n')
        stagingfile.write('    <filter>/\\.DS_Store$</filter>\n')
        stagingfile.write('  </contents>\n')
        stagingfile.write('</pkgref>\n')

        contentsFileName = os.path.join(self.packageDir, '01staging-contents.xml')
        contentsfile = open(contentsFileName, 'w')
        contentsfile.write('<pkg-contents spec="1.12">\n')
        contentsfile.write('<f n="staging" o="root" g="wheel" p="16877" pt="' + self.stagingRootDir + '" m="true" t="file">\n')
        root = ListToTree(stagingDir.GetStagingObjectList())
        root.AsXML(contentsfile, '')
        contentsfile.write('</f>\n')
        contentsfile.write('</pkg-contents>\n')

    ##
    # Generate the index.xml file
    #
    def GenerateIndexXml(self):

        indexFileName = os.path.join(self.packageDir, 'index.xml')
        indexfile = open(indexFileName, 'w')
        indexfile.write('<pkmkdoc spec="1.12">\n')
        indexfile.write('  <properties>\n')
        indexfile.write('    <title>System Center Cross-Platform Agent</title>\n')
        indexfile.write('    <build>' + os.path.join(self.targetDir, 'scx' + self.GetPackageExtension()) + '</build>\n')
        indexfile.write('    <organization>com.microsoft</organization>\n')
        indexfile.write('    <userSees ui="easy"/>\n')
        indexfile.write('    <min-target os="3"/>\n')
        indexfile.write('    <domain system="true"/>\n')
        indexfile.write('  </properties>\n')
        indexfile.write('  <distribution>\n')
        indexfile.write('    <versions min-spec="1.000000"/>\n')
        indexfile.write('    <scripts></scripts>\n')
        indexfile.write('  </distribution>\n')
        indexfile.write('  <description>Installs and configures the Microsoft SCX Agent stack for Apple Mac OS X.</description>\n')
        indexfile.write('  <contents>\n')
        indexfile.write('    <choice title="SCX Installation Files" id="scx" tooltip="Installs SCX Management Files" ' + \
                            'description="Installs and configures the SCX Management Files, including SCX Core Code and OpenPegasus." ' + \
                            'starts_selected="true" starts_enabled="true" starts_hidden="false">\n')
        indexfile.write('      <customLoc>/</customLoc>\n')
        indexfile.write('      <pkgref id="com.microsoft.scx"/>\n')
        indexfile.write('    </choice>\n')
        indexfile.write('  </contents>\n')
        indexfile.write('  <resources bg-scale="none" bg-align="topleft">\n')
        indexfile.write('    <locale lang="en">\n')
        indexfile.write('      <resource mod="true" type="license">' + self.graphicalLicensePath + '</resource>\n')
        indexfile.write('      <resource mod="true" type="readme">' + self.graphicalReadmePath + '</resource>\n')
        indexfile.write('      <resource mod="true" type="welcome">' + self.graphicalWelcomePath + '</resource>\n')
        indexfile.write('      <resource mod="true" type="conclusion">' + self.graphicalConclusionPath + '</resource>\n')
        indexfile.write('    </locale>\n')
        indexfile.write('  </resources>\n')
        indexfile.write('  <requirements>\n')
        indexfile.write('    <requirement id="tosv" operator="ge" value="\'10.5.0\'">\n')
        indexfile.write('      <message>Microsoft SCX Agents are designed for Mac OS X 10.5 and newer.</message>\n')
        indexfile.write('    </requirement>\n')
        indexfile.write('  </requirements>\n')
        indexfile.write('  <flags/>\n')
        indexfile.write('  <item type="file">01staging.xml</item>\n')
        indexfile.write('  <mod>properties.title</mod>\n')
        indexfile.write('  <mod>description</mod>\n')
        indexfile.write('  <mod>properties.anywhereDomain</mod>\n')
        indexfile.write('  <mod>properties.systemDomain</mod>\n')
        indexfile.write('</pkmkdoc>\n')


    ##
    # Returns the command line to use for building packages.
    #
    def GetCommandLine(self, depotfilename):
        return '/Developer/usr/bin/packagemaker --doc ' + self.packageDir + ' --out ' + depotfilename + \
            ' --target 10.5 --id com.microsoft.scx'

    ##
    # Builds the actual package.
    # \param[in] packageFileName Path to package to build.
    # \param[in] stagingDir Ignored for 10.5.
    #
    def BuildPackage(self, targetDir, packageBaseFileName, stagingDir):
        packageFileName = os.path.join(targetDir, packageBaseFileName + self.GetPackageExtension())
        os.system(self.GetCommandLine(packageFileName))

