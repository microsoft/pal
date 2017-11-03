import sys
import re
import os

# Valid sections and the sets they belong to
FILE_SECTIONS = [ "Files", "Directories", "Links" ]
SCRIPT_SECTIONS = [ "Preinstall", "Postinstall", "Preuninstall", "Postuninstall", "iConfig", "rConfig", "Preupgrade" ]
DEPENDENCY_SECTIONS = [ "Dependencies" ]
VAR_SECTIONS = [ "Variables", "Defines" ]

def error(s):
    sys.stderr.write("Internal Error: %s \n" % s)
    exit(1)

def error(s, line):
    sys.stderr.write("Error[%s:%s]: \n" % (line[1], line[2]) +  s)
    exit(1)

def error_section(s, section):
    sys.stderr.write("Error[%s]: " % section +  s)
    exit(1)

def warning(s, line):
    sys.stderr.write("Warning[%s:%s]: \n" % (line[1], line[2]) +  s)

def info(s, line):
    sys.stderr.write("Info[%s:%s]: \n" % (line[1], line[2]) +  s)

def invalid_varname(s):
    if " " in s:
        return True
    return False

def CheckIfCommand(s):
    if s in ["#if", "#ifdef", "#ifndef", "#elseif", "#else", "#elseifdef", "#endif", "#include"]:
        return True
    return False

variable_usage = "Invalid variable line entry.  Usage: VARIABLE_NAME: 'VALUE'"
define_usage = "Invalid define line entry.  Usage: DEFINE_NAME"
too_many_ifs = "There is at least one open conditional (#if) that has not been closed by the end of this section"
too_many_endifs = "There is at least one extra end conditional (#endif) in this section."

def edge_quotes_match(s):
    if len(s) < 2:
        return False
    if s[0] == "'" and s[-1] == "'":
        return True
    if s[0] == '"' and s[-1] == '"':
        return True
    return False

# FileEntry
# param: tokens - a list containing [ stagedLocation, baseLocation, permissions, owner, group (, type) ]
#        which correspond exactly with a line in a Files section (the tokens separated by semicolons)
class FileEntry:
    def __init__(self, tokens, line):
        if len(tokens) < 5 or len(tokens) > 6:
            error("Incorrect number of tokens in File entry", line)

        self.stagedLocation = tokens[0]
        self.baseLocation = tokens[1]
        self.permissions = tokens[2]
        self.owner = tokens[3]
        self.group = tokens[4]
        if len(tokens) == 6:
            self.type = tokens[5]
        else:
            self.type = ""

    def __str__(self):
        return self.stagedLocation + "; " + self.baseLocation + "; " + self.permissions + "; " + self.owner + "; " + self.group + "; " + self.type

# LinkEntry
# param: tokens - a list containing [ stagedLocation, baseLocation, permissions, owner, group ]
#        which correspond exactly with a line in a Links section (the tokens separated by semicolons)
class LinkEntry:
    def __init__(self, tokens, line):
        if len(tokens) != 5:
            error("Incorrect number of tokens in Link entry", line)

        self.stagedLocation = tokens[0]
        self.baseLocation = tokens[1]
        self.permissions = tokens[2]
        self.owner = tokens[3]
        self.group = tokens[4]
        self.type = ""

    def __str__(self):
        return self.stagedLocation + "; " + self.baseLocation + "; " + self.permissions + "; " + self.owner + "; " + self.group

# DirectoryEntry
# param: tokens - a list containing [ stagedLocation, permissions, owner, group (, type) ]
#        which correspond exactly with a line in a Directories section (the tokens separated by semicolons)
class DirectoryEntry:
    def __init__(self, tokens, line):
        if len(tokens) < 4 or len(tokens) > 5:
            error("Incorrect number of tokens in Directory entry", line)

        self.stagedLocation = tokens[0]
        self.permissions = tokens[1]
        self.owner = tokens[2]
        self.group = tokens[3]
        if len(tokens) == 5:
            self.type = tokens[4]
        else:
            self.type = ""

    def __str__(self):
        return self.stagedLocation + "; " + self.permissions + "; " + self.owner + "; " + self.group + "; " + self.type

