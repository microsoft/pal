// To compile:
// Solaris:
// CC zombie.cpp -o zombie
// Linux:
// g++ zombie.cpp -o zombie
// AIX:
// xlC zombie.cpp -o zombie
// HPUX:
// aCC zombie.cpp -o zombie
#include<unistd.h>
#include<stdio.h>
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
        return -1;
    }
    if (pid == 0)
    {
        return 0;
    }
    while (lifetime > 0)
    {
        printf(" zombie lives %i more seconds\n", lifetime);
        sleep(1);
        lifetime--;
    }
    return 0;
}

