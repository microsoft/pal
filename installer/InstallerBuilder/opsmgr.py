#coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#

from product import Product
from scriptgenerator import Script

##
# OpsMgr product definition
#
class OpsMgr(Product):
    def __init__(self, version, buildNr, platform, buildInfo):
        self.ShortName = 'scx'
        self.LongName = 'Microsoft system center cross platform.'
        self.Description = 'Provides monitoring capabilities to the heterogenous platform'
        self.License = 'none'
        self.Vendor = 'http://www.microsoft.com'
        self.Copyright = 'For copyright and license information please refer to\n/opt/microsoft/scx/COPYRIGHT and /opt/microsoft/scx/LICENSE.\n'
        self.Version = version
        self.BuildNr = buildNr
        self.platform = platform
        self.buildInfo = buildInfo

    ##
    # Returns an object implementing BuildInfo.
    #
    def GetBuildInfo(self):
        return self.buildInfo
    
    ##
    # Returns the path to a file that can be used to test if the product is installed
    # on the system. If the file is there, then the product is considered installed.
    # This is needed in order to determine if in upgrade or install mode for certain
    # installer formats.
    #
    def GetInstallationMarkerFile(self):
        return '/etc/opt/microsoft/scx/conf/installinfo.txt'

    ##
    # The returned object must have a method WriteTo which takes an opened file.
    #
    def WritePreInstallToScript(self, script):
        if self.platform.OS == 'AIX' or self.platform.OS == 'SunOS':
            script.CallFunction(script.CheckAdditionalDeps())

    ##
    # This method will be called both after a clean install
    # AND after an upgrade. Make sure that the script does not
    # disturb any user configuration.
    #
    def WritePostInstallToScript(self, script):
        if self.buildInfo.GetBuildType() == 'Bullseye':
            script.WriteLn('COVFILE=/var/opt/microsoft/scx/log/OpsMgr.cov' + '\n')
            script.WriteLn('export COVFILE' + '\n')

        if self.platform.OS == 'SunOS' and (self.platform.MajorVersion >= 6 or self.platform.MinorVersion >= 10):
            script.CallFunction(script.CreateLink_usr_sbin_scxadmin())

        script.CallFunction(script.WriteInstallInfo(self.Version, self.BuildNr))
        script.CallFunction(script.GenerateCertificate())
        script.CallFunction(script.ConfigurePAM())
        script.CallFunction(script.ConfigureRunas())
        script.CallFunction(script.ConfigurePegasusService())
        script.CallFunction(script.StartPegasusService())
        script.CallFunction(script.RegisterExtProviders())

    ##
    # Note that this script will NOT be called by the solaris installer since
    # this installer doesn't have any notion of upgrade. On solaris there will
    # be an uninstall and then an install.
    #
    def WritePreUpgradeToScript(self, script):
        if self.platform.OS == 'MacOS':
            script.WriteLn('# Uninstall/unconfigure services')
            script.WriteLn('echo "Uninstalling/unconfigurating services ..."')

        script.CallFunction(script.StopPegasusService())
        script.CallFunction(script.RemovePegasusService())

        if self.platform.OS == 'HPUX':
            script.CallFunction(script.RemoveDeletedFiles())

    def WritePreRemoveToScript(self, script):
        if self.platform.OS == 'MacOS':
            script.WriteLn('# Uninstall/unconfigure services')
            script.WriteLn('echo "Uninstalling/unconfigurating services ..."')

        script.CallFunction(script.StopPegasusService())
        script.CallFunction(script.RemovePegasusService())
        script.CallFunction(script.UnconfigurePAM())
        script.CallFunction(script.RemoveAdditionalFiles())

    def WritePostRemoveToScript(self, script):
        if self.platform.OS == 'HPUX':
            script.CallFunction(script.RemoveEmptyDirectoryRecursive(), '/opt/microsoft')
            script.CallFunction(script.RemoveEmptyDirectoryRecursive(), '/etc/opt/microsoft')
            script.CallFunction(script.RemoveEmptyDirectoryRecursive(), '/var/opt/microsoft')

        if self.platform.OS == 'SunOS' and (self.platform.MajorVersion >= 6 or self.platform.MinorVersion >= 10):
            script.CallFunction(script.RemoveLink_usr_sbin_scxadmin())

    def GetPrerequisites(self):
        if self.platform.OS == 'AIX':
            return [
                'openssl.base 0.9.8.4',
                'xlC.rte 9.0.0.2',
                'xlC.aix50.rte 9.0.0.2',
                'bos.rte.libc 5.3.0.65'
                ]

        if self.platform.OS == 'HPUX':
            # Require a late version of ssl 0.9.7 or a late version of 0.9.8.
            return [
                'openssl.OPENSSL-LIB,r>=A.00.09.07l.003,r<A.00.09.08 | openssl.OPENSSL-LIB,r>=A.00.09.08d.002'
                ]

        if self.platform.OS == 'SunOS':
            if self.platform.MinorVersion == 8:
                return [
                    # We need some core os packages
                    'SUNWcsl\tCore Solaris, (Shared Libs)',
                    'SUNWlibC\tSun Workshop Compilers Bundled libC',
                    'SUNWlibms\tSun WorkShop Bundled shared libm',

                    # We need openssl
                    'SMCossl\topenssl'
                    ]
            elif self.platform.MinorVersion == 9:
                return [
                    # We neend some core os packages
                    'SUNWcsl\tCore Solaris, (Shared Libs)',
                    'SUNWlibC\tSun Workshop Compilers Bundled libC',
                    'SUNWlibms\tSun WorkShop Bundled shared libm',
                
                    # We need openssl
                    'SMCosslg\topenssl'
                    ]
            else:
                return [
                    # We need some core os packages
                    'SUNWcsr\tCore Solaris, (Root)', # This includes PAM
                    'SUNWcslr\tCore Solaris Libraries (Root)',
                    'SUNWcsl\tCore Solaris, (Shared Libs)',
                    'SUNWlibmsr\tMath & Microtasking Libraries (Root)',
                    'SUNWlibC\tSun Workshop Compilers Bundled libC',
                
                    # We need openssl
                    'SUNWopenssl-libraries\tOpenSSL Libraries (Usr)'
                    ]

        if self.platform.OS == 'Linux':
            if self.platform.Distribution == 'SUSE':
                if self.platform.MajorVersion == 11:
                    return [
                        'glibc >= 2.9-7.18',
                        'openssl >= 0.9.8h-30.8',
                        'pam >= 1.0.2-17.2'
                        ]
                elif self.platform.MajorVersion == 10:
                    return [
                        'glibc >= 2.4-31.30',
                        'openssl >= 0.9.8a-18.15',
                        'pam >= 0.99.6.3-28.8'
                        ]
                elif self.platform.MajorVersion == 9:
                    return [
                        'glibc >= 2.3.3-98.28',
                        'libstdc++-41 >= 4.1.2',
                        'libgcc-41 >= 4.1.2',
                        'openssl >= 0.9.7d-15.10',
                        'pam >= 0.77-221.1'
                        ]
                else:
                    raise PlatformNotImplementedError(self.platform.Distribution + self.platform.MajorVersion) 
            elif self.platform.Distribution == 'REDHAT':
                if self.platform.MajorVersion == 6:
                    return [
                        'glibc >= 2.11.1-1',
                        'openssl >= 1.0.0-0.19',
                        'pam >= 1.1.1-2'
                        ]
                elif self.platform.MajorVersion == 5:
                    return [
                        'glibc >= 2.5-12',
                        'openssl >= 0.9.8b-8.3.el5',
                        'pam >= 0.99.6.2-3.14.el5'
                        ]
                elif self.platform.MajorVersion == 4:
                    return [
                        'glibc >= 2.3.4-2',
                        'openssl >= 0.9.7a-43.1',
                        'pam >= 0.77-65.1'
                        ]
                else:
                    raise PlatformNotImplementedError(self.platform.Distribution + self.platform.MajorVersion)

            elif self.platform.Distribution == 'UBUNTU':
                return [
                    'libc6 (>= 2.3.6)',
                    'libssl0.9.8 (>= 0.9.8a-7)',
                    'libpam-runtime (>= 0.79-3)'
                    ]
            else:
                raise PlatformNotImplementedError(self.platform.Distribution)