# ConditionalLevel
# param: has_executed - whether this conditional level has evaluated to true before
# param: currently_executing - whether this level is currently executing
class ConditionalLevel:
    def __init__(self, has_executed, currently_executing):
        self.has_executed = has_executed
        self.currently_executing = currently_executing

# ConditionalStack
# Description:
#  This stack is the data structure used to keep track of the many levels of conditionals that can exist.
class ConditionalStack:
    def __init__(self):
        self.level_stack = []

    def IsCodePathActive(self):
        for level in self.level_stack:
            if not level.has_executed:
                return False
            if not level.currently_executing:
                return False
        return True

    # Adds a new level to the conditional stack.  This occurs when there's a #if*.
    def AddLevel(self):
        self.level_stack.append(ConditionalLevel(False, False))

    # Removes a level from the conditional stack.  This occurs when there's a #endif
    def RemoveLevel(self):
        if len(self.level_stack) == 0:
            error("Cannot RemoveLevel, there are no levels on the ConditionalStack")

        self.level_stack.pop()

    # This notifies the conditional stack that we want to execute the current level of the stack.
    def ExecuteCurrentLevel(self):
        level = self.level_stack.pop()
        if level.has_executed == True or level.currently_executing == True:
            error("Trying to execute a conditional level that has already been executed or is currently executing.")
        self.level_stack.append(ConditionalLevel(True, True))

    # This is called in an #else* clause.  This is to turn off the "currently_executing" flag if it is true.
    def NextConditional(self):
        # End execution if currently_executing
        level = self.level_stack.pop()
        if level.currently_executing == True:
            level.currently_executing = False
        self.level_stack.append(level)

    # This is called in an #else* clause.
    # returns - True if the current level has not been executed yet, False otherwise.
    def CurrentLevelHasNotBeenExecutedYet(self):
        if self.Empty():
            error("Expecting ConditionalStack to have at least one level in it")

        return not self.level_stack[-1].has_executed

    def Empty(self):
        return len(self.level_stack) == 0
        
