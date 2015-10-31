/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved.

*/
/**
    \file

    \brief       Utilities to facilitate implementation of provider tests

    \date        08-10-128 11:23:02


*/
/*----------------------------------------------------------------------------*/

#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxthread.h>
#include <testutils/scxunit.h>
#include <testutils/providertestutils.h>
#include <netdb.h>
#include <limits.h>
#include <sys/utsname.h>
#include <sys/wait.h>

// Mock of Filter (sent to EnumerateInstances)
namespace TestableFilter
{
    // Mock of filter's function table's GetExpression from MI.h
    MI_Result GetExpression(const MI_Filter* self, const MI_Char** lang, const MI_Char** expr);

    // Because GetExpression must assign the address of a string, they must be defined here
    std::string m_lang;
    std::string m_expr;

    // Because GetExpression must assign the address of a function table, it must be defined here
    MI_FilterFT filterFT;

    /* Use this function to get a filter object whose address you can pass
     * to EI and will return the provided query for GetExpression */
    MI_Filter
    SetUp(std::string expression, std::string language)
    {
        // Sort of constructor that sets our test query
        m_expr = expression;
        m_lang = language;
        // Create the mock filter object
        MI_Filter filter;
        // Assign the function table to filter's function table pointer
        filter.ft = &filterFT;
        // Assign the GetExpression function to the function table's GetExpression function pointer
        filterFT.GetExpression = GetExpression;

        return filter;
    }

    MI_Result
    GetExpression(const MI_Filter* self, const MI_Char** lang, const MI_Char** expr)
    {
        // Assign the strings via C-style reference
        *expr = TestableFilter::m_expr.c_str();
        *lang = TestableFilter::m_lang.c_str();
        return MI_RESULT_OK;
    }
}

// Declare storage for static members of class
std::map<MI_Context *, TestableContext *> TestableContext::ms_map;

bool TestableInstance::PropertyInfo::GetValue_MIBoolean(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_BOOLEAN, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.boolean;
}

std::wstring TestableInstance::PropertyInfo::GetValue_MIString(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_STRING, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return SCXCoreLib::StrFromUTF8(std::string(value.string));
}

MI_Uint8 TestableInstance::PropertyInfo::GetValue_MIUint8(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_UINT8, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.uint8;
}

MI_Uint16 TestableInstance::PropertyInfo::GetValue_MIUint16(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_UINT16, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.uint16;
}

MI_Uint32 TestableInstance::PropertyInfo::GetValue_MIUint32(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_UINT32, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.uint32;
}

MI_Uint64 TestableInstance::PropertyInfo::GetValue_MIUint64(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_UINT64, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.uint64;
}

MI_Sint8 TestableInstance::PropertyInfo::GetValue_MISint8(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_SINT8, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.sint8;
}

MI_Sint16 TestableInstance::PropertyInfo::GetValue_MISint16(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_SINT16, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.sint16;
}

MI_Sint32 TestableInstance::PropertyInfo::GetValue_MISint32(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_SINT32, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.sint32;
}

MI_Sint64 TestableInstance::PropertyInfo::GetValue_MISint64(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_SINT64, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.sint64;
}

MI_Datetime TestableInstance::PropertyInfo::GetValue_MIDatetime(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_DATETIME, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    return value.datetime;
}

std::vector<MI_Uint16> TestableInstance::PropertyInfo::GetValue_MIUint16A(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_UINT16A, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    std::vector<MI_Uint16> ret;
    MI_Uint32 i;
    for (i = 0; i < value.uint16a.size; i++)
    {
        ret.push_back(value.uint16a.data[i]);
    }
    return ret;
}

std::vector<std::wstring> TestableInstance::PropertyInfo::GetValue_MIStringA(std::wstring errMsg) const
{
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_STRINGA, type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, exists);
    std::vector<std::wstring> ret;
    MI_Uint32 i;
    for (i = 0; i < value.stringa.size; i++)
    {
        ret.push_back(SCXCoreLib::StrFromUTF8(std::string(value.stringa.data[i])));
    }
    return ret;
}

MI_Uint32 TestableInstance::GetNumberOfKeys() const
{
    const MI_Instance* self = GetInstance();
    const MI_ClassDecl* cd = self->classDecl;
    MI_Uint32 i, keyCount = 0;

    for (i = 0; i < cd->numProperties; i++)
    {
        const MI_PropertyDecl* pd = cd->properties[i];
        const Field* field = (Field*)((const char*)self + pd->offset);

        if (Field_GetExists(field, static_cast<MI_Type> (pd->type)))
        {
            if (pd->flags & MI_FLAG_KEY)
                keyCount++;
        }
    }

    return keyCount;
}

