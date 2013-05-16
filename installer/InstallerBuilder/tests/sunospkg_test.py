import unittest
import sys
sys.path.append('../')
import sunospkg
import scriptgenerator
import filegenerator
import testutils
import re
from platforminformation import PlatformInformation
from opsmgr import OpsMgr
import executablefile

class TestablePackageMaker(sunospkg.SunOSPKGFile):
    def __init__(self, stagingDir, product, platform):
        executablefile.open = testutils.FakeOpen
        sunospkg.SunOSPKGFile.__init__(self, stagingDir, product, platform, 'Debug')
        sunospkg.open = testutils.FakeOpen

    def GetPkgInfoSetting(self, key):
        pattern = re.compile(key + '=(.*)')
        return pattern.search(testutils.GetFileContent(self.product.buildInfo.TempDir + '/pkginfo')).group(1)
        
    def GetProtoTypeLineMatching(self, regex):
        pattern = re.compile(regex)
        return pattern.search(testutils.GetFileContent(self.product.buildInfo.TempDir + '/prototype')).group(0)

##
# Test cases for the sunos package.
#
class SunOSPKGFileTestCase(unittest.TestCase):
    def setUp(self):
        testutils.thefakefiles = {}
        self.product = testutils.TestProduct()

    def GivenAPlatform(self, pfmajor, pfminor):
        platform = PlatformInformation()
        platform.OS = 'SunOS'
        platform.MajorVersion = pfmajor
        platform.MinorVersion = pfminor
        return platform

    def GivenAPackageMaker(self, pfmajor, pfminor):
        platform = self.GivenAPlatform(pfmajor, pfminor)
        stagingDir = testutils.FakeStagingDir()
        stagingDir.Add(filegenerator.SoftLink('usr/sbin/scxadmin', '', 755, 'root', 'root'))
        stagingDir.Add(filegenerator.FileCopy('opt/microsoft/scx/bin/tools/scx-cimd', '', 555, 'root', 'bin'))
        stagingDir.Add(filegenerator.FileCopy('etc/opt/microsoft/scx/conf/scxlog.conf', '', 555, 'root', 'bin', 'conffile'))
        return TestablePackageMaker(stagingDir, self.product, platform)

    def testZonesFlagsSetToInstallInCurrentZoneOnly(self):
        package = self.GivenAPackageMaker(5, 10)
        package.GeneratePackageDescriptionFiles()
        self.assertEqual('false', package.GetPkgInfoSetting('SUNW_PKG_ALLZONES'))
        self.assertEqual('false', package.GetPkgInfoSetting('SUNW_PKG_HOLLOW'))
        self.assertEqual('true', package.GetPkgInfoSetting('SUNW_PKG_THISZONE'))
        
    def testServiceScriptFileExists(self):
        package = self.GivenAPackageMaker(5, 10)
        package.GeneratePackageDescriptionFiles()
        self.assertEqual('f none /opt/microsoft/scx/bin/tools/scx-cimd',
                         package.GetProtoTypeLineMatching('.*/opt/microsoft/scx/bin/tools/scx-cimd'))

    def testPrototypeFileIsCorrect(self):
        package = self.GivenAPackageMaker(5, 10)
        package.GeneratePackageDescriptionFiles()
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/prototype'))
        self.assertEqual(
            'i pkginfo=' + self.product.buildInfo.TempDir + '/pkginfo\ni depend=' + self.product.buildInfo.TempDir + '/depend\ni preinstall=' + self.product.buildInfo.TempDir + '/preinstall.sh\ni postinstall=' + self.product.buildInfo.TempDir + '/postinstall.sh\ni preremove=' + self.product.buildInfo.TempDir + '/preuninstall.sh\ni postremove=' + self.product.buildInfo.TempDir + '/postuninstall.sh\ni i.config=' + self.product.buildInfo.TempDir + '/i.config\ni r.config=' + self.product.buildInfo.TempDir + '/r.config\ns none /usr/sbin/scxadmin=\nf none /opt/microsoft/scx/bin/tools/scx-cimd 555 root bin\nf config /etc/opt/microsoft/scx/conf/scxlog.conf 555 root bin\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/prototype'))

    def testPKGInfoFileDefinesAllClasses(self):
        package = self.GivenAPackageMaker(5, 10)
        package.GeneratePackageDescriptionFiles()
        self.assertEqual('application config none', package.GetPkgInfoSetting('CLASSES'))


