#include <iostream>
#include <unistd.h>
#include <sys/select.h>

int main()
{
    int nrOpenDescriptors = 0;
    for (int fd = 3; fd < FD_SETSIZE; ++fd)
    {
        if (-1 != close(fd))
        {
            ++nrOpenDescriptors;
        }
    }

    std::cout << nrOpenDescriptors << std::endl;
}
