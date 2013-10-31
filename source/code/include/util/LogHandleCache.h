/*--------------------------------------------------------------------------------
Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
\file        LogHandleCache.h

\brief       Creates and maintains a cache of log handles. 

\date        07-12-11 02:46:03

\author      Jayashree Singanallur (jayasing)

*/
/*----------------------------------------------------------------------------*/

#ifndef LOGHANDLECACHE_H
#define LOGHANDLECACHE_H

#include <scxcorelib/scxsingleton.h>
#include <scxcorelib/scxthreadlock.h>
#include <scxcorelib/scxlog.h>

#include <string>
#include <map>

namespace SCX
{
    namespace Util
    {

        class LogHandleCache : public SCXCoreLib::SCXSingleton<LogHandleCache>
        {

            // Making the class a friend to enable destruction 
            friend class SCXCoreLib::SCXSingleton<LogHandleCache>;

        public:

            typedef SCXCoreLib::SCXHandle<SCXCoreLib::SCXLogHandle> SCXLogHandlePtr;

            LogHandleCache() : m_cacheLockHandle(SCXCoreLib::ThreadLockHandleGet())
            {}

            ~LogHandleCache()
            {}

            /*----------------------------------------------------------------------------*/
            /**
               Get LogHandle by name

               \param [in]  name   Name of the log handle, typically the module name, 
                                   which should be unique

               \Return             Cached SCXLogHandle to be used for logging

            */
            SCXCoreLib::SCXLogHandle GetLogHandle(const std::string& name);

        private:

            // Internal map for loghandle lookup
            std::map<std::string, SCXLogHandlePtr> m_logHandleMap;

            // Lock for log handle cache
            SCXCoreLib::SCXThreadLockHandle        m_cacheLockHandle;
        };
    }
}

#endif /* LOGHANDLECACHE_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
