import unittest
import sys
sys.path.append('../')
import hpuxpackage
from product import Product
from platforminformation import PlatformInformation
from cStringIO import StringIO
import testutils
import filegenerator
from testutils import TestProduct

class HPUXPackageFileTestCase(unittest.TestCase):
    def setUp(self):
        self.product = TestProduct()

        self.platform = PlatformInformation()
        self.platform.OS = 'HPUX'
        self.platform.Architecture = 'ia64'
        self.platform.MajorVersion = 11
        self.platform.MinorVersion = 30

    ##
    # The following tests were written to cover existing behavior.
    # They are not really descriptive but were the quickest way to create a
    # firm harness so that refactoring could be safely done on the source.
    #
    
    def test1130_SpecFileHasCorrectContent(self):
        installer = hpuxpackage.HPUXPackageFile(None, self.product, self.platform)
        installer.stagingDir = testutils.FakeStagingDir()
        self.product.prereqs = ['openssl.OPENSSL-LIB,r>=A.00.09.07l.003,r<A.00.09.08 | openssl.OPENSSL-LIB,r>=A.00.09.08d.002']
        output = StringIO()
        installer.WriteSpecTo(output)
        self.assertEqual(
            'depot\n  layout_version   1.0\n\n# Vendor definition:\nvendor\n  tag           MSFT\n  title         Microsoft\ncategory\n  tag           scx\n  revision      1.2.3-4\nend\n\n# Product definition:\nproduct\n  tag            scx\n  revision       1.2.3-4\n  architecture   HP-UX_B.11.00_32/64\n  vendor_tag     MSFT\n\n  title          scx\n  number         4\n  category_tag   scx\n\n  description    Description text\n  copyright      \n  machine_type   ia64*\n  os_name        HP-UX\n  os_release     ?.11.*\n  os_version     ?\n\n  directory      /\n  is_locatable   false\n\n  # Fileset definitions:\n  fileset\n    tag          core\n    title        scx Core\n    revision     1.2.3-4\n\n    # Dependencies\n    prerequisites openssl.OPENSSL-LIB,r>=A.00.09.07l.003,r<A.00.09.08 | openssl.OPENSSL-LIB,r>=A.00.09.08d.002\n\n    # Control files:\n    configure     ' + self.product.buildInfo.TempDir + '/configure.sh\n    unconfigure   ' + self.product.buildInfo.TempDir + '/unconfigure.sh\n    preinstall    ' + self.product.buildInfo.TempDir + '/preinstall.sh\n    postremove    ' + self.product.buildInfo.TempDir + '/postremove.sh\n\n    # Files:\n\n  end # core\n\nend  # SD\n',
            output.getvalue())


class GivenHPUXGeneratedScripts(unittest.TestCase):
    def setUp(self):
        self.product = TestProduct()

        self.platform = PlatformInformation()
        self.platform.OS = 'HPUX'
        self.platform.Architecture = 'ia64'
        self.platform.MajorVersion = 11
        self.platform.MinorVersion = 30

        stagingDir = testutils.FakeStagingDir()
        stagingDir.Add(filegenerator.SoftLink('usr/sbin/scxadmin', '', 755, 'root', 'root'))
        stagingDir.Add(filegenerator.FileCopy('opt/microsoft/scx/bin/tools/scx-cimd', '', 555, 'root', 'bin'))
        stagingDir.Add(filegenerator.FileCopy('etc/opt/microsoft/scx/conf/scxlog.conf', '', 555, 'root', 'bin', 'conffile'))
        installer = hpuxpackage.HPUXPackageFile(stagingDir, self.product, self.platform)
        installer.GenerateScripts()

    def testPreInstallScriptBacksUpConfigFiles(self):
        self.assertEqual(
            '#!/usr/bin/sh\n\nbackup_configuration_file() {\n  mv "$1" "$1.swsave" > /dev/null 2>&1\n}\n\n\n#\n# Main section\n#\nif [ -f InstallationMarker ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\nbackup_configuration_file /etc/opt/microsoft/scx/conf/scxlog.conf\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/preinstall.sh'))

    def testPostInstallScriptRestoresConfigFiles(self):
        self.assertEqual(
            '#!/usr/bin/sh\n\nrestore_configuration_file() {\n  mv "$1.swsave" "$1"\n}\n\n\n#\n# Main section\n#\nrestore_configuration_file /etc/opt/microsoft/scx/conf/scxlog.conf\n' + self.product.postInstallContent + '\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/configure.sh'))

    def testPostRemoveScriptIsCorrect(self):
        self.assertEqual(
            '#!/usr/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/postremove.sh'))

    def testPreRemoveScriptIsCorrect(self):
        self.assertEqual(
            '#!/usr/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.preRemoveContent + '\n',
            testutils.GetFileContent(self.product.buildInfo.TempDir + '/unconfigure.sh'))
    
