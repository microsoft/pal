import unittest
import os
import shutil
import re
import sys
sys.path.append('../')
from scriptgenerator import Script
from cStringIO import StringIO
from platforminformation import PlatformInformation
from product import Product
from executablefile import ExecutableFile

##
# Simple utility class to create scripts
# that automatically delete themselves
# when we are done with them.
#
class SelfDeletingScript(Script):
    def __init__(self, path, platform):
        Script.__init__(self, platform)
        self.path = path

    def __del__(self):
        os.remove(self.path)
        
##
# Class for easy testing of pam.conf file functionality.
# This class will delete the test file when disposed.
#
class PamConfFile:
    def __init__(self, path):
        self.path = path
        out = open(self.path, 'w')
        out.write('# Test PAM configuration\n')

    def __del__(self):
        os.remove(self.path)
        
    def ConfigureService(self, service):
        out = open(self.path, 'a')
        out.write(service + '    auth requisite          what.so.1\n')
        out.write(service + '    auth required           ever.so.1\n')
        out.write(service + '    auth required           i.so.1\n')
        out.write(service + '    auth required           do.so.1\n')
        out.write(service + '    account requisite       not.so.1\n')
        out.write(service + '    account required        care.so.1\n')
        
    def GetConf(self, service):
        conf = []
        regex = re.compile('^[#\s]*' + service)
        content = open(self.path)
        for line in content:
            if regex.match(line):
                conf.append(line.replace(service, ''))
        return conf

##
# Class for easy testing of pam.d directory functionality.
# This class will delete the directory when disposed.
#
class PamConfDir:
    def __init__(self, path):
        self.path = path
        os.mkdir(self.path)

    def __del__(self):
        shutil.rmtree(self.path, 1)

    def ConfigureService(self, service):
        out = open(os.path.join(self.path, service), 'w')
        out.write('#%PAM-1.0\n')
        out.write('auth requisite          what.so.1\n')
        out.write('auth required           ever.so.1\n')
        out.write('auth required           i.so.1\n')
        out.write('auth required           do.so.1\n')
        out.write('account requisite       not.so.1\n')
        out.write('account required        care.so.1\n')
        
    def GetConf(self, service):
        conf = []
        content = open(os.path.join(self.path, service))
        for line in content:
            if line[0] != '#':
                conf.append(line)
        return conf

