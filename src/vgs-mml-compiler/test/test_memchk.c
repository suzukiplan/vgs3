#include <stdio.h>
#include <string.h>
#include "vgsmml.h"

int main(int argc, char* argv[])
{
    struct VgsMmlErrorInfo err;
    char foo[4];
    strcpy(foo, "foo");
    if (NULL == vgsmml_compile_from_memory(foo, 3, &err) && VGSMML_ERR_INVALID == err.code) {
        puts(err.message);
        puts("test succeed");
        return 0;
    }
    puts("test failed");
    return -1;
}
