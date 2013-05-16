import unittest
import sys
sys.path.append('../')
import linuxrpm
import scriptgenerator
import filegenerator
import testutils
import re
from product import Product
from platforminformation import PlatformInformation
from cStringIO import StringIO

class TestablePackageMaker(linuxrpm.LinuxRPMFile):
    def __init__(self, stagingDir, product, platform):
        testutils.thefakefiles = {}
        linuxrpm.LinuxRPMFile.__init__(self, stagingDir, product, platform)
        linuxrpm.open = testutils.FakeOpen
        self.TheFakeOS = testutils.FakeOS()
        linuxrpm.os.unlink = self.TheFakeOS.unlink
        linuxrpm.os.system = self.TheFakeOS.system
        linuxrpm.os.popen4 = self.TheFakeOS.popen4
        linuxrpm.os.path.exists = lambda path: False

##
# Test cases for the linux RPM spec file used in packaging.
#
class LinuxRPMSpecFileTestCase(unittest.TestCase):
    def setUp(self):
        self.product = testutils.TestProduct()
        self.product.prereqs = [ 'foo >= 1.2.3', 'bar >= 4.5.6' ]
        self.stagingDir = testutils.FakeStagingDir()
        self.stagingDir.Add(filegenerator.SoftLink('usr/sbin/scxadmin', '', 755, 'root', 'root'))
        self.stagingDir.Add(filegenerator.FileCopy('opt/microsoft/scx/bin/tools/scx-cimd', '', 555, 'root', 'bin'))

    def GivenAPlatform(self, distro, major, minor):
        platform = PlatformInformation()
        platform.OS = 'Linux'
        platform.Distribution = distro
        platform.Architecture = 'x86'
        platform.MajorVersion = major
        platform.MinorVersion = minor
        return platform

    def GivenASpecFile(self, distro, major, minor):
        platform = self.GivenAPlatform(distro, major, minor)
        return linuxrpm.LinuxRPMSpecFile(self.product, platform, self.stagingDir)
        
    def VerifyGeneratedSpecFile(self, expectedContent, distro, major, minor):
        specFile = self.GivenASpecFile(distro, major, minor)
        output = StringIO()
        specFile.WriteTo(output)
        self.assertEqual(expectedContent, output.getvalue())
    
    def testWhenDistroIsSUSE9_GeneratedSpecFileHasCorrectContent(self):
        self.VerifyGeneratedSpecFile(
            '%define __find_requires %{nil}\n%define _use_internal_dependency_generator 0\n\nName: scx\nVersion: 1.2.3\nRelease: 4\nSummary: Microsoft system center cross platform.\nGroup: Applications/System\nLicense: License\nVendor: Microsoft\nRequires: foo >= 1.2.3, bar >= 4.5.6\nProvides: cim-server\nConflicts: %{name} < %{version}-%{release}\nObsoletes: %{name} < %{version}-%{release}\n%description\nDescription text\n%files\n%defattr(755,root,root)\n/usr/sbin/scxadmin\n%defattr(555,root,bin)\n/opt/microsoft/scx/bin/tools/scx-cimd\n%pre\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 2 ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\n%post\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n%preun\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 0 ]; then\n' + self.product.preRemoveContent + '\n:\nfi\nexit 0\n%postun\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            'SUSE', 9, 0)

    def testWhenDistroIsSUSE10_GeneratedSpecFileHasCorrectContent(self):
        self.VerifyGeneratedSpecFile(
            '%define __find_requires %{nil}\n%define _use_internal_dependency_generator 0\n\nName: scx\nVersion: 1.2.3\nRelease: 4\nSummary: Microsoft system center cross platform.\nGroup: Applications/System\nLicense: License\nVendor: Microsoft\nRequires: foo >= 1.2.3, bar >= 4.5.6\nProvides: cim-server\nConflicts: %{name} < %{version}-%{release}\nObsoletes: %{name} < %{version}-%{release}\n%description\nDescription text\n%files\n%defattr(755,root,root)\n/usr/sbin/scxadmin\n%defattr(555,root,bin)\n/opt/microsoft/scx/bin/tools/scx-cimd\n%pre\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 2 ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\n%post\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n%preun\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 0 ]; then\n' + self.product.preRemoveContent + '\n:\nfi\nexit 0\n%postun\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            'SUSE', 10, 1)

    def testWhenDistroIsSUSE11_GeneratedSpecFileHasCorrectContent(self):
        self.VerifyGeneratedSpecFile(
            '%define __find_requires %{nil}\n%define _use_internal_dependency_generator 0\n\nName: scx\nVersion: 1.2.3\nRelease: 4\nSummary: Microsoft system center cross platform.\nGroup: Applications/System\nLicense: License\nVendor: Microsoft\nRequires: foo >= 1.2.3, bar >= 4.5.6\nProvides: cim-server\nConflicts: %{name} < %{version}-%{release}\nObsoletes: %{name} < %{version}-%{release}\n%description\nDescription text\n%files\n%defattr(755,root,root)\n/usr/sbin/scxadmin\n%defattr(555,root,bin)\n/opt/microsoft/scx/bin/tools/scx-cimd\n%pre\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 2 ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\n%post\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n%preun\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 0 ]; then\n' + self.product.preRemoveContent + '\n:\nfi\nexit 0\n%postun\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            'SUSE', 11, 0)

    def testWhenDistroIsREDHAT4_GeneratedSpecFileHasCorrectContent(self):
        self.VerifyGeneratedSpecFile(
            '%define __find_requires %{nil}\n%define _use_internal_dependency_generator 0\n\nName: scx\nVersion: 1.2.3\nRelease: 4\nSummary: Microsoft system center cross platform.\nGroup: Applications/System\nLicense: License\nVendor: Microsoft\nRequires: foo >= 1.2.3, bar >= 4.5.6\nProvides: cim-server\nConflicts: %{name} < %{version}-%{release}\nObsoletes: %{name} < %{version}-%{release}\n%description\nDescription text\n%files\n%defattr(755,root,root)\n/usr/sbin/scxadmin\n%defattr(555,root,bin)\n/opt/microsoft/scx/bin/tools/scx-cimd\n%pre\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 2 ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\n%post\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n%preun\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 0 ]; then\n' + self.product.preRemoveContent + '\n:\nfi\nexit 0\n%postun\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            'REDHAT', 4, 0)

    def testWhenDistroIsREDHAT5_GeneratedSpecFileHasCorrectContent(self):
        self.VerifyGeneratedSpecFile(
            '%define __find_requires %{nil}\n%define _use_internal_dependency_generator 0\n\nName: scx\nVersion: 1.2.3\nRelease: 4\nSummary: Microsoft system center cross platform.\nGroup: Applications/System\nLicense: License\nVendor: Microsoft\nRequires: foo >= 1.2.3, bar >= 4.5.6\nProvides: cim-server\nConflicts: %{name} < %{version}-%{release}\nObsoletes: %{name} < %{version}-%{release}\n%description\nDescription text\n%files\n%defattr(755,root,root)\n/usr/sbin/scxadmin\n%defattr(555,root,bin)\n/opt/microsoft/scx/bin/tools/scx-cimd\n%pre\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 2 ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\n%post\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n%preun\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 0 ]; then\n' + self.product.preRemoveContent + '\n:\nfi\nexit 0\n%postun\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            'REDHAT', 5, 0)

    def testWhenDistroIsREDHAT6_GeneratedSpecFileHasCorrectContent(self):
        self.VerifyGeneratedSpecFile(
            '%define __find_requires %{nil}\n%define _use_internal_dependency_generator 0\n\nName: scx\nVersion: 1.2.3\nRelease: 4\nSummary: Microsoft system center cross platform.\nGroup: Applications/System\nLicense: License\nVendor: Microsoft\nRequires: foo >= 1.2.3, bar >= 4.5.6\nProvides: cim-server\nConflicts: %{name} < %{version}-%{release}\nObsoletes: %{name} < %{version}-%{release}\n%description\nDescription text\n%files\n%defattr(755,root,root)\n/usr/sbin/scxadmin\n%defattr(555,root,bin)\n/opt/microsoft/scx/bin/tools/scx-cimd\n%pre\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 2 ]; then\n' + self.product.preUpgradeContent + '\n:\nelse\n' + self.product.preInstallContent + '\n:\nfi\n%post\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postInstallContent + '\nexit 0\n%preun\n#!/bin/sh\n\n\n#\n# Main section\n#\nif [ $1 -eq 0 ]; then\n' + self.product.preRemoveContent + '\n:\nfi\nexit 0\n%postun\n#!/bin/sh\n\n\n#\n# Main section\n#\n' + self.product.postRemoveContent + '\nexit 0\n',
            'REDHAT', 6, 0)

##
# Test cases for the linux RPM package.
#
class LinuxRPMFileTestCase(unittest.TestCase):

    def setUp(self):
        self.product = testutils.TestProduct()

    def GivenAPlatform(self, distro, major, minor):
        platform = PlatformInformation()
        platform.OS = 'Linux'
        platform.Distribution = distro
        platform.Architecture = 'x86'
        platform.MajorVersion = major
        platform.MinorVersion = minor
        return platform

    def testBuildingPackage_TheCorrectCommandsAreCalled(self):
        platform = self.GivenAPlatform('REDHAT', 6, 0)
        package = TestablePackageMaker(testutils.FakeStagingDir(), self.product, platform)
        package.BuildPackage()
        self.assertEqual('rpmbuild --buildroot  -bb temp/scx.spec', package.TheFakeOS.systemCalls[0])
        self.assertEqual('rpm -q --specfile --qf "%{arch}\n" temp/scx.spec', package.TheFakeOS.systemCalls[1])
        self.assertEqual('mv "target/RPM-packages/RPMS/scx-1.2.3-4..rpm" "target/scx-1.2.3-4.rhel.6.x86.rpm"', package.TheFakeOS.systemCalls[2])
