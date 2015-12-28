#include <scxcorelib/scxprocess.h>
#include <scxcorelib/scxstream.h>
#include <scxcorelib/stringaid.h>
#include <scxcorelib/scxcmn.h>
#include <scxsystemlib/networkinterfaceconfigurationenumeration.h>
#include <scxcorelib/stringaid.h>
#include <testutils/scxunit.h>
#include <scxcorelib/scxfile.h>
#include <testutils/scxtestutils.h> // For SelfDeletingFilePath
#include <scxcorelib/scxnameresolver.h>
#include <scxsystemlib/scxnetworkadapterip_test.h>

using namespace std;

void getIPAddrfromIfconfig(const wstring &ifName, set<wstring> &IPaddrSet)
{
    std::istringstream streamInstr;
    std::ostringstream streamOutstr;
    std::ostringstream streamErrstr;
    std::string stdoutStr;
    SCXCoreLib::SCXStream::NLFs nlfs;
    vector<wstring> outLines;
    vector<wstring> tokens;
    wstring cmdIfconfig;

#if defined(sun) || defined(aix)
    cmdIfconfig = L"ifconfig -a";
#elif defined(linux)
    cmdIfconfig = L"/sbin/ifconfig -a";
#elif defined(hpux)
    cmdIfconfig = L"ifconfig " + ifName;
#endif

    int procRet = SCXCoreLib::SCXProcess::Run(cmdIfconfig, streamInstr, streamOutstr, streamErrstr, 150000);

    if (procRet == 0 && streamErrstr.str().empty())
    {
        stdoutStr = streamOutstr.str();
        std::istringstream stdInStr(stdoutStr);
        SCXCoreLib::SCXStream::ReadAllLinesAsUTF8(stdInStr, outLines, nlfs);
        
#if defined(linux) || defined(aix)
        bool enterIf = false;
#endif
    
        for(unsigned int i=0; i<outLines.size(); i++)
        {
            StrTokenize(outLines[i], tokens, L" ");
#if defined(hpux)
            /*-------ifconfig lan0 output for hpux -----------
              lan0: flags=1843<UP,BROADCAST,RUNNING,MULTICAST,CKO>
              inet 10.195.173.109 netmask fffffe00 broadcast 10.195.173.255
              lan0: flags=4800841<UP,RUNNING,MULTICAST,PRIVATE,ONLINK>
              inet6 fe80::21c:c4ff:fe39:ff63  prefix 10
              ----------------------------------------------*/
            if (tokens.size() > 1 && 0 == tokens[0].compare(0, 4, L"inet"))
            {
                IPaddrSet.insert(tokens[1]); 
            }
#elif defined(sun)
            /*-------ifconfig -a output for sun machine--------
              lo0: flags=2001000849<UP,LOOPBACK,RUNNING,MULTICAST,IPv4,VIRTUAL> mtu 8232 index 1
              inet 127.0.0.1 netmask ff000000
              net0: flags=1004843<UP,BROADCAST,RUNNING,MULTICAST,DHCP,IPv4> mtu 1500 index 2
              inet 10.217.2.215 netmask fffffe00 broadcast 10.217.3.255
              lo0: flags=2002000849<UP,LOOPBACK,RUNNING,MULTICAST,IPv6,VIRTUAL> mtu 8252 index 1
              inet6 ::1/128
              net0: flags=20002000841<UP,RUNNING,MULTICAST,IPv6> mtu 1500 index 2
              inet6 fe80::214:4fff:fefb:89d3/10
              net0:1: flags=20002080841<UP,RUNNING,MULTICAST,ADDRCONF,IPv6> mtu 1500 index 2
              inet6 2001:4898:e0:3206:214:4fff:fefb:89d3/64
              ---------------------------------------------------*/

            if (tokens.size() > 0 && 0 != tokens[0].compare(0, 3, L"inet")) //The line contains interface name
            {
                vector<wstring> SecTokens;
                wstring ipAddrs; 

                //parsing interface name
                StrTokenize(tokens[0], SecTokens, L":"); 
                wstring interfaceName = SecTokens[0];

                if (interfaceName == ifName) //ip address in next line starting with inet word
                {
                    i++;
                    StrTokenize(outLines[i], tokens, L" ");

                    //parsing the ip address for interface being tested, ip addr is the one next after inet/inet6 word
                    if ((tokens.size() > 1) && (0 == tokens[0].compare(0, 4, L"inet"))) 
                    {
                        StrTokenize(tokens[1], SecTokens, L"/"); //trim off extra stuff
                        ipAddrs = SecTokens[0];
                        IPaddrSet.insert(ipAddrs);
                        // (WI 525683: Exclude blank IP addresses from the expected values.
                        if (ipAddrs.compare(L"::") != 0)
                        {
                            IPaddrSet.insert(ipAddrs);
                        }
                    }
                    else //this should never happen unless ifconfig output changed format
                    {
                        i++;
                    }
                }
            }
#elif defined(linux) 
            /*
              --------- /sbin/ifconfig -a for linux machine ------
              eth0      Link encap:Ethernet  HWaddr 00:15:5D:03:1C:28
              inet addr:10.217.2.89  Bcast:10.217.3.255  Mask:255.255.254.0
              inet6 addr: 2001:4898:e0:3206:215:5dff:fe03:1c28/64 Scope:Global
              inet6 addr: fe80::215:5dff:fe03:1c28/64 Scope:Link
              UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
              RX packets:4465446 errors:0 dropped:0 overruns:0 frame:0
              TX packets:349303 errors:0 dropped:0 overruns:0 carrier:0
              collisions:0 txqueuelen:1000
              RX bytes:2766632992 (2.5 GiB)  TX bytes:51275171 (48.8 MiB)
              Interrupt:9 Base address:0xc000

              lo        Link encap:Local Loopback
              inet addr:127.0.0.1  Mask:255.0.0.0
              inet6 addr: ::1/128 Scope:Host
              UP LOOPBACK RUNNING  MTU:16436  Metric:1
              RX packets:369305 errors:0 dropped:0 overruns:0 frame:0
              TX packets:369305 errors:0 dropped:0 overruns:0 carrier:0
              collisions:0 txqueuelen:0
              RX bytes:2530938162 (2.3 GiB)  TX bytes:2530938162 (2.3 GiB)
              ----------------------------------------------------------
              ----------For RedHat 7, output is slightly different:-----
              ----------------------------------------------------------
              eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
              inet 157.59.133.192  netmask 255.255.252.0  broadcast 157.59.135.255
              inet6 2001:4898:28:4:215:5dff:fea8:840e  prefixlen 64  scopeid 0x0<global>
              inet6 fe80::215:5dff:fea8:840e  prefixlen 64  scopeid 0x20<link>
              ether 00:15:5d:a8:84:0e  txqueuelen 1000  (Ethernet)
              RX packets 9560716  bytes 1411594372 (1.3 GiB)
              RX errors 0  dropped 5  overruns 0  frame 0
              TX packets 107591  bytes 22745425 (21.6 MiB)
              TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

              lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
              inet 127.0.0.1  netmask 255.0.0.0
              inet6 ::1  prefixlen 128  scopeid 0x10<host>
              loop  txqueuelen 0  (Local Loopback)
              RX packets 56598  bytes 324953152 (309.8 MiB)
              RX errors 0  dropped 0  overruns 0  frame 0
              TX packets 56598  bytes 324953152 (309.8 MiB)
              TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
              ----------------------------------------------------------
            */

            if ( tokens.size()== 0 ) //finished one interface
            {
                if (enterIf == false) //interface being tested has not found yet
                {
                    continue;
                }
                else //finished interface section being tested
                {
                    break;
                }
            }

            // Depending on the platform, the interface can look like this:
            //   eth0      Link encap:Ethernet  HWaddr 00:15:5D:AF:04:08
            //             inet addr:10.123.175.62  Bcast:10.123.175.255  Mask:255.255.254.0
            // or this:
            //   eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
            //           inet 10.123.174.81  netmask 255.255.254.0  broadcast 10.123.175.255
            //
            // If tokens[0] contains a ":", sense that

            wstring intSep(L"");
            if ( tokens.size() > 0 && string::npos != tokens[0].find(L":") )
            {
                intSep = L":";
            }

            if (tokens.size() > 0 && tokens[0] == ifName + intSep) //entered the interface section being tested
            {
                if (enterIf == false)
                {
                    enterIf = true;
                }
            }
            else
            {
                if (enterIf == false) //not in the tested interface section, continue searching
                {
                    continue;
                }
                else //inside tested interface section
                {
                    vector<wstring> SecTokens;
                    wstring ipAddrs; 
                    StrTokenize(outLines[i], tokens, L" ");
                    if ((tokens.size() > 1) && (0 == tokens[0].compare(0, 5, L"inet6")))
                    {
                        size_t tokenNum = 1;
                        if ( (tokens.size()) > 2 && (L"addr:" == tokens[tokenNum]) )
                        {
                            tokenNum++;  // Skip this token
                        }
                        StrTokenize(tokens[tokenNum], SecTokens, L"/");
                        ipAddrs = SecTokens[0];
                        IPaddrSet.insert(ipAddrs);
                    }
                    else if ((tokens.size() > 1) && (tokens[0] == L"inet"))
                    {
                        size_t tokenNum = 1;
                        if ( (tokens.size()) > 2 && (L"addr:" == tokens[tokenNum]) )
                        {
                            tokenNum++;  // Skip this token
                        }
                        StrTokenize(StrStripL(tokens[tokenNum], L"addr:"), SecTokens, L"/");
                        ipAddrs = SecTokens[0];
                        IPaddrSet.insert(ipAddrs);
                    }
                }
            }
#elif defined(aix)
            /*-------------aix sample output for command: ifonfig -a -------------
              en0: flags=1e080863,480<UP,BROADCAST,NOTRAILERS,RUNNING,SIMPLEX,MULTICAST,GROUPRT,64BIT,CHECKSUM_OFFLOAD(ACTIVE),CHAIN>
              inet 10.177.118.51 netmask 0xfffffe00 broadcast 10.177.119.255
              inet6 2001:4898:e0:f24e:887a:4eff:feff:9c0b/64
              inet6 3ffe::aaaa:887a:4eff:feff:9c0b/64
              inet6 fe80::887a:4eff:feff:9c0b/64
              lo0: flags=e08084b,c0<UP,BROADCAST,LOOPBACK,RUNNING,SIMPLEX,MULTICAST,GROUPRT,64BIT,LARGESEND,CHAIN>
              inet 127.0.0.1 netmask 0xff000000 broadcast 127.255.255.255
              inet6 ::1%1/128
              tcp_sendspace 131072 tcp_recvspace 131072 rfc1323 1
              ----------------------------------------------------------------*/
            wstring interfaceName = ifName + L":";
            if (tokens.size() > 0)
            {
                unsigned int strSize = tokens[0].size();
                if (0 == tokens[0].compare(strSize-1, 1, L":")) //entering interface section
                {
                    if (tokens[0] == interfaceName)
                    {
                        enterIf = true; 
                    }
                    else
                    {
                        if (enterIf == true) //already visited the interface section being tested
                        {
                            break;
                        }
                    }
                }
                else
                {
                    if (enterIf == false) //not in the tested interface section, continue searching
                    {
                        continue;
                    }

                    //inside tested interface section 
                    vector<wstring> SecTokens;
                    wstring ipAddrs; 
                    StrTokenize(outLines[i], tokens, L" ");
                    if ((tokens.size() > 1) && (0 == tokens[0].compare(0, 5, L"inet6")))
                    {
                        StrTokenize(tokens[1], SecTokens, L"/");
                        ipAddrs = SecTokens[0];
                        IPaddrSet.insert(ipAddrs);
                    }
                    else if ((tokens.size() > 1) && (0 == tokens[0].compare(0, 4, L"inet")))
                    {
                        ipAddrs = tokens[1];
                        IPaddrSet.insert(ipAddrs);
                    }
                }
            }
#endif
        }
    }
    else
    {
        cout << "Command Run failed. The return value is : " << procRet << endl;
        cout << "The ErrorString is : " << streamErrstr.str() << endl;
    }
}
