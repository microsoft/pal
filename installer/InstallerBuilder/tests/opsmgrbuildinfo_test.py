import unittest
import sys
sys.path.append('../')
from opsmgrbuildinfo import OpsMgrBuildInformation
from platforminformation import PlatformInformation

class OpsMgrBuildInfoTestCase(unittest.TestCase):
    def setUp(self):
        self.platform = PlatformInformation()
        self.platform.OS = 'Linux'
        self.platform.Distribution = 'SUSE'
        self.platform.Architecture = 'x86'
        self.platform.MajorVersion = 10
        self.platform.MinorVersion = 2
        self.platform.MinorVersionString = '2'
        self.platform.Width = 32
        
    def testGetTempDirReturnsCorrectPath(self):
        buildInfo = OpsMgrBuildInformation('basedir', self.platform, 'Debug', False)
        self.assertEqual('basedir/installer/intermediate', buildInfo.GetTempDir())

    def testGetTargetDirReturnsCorrectPath(self):
        buildInfo = OpsMgrBuildInformation('basedir', self.platform, 'Debug', False)
        self.assertEqual('basedir/target/Linux_SUSE_10.2_x86_32_Debug', buildInfo.GetTargetDir())

