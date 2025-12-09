#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

int main()
{
    // Show current RLIMIT_MEMLOCK for context
    struct rlimit limit {};
    if (getrlimit(RLIMIT_MEMLOCK, &limit) == 0)
    {
        std::cout << "RLIMIT_MEMLOCK: soft=" << limit.rlim_cur
                  << " bytes, hard=" << limit.rlim_max << " bytes\n";
    }
    else
    {
        std::cerr << "getrlimit() failed: " << std::strerror(errno) << "\n";
    }

    // Attempt to lock all current and future mappings
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
    {
        int e = errno;
        std::cerr << "mlockall() failed: " << std::strerror(e) << "\n";

        if (e == ENOMEM)
        {
            std::cerr
                << "Cause: Your process attempted to lock more memory than "
                   "allowed by RLIMIT_MEMLOCK.\n"
                << "Fix: Increase the limit in the shell before running:\n"
                << "    ulimit -l <kilobytes>\n"
                << "Or adjust /etc/security/limits.conf for a permanent fix.\n";
        }
        else if (e == EPERM)
        {
            std::cerr
                << "Cause: Insufficient privileges to lock memory.\n"
                << "Fix: Your user may need CAP_IPC_LOCK or membership in a "
                   "realtime group.\n";
        }

        return 1;
    }

    std::cout << "mlockall() succeeded. All current and future memory is locked.\n";
    std::cout << "Your system should not swap out this process.\n";

    // Keep the program alive briefly so the user can run vmstat in another terminal.
    std::cout << "Sleeping for 10 seconds — check 'vmstat 1' in another terminal.\n";
    sleep(10);

    // Optional: unlock (you normally don't do this in realtime code)
    munlockall();
    return 0;
}