MI_Uint32 TestableInstance::GetNumberOfProperties() const
{
    const MI_Instance* self = GetInstance();
    const MI_ClassDecl* cd = self->classDecl;
    MI_Uint32 i, propCount = 0;

    for (i = 0; i < cd->numProperties; i++)
    {
        const MI_PropertyDecl* pd = cd->properties[i];
        const Field* field = (Field*)((const char*)self + pd->offset);

        if (Field_GetExists(field, static_cast<MI_Type> (pd->type)))
        {
            propCount++;
        }
    }

    return propCount;
}

MI_Result TestableInstance::FindProperty(const char* name, struct PropertyInfo& info) const
{
    const MI_Instance* self = GetInstance();
    const MI_ClassDecl* cd = self->classDecl;

    for (MI_Uint32 i = 0; i < cd->numProperties; i++)
    {
        const MI_PropertyDecl* pd = cd->properties[i];
        const Field* field = (Field*)((const char*)self + pd->offset);

        // Did we find the field being requested?
        if (0 == strcmp(name, pd->name))
        {
            info.name = SCXCoreLib::StrFromUTF8(std::string(pd->name));
            info.isKey = (pd->flags & MI_FLAG_KEY);
            info.type = static_cast<MI_Type> (pd->type);
            Field_Extract(field, info.type, &info.value, &info.exists, &info.flags);
            return MI_RESULT_OK;
        }
    }

    // Not found.
    return MI_RESULT_NOT_FOUND;
}

MI_Result TestableInstance::FindProperty(MI_Uint32 index, struct PropertyInfo& info, bool keysOnly) const
{
    const MI_Instance* self = GetInstance();
    const MI_ClassDecl* cd = self->classDecl;
    MI_Uint32 i, propCount = 0;

    for (i = 0; i < cd->numProperties; i++)
    {
        const MI_PropertyDecl* pd = cd->properties[i];
        const Field* field = (Field*)((const char*)self + pd->offset);

        // Did we find the field being requested?
        if (Field_GetExists(field, static_cast<MI_Type> (pd->type)) &&
            ((!keysOnly) || (keysOnly && (pd->flags & MI_FLAG_KEY))))
        {
            if (index == propCount)
            {
                info.name = SCXCoreLib::StrFromUTF8(std::string(pd->name));
                info.isKey = (pd->flags & MI_FLAG_KEY);
                info.type = static_cast<MI_Type> (pd->type);
                Field_Extract(field, info.type, &info.value, &info.exists, &info.flags);
                return MI_RESULT_OK;
            }
            propCount++;
        }
    }

    // Not found.
    return MI_RESULT_NOT_FOUND;
}

MI_Boolean TestableInstance::GetMIReturn_MIBoolean(std::wstring errMsg) const
{
    struct PropertyInfo info;
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, FindProperty("MIReturn", info));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, false, info.isKey);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_BOOLEAN, info.type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, info.exists);
    return info.value.boolean;
}

std::wstring TestableInstance::GetKey(const wchar_t *name, std::wstring errMsg) const
{
    struct PropertyInfo info;
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK,
        FindProperty(SCXCoreLib::StrToUTF8(name).c_str(), info));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, info.isKey);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_STRING, info.type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, info.exists);
    return SCXCoreLib::StrFromUTF8(std::string(info.value.string));
}

void TestableInstance::GetKey(MI_Uint32 index, std::wstring &name, std::wstring &value, std::wstring errMsg) const
{
    struct PropertyInfo info;
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, FindProperty(index, info, true));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, info.isKey);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_STRING, info.type);
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, info.exists);
    name = info.name;
    value = SCXCoreLib::StrFromUTF8(std::string(info.value.string));
}

std::wstring TestableInstance::GetKeyName(MI_Uint32 index, std::wstring errMsg) const
{
    std::wstring name;
    struct PropertyInfo info;
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, FindProperty(index, info, true));
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, true, info.isKey);
    name = info.name;
    return name;
}

std::wstring TestableInstance::GetKeyValue(MI_Uint32 index, std::wstring errMsg) const
{
    std::wstring name;
    std::wstring value;
    GetKey(index, name, value, CALL_LOCATION(errMsg));
    return value;
}

