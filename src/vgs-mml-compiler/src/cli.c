#include <stdio.h>
#include "vgsmml.h"

int main(int argc, char* argv[])
{
    FILE* fp;
    struct VgsBgmData* bgm;
    struct VgsMmlErrorInfo err;
    if (argc < 3) {
        puts("usage: vgs2mml mml-file bgm-file");
        return 1;
    }
    bgm = vgsmml_compile_from_file(argv[1], &err);
    if (NULL == bgm) {
        if (err.line) {
            printf("error(%d) line=%d: %s\n", err.code, err.line, err.message);
        } else {
            printf("error(%d) %s\n", err.code, err.message);
        }
        return 2;
    }
    printf("bgm-size: %ld\n", bgm->size);
    fp = fopen(argv[2], "wb");
    if (NULL == fp) {
        puts("file open error.");
        return 3;
    }
    if (bgm->size != fwrite(bgm->data, 1, bgm->size, fp)) {
        puts("file write error.");
        fclose(fp);
        return 4;
    }
    fclose(fp);
    vgsmml_free_bgm_data(bgm);
    return 0;
}
