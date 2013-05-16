import scxutil

class ExecutableFile:
    def __init__(self, path, contentWriter):
        output = open(path, 'w')
        contentWriter.WriteTo(output)
        scxutil.ChMod(path, '0755')

