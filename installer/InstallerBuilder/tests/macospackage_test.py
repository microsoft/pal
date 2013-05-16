import unittest
import sys
sys.path.append('../')
import macospackage
import scriptgenerator
from scxexceptions import PlatformNotImplementedError
import testutils
from platforminformation import PlatformInformation
from opsmgr import OpsMgr
import executablefile
import filegenerator

##
# A MacOSPackageFile with a faked os.system.
#
class TestableMacOSPackageFile(macospackage.MacOSPackageFile):
    def __init__(self, stagingDir, product, platform):
        macospackage.MacOSPackageFile.__init__(self, stagingDir, product, platform)

##
# Test cases to the macospackage package.
#
class MacOSPackageFileTestCase(unittest.TestCase):

    def setUp(self):
        self.TheFakeOS = testutils.FakeOS()
        macospackage.open = testutils.FakeOpen
        executablefile.open = testutils.FakeOpen
        scriptgenerator.open = testutils.FakeOpen
        macospackage.os.system = self.TheFakeOS.system
        thefakefiles = {}
        self.product = testutils.TestProduct()

    def GivenTestableMacOSPackageFile(self, pfmajor, pfminor_str):
        platform = PlatformInformation()
        platform.OS = 'MacOS'
        platform.MajorVersion = pfmajor
        platform.MinorVersion = int(pfminor_str)
        platform.MinorVersionString = pfminor_str
        platform.Architecture = '<pfarch>'

        stagingDir = testutils.FakeStagingDir()
        stagingDir.Add(filegenerator.SoftLink('usr/sbin/scxadmin', '', 755, 'root', 'root'))
        stagingDir.Add(filegenerator.FileCopy('opt/microsoft/scx/bin/tools/scx-cimd', '', 555, 'root', 'bin'))
        stagingDir.Add(filegenerator.FileCopy('etc/opt/microsoft/scx/conf/scxlog.conf', '', 555, 'root', 'bin', 'conffile'))

        return TestableMacOSPackageFile(stagingDir, self.product, platform)
        
    def ExpectedInfoContent(self, subpackage, versionstring, shortversion):
        return \
            '<?xml version="1.0" encoding="UTF-8"?>\n' + \
            '<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n' + \
            '<plist version="1.0">\n' + \
            '  <dict>\n' + \
            '    <key>CFBundleIdentifier</key>\n' + \
            '    <string>com.microsoft.scx.' + subpackage + '</string>\n' + \
            '    <key>CFBundleVersion</key>\n' + \
            '    <string>' + versionstring + '</string>\n' + \
            '    <key>CFBundleShortVersionString</key>\n' + \
            '    <string>' + shortversion + '</string>\n' + \
            '  </dict>\n' + \
            '</plist>\n'

    def ExpectedDescContent(self, subpackage, subpackageroot):
        return \
            '<?xml version="1.0" encoding="UTF-8"?>\n' + \
            '<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n' + \
            '<plist version="1.0">\n' + \
            '  <dict>\n' + \
            '    <key>IFPkgDescriptionDescription</key>\n' + \
            '    <string>Installs all parts of the Microsoft SCX Agent stack for Apple Mac OS X that reside under /' + subpackageroot + '.</string>\n' + \
            '    <key>IFPkgDescriptionTitle</key>\n' + \
            '    <string>SCX ' + subpackage + '</string>\n' + \
            '  </dict>\n' + \
            '</plist>\n'

    def test10_4PackageFileNameFormat(self):
        package = self.GivenTestableMacOSPackageFile(10, '4')
        self.assertEqual('target/scx-1.2.3-4.macos.10.4.<pfarch>.dmg',
                         package.GetPackageFilePath())

    def test10_5PackageFileNameFormat(self):
        package = self.GivenTestableMacOSPackageFile(10, '5')
        self.assertEqual('target/scx-1.2.3-4.macos.10.5.<pfarch>.pkg',
                         package.GetPackageFilePath())

    def testRequire10_5OSVersion(self):
        packagehelper = macospackage.MacOS10_5PackageHelper('', '', 'targetdir', '', '', '', '', '', '')
        packagehelper.GenerateIndexXml()
        content = testutils.GetFileContent('targetdir/SCX.pmdoc/index.xml')
        self.assert_(content.find('<requirement id="tosv" operator="ge" value="\'10.5.0\'">') <> -1)
        self.assert_(content.find('<message>Microsoft SCX Agents are designed for Mac OS X 10.5 and newer.</message>\n') <> -1)

    def test10_5BuildsWithTarget10_5(self):
        package = self.GivenTestableMacOSPackageFile(10, '5')
        package.BuildPackage()
        self.assert_(self.TheFakeOS.systemCalls[1].find('--target 10.5') <> -1)

    def testPackageMakerCommandRemovesOldPackage(self):
        package = self.GivenTestableMacOSPackageFile(10, '5')
        package.BuildPackage()
        self.assertEqual('rm -rf ' + package.GetPackageFilePath(), self.TheFakeOS.systemCalls[0])
        
    def testPackageMakerPathOn10_5IsInUsrBin(self):
        packagehelper = macospackage.MacOS10_5PackageHelper('', '', '', '', '', '', '', '', '')
        self.assertEqual(0, packagehelper.GetCommandLine('').find('/Developer/usr/bin/packagemaker'))

    def test11_5PackageThrows(self):
        try:
            self.GivenTestableMacOSPackageFile(11, '5')
            self.fail('MacOS X 11.5 should not be implemented at this stage.')
        except PlatformNotImplementedError:
            pass
        
    def test10_3PackageThrows(self):
        try:
            self.GivenTestableMacOSPackageFile(10, '3')
            self.fail('MacOS X 10.3 should not be implemented at this stage.')
        except PlatformNotImplementedError:
            pass
        
    def testBuild10_4PackageGeneratesEtcInfoFileInTempdir(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        self.assert_(testutils.FileHasBeenWritten('tempdir/Info-etc.plist'))

    def testBuild10_4PackageGeneratesEtcInfoFileInCorrectDir(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'otherdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        self.assert_(testutils.FileHasBeenWritten('otherdir/Info-etc.plist'))
        
    def testBuild10_4PackageEtcInfoFileContent(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '1.0.4', '704', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedInfoContent('etc', '1.0.4.704', '1.0.4')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Info-etc.plist'))
        
    def testBuild10_4PackageEtcInfoFilContentHasCorrectVersionData(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '10.01.40', '4711', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedInfoContent('etc', '10.01.40.4711', '10.01.40')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Info-etc.plist'))

    def testBuild10_4PackageGeneratesVarInfoFileInTempdir(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        self.assert_(testutils.FileHasBeenWritten('tempdir/Info-var.plist'))

    def testBuild10_4PackageGeneratesVarInfoFileInCorrectDir(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'otherdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        self.assert_(testutils.FileHasBeenWritten('otherdir/Info-var.plist'))

    def testBuild10_4PackageVarInfoFileContent(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '1.0.4', '704', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedInfoContent('var', '1.0.4.704', '1.0.4')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Info-var.plist'))
        
    def testBuild10_4PackageVarInfoFilContentHasCorrectVersionData(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '10.01.40', '4711', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedInfoContent('var', '10.01.40.4711', '10.01.40')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Info-var.plist'))

    def testBuild10_4PackageGeneratesCorrectLibexecInfoFile(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '1.0.4', '704', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedInfoContent('libexec', '1.0.4.704', '1.0.4')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Info-libexec.plist'))
        
    def testBuild10_4PackageGeneratesEtcDescFileInTempdir(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        self.assert_(testutils.FileHasBeenWritten('tempdir/Desc-etc.plist'))

    def testBuild10_4PackageGeneratesEtcDescFileInCorrectDir(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'otherdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        self.assert_(testutils.FileHasBeenWritten('otherdir/Desc-etc.plist'))

    def testBuild10_4PackageEtcDescFileContent(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedDescContent('etc', 'private/etc')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Desc-etc.plist'))

    def testBuild10_4PackageVarDescFileContent(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedDescContent('var', 'private/var')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Desc-var.plist'))

    def testBuild10_4PackageLibexecDescFileContent(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '', '', '')
        packagehelper.BuildPackage('', '', testutils.FakeStagingDir())
        expected = self.ExpectedDescContent('libexec', 'usr/libexec')
        self.assertEqual(expected, testutils.GetFileContent('tempdir/Desc-libexec.plist'))

    def test10_4SubPackageCommandLine(self):
        subpackage = macospackage.MacOS10_4SubPackage('stagingdir', 'private/etc', 'etc', 'tempdir', 'targetdir', '1.0.4', '704')
        targetpath = 'targetdir/scx-1.0.4-704.macos.10.4.etc.pkg'
        subpackage.Generate()
        expected = \
            '/Developer/Tools/packagemaker -build -p ' + targetpath + \
            ' -f stagingdir/private/etc' + \
            ' -ds' + \
            ' -i tempdir/Info-etc.plist -d tempdir/Desc-etc.plist'

        self.assertEqual('rm -rf ' + targetpath, self.TheFakeOS.systemCalls[0])
        self.assertEqual(expected, self.TheFakeOS.systemCalls[1])

    def test10_4SubPackageCommandLineHonorsStagindDirParam(self):
        subpackage = macospackage.MacOS10_4SubPackage('otherStagingDir', '', '', '', '', '', '')
        subpackage.Generate()
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('-f otherStagingDir/'))
        self.assertEqual(-1, self.TheFakeOS.systemCalls[1].find('-f stagingdir/'))

    def test10_4SubPackageCommandLineHonorsSubPackageNameParam(self):
        subpackage = macospackage.MacOS10_4SubPackage('stagingdir', 'private/var', 'var', '', '', '1.0.4', '704')
        subpackage.Generate()

        self.assert_(-1 != self.TheFakeOS.systemCalls[0].find('macos.10.4.var.pkg'))
        self.assertEqual(-1, self.TheFakeOS.systemCalls[0].find('macos.10.4.etc.pkg'))
        
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('macos.10.4.var.pkg'))
        self.assertEqual(-1, self.TheFakeOS.systemCalls[1].find('macos.10.4.etc.pkg'))
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('-f stagingdir/private/var'))
        self.assertEqual(-1, self.TheFakeOS.systemCalls[1].find('-f stagingdir/private/etc'))
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('Info-var.plist'))
        self.assertEqual(-1, self.TheFakeOS.systemCalls[1].find('Info-etc.plist'))
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('Desc-var.plist'))
        self.assertEqual(-1, self.TheFakeOS.systemCalls[1].find('Desc-etc.plist'))


    def test10_4SubPackageCommandLineHonorsTempDirParam(self):
        subpackage = macospackage.MacOS10_4SubPackage('', '', '', 'OtherTempDir', '', '', '')
        subpackage.Generate()

        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('-i OtherTempDir/'))
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('-d OtherTempDir/'))

    def test10_4SubPackageCommandLineHonorsVersionStringParam(self):
        subpackage = macospackage.MacOS10_4SubPackage('', '', '', '', '', '10.04.40', '')
        subpackage.Generate()

        self.assert_(-1 != self.TheFakeOS.systemCalls[0].find('scx-10.04.40'))
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('scx-10.04.40'))

    def test10_4SubPackageCommandLineHonorsBuildNrStringParam(self):
        subpackage = macospackage.MacOS10_4SubPackage('', '', '', '', '', '', '4711')
        subpackage.Generate()

        self.assert_(-1 != self.TheFakeOS.systemCalls[0].find('-4711'))
        self.assert_(-1 != self.TheFakeOS.systemCalls[1].find('-4711'))

    def test10_4SubPackageReplacesDefaultLocationInInfoFile(self):
        fakefile = testutils.FakeFile()
        fakefile.content = \
            'Some funky\n' + \
            'text\n' + \
            'DefaultLocation something\n' + \
            'Some other text\n'
        filepath = 'targetdir/scx-1.0.4-704.macos.10.4.etc.pkg/Contents/Resources/English.lproj/scx-1.0.4-704.macos.10.4.etc.info'
        testutils.thefakefiles[filepath] = fakefile

        expected = fakefile.content.replace('something', '/private/etc')

        subpackage = macospackage.MacOS10_4SubPackage('', 'private/etc', 'etc', '', 'targetdir', '1.0.4', '704')
        subpackage.Generate()
            
        self.assertEqual(expected, testutils.GetFileContent(filepath))

    def test10_4SubPackageDoesntTouchOtherLinesInInfoFile(self):
        fakefile = testutils.FakeFile()
        fakefile.content = \
            'Some other funky\n' + \
            'text lines\n' + \
            'DefaultLocation something\n' + \
            'Another text line\n'
        filepath = 'targetdir/scx-1.0.4-704.macos.10.4.etc.pkg/Contents/Resources/English.lproj/scx-1.0.4-704.macos.10.4.etc.info'
        testutils.thefakefiles[filepath] = fakefile

        expected = fakefile.content.replace('something', '/private/etc')

        subpackage = macospackage.MacOS10_4SubPackage('', 'private/etc', 'etc', '', 'targetdir', '1.0.4', '704')
        subpackage.Generate()

        self.assertEqual(expected, testutils.GetFileContent(filepath))

    def test10_4SubPackageHonorsSubPackageName(self):
        fakefile = testutils.FakeFile()
        fakefile.content = 'DefaultLocation something\n'
        filepath = 'targetdir/scx-1.0.4-704.macos.10.4.var.pkg/Contents/Resources/English.lproj/scx-1.0.4-704.macos.10.4.var.info'
        testutils.thefakefiles[filepath] = fakefile

        expected = fakefile.content.replace('something', '/private/var')

        subpackage = macospackage.MacOS10_4SubPackage('', 'private/var', 'var', '', 'targetdir', '1.0.4', '704')
        subpackage.Generate()

        self.assertEqual(expected, testutils.GetFileContent(filepath))

    def test10_4PackageWritesInfoFile(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '1.0.4', '4711', '')
        packagehelper.GeneratePackageDescriptionFiles(testutils.FakeStagingDir())

        expected = \
            '<?xml version="1.0" encoding="UTF-8"?>\n' + \
            '<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n' + \
            '<plist version="1.0">\n' + \
            '  <dict>\n' + \
            '    <key>CFBundleIdentifier</key>\n' + \
            '    <string>com.microsoft.scx</string>\n' + \
            '    <key>CFBundleShortVersionString</key>\n' + \
            '    <string>1.0.4</string>\n' + \
            '    <key>CFBundleVersion</key>\n' + \
            '    <string>1.0.4.4711</string>\n' + \
            '    <key>IFPkgFlagInstalledSize</key>\n' + \
            '    <integer>0</integer>\n' + \
            '  </dict>\n' + \
            '</plist>\n'

        self.assertEqual(expected, testutils.GetFileContent('tempdir/Info.plist'))
        
    def test10_4PackageInfoFileHonorsTempDirParam(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'OtherTempDir', '', '', '')
        packagehelper.GeneratePackageDescriptionFiles(testutils.FakeStagingDir())
        self.assert_(testutils.FileHasBeenWritten('OtherTempDir/Info.plist'))

    def test10_4PackageInfoFileHonorsVersionStringParam(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '10.04.40', '4711', '')
        packagehelper.GeneratePackageDescriptionFiles(testutils.FakeStagingDir())

        self.assert_(-1 != testutils.GetFileContent('tempdir/Info.plist').find('<string>10.04.40</string>'))
        self.assertEqual(-1, testutils.GetFileContent('tempdir/Info.plist').find('<string>1.0.4</string>'))

    def test10_4PackageInfoFileHonorsBuildNrStringParam(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '1.0.4', '111', '')
        packagehelper.GeneratePackageDescriptionFiles(testutils.FakeStagingDir())

        self.assert_(-1 != testutils.GetFileContent('tempdir/Info.plist').find('<string>1.0.4.111</string>'))
        self.assertEqual(-1, testutils.GetFileContent('tempdir/Info.plist').find('<string>1.0.4.4711</string>'))

    def test10_4PackageWritesDescFile(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', 'tempdir', '1.0.4', '4711', '')
        packagehelper.GeneratePackageDescriptionFiles(testutils.FakeStagingDir())

        expected = \
            '<?xml version="1.0" encoding="UTF-8"?>\n' + \
            '<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n' + \
            '<plist version="1.0">\n' + \
            '  <dict>\n' + \
            '    <key>IFPkgDescriptionDescription</key>\n' + \
            '    <string>Installs and configures the Microsoft SCX Agent stack for Apple Mac OS X.</string>\n' + \
            '    <key>IFPkgDescriptionTitle</key>\n' + \
            '    <string>SCX</string>\n' + \
            '  </dict>\n' + \
            '</plist>\n'

        self.assertEqual(expected, testutils.GetFileContent('tempdir/Desc.plist'))
        
    def test10_4PackageGeneratesDmgFileAtEnd(self):
        packagehelper = macospackage.MacOS10_4PackageHelper('', '', '', '', '')
        packagehelper.BuildPackage('someTargetDir', 'filename', testutils.FakeStagingDir())

        expected = 'hdiutil create -srcfolder filename.mpkg -volname scx someTargetDir/filename.dmg'
        self.assertEqual('rm -f someTargetDir/filename.dmg', self.TheFakeOS.systemCalls[-2])
        self.assertEqual(expected, self.TheFakeOS.systemCalls[-1])

    def testPreInstallScriptIsInTempDirScripts(self):
        package = self.GivenTestableMacOSPackageFile(10, '4')
        package.GenerateScripts()
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/scripts/preinstall'))

    def testPreInstallScriptBacksUpConfigFiles(self):
        package = self.GivenTestableMacOSPackageFile(10, '4')
        package.GenerateScripts()
        self.assertEqual(
            '#!/bin/sh\n\nbackup_configuration_file() {\n  mv "$1" "$1.swsave" > /dev/null 2>&1\n}\n\n\n#\n# Main section\n#\nif [ -f /usr/libexec/microsoft/scx/bin/scxUninstall.sh ]; then\n  /usr/libexec/microsoft/scx/bin/scxUninstall.sh upgrade\nfi\nPre Install\nbackup_configuration_file /etc/opt/microsoft/scx/conf/scxlog.conf\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/scripts/preinstall'))

    def testPostInstallScriptIsInTempDirScripts(self):
        package = self.GivenTestableMacOSPackageFile(10, '4')
        package.GenerateScripts()
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/scripts/postinstall'))

    def testGenerateScriptsCreatesTempDirScriptsDirectory(self):
        package = self.GivenTestableMacOSPackageFile(10, '4')
        package.GenerateScripts()
        self.assertEqual('mkdir ' + self.product.buildInfo.TempDir + '/scripts > /dev/null 2>&1', self.TheFakeOS.systemCalls[0])