# DataFileParser
# Description:
#  This class reads the datafiles, parses them, and evaluates the commands.
class DataFileParser:
    def __init__(self):
        self.variables = dict()
        self.defines = []
        
    # Used for debugging
    def PrintSections(self):
        sorted_keys = sorted(self.sections.keys())
        for key in sorted_keys:
            print("****************************************** " + key + " ******************************************")
            for line in self.sections[key]:
                print(str(line))

    # This function handles all commands.  Conditionals will be evaluted to either True and False and the conditional stack will be updated.
    # Includes will insert the included section into the including section (which is variable name 'section').
    # param: line - The command to be evaluated.
    # param: ifstack - This is the class' ConditionalStack/
    # param: section - The entire section currently being processed.
    # param: linenum - Used for error messages.
    # returns: 1. ifstack
    #          2. section (modified if there is an #include)
    #          3. linenum (modified if there is an #include)
    def HandleCommand(self, line, ifstack, section, linenum):
        commandline = line[0]
        if len(commandline) == 0:
            return ifstack
        
        tokens = commandline.split()
        firsttoken = tokens[0]

        if firsttoken == "#if":
            ifstack.AddLevel()
            if self.Evaluate(tokens[1:], line) == True:
                ifstack.ExecuteCurrentLevel()
        elif firsttoken == "#ifdef":
            ifstack.AddLevel()
            if self.IsDefined(tokens[1:], line) == True:
                ifstack.ExecuteCurrentLevel()
        elif firsttoken == "#ifndef":
            ifstack.AddLevel()
            if self.IsDefined(tokens[1:], line) == False:
                ifstack.ExecuteCurrentLevel()

        elif firsttoken == "#elseif":
            ifstack.NextConditional()
            
            if ifstack.CurrentLevelHasNotBeenExecutedYet() and self.Evaluate(tokens[1:], line) == True:
                ifstack.ExecuteCurrentLevel()
        elif firsttoken == "#elseifdef":
            ifstack.NextConditional()
            
            if ifstack.CurrentLevelHasNotBeenExecutedYet() and self.IsDefined(tokens[1:], line) == True:
                ifstack.ExecuteCurrentLevel()

        elif firsttoken == "#else":
            ifstack.NextConditional()

            if ifstack.CurrentLevelHasNotBeenExecutedYet():
                ifstack.ExecuteCurrentLevel()

        elif firsttoken == "#endif":
            ifstack.RemoveLevel()

        elif firsttoken == "#include":
            middle_section = []
            for L in self.EvaluateSection(tokens[1]):
                middle_section.append(L)
            section = section[0:linenum-1] + middle_section + section[linenum-1:]
            linenum += len(middle_section)
        return ifstack, section, linenum

    # Expression evaluator.  Returns True if expression is True, False otherwise.
    def Evaluate(self, expressions, line):
        expr = expressions
        if len(expr) < 3:
            error("Bad syntax for #if")

        # This will be in the format of:  #if VAR OP VALUE (and/or VAR OP VALUE)*
        var = expr[0]
        op = expr[1]
        value = expr[2]

        if self.variables.get(var) == None:
            error("Unable to find variable " + var + " in defined variables", line)

        if op == "==":
            return self.variables[var] == value
        elif op == "!=":
            return self.variables[var] != value
        elif op == ">":
            return self.variables[var] > float(value)
        elif op == ">=":
            return self.variables[var] >= float(value)
        elif op == "<":
            return self.variables[var] < float(value)
        elif op == "<=":
            return self.variables[var] <= float(value)
        else:
            error("operator %s is not valid" % op, line);

    # Returns True if the expression exists in the defines or variables, otherwise False.
    def IsDefined(self, expressions, line):
        expr = expressions
        if len(expr) < 1:
            error("Bad syntax for #ifdef", line)

        var = expr[0]
        if var in self.defines:
            return True
        if var in self.variables.keys():
            return True
        return False

    # This is called on every line that isn't in the Variables or Defines sections, and it replaces any text inside 
    # two sets of braces starting with a dollar sign with the value in the self.variables dict.  "${{VAR_NAME}}"
    def ReplaceVariables(self, line):
        # replaces all variables that appear in the form of ${{\w+}}
        var_rex = re.compile(r"\$\{\{\w+\}\}")
        for m in var_rex.findall(line):
            line = line.replace(m, self.variables[m[3:-2]])
        return line

    # This function combines, in numeric order, all of the script sections for a given 'name' (ex. "Preinstall") for all numeric values appended to the 'name'.
    # So if there are sections named Preinstall_10 and Preinstall_50 and Preinstall_3, this function when called for name="Preinstall" will return a section
    # that contains all of the lines in Preinstall_3, followed by all of the lines in Preinstall_10, followed by all of the lines in Preinstall_50.
    def GetCombinedInOrder(self, name):
        returnList = []
        orderedSectionList = []

        # Find all sections that match 'name' followed by some optional underscores, followed by any combination of digits, 
        # then store those digits for numeric sorting and combining scripts
        for section in self.sections.keys():
            section_rex = re.compile(r"%s_*(\d*)" % name)
            m = section_rex.match(section)
            if m != None:
                sectionPriority = m.group(1)
                if sectionPriority == "":
                    sectionPriority = "0"
                orderedSectionList.append( (int(sectionPriority), section) )

        orderedSectionList.sort()

        for orderedSection in orderedSectionList:
            for line in self.sections[ orderedSection[1] ]:
                returnList.append(line)

        return returnList

    # Reads all lines in each datafile and inserts all lines from each section into self.sections.
    def InhaleDataFiles(self, directory, filenames):
        sections = dict()
        for filename in filenames:
            state = None

            f = open(os.path.join(directory,filename))
            lines = f.readlines()
            f.close()

            linenumber = 0
            for line in lines:
                linenumber += 1
                line = line.strip("\n")

                if len(line) > 0 and line[0] == '%':
                    if len(line) > 1 and line[1] == '%':
                        # This is a comment, ignore it
                        continue
                    # Begin new section
                    state = line[1:]
                    if sections.get(state) == None:
                        sections[state] = []
                    elif state not in FILE_SECTIONS + DEPENDENCY_SECTIONS + VAR_SECTIONS:
                        error_section("This script section is defined more than once.", state)
                        
                else:
                    # Handle states
                    sections[state].append( (line, filename, linenumber) )

        self.sections = sections

    # This function is called after the datafiles have been inhaled, and it parses and handles each entry in the Variables and Defines sections.
    # This allows for later sections to then rely on variables referenced by ${{VAR_NAME}}.
    def EvaluateVariablesAndDefines(self):
        # Add variables/defines from data files
        if "Variables" in self.sections:
            ifstack = ConditionalStack()
            for line in self.sections["Variables"]:
                varline = line[0].strip()
                if len(varline) == 0:
                    continue

                if CheckIfCommand(varline.split(" ", 1)[0]):
                    ifstack, dummy, dummy = self.HandleCommand(line, ifstack, [], 0)
                    continue
                
                # If we're currently in a conditional part of the data file that evaluates to false
                if not ifstack.IsCodePathActive():
                    continue
                
                tokens = varline.split(":", 1)

                if len(tokens) != 2:
                     error(variable_usage, line)

                key = tokens[0].strip()
                value = tokens[1].strip()

                if len(value) < 2 or not edge_quotes_match(value):
                    error(variable_usage, line)

                # remove the quotes around the variable value
                value = value[1:-1]
                if self.variables.get(key) != None:
                    info("Variable %s is already defined." % key, line)

                # add the parsed variable to the variables dict
                self.variables[key] = value
                
            if not ifstack.Empty():
                error_section(too_many_ifs, "Variables")

        if "Defines" in self.sections:
            ifstack = ConditionalStack()
            for line in self.sections["Defines"]:
                defline = line[0].strip()
                if len(defline) == 0:
                    continue

                if CheckIfCommand(defline.split(" ", 1)[0]):
                    ifstack, dummy, dummy = self.HandleCommand(line, ifstack, [], 0)
                    continue                

                if not ifstack.IsCodePathActive():
                    continue

                if invalid_varname(defline):
                    error(define_usage, line)

                if defline in self.defines:
                    info("Define %s has already been defined." % defline, line)

                # add the define to the define list
                self.defines.append(defline)

            if not ifstack.Empty():
                error_section(too_many_ifs, "Defines")

    # This evaluates all commands in a given section denoted by 'section', then returns the evaluated lines for the section.
    def EvaluateSection(self, section):
        newsection = []
        ifstack = ConditionalStack()

        if section in SCRIPT_SECTIONS:
            # Combine and evaluate each script section
            cursection = self.GetCombinedInOrder(section)
        else:
            cursection = self.sections[section]

        linenum = 0
        for line in cursection:
            linenum += 1
            line_literal = line[0]
            if section in FILE_SECTIONS + DEPENDENCY_SECTIONS + VAR_SECTIONS:
                if len(line_literal.strip()) == 0:
                    continue                

            if CheckIfCommand(line_literal.split(" ", 1)[0]):
                ifstack, newsection, linenum = self.HandleCommand(line, ifstack, newsection, linenum)
                continue

            if not ifstack.IsCodePathActive():
                continue

            line_literal = self.ReplaceVariables(line_literal)
            if section in FILE_SECTIONS:
                tokens = line_literal.split(";")
                newtokens = []
                for token in tokens:
                    newtokens.append(token.strip())
                
                if section == "Files":
                    newsection.append(FileEntry(newtokens, line))
                elif section == "Directories":
                    newsection.append(DirectoryEntry(newtokens, line))
                elif section == "Links":
                    newsection.append(LinkEntry(newtokens, line))
                else:
                    error("Failing to parse line type in '" + section + "'.", line)
            else:
                newsection.append(line_literal)
                
        if not ifstack.Empty():
            error_section(too_many_ifs, section)

        return newsection

    # This calls EvaluateSection on each "Base section" as mentioned in the README.
    def EvaluateAllSections(self):
        # Read through each line, evaluating #if's/#define's, evaluating variables, tokenizing/classing relevant lines, adding to new sections list
        newsections = dict()
        section_keys = FILE_SECTIONS + SCRIPT_SECTIONS + DEPENDENCY_SECTIONS
        for section in section_keys:
            newsections[section] = self.EvaluateSection(section)

        self.sections = newsections

    # Used for debugging.
    def Debug(self):
        self.PrintSections()
        print("****************************** Variables ******************************")
        print(self.variables)
        print("******************************  Defines  ******************************")
        print(self.defines)