##
# Tests functionality of the scriptgenerator.
#
class ScriptGeneratorTestCase(unittest.TestCase):

    def GivenPlatform(self, pf):
        platform = PlatformInformation()
        platform.OS = pf
        platform.Distribution = 'SUSE'
        platform.Architecture = 'x86'
        platform.MajorVersion = 10
        platform.MinorVersion = 1
        return platform

    def AssertScriptContent(self, expected, script):
        output = StringIO()
        script.WriteTo(output)
        self.assertEqual(expected, output.getvalue())
    
    def testWhenPlatformIsLinux_ShellPathIsCorrect(self):
        platform = self.GivenPlatform('Linux')
        script = Script(platform)
        self.AssertScriptContent('#!/bin/sh\n\n\n#\n# Main section\n#\n', script)

    def testWhenPlatformIsSun_ShellPathIsCorrect(self):
        platform = self.GivenPlatform('SunOS')
        script = Script(platform)
        self.AssertScriptContent('#!/usr/bin/sh\n\n\n#\n# Main section\n#\n', script)

    def testWhenPlatformIsHPUX_ShellPathIsCorrect(self):
        platform = self.GivenPlatform('HPUX')
        script = Script(platform)
        self.AssertScriptContent('#!/usr/bin/sh\n\n\n#\n# Main section\n#\n', script)
        
    def testWhenPlatformIsAIX_ShellPathIsCorrect(self):
        platform = self.GivenPlatform('AIX')
        script = Script(platform)
        self.AssertScriptContent('#!/bin/sh\n\n\n#\n# Main section\n#\n', script)

    def testWhenPlatformIsMAC_ShellPathIsCorrect(self):
        platform = self.GivenPlatform('MacOS')
        script = Script(platform)
        self.AssertScriptContent('#!/bin/sh\n\n\n#\n# Main section\n#\n', script)

    def testInstallInfoHasVersionAndBuildNr(self):
        script = Script(self.GivenPlatform('HPUX'))
        script.CallFunction(script.WriteInstallInfo('1.2.3', '4'))
        self.AssertScriptContent('#!/usr/bin/sh\n\nwrite_install_info() {\n  date +%Y-%m-%dT%T.0Z > /etc/opt/microsoft/scx/conf/installinfo.txt\n  echo 1.2.3-4 >> /etc/opt/microsoft/scx/conf/installinfo.txt\n  return 0\n}\n\n\n#\n# Main section\n#\nwrite_install_info \n', script)

    def testSun59CertGenerationIsCorrect(self):
        platform = self.GivenPlatform('SunOS')
        platform.MajorVersion = 5
        platform.MinorVersion = 9
        script = Script(platform)
        script.CallFunction(script.GenerateCertificate())
        self.AssertScriptContent('#!/usr/bin/sh\n\ngenerate_certificate() {\n  if [ ! "/etc/opt/microsoft/scx/ssl" = "" ] && [ -d /etc/opt/microsoft/scx/ssl ]; then\n   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/ssl/lib\n   export LD_LIBRARY_PATH\n    if [ -f /etc/opt/microsoft/scx/ssl/scx-seclevel1-key.pem ] && [ ! -f /etc/opt/microsoft/scx/ssl/scx-key.pem ]; then\n      mv -f /etc/opt/microsoft/scx/ssl/scx-seclevel1-key.pem /etc/opt/microsoft/scx/ssl/scx-key.pem\n    fi\n    if [ -f /etc/opt/microsoft/scx/ssl/scx-seclevel1.pem ] && [ ! -f /etc/opt/microsoft/scx/ssl/scx.pem ]; then\n      mv -f /etc/opt/microsoft/scx/ssl/scx-seclevel1.pem /etc/opt/microsoft/scx/ssl/scx-host-`hostname`.pem\n      ln -s -f /etc/opt/microsoft/scx/ssl/scx-host-`hostname`.pem /etc/opt/microsoft/scx/ssl/scx.pem\n    fi\n    /opt/microsoft/scx/bin/tools/scxsslconfig\n    if [ $? -ne 0 ]; then\n      exit 1\n    fi\n  else\n    # /etc/opt/microsoft/scx/ssl : directory does not exist\n    exit 1\n  fi\n  return 0\n}\n\n\n#\n# Main section\n#\ngenerate_certificate \n', script)

