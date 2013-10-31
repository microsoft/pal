/*------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/
/**
    \file

    \brief     Wql query support related exceptions

    \author    George Yan <georgeya@microsoft.com>

*/
/*----------------------------------------------------------------------------*/

#ifndef SCXWQL_H
#define SCXWQL_H

#include "scxcorelib/scxexception.h"
#include <map>
 

namespace SCXCoreLib
{
    /** Exception for Wql query string parsing failure */
    class SCXParseException : public SCXException {
		public:
			//! Ctor
			SCXParseException(std::wstring reason, 
                                 const SCXCodeLocation& l) : SCXException(l),
                                                             m_Reason(reason)
			{};

			std::wstring What() const { return L"Parsing of the wql query string failed: " + m_Reason;};
		protected:
			//! Description of internal error
			std::wstring   m_Reason;
    };

    /** Exception for Wql query string semantic analysis failure */
    class SCXAnalyzeException : public SCXException {
		public:
			//! Ctor
			SCXAnalyzeException(std::wstring reason, 
                                 const SCXCodeLocation& l) : SCXException(l),
                                                             m_Reason(reason)
			{};

			std::wstring What() const { return L"Semantic analysis of the wql query string failed: " + m_Reason;};
		protected:
			//! Description of internal error
			std::wstring   m_Reason;
    };
}


#endif /* SCXWQL_H */
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
