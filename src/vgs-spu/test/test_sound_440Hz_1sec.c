#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <stdio.h>
#include "vgsspu.h"

#ifdef _WIN32
static void usleep(int usec)
{
    Sleep(usec / 1000);
}
#endif

static unsigned int hz;
static unsigned int pw = 22050;

static void buffering(void* buffer, size_t size)
{
    short* sbuf = (short*)buffer;
    size_t s2 = size >> 1;
    size_t s1;
    for (s1 = 0; s1 < s2; s1++, sbuf++, hz++) {
        if (hz % 440 < 220) {
            *sbuf = (short)((16384 * pw) / 22050);
        } else {
            *sbuf = (short)((-16384 * pw) / 22050);
        }
        if (pw) pw--;
    }
}

int main(int argc, char* argv[])
{
    void* context = vgsspu_start(buffering);
    if (NULL == context) {
        puts("failed");
        return -1;
    }
    usleep(1000000);
    vgsspu_end(context);
    return 0;
}