##
# Tests PAM related functionality of the scriptgenerator.
#
class ScriptGeneratorPAMTestCase(unittest.TestCase):
    def setUp(self):
        self.pamconffile = './pam.conf'
        self.pamconfdir = './pam.d'
        self.platform = PlatformInformation()
        self.platform.OS = 'SunOS'
        self.platform.MinorVersion = '10'
                    
    def RunScript(self, script):
        os.system('sh ' + script.path)

    def GetPermissions(self, path):
        out = os.popen('ls -l ' + path).read()
        return out.split()[0]

    def GivenAScript(self, filename):
        script = SelfDeletingScript(filename, self.platform)
        script.variableMap['PAM_CONF_FILE'] = self.pamconffile
        script.variableMap['PAM_CONF_DIR'] = self.pamconfdir
        return script

    def GivenAConfigureScript(self, filename):
        script = self.GivenAScript(filename)
        script.CallFunction(script.ConfigurePAM())
        ExecutableFile(filename, script)
        return script
        
    def GivenAnUnconfigureScript(self, filename):
        script = self.GivenAScript(filename)
        script.CallFunction(script.UnconfigurePAM())
        ExecutableFile(filename, script)
        return script
        
    def GivenAPamConfFile(self):
        conffile = PamConfFile(self.pamconffile)
        return conffile

    def GivenAPamConfDir(self):
        return PamConfDir(self.pamconfdir)

    def GivenAPamConfFileWithSshd(self):
        conffile = self.GivenAPamConfFile()
        conffile.ConfigureService('sshd')
        return conffile

    def GivenAPamConfFileWithScx(self):
        conffile = self.GivenAPamConfFile()
        conffile.ConfigureService('scx')
        return conffile

    def GivenAPamConfDirWithSshd(self):
        confdir = self.GivenAPamConfDir()
        confdir.ConfigureService('sshd')
        return confdir

    def GivenAPamConfDirWithScx(self):
        confdir = self.GivenAPamConfDir()
        confdir.ConfigureService('scx')
        return confdir

    def testConfigureWhenNoConf(self):
        script = self.GivenAConfigureScript('./script.sh')
        self.RunScript(script)
        self.assert_( not os.path.exists(self.pamconffile) )
    
    def testConfigureWhenNoSSH_file(self):
        pamConfFile = self.GivenAPamConfFile()
        script = self.GivenAConfigureScript('./script.sh')

        self.RunScript(script)

        self.assertEqual(pamConfFile.GetConf('scx'), [
            '    auth requisite          pam_authtok_get.so.1\n',
            '    auth required           pam_dhkeys.so.1\n',
            '    auth required           pam_unix_cred.so.1\n',
            '    auth required           pam_unix_auth.so.1\n',
            '    account requisite       pam_roles.so.1\n',
            '    account required        pam_unix_account.so.1\n'
            ])

    def testConfigureWhenNoSSH_dir(self):
        pamConfDir = self.GivenAPamConfDir()
        script = self.GivenAConfigureScript('./script.sh')

        self.RunScript(script)

        self.assertEqual(pamConfDir.GetConf('scx'), [
            'auth requisite          pam_authtok_get.so.1\n',
            'auth required           pam_dhkeys.so.1\n',
            'auth required           pam_unix_cred.so.1\n',
            'auth required           pam_unix_auth.so.1\n',
            'account requisite       pam_roles.so.1\n',
            'account required        pam_unix_account.so.1\n'
            ])
            
    def testConfigureCopiesSSH_file(self):
        pamConfFile = self.GivenAPamConfFileWithSshd()
        script = self.GivenAConfigureScript('./script.sh')

        self.RunScript(script)

        self.assertEqual(pamConfFile.GetConf('scx'), pamConfFile.GetConf('sshd'))

    def testConfigureCopiesSSH_dir(self):
        pamConfDir = self.GivenAPamConfDirWithSshd()
        script = self.GivenAConfigureScript('./script.sh')

        self.RunScript(script)

        self.assertEqual(pamConfDir.GetConf('scx'), pamConfDir.GetConf('sshd'))

    def testUnconfigureSolarisRemovesDefaultConfig_file(self):
        pamConfFile = self.GivenAPamConfFile()
        script1 = self.GivenAConfigureScript('./script1.sh')
        script2 = self.GivenAnUnconfigureScript('./script2.sh')

        self.RunScript(script1)
        self.RunScript(script2)
        
        self.assertEqual(pamConfFile.GetConf('scx'), [])

    def testUnconfigureSolarisRemovesDefaultConfig_dir(self):
        pamConfDir = self.GivenAPamConfDir()
        script1 = self.GivenAConfigureScript('./script1.sh')
        script2 = self.GivenAnUnconfigureScript('./script2.sh')
        self.RunScript(script1)
        self.RunScript(script2)
        self.assert_( os.path.exists(pamConfDir.path) )
        self.assert_( not os.path.exists(os.path.join(pamConfDir.path, 'scx')) )

    def testUnconfigureSolarisDoesNotRemoveCustomConfig_file(self):
        pamConfFile = self.GivenAPamConfFileWithScx()
        script = self.GivenAnUnconfigureScript('./script.sh')

        scxConfBefore = pamConfFile.GetConf('scx')
        self.RunScript(script)

        self.assertEqual(pamConfFile.GetConf('scx'), scxConfBefore)

    def testUnconfigureSolarisDoesNotRemoveCustomConfig_dir(self):
        pamConfDir = self.GivenAPamConfDir()
        scxconf = open(os.path.join(pamConfDir.path, 'scx'), 'w')
        scxconf.write('# Just a random line\nsomething something\n')
        scxconf.close()
        script = self.GivenAnUnconfigureScript('./script.sh')
        self.RunScript(script)
        self.assert_( os.path.exists(os.path.join(pamConfDir.path, 'scx')) )
        data = open(os.path.join(pamConfDir.path, 'scx')).read()
        self.assertEqual(data, '# Just a random line\nsomething something\n')

    def testUnconfigureNonSolarisDoesRemoveCustomConfig_file(self):
        self.platform.OS = 'HPUX'
        pamConfFile = self.GivenAPamConfFileWithScx()
        script = self.GivenAnUnconfigureScript('./script.sh')

        self.RunScript(script)

        self.assertEqual(pamConfFile.GetConf('scx'), [])
        
    def testUnconfigureNonSolarisDoesRemoveCustomConfig_dir(self):
        self.platform.OS = 'HPUX'
        pamConfDir = self.GivenAPamConfDirWithScx()
        script = self.GivenAnUnconfigureScript('./script.sh')

        self.RunScript(script)

        self.assert_( not os.path.exists(os.path.join(pamConfDir.path, 'scx')) )
        
    def testUnconfigureFilePreservesPermissions(self):
        pamConfFile = self.GivenAPamConfFile()
        os.chmod(self.pamconffile, 0644)
        self.assertEqual(self.GetPermissions(self.pamconffile), '-rw-r--r--')
        
        oldUmask = os.umask(0003)
        script1 = self.GivenAConfigureScript('./script1.sh')
        script2 = self.GivenAnUnconfigureScript('./script2.sh')

        self.RunScript(script1)
        self.RunScript(script2)
        
        os.umask(oldUmask)
        self.assertEqual(self.GetPermissions(self.pamconffile), '-rw-r--r--')

    def testConfigureFilePreservesPermissions(self):
        pamConfFile = self.GivenAPamConfFile()
        os.chmod(self.pamconffile, 0644)
        self.assertEqual(self.GetPermissions(self.pamconffile), '-rw-r--r--')

        oldUmask = os.umask(0003)
        script = self.GivenAConfigureScript('./script1.sh')

        self.RunScript(script)
        
        os.umask(oldUmask)
        self.assertEqual(self.GetPermissions(self.pamconffile), '-rw-r--r--')

