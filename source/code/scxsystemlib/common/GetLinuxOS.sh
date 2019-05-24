#!/bin/sh

#####################################################################
## Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
## Name: GetLinuxOS.sh
## Date: 7/1/2012
## Version: 1.0
## Description: Release file parsing for Linux Operating Systems
#####################################################################

Hostname=`uname -n`
OSName=`uname -s`
Version=`uname -r`      # Version
Arch=`uname -m`         # Overridden as uname -p on some platforms
ReleaseFile=""
EtcPath="/etc"

# Create destination directory if it does not exist
RelDir="<RelDir>"
RelFile="$RelDir/scx-release"
DisableFile="$RelDir/disablereleasefileupdates"

if [ ! -e ${RelDir} ]; then
    mkdir -p $RelDir
fi

## Function used to get Linux Distro and Version
######################################################################
GetKitType() {
    # The os-release file doesn't tell us the type of kit, so try to determine
    if [ `rpm -q rpm 2> /dev/null | /bin/egrep '^rpm-[0-9].*' | wc -l` = 1 ]; then
        OSAlias="UniversalR"
    elif [ `dpkg -l dpkg 2> /dev/null | egrep "^ii.*"| wc -l` = 1 ]; then
        OSAlias="UniversalD"
    else
        OSAlias="Universal?"
    fi
}

