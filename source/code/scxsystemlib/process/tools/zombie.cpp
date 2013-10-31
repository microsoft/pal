/*----------------------------------------------------------------------------
  Copyright (c) Microsoft Corporation. All rights reserved. See license.txt for license information.

*/

// To compile:
// Solaris:
// CC zombie.cpp -o zombie
// Linux (2.6.9 and newer kernels):
// g++ zombie.cpp -o zombie
// Linux (kernels older than 2.6.9):
// g++ zombie.cpp -Dold_linux -o zombie
// AIX:
// xlC zombie.cpp -o zombie
// HPUX:
// aCC zombie.cpp -o zombie

#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<sys/wait.h>
#include<sstream>
#include<fstream>
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Incorrect parameters! Usage:\nzombie LIFETIME_IN_SECONDS\nExample:\nzombie 100\n");
        return -1;
    }
    int lifetime = 0;
    if (sscanf(argv[1], "%i", &lifetime) != 1)
    {
        printf("Could not interpret LIFETIME_IN_SECONDS parameter!\n");
        return -1;
    }
    if (lifetime < 0)
    {
        printf("LIFETIME_IN_SECONDS parameter must be positive!\n");
        return -1;
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        printf("fork() failed!\n");
        return -1;
    }
    if (pid == 0)
    {
//        sleep(5);
        return 0;
    }
#if !defined(old_linux)
    siginfo_t info;
    memset(&info, 0, sizeof(info));
    if (waitid(P_PID, pid, &info, WEXITED|WNOWAIT) != 0)
    {
        printf("waitid() failed!\n");
        return -1;
    }
    // Call for the second time just to verify zombie is still arround after the first call.
    memset(&info, 0, sizeof(info));
    if (waitid(P_PID, pid, &info, WEXITED|WNOWAIT) != 0)
    {
        printf("waitid() failed!\n");
        return -1;
    }
    if (info.si_code == CLD_EXITED)
    {
        printf("Zombie created!\n");
    }
    else
    {
        printf("Failed to create zombie!\n");
        return -1;
    }
#else
    std::stringstream pidStr;
    pidStr << "/proc/" << pid << "/stat";
    unsigned int timeout = 10;
    unsigned int t;
    for (t = 0; t < timeout; t++)
    {
        std::ifstream proc(pidStr.str().c_str());
        pid_t procPid = 0;
        std::string procName;
        char procState;
        proc >> procPid >> procName >> procState;
        if (proc.good() != true)
        {
            printf("Failed to create zombie - error in zombie process file!\n");
            return -1;
        }
        if (procPid != pid)
        {
            printf("Failed to create zombie - error in zombie process file - pid doesn't match!\n");
            return -1;
        }
        if (procState == 'Z')
        {
            printf("Zombie created!\n");
            break;
        }
        sleep(1);
    }
    if (t >= timeout)
    {
        printf("Timed out waiting for the zombie!\n");
        return -1;
    }
#endif
    while (lifetime > 0)
    {
        printf(" zombie lives %i more seconds\n", lifetime);
        sleep(1);
        lifetime--;
    }
    return 0;
}