##
# Tests service related functionality of the scriptgenerator.
#
class ScriptGeneratorServiceTestCase(unittest.TestCase):
    def GivenUbuntuPlatform(self):
        platform = PlatformInformation()
        platform.OS = 'Linux'
        platform.Distribution = 'UBUNTU'
        platform.MajorVersion = 6
        return platform

    def GivenSuse9Platform(self):
        platform = PlatformInformation()
        platform.OS = 'Linux'
        platform.Distribution = 'SUSE'
        platform.MajorVersion = 9
        return platform

    def GivenSuse10Platform(self):
        platform = PlatformInformation()
        platform.OS = 'Linux'
        platform.Distribution = 'SUSE'
        platform.MajorVersion = 10
        return platform
            
    def GivenSolarisPlatform(self, major, minor):
        platform = PlatformInformation()
        platform.OS = 'SunOS'
        platform.MajorVersion = major
        platform.MinorVersion = minor
        return platform
            
    def GivenHPUXPlatform(self):
        platform = PlatformInformation()
        platform.OS = 'HPUX'
        platform.MajorVersion = ''
        platform.MinorVersion = ''
        return platform

    def GivenAIXPlatform(self):
        platform = PlatformInformation()
        platform.OS = 'AIX'
        platform.MajorVersion = ''
        platform.MinorVersion = ''
        return platform
            
    def testUbuntuRegisterServiceCommand(self):
        platform = self.GivenUbuntuPlatform()
        shfunction = Script(platform).ConfigurePegasusService()
        self.assertEqual(shfunction.body, ['update-rc.d scx-cimd defaults'])

    def testSuSERegisterServiceCommand(self):
        platform = self.GivenSuse9Platform()
        shfunction = Script(platform).ConfigurePegasusService()
        self.assertEqual(shfunction.body, ['/usr/lib/lsb/install_initd /etc/init.d/scx-cimd'])

    def testSolaris59RegisterServiceCommand(self):
        platform = self.GivenSolarisPlatform(5, 9)
        shfunction = Script(platform).ConfigurePegasusService()
        self.assertEqual(shfunction.body, ['return 0'])

    def testHPUXRegisterServiceCommand(self):
        platform = self.GivenHPUXPlatform()
        shfunction = Script(platform).ConfigurePegasusService()
        self.assertEqual(shfunction.body, ['return 0'])

    def testAIXRegisterServiceCommand(self):
        platform = self.GivenAIXPlatform()
        shfunction = Script(platform).ConfigurePegasusService()
        self.assertEqual(shfunction.body, ['mkssys -s scx-cimd -p %(SCXHOME)s/bin/scxcimserver -u root -S -n 15 -f 9 -G scx', 'grep scx-cimd /etc/inittab > /dev/null 2>&1', 'if [ $? -eq 0 ]; then', 'rmitab scx-cimd', 'fi', 'mkitab "scx-cimd:2:once:/usr/bin/startsrc -s scx-cimd > /dev/console 2>&1"'])

    def testSolaris510RegisterServiceCommand(self):
        platform = self.GivenSolarisPlatform(5, 10)
        shfunction = Script(platform).ConfigurePegasusService()
        self.assertEqual(shfunction.body, ['svccfg import /var/svc/manifest/application/management/scx-cimd.xml'])

    def testUbuntuStartServiceCommand(self):
        platform = self.GivenUbuntuPlatform()
        shfunction = Script(platform).StartPegasusService()
        self.assertEqual(shfunction.body, ['invoke-rc.d scx-cimd start'])
        
    def testUbuntuStopServiceCommand(self):
        platform = self.GivenUbuntuPlatform()
        shfunction = Script(platform).StopPegasusService()
        self.assertEqual(shfunction.body, ['invoke-rc.d scx-cimd stop'])
        
    def testUbuntuRemoveServiceCommand(self):
        platform = self.GivenUbuntuPlatform()
        shfunction = Script(platform).RemovePegasusService()
        self.assertEqual(shfunction.body, ['update-rc.d -f scx-cimd remove'])

    def testSuse9StartServiceCommand(self):
        platform = self.GivenSuse9Platform()
        shfunction = Script(platform).StartPegasusService()
        self.assertEqual(shfunction.body, ['/etc/init.d/scx-cimd start'])

    def testSuse10StartServiceCommand(self):
        platform = self.GivenSuse10Platform()
        shfunction = Script(platform).StartPegasusService()
        self.assertEqual(shfunction.body, ['service scx-cimd start'])
        
    def testSuse9StopServiceCommand(self):
        platform = self.GivenSuse9Platform()
        shfunction = Script(platform).StopPegasusService()
        self.assertEqual(shfunction.body, ['/etc/init.d/scx-cimd stop'])

    def testSuse10StopServiceCommand(self):
        platform = self.GivenSuse10Platform()
        shfunction = Script(platform).StopPegasusService()
        self.assertEqual(shfunction.body, ['service scx-cimd stop'])
        