GetLinuxInfo() {
    IsLinux="true"

    # Return kernel version as OS version - will be updated for specific cases below
    Version=`uname -r | cut -d. -f1,2`

    # Determine release file

    # Try to find -release file
    if [ -z ${ReleaseFile} ]; then
        ReleaseFile=`ls -F ${EtcPath}/*-release 2>/dev/null | grep -v lsb-release |grep -v release@| grep -v scx-release| sed -n '1p'`
    fi

    # Fall back to lsb-release (e.g. Ubunutu)
    if [ -z ${ReleaseFile} ]; then
        if [ -e "${EtcPath}/lsb-release" ]; then ReleaseFile="${EtcPath}/lsb-release"; fi
    fi

    # Debian (no lsb-release, but debian_version exists)
    if [ -z ${ReleaseFile} ]; then
        TestFile="${EtcPath}/debian_version"
        if [ -e $TestFile ]; then ReleaseFile=$TestFile; fi
    fi

    # Try RHEL/CentOS
    TestFile="${EtcPath}/redhat-release"
    if [ -f $TestFile ]; then ReleaseFile=$TestFile; fi

    # Try OEL
    TestFile="${EtcPath}/oracle-release"
    if [ -f $TestFile ]; then ReleaseFile=$TestFile; fi

    # Try NeoKylin
    TestFile="${EtcPath}/neokylin-release"
    if [ -f $TestFile ]; then ReleaseFile=$TestFile; fi

    # Try SLES
    TestFile="${EtcPath}/SuSE-release"
    if [ -f $TestFile ]; then ReleaseFile=$TestFile; fi


    # Get OS Name
    if [ ! -z $ReleaseFile ]; then
      OSName=`/bin/egrep -o 'Red Hat Enterprise Linux|SUSE Linux Enterprise Server|SUSE LINUX Enterprise Server' $ReleaseFile`
    fi
    ## Extract Version from file
    ## Could also use /etc/*release (not Ubuntu)
    if [ "${OSName}" = "Red Hat Enterprise Linux" ]; then
        Version=`grep 'Red Hat Enterprise' $ReleaseFile | sed s/.*release\ // | sed s/\ .*//`
        if [ "${Version}" != "" ]; then
            OSAlias="RHEL"
            OSManufacturer="Red Hat, Inc."
            OSFullName=`cat $ReleaseFile`
            OSShortName="RHEL_"
        fi
    elif [ "${OSName}" = "SUSE Linux Enterprise Server" ]; then
        # SLES 10 uses "Linux". Need to parse the minor version as SLES 10.0 is not supported, only 10.1 and up
        Version=`grep 'SUSE Linux Enterprise Server' $ReleaseFile | sed s/.*Server\ // | sed s/\ \(.*//`
        # Discovery Wizard wants "10.X" not "10 SPX"
        if [ `echo ${Version} | grep 'SP' | wc -l` -eq 1 ]; then
            Version=`echo ${Version} | awk '{print $1"."$2}'| sed s/SP//`
        else
            VersionPL=`grep PATCHLEVEL $ReleaseFile | sed s/.*PATCHLEVEL\ =\ //`
            if [ "${VersionPL}" !=  "" ]; then
                Version=`echo ${Version}.${VersionPL}`
            fi
        fi
        if [ "${Version}" != "" ]; then
            OSAlias="SLES"
            OSManufacturer="SUSE GmbH"
            OSShortName="SUSE_"
        fi
    elif [ "${OSName}" = "SUSE LINUX Enterprise Server" ]; then
        # SLES 9 uses "LINUX". No need to parse minor version as Agent supports 9.0 and up.
        Version=`grep 'SUSE LINUX Enterprise Server' $ReleaseFile | sed s/.*Server\ // | sed s/\ \(.*//`
        if [ "${Version}" != "" ]; then
            OSAlias="SLES"
            OSManufacturer="SUSE GmbH"
            OSShortName="SUSE_"
        fi
    else
        OSAlias="Universal"
        OSName="Linux"
        Version=`uname -r | cut -d. -f1,2`

        # Do we have the (newer) os-release standard file?
        # If so, that trumps everything else
        if [ -e "${EtcPath}/os-release" ]; then
            OSReleaseFile="${EtcPath}/os-release"
            GetKitType

            # The os-release files contain TAG=VALUE pairs; just read it in
            . "$OSReleaseFile"

            # Some fields are optional, for details see the WWW site:
            #   http://www.freedesktop.org/software/systemd/man/os-release.html
            [ ! -z "$NAME" ] && OptionalOSName="$NAME"
            [ ! -z "$VERSION_ID" ] && Version="$VERSION_ID"

            # Set the manufacturer if we know this ID
            # (Set OSAlias for unit test purposes; test injection won't inject that)
            [ -z "$ID" ] && ID="linux"
            case $ID in
                debian)
                    OSManufacturer="Software in the Public Interest, Inc."
                    OSAlias="UniversalD"
                    if [ "${Version}" != "" ]; then
                        OSShortName="Debian_"
                    else
                        OSShortName="Debian"
                    fi
                    ;;

                opensuse)
                    OSManufacturer="SUSE GmbH"
                    OSAlias="UniversalR"
                    if [ "${Version}" != "" ]; then
                        OSShortName="OpenSUSE_"
                    else
                        OSShortName="OpenSUSE"
                    fi
                    ;;

                centos)
                    OSManufacturer="Central Logistics GmbH"
                    OSAlias="UniversalR"
                    if [ "${Version}" != "" ]; then
                        OSShortName="CentOS_"
                    else
                        OSShortName="CentOS"
                    fi
                    ;;

                ubuntu)
                    OSManufacturer="Canonical Group Limited"
                    OSAlias="UniversalD"
                    if [ "${Version}" != "" ]; then
                        OSShortName="Ubuntu_"
                    else
                        OSShortName="Ubuntu"
                    fi
                    ;;
            esac
        fi
        if [ ! -z $ReleaseFile ]; then
            # Set DefaultOSName to release file contents for evaluation.  If parsing logic is not known, Release File contents will be used as OSName.
            DefaultOSName=`sed '/^$/d' ${ReleaseFile} | head -1`

            # Try known cases for OSName/Version

            # ALT Linux
            if [ `echo $DefaultOSName | grep "ALT Linux" | wc -l` -gt 0 ]; then
                [ -z "$OptionalOSName" ] && OSName="ALT Linux"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalR"
                [ -z "$OSManufacturer" ] && OSManufacturer="ALT Linux Ltd"
                Version=`grep 'ALT Linux' $ReleaseFile | sed s/.*Linux\ // | sed s/\ \.*//`
                [ -z "$OSShortName" ] && OSShortName="ALTLinux_"
            fi

            # Enterprise Linux Server
            if [ `echo $DefaultOSName | grep "Enterprise Linux Enterprise Linux Server" | wc -l` -gt 0 ]; then
                [ -z "$OptionalOSName" ] && OSName="Enterprise Linux Server"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalR"
                [ -z "$OSManufacturer" ] && OSManufacturer="Oracle Corporation"
                Version=`grep 'Enterprise Linux Enterprise Linux Server' $ReleaseFile | sed s/.*release\ // | sed s/\ \(.*//`
                [ -z "$OSShortName" ] && OSShortName="Oracle_"
            fi

            # Oracle Enterprise Linux Server
            if [ `echo $DefaultOSName | grep "Oracle Linux Server" | wc -l` -gt 0 ]; then
                [ -z "$OptionalOSName" ] && OSName="Oracle Linux Server"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalR"
                [ -z "$OSManufacturer" ] && OSManufacturer="Oracle Corporation"
                Version=`grep 'Oracle Linux Server release' $ReleaseFile | sed s/.*release\ // | sed s/\ \(.*//`
                [ -z "$OSShortName" ] && OSShortName="Oracle_"
            fi

            # NeoKylin Linux Advanced Server
            if [ `echo $DefaultOSName | grep "NeoKylin Linux Advanced Server" | wc -l` -gt 0 ]; then
                [ -z "$OptionalOSName" ] && OSName="NeoKylin Linux Server"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalR"
                [ -z "$OSManufacturer" ] && OSManufacturer="China Standard Software Co., Ltd."
                Version=`grep 'NeoKylin Linux Advanced Server release' $ReleaseFile | sed s/.*release\ // | sed s/\ \(.*//`
                [ -z "$OSShortName" ] && OSShortName="NeoKylin_"
            fi

            # OpenSUSE
            if [ `echo $DefaultOSName | grep -i "openSUSE" | wc -l` -gt 0 ]; then
                Version=`echo $DefaultOSName | awk '{print $2}'`
                [ -z "$OptionalOSName" ] && OSName="openSUSE"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalR"
                [ -z "$OSManufacturer" ] && OSManufacturer="SUSE GmbH"
                [ -z "$OSShortName" ] && OSShortName="OpenSUSE_"
            fi

            # Debian
            if [ "$ReleaseFile" = "${EtcPath}/debian_version" ]; then
                [ -z "$OptionalOSName" ] && OSName="Debian"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalD"
                [ -z "$OSManufacturer" ] && OSManufacturer="Software in the Public Interest, Inc."
                Version=`cat ${EtcPath}/debian_version`
                [ -z "$OSShortName" ] && OSShortName="Debian_"
            fi

            # Ubuntu
            if [ `echo $DefaultOSName | grep "Ubuntu" | wc -l` -gt 0 ]; then
                [ -z "$OptionalOSName" ] && OSName="Ubuntu"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalD"
                [ -z "$OSManufacturer" ] && OSManufacturer="Canonical Group Limited "
                Version=`grep 'DISTRIB_RELEASE' $ReleaseFile | cut -d'=' -f2`
                [ -z "$OSShortName" ] && OSShortName="Ubuntu_"
            fi

            # Fedora
            if [ `echo $DefaultOSName | grep "Fedora" | wc -l` -gt 0 ]; then
                [ -z "$OptionalOSName" ] && OSName="Fedora"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalR"
                [ -z "$OSManufacturer" ] && OSManufacturer="Red Hat, Inc."
                Version=`grep 'Fedora' $ReleaseFile | sed s/.*release\ // | sed s/\ .*//`
                [ -z "$OSShortName" ] && OSShortName="Fedora_"
            fi

            # CentOS
            if [ `echo $DefaultOSName | grep "CentOS" | wc -l` -gt 0 ]; then
                [ -z "$OptionalOSName" ] && OSName="CentOS"
                [ -z "$OSAlias" -o "$OSAlias"="Universal" ] && OSAlias="UniversalR"
                [ -z "$OSManufacturer" ] && OSManufacturer="Central Logistics GmbH"
                Version=`grep 'CentOS' $ReleaseFile | sed s/.*release\ // | sed s/\ .*//`
                [ -z "$OSShortName" ] && OSShortName="CentOS_"
            fi

            # If distro is not known, determine whether RPM or DPKG is installed
            if [ "${OSAlias}" = "Universal" ]; then
                # Identify package manager
                GetKitType
            fi

            # If Version is null, something went wrong in release file parsing, reset to kernel version
            if [ "$Version" = "" ]; then
                Version=`uname -r`
            fi

            # If OptionalOSName is not null, and OSName still not get or it is "Linux", then we need to set OptionalOSName as OSName
            # else if OSName is null, something went wrong in release file parsing, reset to Linux
            if [ ! -z "$OptionalOSName" ] && [ "$OSName" = "" -o "$OSName" = "Linux" ]; then
                OSName="$OptionalOSName";
            elif [ "$OSName" = "" ]; then
                OSName="Linux"
                OSManufacturer="Universal"
                OSShortName="$OSName"
            fi

        else
            GetKitType

            Version=`uname -r`
            OSName="Linux"
            OSManufacturer="Universal"
            OSShortName="${OSName}_"
        fi
    fi

    if [ -z `echo ${Version} | grep '\.'` ]; then
        Version="$Version.0"
    fi

    # Tack the version number onto the OSShortName if we have one
    if [ -n "$Version" ]; then
        OSShortName="${OSShortName}${Version}"
    fi

    if [ -z "$OSFullName" ]; then
        # Construct OSFullName string
        OSFullName="$OSName $Version ($Arch)"
    fi
}

## End Linux distro function
######################################################################

GetLinuxInfo

# If the touch file does not exist or RelFile does not exist, write the rel file
if [ ! -e $DisableFile ] || [ ! -e $RelFile ]; then
    # Update scx-release
    printf "OSName=$OSName\n" > $RelFile
    printf "OSVersion=$Version\n" >> $RelFile
    printf "OSFullName=$OSFullName\n" >>$RelFile
    printf "OSAlias=$OSAlias\n" >>$RelFile
    printf "OSManufacturer=$OSManufacturer\n" >>$RelFile
    printf "OSShortName=$OSShortName\n" >> $RelFile

    # Verify that it's W:R so non-priv'ed users can read
    chmod 644 $RelFile
fi

