/* basic decoder sample */
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _open open
#define _write write
#define _lseek lseek
#define _close close
#endif
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "vgsdec.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

struct DataHeader {
    char riff[4];
    unsigned int fsize;
    char wave[4];
    char fmt[4];
    unsigned int bnum;
    unsigned short fid;
    unsigned short ch;
    unsigned int sample;
    unsigned int bps;
    unsigned short bsize;
    unsigned short bits;
    char data[4];
    unsigned int dsize;
};

static char* sec(int hz)
{
    static char result[8];
    int s, m;
    s = hz / 22050;
    m = s / 60;
    s %= 60;
    sprintf(result, "%02d:%02d", m, s);
    return result;
}

static void dump(void* context);

int main(int argc, char* argv[])
{
    char buf[1024];
    void* context;
    FILE* wav;
    struct DataHeader dh;

    if (argc < 3) {
        puts("usage: decoder bgm_file wav_file");
        return 1;
    }

    /* initialize context */
    context = vgsdec_create_context();
    if (NULL == context) {
        puts("vgsdec_create_context error.");
        return 255;
    }

    /* load bgm data */
    if (vgsdec_load_bgm_from_file(context, argv[1])) {
        puts("load error.");
        vgsdec_release_context(context);
        return 2;
    }

    /* open wave file */
    wav = fopen(argv[2], "wb");
    if (NULL == wav) {
        puts("open error.");
        vgsdec_release_context(context);
        return 3;
    }

    /* initialize wave header */
    memset(&dh, 0, sizeof(dh));
    strncpy(dh.riff, "RIFF", 4);
    strncpy(dh.wave, "WAVE", 4);
    strncpy(dh.fmt, "fmt ", 4);
    strncpy(dh.data, "data", 4);
    dh.bnum = 16;
    dh.fid = 1;
    dh.ch = 1;
    dh.sample = 22050;
    dh.bps = 44100;
    dh.bsize = 2;
    dh.bits = 16;
    dh.dsize = 0;
    fwrite(&dh, 1, sizeof(dh), wav);

    dump(context);

    /* decoding loop */
    while (vgsdec_get_value(context, VGSDEC_REG_PLAYING) && vgsdec_get_value(context, VGSDEC_REG_LOOP_COUNT) == 0) {
        vgsdec_execute(context, buf, sizeof(buf));
        fwrite(buf, sizeof(buf), 1, wav);
        dh.dsize += sizeof(buf);
    }

    /* waiting for the end of fadeout if looped */
    if (vgsdec_get_value(context, VGSDEC_REG_LOOP_COUNT)) {
        vgsdec_set_value(context, VGSDEC_REG_FADEOUT, 1);
        while (vgsdec_get_value(context, VGSDEC_REG_PLAYING)) {
            vgsdec_execute(context, buf, sizeof(buf));
            fwrite(buf, sizeof(buf), 1, wav);
            dh.dsize += sizeof(buf);
        }
    }

    fclose(wav);
    vgsdec_release_context(context);

    /* update wave header */
    dh.fsize = dh.dsize + sizeof(dh) - 8;
    {
        int fd = _open(argv[2], O_RDWR | O_BINARY);
        if (-1 != fd) {
            _lseek(fd, 0, 0);
            _write(fd, &dh, sizeof(dh));
            _close(fd);
        }
    }
    return 0;
}

static void dump(void* context)
{
    int i, v;
    struct VgsMetaHeader* mhead;
    struct VgsMetaData* mdata;

    /* meta header if exist */
    mhead = vgsdec_get_meta_header(context);
    printf("META-HEADER: ");
    if (NULL != mhead) {
        printf("\n");
        printf(" - format: %s\n", mhead->format);
        printf(" - genre: %s\n", mhead->genre);
        printf(" - data count: %d\n", (int)mhead->num);
    } else {
        printf("n/a\n");
    }

    /* meta data if exist */
    for (i = 0; NULL != (mdata = vgsdec_get_meta_data(context, i)); i++) {
        printf("META-DATA #%d:\n", i + 1);
        printf(" - year: %d\n", (int)mdata->year);
        printf(" - aid: %d\n", (int)mdata->aid);
        printf(" - track: %d\n", (int)mdata->track);
        printf(" - album: %s\n", mdata->album);
        printf(" - song: %s\n", mdata->song);
        printf(" - team: %s\n", mdata->team);
        printf(" - creator: %s\n", mdata->creator);
        printf(" - right: %s\n", mdata->right);
        printf(" - code: %s\n", mdata->code);
    }

    /* length info */
    printf("NUMBER OF NOTES: %d\n", vgsdec_get_value(context, VGSDEC_REG_LENGTH));
    printf("LOOP-INDEX: %d\n", vgsdec_get_value(context, VGSDEC_REG_LOOP_INDEX));
    printf("TIME: %s\n", sec(vgsdec_get_value(context, VGSDEC_REG_TIME_LENGTH)));
    printf("LOOP-START: %s\n", sec(vgsdec_get_value(context, VGSDEC_REG_LOOP_TIME)));

    /* channel volume */
    printf("CHANNEL-VOLUME:");
    for (i = 0; i < 6; i++) {
        vgsdec_set_value(context, VGSDEC_REG_VOLUME_RATE_0 + i, 100);
        if (i) printf(",");
        printf(" CH%d=%d", i, vgsdec_get_value(context, VGSDEC_REG_VOLUME_RATE_0 + i));
    }
    printf("\n");

    /* master volume */
    vgsdec_set_value(context, VGSDEC_REG_VOLUME_RATE, 100);
    printf("MASTER-VOLUME: %d\n", vgsdec_get_value(context, VGSDEC_REG_VOLUME_RATE));

    /* channel mute */
    printf("CHANNEL-MUTE:");
    for (i = 0; i < 6; i++) {
        vgsdec_set_value(context, VGSDEC_REG_MUTE_0 + i, 0);
        /* vgsdec_set_value(context, VGSDEC_REG_MUTE_0 + i, (i == 0 || i == 1) ? 1 : 0); // ex: mute melody */
        if (i) printf(",");
        printf(" CH%d=%d", i, vgsdec_get_value(context, VGSDEC_REG_MUTE_0 + i));
    }
    printf("\n");

    /* scale up/down */
    printf("SCALE:");
    for (i = 0; i < 6; i++) {
        vgsdec_set_value(context, VGSDEC_REG_ADD_KEY_0 + i, -1);
        if (i) printf(",");
        v = vgsdec_get_value(context, VGSDEC_REG_ADD_KEY_0 + i);
        printf(" CH%d=%s%d", i, v < 0 ? "" : "+", v);
    }
    printf("\n");
}
