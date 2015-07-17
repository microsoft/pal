#include <scxcorelib/scxip.h>
#include <arpa/inet.h>

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
}
