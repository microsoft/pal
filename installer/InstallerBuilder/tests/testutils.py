from product import Product

##
# This is a generic product to use when testing the framework.
#
class TestProduct(Product):
    def __init__(self):
        self.ShortName = 'scx'
        self.LongName = 'Microsoft system center cross platform.'
        self.Version = '1.2.3'
        self.BuildNr = '4'
        self.Description = 'Description text'
        self.License = 'License'
        self.Vendor = 'Microsoft'

        self.preInstallContent = 'Pre Install'
        self.postInstallContent = 'Post Install'
        self.preUpgradeContent = 'Pre Upgrade'
        self.preRemoveContent = 'Pre Remove'
        self.postRemoveContent = 'Post Remove'

        self.buildInfo = TestBuildInfo()
        self.prereqs = []

    def WritePreInstallToScript(self, script):
        script.WriteLn(self.preInstallContent)

    def WritePostInstallToScript(self, script):
        script.WriteLn(self.postInstallContent)

    def WritePreUpgradeToScript(self, script):
        script.WriteLn(self.preUpgradeContent)

    def WritePreRemoveToScript(self, script):
        script.WriteLn(self.preRemoveContent)

    def WritePostRemoveToScript(self, script):
        script.WriteLn(self.postRemoveContent)

    def GetInstallationMarkerFile(self):
        return 'InstallationMarker'

    def GetBuildInfo(self):
        return self.buildInfo

    def GetPrerequisites(self):
        return self.prereqs

class TestBuildInfo:
    def __init__(self):
        self.TempDir = 'temp'
        self.TargetDir = 'target'
        self.BuildType = ''
        
    def GetTempDir(self):
        return self.TempDir

    def GetTargetDir(self):
        return self.TargetDir

    def GetBuildType(self):
        return self.BuildType

    
##
# Makes sure we can fake all file writing so that we write to strings instead.
#
class FakeFile:
    def __init__(self):
        self.content = ''

    def write(self, text):
        self.content = self.content + text

    def readlines(self):
        return self.content.splitlines(True)

    def read(self):
        return self.content

    def close(self):
        pass
    
thefakefiles = {}

##
# Fakes the open system command.
#
def FakeOpen(path, mode):
    if mode == 'r':
        try:
            return thefakefiles[path]
        except KeyError:
            return FakeFile()
    thefakefiles[path] = FakeFile()
    return thefakefiles[path]

##
# Returns true if the file passed as argument
# has been written to. Otherwise false.
#
def FileHasBeenWritten(filename):
    try:
        thefakefiles[filename]
        return True
    except:
        return False

##
# Returns the contents of a file.
#
def GetFileContent(filename):
    return thefakefiles[filename].content

##
# Fakes parts of the os module.
#
class FakeOS:
    def __init__(self):
        self.systemCalls = []
        self.unlinkedFiles = []

    def system(self, command):
        self.systemCalls.append(command)

    def popen4(self, command):
        self.systemCalls.append(command)
        return FakeFile(), FakeFile()

    def unlink(self, filepath):
        self.unlinkedFiles.append(filepath)

    def mkdir(self, path):
        pass

##
# Fake scripts
# Lets us inspect the main section of generated scripts.
#
class FakeScript:
    def __init__(self):
        self.mainSection = ''

    def WriteLn(self, line):
        self.mainSection = self.mainSection + line + '\n'

##
# Stubbes out the staging dir class.
#
class FakeStagingDir:
    def __init__(self):
        self.objects = []

    def Add(self, object):
        self.objects.append(object)

    def GetStagingObjectList(self):
        return self.objects
  
    def GetRootPath(self):
        return ''

