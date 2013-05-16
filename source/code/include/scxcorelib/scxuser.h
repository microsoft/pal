/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation.  All rights reserved. 
    
*/
/**
    \file        

    \brief       Defines the user PAL.
    
    \date        2008-03-25 11:22:33

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXUSER_H
#define SCXUSER_H

#include <string>
#if defined(SCX_UNIX)
#include <sys/types.h>
#elif defined(WIN32)
typedef short uid_t; //!< User ID type matching type in stat64 struct.
typedef short gid_t; //!< Group ID type matching type in stat64 struct.
#else
#error "Platform not supported"
#endif

namespace SCXCoreLib
{
/*----------------------------------------------------------------------------*/
/**
    Represents a user ID.    
*/
    typedef uid_t SCXUserID;

/*----------------------------------------------------------------------------*/
/**
    Represents a group ID.    
*/
    typedef gid_t SCXGroupID;

/*----------------------------------------------------------------------------*/
/**
    Represents a user.
*/
    class SCXUser
    {
    public:
        SCXUser();
        SCXUser(SCXUserID uid); 
        virtual ~SCXUser();

        SCXUserID GetUID();
        std::wstring GetName();
        bool IsRoot();

        const std::wstring DumpString() const;
    private:
        SCXUserID m_uid; //!< User ID.
        std::wstring m_name; //!< User name.
        void SetName();
    };
} /* namespace SCXCoreLib*/
#endif /* SCXUSER_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
