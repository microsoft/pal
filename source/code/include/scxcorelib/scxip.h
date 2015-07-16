#include <string>

namespace SCXCoreLib
{
    class IP
    {
    public:
        static int IsValidIPAddress(const char *ipAddress);
        static int IsValidIPAddress(const std::string &ipAddress);
        static std::wstring ConvertIpAddressToHex(std::wstring ipAddress);
        static std::wstring ConvertHexToIpAddress(std::wstring hex);
    };
}
