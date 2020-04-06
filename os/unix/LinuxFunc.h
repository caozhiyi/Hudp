#ifdef __linux__

#ifndef HEADER_OS_UNIX_LINUXFUNC
#define HEADER_OS_UNIX_LINUXFUNC

namespace hudp {

    int SetSocketNoblocking(unsigned int sock);

    int SetReusePort(unsigned int sock);

    void SetCoreFileUnlimit();

}

#endif
#endif // __linux__
