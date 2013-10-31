/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/

#include <util/LogHandleCache.h>

#include <string>
#include <iostream>

using namespace SCX::Util;

SCXCoreLib::SCXLogHandle LogHandleCache::GetLogHandle(const std::string& name) 
{

    // Acquire the Cache lock
    SCXCoreLib::SCXThreadLock lock(m_cacheLockHandle);

    SCXASSERT(!name.empty());

    // Query the internal map to see if we have a cached handle
    std::map<std::string, SCXLogHandlePtr>::const_iterator iter = m_logHandleMap.find(name);
    SCXLogHandlePtr logHandlePtr;

    // Something is cached
    if (iter != m_logHandleMap.end())
    {
        // Get the logHandle
        logHandlePtr = iter->second;
    }
    else
    {
        // Create a new one and insert that
        logHandlePtr = SCXLogHandlePtr (new SCXCoreLib::SCXLogHandle());        
        *logHandlePtr = SCXCoreLib::SCXLogHandleFactory::GetLogHandle(SCXCoreLib::StrFromUTF8(name));
        m_logHandleMap.insert(std::make_pair<std::string, SCXLogHandlePtr> (name, logHandlePtr));
    }

    return *logHandlePtr;
}
