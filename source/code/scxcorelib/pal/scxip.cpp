#include <scxcorelib/scxip.h>
#include <arpa/inet.h>
#include <sstream>
#include <scxcorelib/stringaid.h>

namespace SCXCoreLib
{

    //! A function to check if an IPv4 or an IPv6 address is valid.
    //! \param      ipAddress
    //! \returns    1 on success, 0 on failure.  At no point will the address family be anything other than the two possible choices; 

    int IP::IsValidIPAddress(const char *ipAddress)
    {
#if defined (linux)
        struct sockaddr_in sa;
        struct sockaddr_in6 sa6;

        return inet_pton(AF_INET, ipAddress , &(sa.sin_addr)) || inet_pton(AF_INET6, ipAddress, &(sa6.sin6_addr));
#else
//do nothing
        return 0;
#endif

    }

    int IP::IsValidIPAddress(const std::string &ipAddress)
    {
#if defined (linux)
        struct sockaddr_in sa;
        struct sockaddr_in6 sa6;

        return inet_pton(AF_INET, ipAddress.c_str() , &(sa.sin_addr)) || inet_pton(AF_INET6, ipAddress.c_str(), &(sa6.sin6_addr));
#else
//do nothing
        return 0;
#endif
    }

    std::wstring IP::ConvertHexToIpAddress(std::wstring hex)
    {
#if defined (linux)

        std::stringstream ss;        
        ss << StrToUTF8(hex);
        uint32_t val;
        ss  >> std::hex >>  val;
        struct in_addr addr;
        addr.s_addr = htonl(val);
        std::string ipaddress = inet_ntoa(addr);
        return StrFromUTF8(ipaddress);  
#else
//do nothing
        return L"REQUEST_NOT_AVAILABLE";
#endif
    }

    std::wstring IP::ConvertIpAddressToHex(std::wstring ipAddress)
    {
#if defined (linux)
        const unsigned bits_per_term = 8;
        const unsigned num_terms = 4;

        std::istringstream ip(StrToUTF8(ipAddress));
        uint32_t packed = 0;
        for (unsigned int i = 0; i < num_terms; ++i)
        {
            unsigned int term;
            ip >> term;
            ip.ignore();
            packed += term << (bits_per_term * (num_terms-i-1));
        }

        std::wstringstream wss;
        wss << L"00000000" << std::hex << std::uppercase << packed;
        
        int pos = static_cast<uint32_t>(wss.str().length()) - 8;
        return wss.str().substr(pos);
    
#else
//do nothing
        return L"REQUEST_NOT_AVAILABLE";
#endif
}
}