void TestableContext::Print() const
{
    std::cout<<"------------------------------------------------------------------------------------"<<std::endl;
    std::cout<<"TestableContext size: "<<m_inst.size()<<std::endl;
    for (size_t i = 0; i < m_inst.size(); i++)
    {
        m_inst[i].Print();
    }
}

MI_Result MI_CALL TestableContext::PostResult(MI_Context* context, MI_Result result)
{
    std::map<MI_Context*,TestableContext*>::iterator it = TestableContext::ms_map.find(context);
    CPPUNIT_ASSERT_MESSAGE("Unable to find TestableContext!", TestableContext::ms_map.end() != it);
    it->second->m_result = result;
    it->second->m_wasResultPosted = true;

    return MI_RESULT_OK;
}

MI_Result MI_CALL TestableContext::PostInstance(MI_Context *context, const MI_Instance* instance)
{
    std::map<MI_Context*,TestableContext*>::iterator it = TestableContext::ms_map.find(context);
    CPPUNIT_ASSERT_MESSAGE("Unable to find TestableContext!", TestableContext::ms_map.end() != it);

#if 0
    // If the instance that we have has no function table, give it one
    // (Going through hoops to get rid fo const)
    MI_Instance sourceInstance;
    memcpy(&sourceInstance, instance, sizeof(MI_Instance));

    // Hack to eliminate instance.h need (temporary)
    extern MI_InstanceFT __mi_instanceFT;
    sourceInstance.ft = &__mi_instanceFT;

    MI_Instance *inst;
    CPPUNIT_ASSERT_EQUAL( MI_RESULT_OK, MI_Instance_Clone(&sourceInstance,&inst) );
    it->second->m_inst.push_back( inst );
#else
    // Can we build an Instance out of this, and will we live with multiple instances?
    TestableInstance sourceInst(instance);
    sourceInst.__setCopyOnWrite(true);

    it->second->m_inst.push_back( sourceInst );
#endif

    return MI_RESULT_OK;
}

MI_Result MI_CALL TestableContext::RefuseUnload(MI_Context* context)
{
    std::map<MI_Context*,TestableContext*>::iterator it = TestableContext::ms_map.find(context);
    CPPUNIT_ASSERT_MESSAGE("Unable to find TestableContext!", TestableContext::ms_map.end() != it);
    it->second->m_wasRefuseUnloadCalled = true;

    return MI_RESULT_OK;
}


TestableContext::TestableContext()
    : m_pCppContext(NULL),
      m_result(MI_RESULT_SERVER_IS_SHUTTING_DOWN),
      m_wasResultPosted(false),
      m_wasRefuseUnloadCalled(false)
{
    memset(&m_miContext, 0, sizeof(m_miContext));
    memset(&m_contextFT, 0, sizeof(m_contextFT));
    memset(&m_miPropertySet, 0, sizeof(m_miPropertySet));
    m_pPropertySet = new mi::PropertySet(&m_miPropertySet);

    // Set up the FT table for unit test purposes
    m_contextFT.PostResult = PostResult;
    m_contextFT.PostInstance = PostInstance;
    m_contextFT.RefuseUnload = RefuseUnload;

    m_miContext.ft = &m_contextFT;

    // Be able to find ourselves from the static methods
    TestableContext::ms_map[&m_miContext] = this;
}

TestableContext::~TestableContext()
{
#if 0
    for (std::vector<MI_Instance*>::iterator it = m_inst.begin(); it != m_inst.end(); ++it)
    {
        CPPUNIT_ASSERT_EQUAL( MI_RESULT_OK, MI_Instance_Delete(*it) );
        it = m_inst.erase(it);
    }
#endif

    delete m_pCppContext;
    delete m_pPropertySet;
}

void TestableContext::Reset()
{
    delete m_pCppContext;
    m_pCppContext = NULL;
    m_result = MI_RESULT_SERVER_IS_SHUTTING_DOWN;
    m_wasRefuseUnloadCalled = false;
    m_inst.clear();
}

void TestableContext::WaitForResult()
{
    int count = 480;   // Two minutes worth of 0.25 second sleeps
    while ( ! m_wasResultPosted && --count > 0 )
    {
        SCXCoreLib::SCXThread::Sleep( 250 );
    }
    CPPUNIT_ASSERT_EQUAL(true, (bool) m_wasResultPosted);
}

TestableContext::operator mi::Context&()
{
    delete m_pCppContext;
    m_pCppContext = new mi::Context(&m_miContext);
    return *m_pCppContext;
}

