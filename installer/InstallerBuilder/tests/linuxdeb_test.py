import unittest
import sys
sys.path.append('../')
import linuxdeb
import scriptgenerator
import filegenerator
import testutils
from platforminformation import PlatformInformation
from opsmgr import OpsMgr
import executablefile
import scxutil

class TestablePackageMaker(linuxdeb.LinuxDebFile):
    def __init__(self, product, platform):
        stagingDir = testutils.FakeStagingDir()
        stagingDir.Add(filegenerator.SoftLink('usr/sbin/scxadmin', '', 755, 'root', 'root'))
        stagingDir.Add(filegenerator.FileCopy('opt/microsoft/scx/bin/tools/scx-cimd', '', 555, 'root', 'bin'))
        linuxdeb.LinuxDebFile.__init__(self, stagingDir, product, platform)
        linuxdeb.open = testutils.FakeOpen
        scxutil.os = testutils.FakeOS()
        executablefile.open = testutils.FakeOpen

    def __del__(self):
        testutils.thefakefiles = {}

##
# Test cases for the linux deb package.
#
class LinuxDebFileTestCase(unittest.TestCase):
    def setUp(self):
        self.product = testutils.TestProduct()
        
    def GivenAPlatform(self, pfmajor, pfminor):
        platform = PlatformInformation()
        platform.OS = 'Linux'
        platform.Distribution = 'UBUNTU'
        platform.MajorVersion = pfmajor
        platform.MinorVersion = pfminor
        return platform

    def AssertScriptContent(self, expectedContent, scriptFile, pfmajor, pfminor):
        platform = self.GivenAPlatform(pfmajor, pfminor)
        package = TestablePackageMaker(self.product, platform)
        package.GenerateScripts()
        self.assert_(testutils.FileHasBeenWritten(scriptFile))
        self.assertEqual(expectedContent, testutils.GetFileContent(scriptFile))

    def testPreInstallScriptIsCorrect(self):
        self.AssertScriptContent(
            '#!/bin/sh\n\n\n#\n# Main section\n#\nif [ "$1" -eq "upgrade" ]; then\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\nexit 0\n',
            'DEBIAN/preinst', 8, 4)

    def testPostInstallScriptIsCorrect(self):
        self.AssertScriptContent(
            '#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n',
            'DEBIAN/postinst', 8, 4)

    def testPreUninstallScriptIsCorrect(self):
        self.AssertScriptContent(
            '#!/bin/sh\n\n\n#\n# Main section\n#\nif [ "$1" -eq "upgrade" ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preRemoveContent + '\n:\nfi\nexit 0\n',
            'DEBIAN/prerm', 8, 4)

    def testPostUninstallScriptIsCorrect(self):
        self.AssertScriptContent(
            '#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            'DEBIAN/postrm', 8, 4)
