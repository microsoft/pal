# coding: utf-8
#
# Copyright (c) Microsoft Corporation.  All rights reserved.
#
##
# Module containing classes to generate sh scripts used by installers
#
# Date:   2007-09-27 13:45:47
#

##
# Class representing an SH function.
#
class SHFunction:
    ##
    # Ctor.
    # \param[in] name Name of function
    # \param[in] body a list of strings that represent the function body.
    #            Example: ['# A comment',
    #                      'echo example']
    # \param[in] variableMap A map of function local variables and their
    #            values.
    #            Example: {'SCXHOME': '/opt/microsoft/scx'}
    #
    def __init__(self, name, body, variableMap = {}):
        self.name = name
        self.body = body
        self.variableMap = variableMap

    ##
    # Get the definition string.
    # param[in] globalVariables A map of all script global variables.
    #           Example: {'SCXHOME': '/opt/microsoft/scx'}
    # \returns definition string.
    #
    def Definition(self, globalVariables):
        definition = self.name + '() {\n'
        for line in self.body:
            definition = definition + '  ' + line + '\n'
        definition = definition + '}\n'
        allvars = globalVariables.copy()
        allvars.update(self.variableMap)
        return definition % allvars

    ##
    # Return name of function
    # \returns name of function
    #
    def Name(self):
        return self.name