MI_Result FindFieldString(mi::Instance &instance, const char* name, Field* &foundField)
{
    foundField = NULL;

    const MI_Instance* self = instance.GetInstance();
    const MI_ClassDecl* cd = self->classDecl;

    for (MI_Uint32 i = 0; i < cd->numProperties; i++)
    {
        const MI_PropertyDecl* pd = cd->properties[i];
        Field* field = (Field*)((const char*)self + pd->offset);

        // Did we find the field being requested?
        if (0 == strcmp(name, pd->name))
        {
            if (MI_STRING != (static_cast<MI_Type>(pd->type)))
            {
                return MI_RESULT_TYPE_MISMATCH;
            }
            foundField = field;
            return MI_RESULT_OK;
        }
    }

    // Not found.
    return MI_RESULT_NOT_FOUND;
}

void VerifyInstancePropertyNames(const TestableInstance &instance, const std::wstring* expectedPropertiesList,
    size_t expectedPropertiesCnt, std::wstring errMsg)
{
    std::set<std::wstring> expectedProperties(expectedPropertiesList, expectedPropertiesList + expectedPropertiesCnt);

    for (MI_Uint32 i = 0; i < instance.GetNumberOfProperties(); ++i)
    {
        TestableInstance::PropertyInfo info;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, MI_RESULT_OK, instance.FindProperty(i, info));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE + "Property mismatch: " + SCXCoreLib::StrToUTF8(info.name),
            1u, expectedProperties.count(info.name));
    }

    // Be sure that all of the properties in our set exist in the property list.
    for (std::set<std::wstring>::const_iterator iter = expectedProperties.begin();
         iter != expectedProperties.end(); ++iter)
    {
        TestableInstance::PropertyInfo info;
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE + "Missing property: " + SCXCoreLib::StrToUTF8(*iter),
            MI_RESULT_OK, instance.FindProperty((*iter).c_str(), info));
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE + "Property exists flag not set: " + SCXCoreLib::StrToUTF8(*iter),
            true, info.exists);
    }
}

void VerifyInstancePropertyNames(const TestableInstance &instance, const std::wstring* expectedPropertiesList,
    size_t expectedPropertiesCnt, const std::wstring* possiblePropertiesList, size_t possiblePropertiesCnt,
    std::wstring errMsg)
{
    std::vector<std::wstring> propertiesList(expectedPropertiesList, expectedPropertiesList + expectedPropertiesCnt);
    size_t i;
    for (i = 0; i < possiblePropertiesCnt; i++)
    {
        if (instance.PropertyExists(possiblePropertiesList[i].c_str()))
        {
            propertiesList.push_back(possiblePropertiesList[i]);
        }
    }
    VerifyInstancePropertyNames(instance, &propertiesList[0], propertiesList.size(), CALL_LOCATION(errMsg));
}

std::wstring GetFQHostName(std::wstring errMsg)
{
    static std::wstring fqHostName;
    if (fqHostName.empty() != true)
    {
        return fqHostName;
    }
#if defined(sun) || defined(hpux)
    char hostName[MAXHOSTNAMELEN];
#else
    char hostName[HOST_NAME_MAX];
#endif
    CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, 0, gethostname(hostName, sizeof(hostName)));
    hostName[sizeof(hostName) - 1] = 0;

    std::ostringstream processOutput;
    std::ostringstream processErr;
    try
    {
#if defined(linux)
        std::string command = std::string("sh -c \"nslookup -silent ") + hostName + " | grep \'Name:\' | awk \'{print $2}\'\"";
#else
        std::string command = std::string("sh -c \"nslookup ") + hostName + " | grep \'Name:\' | awk \'{print $2}\'\"";
#endif
        std::istringstream processInput;
        int status = SCXCoreLib::SCXProcess::Run(SCXCoreLib::StrFromUTF8(command),
            processInput, processOutput, processErr, 15000);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE, 0, status);
        CPPUNIT_ASSERT_EQUAL_MESSAGE(ERROR_MESSAGE + "; stderr: " + processErr.str(), 0u, processErr.str().size());
        fqHostName = SCXCoreLib::StrTrim(SCXCoreLib::StrFromUTF8(processOutput.str()));
        fqHostName = SCXCoreLib::StrToLower(fqHostName);
    }
    catch(SCXCoreLib::SCXException &e)
    {
        fqHostName.clear();
        CPPUNIT_FAIL(ERROR_MESSAGE + SCXCoreLib::StrToUTF8(L" In GetFQHostName(), executing nslookup threw " +
            e.What() + L" " + e.Where()) + "; stderr: " + processErr.str() + "; stdout: " + processOutput.str());
    }
    return fqHostName;
}

