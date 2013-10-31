/*--------------------------------------------------------------------------------
    Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.
    
*/
/**
    \file         

    \brief       Implementation of utilities to implement DumpString methods 
    
    \date        2007-12-04 14:20:00    
*/  
/*----------------------------------------------------------------------------*/
 
#include <scxcorelib/scxcmn.h>
#include <scxcorelib/scxdumpstring.h>

 
/*----------------------------------------------------------------------------*/
//! Constructs a DumpStringBuilder for a class
//! \param[in]  classname   Class containing the implemented DumpString     
SCXDumpStringBuilder::SCXDumpStringBuilder(std::string classname) {
    m_stream << classname.c_str() << ":";
}
 
/*----------------------------------------------------------------------------*/
//! Appends a scalar value
//! \param[in]  name    Name (role) of the value
//! \param[in]  value   The value
//! \returns    *this 
SCXDumpStringBuilder &SCXDumpStringBuilder::Text(const std::string &name, const std::wstring &value) {
    m_stream << L" " << name.c_str() << L"='" << value << L"'";
    return *this;
}
  
/*----------------------------------------------------------------------------*/
//! Retrieves the built string
//! \returns    Return value for DumpString
std::wstring SCXDumpStringBuilder::Str() const {
    return m_stream.str();
}

/*----------------------------------------------------------------------------*/
//! Retrieves the built string
//! \returns    Return value for DumpString
SCXDumpStringBuilder::operator std::wstring () const {
    return Str();
}
/*----------------------------E-N-D---O-F---F-I-L-E---------------------------*/
