/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief       Definitions for an stdout scxlog backend.

    \date        2008-08-05 13:09:45

*/
/*----------------------------------------------------------------------------*/
#ifndef SCXLOGSTDOUTBACKEND_H
#define SCXLOGSTDOUTBACKEND_H

#include "scxlogbackend.h"
#include <scxcorelib/scxprocess.h>

namespace SCXCoreLib
{
    /*----------------------------------------------------------------------------*/
    /**
        Simple stdout backend.
    */
    class SCXLogStdoutBackend : public SCXLogBackend
    {
    public:
        SCXLogStdoutBackend();

        virtual ~SCXLogStdoutBackend();

        virtual void SetProperty(const std::wstring& key, const std::wstring& value);
        virtual bool IsInitialized() const;

    private:
        void DoLogItem(const SCXLogItem& item);
        const std::wstring Format(const SCXLogItem& item) const;
    };

} /* namespace SCXCoreLib */
#endif /* SCXLOGSTDOUTBACKEND_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
