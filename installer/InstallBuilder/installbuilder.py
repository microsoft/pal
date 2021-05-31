import sys
import os
import datafileparser
import linuxrpm
import linuxdpkg
import sunospkg
import aixlpp
import hpuxpackage
import scxutil

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
        
dfp = datafileparser.DataFileParser()

dfp.InhaleDataFiles(Variables["DATAFILE_PATH"], args)
dfp.EvaluateVariablesAndDefines()

# Include all variables specified at the command line in the variable dict, as well as overwrite any that were specified in data files
for var in Variables.keys():
    dfp.variables[var] = Variables[var]
for define in Defines:
    dfp.defines.append(define)

# Evaluate each section, processing all InstallBuilder commands.
dfp.EvaluateAllSections()

Files = dfp.sections["Files"]
Directories = dfp.sections["Directories"]
Links = dfp.sections["Links"]
Dependencies = dfp.sections["Dependencies"]

baseDir = os.path.abspath(os.path.expanduser(dfp.variables["BASE_DIR"]))
intermediateDir = os.path.abspath(os.path.expanduser(dfp.variables["INTERMEDIATE_DIR"]))
targetDir = os.path.abspath(os.path.expanduser(dfp.variables["TARGET_DIR"]))
stagingDir = os.path.abspath(os.path.expanduser(dfp.variables["STAGING_DIR"]))

if "DEBUG" in dfp.defines:
    dfp.Debug()

# Make the staging directory
retval = os.system('mkdir -p %s' % stagingDir)
if retval != 0:
    sys.stderr.write("Error: Unable to create staging directory %s" % stagingDir)
    exit(1)

for d in Directories:
    scxutil.MkAllDirs(stagingDir + d.stagedLocation)
    pass

for f in Files:
    scxutil.Copy(os.path.join(baseDir, f.baseLocation), 
                 stagingDir + f.stagedLocation)
    pass

for l in Links:
    scxutil.Link(l.baseLocation, 
                 stagingDir + l.stagedLocation)
    pass

if dfp.variables["PF"] == "Linux":
    if dfp.variables["PACKAGE_TYPE"] == "RPM":
        packageObject = linuxrpm.LinuxRPMFile(intermediateDir, 
                                              targetDir, 
                                              stagingDir, 
                                              dfp.variables, 
                                              dfp.sections)
    elif dfp.variables["PACKAGE_TYPE"] == "DPKG":
        packageObject = linuxdpkg.LinuxDebFile(intermediateDir, 
                                               targetDir, 
                                               stagingDir, 
                                               dfp.variables, 
                                               dfp.sections)
elif dfp.variables["PF"] == "SunOS":
    packageObject = sunospkg.SunOSPKGFile(intermediateDir, 
                                          targetDir,
                                          stagingDir,
                                          dfp.variables, 
                                          dfp.sections)
elif dfp.variables["PF"] == "AIX":
    packageObject = aixlpp.AIXLPPFile(intermediateDir,
                                      targetDir,
                                      stagingDir,
                                      dfp.variables,
                                      dfp.sections)
elif dfp.variables["PF"] == "HPUX":
    packageObject = hpuxpackage.HPUXPackageFile(intermediateDir,
                                                targetDir,
                                                stagingDir,
                                                dfp.variables,
                                                dfp.sections)
else:
    sys.stderr.write("Error: Unsupported Platform")
    exit(1)

packageObject.GeneratePackageDescriptionFiles()
packageObject.BuildPackage()