class GivenSunOSGeneratedScripts(unittest.TestCase):
    def setUp(self):
        testutils.thefakefiles = {}
        self.product = testutils.TestProduct()

        self.platform = PlatformInformation()
        self.platform.OS = 'SunOS'
        self.platform.MajorVersion = 5
        self.platform.MinorVersion = 10

        stagingDir = testutils.FakeStagingDir()
        stagingDir.Add(filegenerator.SoftLink('usr/sbin/scxadmin', '', 755, 'root', 'root'))
        stagingDir.Add(filegenerator.FileCopy('opt/microsoft/scx/bin/tools/scx-cimd', '', 555, 'root', 'bin'))
        stagingDir.Add(filegenerator.FileCopy('etc/opt/microsoft/scx/conf/scxlog.conf', '', 555, 'root', 'bin', 'conffile'))

        installer = TestablePackageMaker(stagingDir, self.product, self.platform)

        installer.GenerateScripts()

    def testPreInstallFileIsCorrect(self):
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/preinstall.sh'))
        self.assertEqual(
            '#!/usr/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.preInstallContent + '\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/preinstall.sh'))

    def testPostInstallFileIsCorrect(self):
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/postinstall.sh'))
        self.assertEqual(
            '#!/usr/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/postinstall.sh'))

    def testPreRemoveFileIsCorrect(self):
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/preuninstall.sh'))
        self.assertEqual(
            '#!/usr/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.preRemoveContent + '\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/preuninstall.sh'))

    def testPostRemoveFileIsCorrect(self):
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/postuninstall.sh'))
        self.assertEqual(
            '#!/usr/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/postuninstall.sh'))

    def testIConfigScriptIsCorrect(self):
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/i.config'))
        self.assertEqual(
            '#!/usr/bin/sh\n\ninstall_configuration_file() {\n  # Skip /dev/null\n  [ "$1" = /dev/null ] && return 0\n  \n  # savepath is where original configuration files are stored\n  # for later comparison\n  savepath=$PKGSAV/config$2\n  # Find the directory part of the savepath\n  dirname=`dirname $savepath` || return 1\n  \n  # If directory does not exist, create it.\n  if [ ! -d $dirname ]; then\n    mkdir -p $dirname\n  fi\n  # If there is an old saved version of the configuration file then use it\n  if [ -f "$2.pkgsave" ]; then\n    mv "$2.pkgsave" "$2" || return 1\n    cp $1 $savepath || return 1\n  else\n    echo $2\n    cp $1 $2 || return 1\n    cp $1 $savepath || return 1\n  fi\n  return 0\n}\n\n\n#\n# Main section\n#\nerror=no\nwhile read src dest; do\ninstall_configuration_file $src $dest\nif [ $? -ne 0 ]; then\n  error=yes\nfi\ndone\n[ "$error" = yes ] && exit 2\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/i.config'))
        
    def testRConfigScriptIsCorrect(self):
        self.assert_(testutils.FileHasBeenWritten(self.product.buildInfo.TempDir + '/r.config'))
        self.assertEqual(
            '#!/usr/bin/sh\n\nuninstall_configuration_file() {\n  # savepath is where original configuration files are stored\n  # for comparison\n  savepath=$PKGSAV/config$1\n  \n  /usr/bin/diff $1 $savepath > /dev/null\n  if [ $? -ne 0 ]; then\n    # The configuration file has changed since install time.\n    mv $1 $1.pkgsave\n  else\n    echo $1\n    rm -f $1 || return 1\n  fi\n  rm -f $savepath\n  return 0\n}\n\n\n#\n# Main section\n#\nerror=no\nwhile read dest; do\nuninstall_configuration_file $dest\nif [ $? -ne 0 ]; then\n  error=yes\nfi\ndone\n[ "$error" = yes ] && exit 2\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/r.config'))
