import os
import sys
import inspect

def error(s):
    sys.stderr.write("\nError: %s\n" % s)
    exit(1)

def RunInstallBuilder(datafiles):
    os.system("rm -rf ./staging")
    retval = os.system("""python ../installbuilder.py \
                --BASE_DIR=. \
                --TARGET_DIR=./target \
                --INTERMEDIATE_DIR=./intermediate \
                --STAGING_DIR=./staging \
                --PF=%s \
                --PFDISTRO=%s \
                --PFARCH=%s \
                --PFMAJOR=%s \
                --PFMINOR=%s \
                --DATAFILE_PATH=. \
                --PACKAGE_TYPE=%s \
                %s""" % (Variables["PF"], Variables["PFDISTRO"], Variables["PFARCH"], Variables["PFMAJOR"], Variables["PFMINOR"], PACKAGE_TYPE, datafiles) )
    if retval != 0:
        error("installbuilder.py failed.")

def ReadLines(filename):
    if not os.path.exists(filename):
        error(filename + " does not exist")

    f = open(filename)
    lines = f.readlines()
    f.close()
    stripped_lines = []
    for line in lines:
        line = line.strip("\n")
        stripped_lines.append(line)
    return stripped_lines

def GetScriptAsString(script_type):
    script_string = ""

    if PACKAGE_TYPE == "RPM":
        if script_type == "Preinstall":
            script_section = "%pre"
        elif script_type == "Postinstall":
            script_section = "%post"
        elif script_type == "Preuninstall":
            script_section = "%preun"
        elif script_type == "Postuninstall":
            script_section = "%postun"
        else:
            error("Unsupported script section: " + script_type)

        lines = ReadLines("./intermediate/rpm.spec")

        state = None
        for line in lines:
            if len(line) > 0 and line[0] == "%":
                state = line
                continue
            if state == script_section:
                script_string += line + "\n"

    elif PACKAGE_TYPE == "DPKG":
        if script_type == "Preinstall":
            script_filename = "preinst"
        elif script_type == "Postinstall":
            script_filename = "postinst"
        elif script_type == "Preuninstall":
            script_filename = "prerm"
        elif script_type == "Postuninstall":
            script_filename = "postrm"
        else:
            error("Unsupported script section: " + script_type)

        lines = ReadLines("./staging/DEBIAN/" + script_filename)
        for line in lines:
            script_string += line + "\n"

    elif PACKAGE_TYPE == "LPP":
        if script_type == "Preinstall":
            script_filename = "dummytest.rte.pre_i"
        elif script_type == "Postinstall":
            script_filename = "dummytest.rte.config"
        elif script_type == "Preuninstall":
            script_filename = "dummytest.rte.unconfig"
        else:
            error("Unsupported script section: " + script_type)

        lines = ReadLines("./intermediate/lpp-tmp/" + script_filename)
        for line in lines:
            script_string += line + "\n"

    elif PACKAGE_TYPE == "DEPOT":
        if script_type == "Preinstall":
            script_filename = "preinstall.sh"
        elif script_type == "Postinstall":
            script_filename = "configure.sh"
        elif script_type == "Preuninstall":
            script_filename = "unconfigure.sh"
        elif script_type == "Postuninstall":
            script_filename = "postremove.sh"
        else:
            error("Unsupported script section: " + script_type)

        lines = ReadLines("./intermediate/pkg-tmp/" + script_filename)
        for line in lines:
            script_string += line + "\n"

    elif PACKAGE_TYPE == "PKG":
        if script_type == "Preinstall":
            script_filename = "preinstall"
        elif script_type == "Postinstall":
            script_filename = "postinstall"
        elif script_type == "Preuninstall":
            script_filename = "preremove"
        elif script_type == "Postuninstall":
            script_filename = "postremove"
        else:
            error("Unsupported script section: " + script_type)

        lines = ReadLines("./intermediate/pkg-tmp/MSFTdummytest/install/" + script_filename)
        for line in lines:
            script_string += line + "\n"

    return script_string
    