bool MeetsPrerequisites(std::wstring testName)
{
#if defined(aix)
    // No privileges needed on AIX.
    return true;
#elif defined(linux) | defined(hpux) | defined(sun)
    // Most platforms need privileges to execute Update() method.
    if (0 == geteuid())
    {
        return true;
    }

    std::wstring warnText;

    warnText = L"Platform needs privileges to run " + testName + L" test";

    SCXUNIT_WARNING(warnText);
    return false;
#else
#error Must implement method MeetsPrerequisites for this platform
#endif
}

std::wstring GetDistributionName(std::wstring errMsg)
{
    std::wstring distributionName;
#if defined(sun) || defined(aix) || defined(hpux)
    struct utsname utsName;
    CPPUNIT_ASSERT_MESSAGE(ERROR_MESSAGE, 0 <= uname(&utsName));
    distributionName = SCXCoreLib::StrFromUTF8(utsName.sysname);
#elif defined(linux)
#if defined(PF_DISTRO_SUSE)
    distributionName =  L"SuSE Distribution";
#elif defined(PF_DISTRO_REDHAT)
    distributionName =  L"Red Hat Distribution";
#elif defined(PF_DISTRO_ULINUX)
    // We can't generally mock this above the PAL layer, so just say Unknown
    // (We can't determine if we should mimic a RHEL or SuSE installation)
    distributionName =  L"Unknown Linux Distribution";
#endif // defined(PF_DISTRO_SUSE)
#endif // defined(sun) || defined(aix) || defined(HPUX)
    return distributionName;
}

void MakeZombie()
{
    static bool zombieCreated = false;
    if (zombieCreated == false)
    {
        pid_t zombiePid = fork();
        if (zombiePid == 0)
        {
            exit(EXIT_SUCCESS);
        }
        CPPUNIT_ASSERT_MESSAGE("Failed to create zombie process", -1 != zombiePid );
#if defined(linux) && defined(PF_DISTRO_SUSE) && (PF_MAJOR == 9)
        // Suse 9 has kernel older than 2.6.9 so WNOWAIT is not working, special case. We read /proc/[pid]/stat.
        std::stringstream pidStr;
        pidStr << "/proc/" << zombiePid << "/stat";
        unsigned int timeout = 100;
        unsigned int t;
        for (t = 0; t < timeout; t++)
        {
            std::ifstream proc(pidStr.str().c_str());
            // Stream should be good.
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Error opening file " + pidStr.str(), true, proc.good());
            pid_t procPid = 0;
            std::string procName;
            char procState;
            // We read first three fields, usualy something like this:
            // 5555 (processname) Z some other fields ...
            proc >> procPid >> procName >> procState;
            // Stream should still be good.
            CPPUNIT_ASSERT_EQUAL_MESSAGE("Error reading file " + pidStr.str(), true, proc.good());
            // Just a sanity check, does the file name match the pid in the file. It always should.
            CPPUNIT_ASSERT_EQUAL(zombiePid, procPid);
            if (procState == 'Z')
            {
                // Our child process became a zombie process.
                break;
            }
            SCXCoreLib::SCXThread::Sleep(100);
        }
        CPPUNIT_ASSERT_MESSAGE("Timeout while waiting for the zombie to become undead, process file " + pidStr.str(),
            t < timeout);
#else
        siginfo_t info;
        memset(&info, 0, sizeof(info));
        int waitRet = waitid(P_PID, zombiePid, &info, WEXITED|WNOWAIT);
        int savedErrno = errno;
        std::wstringstream errMsg;
        errMsg << L"waitid(pid = " << zombiePid << L")";
        SCXCoreLib::SCXErrnoException e(errMsg.str() ,savedErrno, SCXSRCLOCATION);
        CPPUNIT_ASSERT_EQUAL_MESSAGE("Error waiting for zombie to become undead, " + SCXCoreLib::StrToUTF8(e.What()),
            0, waitRet);
        // Sanity check. Should never really fail since child exits immediately.
        CPPUNIT_ASSERT_EQUAL(static_cast<int>(CLD_EXITED), info.si_code);
#endif
        zombieCreated = true;
    }
}