# This test is expecting that the "#include" command will include a section in another section.
def Test_IncludeCommand():
    EXPECTED_STRING = """echo "I'm included!"

exit 0
"""
    if Variables["PF"] == "HPUX":
        EXPECTED_STRING = """#!/bin/sh

BackupConfigurationFile() {
    mv "$1" "$1.swsave" > /dev/null 2>&1
}
""" + EXPECTED_STRING

    print("TEST: " + inspect.currentframe().f_code.co_name)
    RunInstallBuilder("base_dummy.data include_command_test.data")

    actual_string = GetScriptAsString("Preinstall")

    if EXPECTED_STRING != actual_string:
        error("Assertion!\nExpected:\n%s\nActual:\n%s\n" % (EXPECTED_STRING, actual_string))
            
    print("PASS")


# This test is expecting that script sections of the same type will be ordered numerically in the output
def Test_SectionsInNumericOrder():
    EXPECTED_STRING = """echo 3
echo 100
echo 500
echo 1000
exit 0
"""
    if Variables["PF"] == "HPUX":
        EXPECTED_STRING = """#!/bin/sh

BackupConfigurationFile() {
    mv "$1" "$1.swsave" > /dev/null 2>&1
}
""" + EXPECTED_STRING

    print("TEST: " + inspect.currentframe().f_code.co_name)
    RunInstallBuilder("base_dummy.data sections_in_numeric_order.data")

    actual_string = GetScriptAsString("Preinstall")

    if EXPECTED_STRING != actual_string:
        error("Assertion!\nExpected:\n%s\nActual:\n%s\n" % (EXPECTED_STRING, actual_string))
            
    print("PASS")


# This test is expecting that a variable can be overridden by a sequential datafile.
def Test_VariableOverride():
    EXPECTED_STRING = """echo CORRECT
exit 0
"""
    if Variables["PF"] == "HPUX":
        EXPECTED_STRING = """#!/bin/sh

RestoreConfigurationFile() {
    mv "$1.swsave" "$1"
}
""" + EXPECTED_STRING

    print("TEST: " + inspect.currentframe().f_code.co_name)
    RunInstallBuilder("base_dummy.data variable_override1.data variable_override2.data")

    actual_string = GetScriptAsString("Postinstall")

    if EXPECTED_STRING != actual_string:
        error("Assertion!\nExpected:\n%s\nActual:\n%s\n" % (EXPECTED_STRING, actual_string))
            
    print("PASS")


# This test is expecting that a variable can be overridden by a sequential datafile.
def Test_LargeConditionalPath():
    EXPECTED_STRING = """echo TRUE PATH 2
echo TRUE PATH 3
echo TRUE PATH 6
exit 0
"""

    print("TEST: " + inspect.currentframe().f_code.co_name)
    RunInstallBuilder("base_dummy.data large_conditional_path.data")

    actual_string = GetScriptAsString("Preuninstall")

    if EXPECTED_STRING != actual_string:
        error("Assertion!\nExpected:\n%s\nActual:\n%s" % (EXPECTED_STRING, actual_string))
            
    print("PASS")


# MAIN
Variables = dict()
Defines = []

# Parse command line arguments
args = []
optlist = []
for arg in sys.argv[1:]:
    if len(arg) < 2:
        # Must be a file
        args.append(arg)
        continue

    if arg[0:2] == "--":
        tokens = arg[2:].split("=",1)
        if len(tokens) == 1:
            # This is a define
            Defines.append(tokens[0])
            Variables[tokens[0]] = ""
        else:
            # This is a variable
            Variables[tokens[0]] = tokens[1]
    else:
        args.append(arg)

if Variables["PF"] == "Linux":
    if (Variables["PFDISTRO"] == "ULINUX" and Variables["PF_DISTRO_ULINUX_KIT"] == "R"):
        PACKAGE_TYPE = "RPM"
    elif (Variables["PFDISTRO"] == "ULINUX" and Variables["PF_DISTRO_ULINUX_KIT"] == "D"):
        PACKAGE_TYPE = "DPKG"
    elif ((Variables["PFDISTRO"] == "REDHAT" or Variables["PFDISTRO"] == "SUSE") and Variables["PFARCH"] == "ppc"):
        PACKAGE_TYPE = "RPM"
    else:
        error("Invalid Platform")
elif Variables["PF"] == "AIX":
    PACKAGE_TYPE = "LPP"
elif Variables["PF"] == "HPUX":
    PACKAGE_TYPE = "DEPOT"
elif Variables["PF"] == "SunOS":
    PACKAGE_TYPE = "PKG"
else:
    error("Invalid Platform")


Test_IncludeCommand()
Test_SectionsInNumericOrder()
Test_VariableOverride()
Test_LargeConditionalPath()
