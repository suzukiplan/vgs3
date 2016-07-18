/* (C)2016, SUZUKI PLAN.
 *----------------------------------------------------------------------------
 * Description: VGS - Sound Processing Unit for Open SL/ES
 *    Platform: Android
 *      Author: Yoji Suzuki (SUZUKI PLAN)
 *----------------------------------------------------------------------------
 */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include "vgsspu.h"

struct SL {
    SLObjectItf slEngObj;
    SLEngineItf slEng;
    SLObjectItf slMixObj;
    SLObjectItf slPlayObj;
    SLPlayItf slPlay;
    SLAndroidSimpleBufferQueueItf slBufQ;
};

struct SPU {
    volatile int active;
    struct SL sl;
    volatile int alive;
    void* buffer;
    void (*callback)(void* buffer, size_t size);
    pthread_mutex_t mt;
    size_t size;
    int sampling;
    int bit;
    int ch;
};

static int init_sl(struct SPU* c);
static int init_sl2(struct SPU* c);
static void cb(SLAndroidSimpleBufferQueueItf bq, void* context);
static void lock(struct SPU* c);
static void unlock(struct SPU* c);

void* vgsspu_start(void (*callback)(void* buffer, size_t size))
{
    return vgsspu_start2(22050, 16, 1, 4410, callback);
}

void* vgsspu_start2(int sampling, int bit, int ch, size_t size, void (*callback)(void* buffer, size_t size))
{
    struct SPU* result;

    result = (struct SPU*)malloc(sizeof(struct SPU));
    if (NULL == result) {
        return NULL;
    }
    memset(result, 0, sizeof(struct SPU));
    result->buffer = malloc(size);
    if (NULL == result->buffer) {
        free(result);
        return NULL;
    }
    memset(result->buffer, 0, size);
    result->sampling = sampling;
    result->bit = bit;
    result->ch = ch;
    result->callback = callback;
    result->size = size;
    pthread_mutex_init(&result->mt, NULL);

    if (init_sl(result)) {
        vgsspu_end(result);
        return NULL;
    }
    return result;
}

void vgsspu_end(void* context)
{
    struct SPU* c = (struct SPU*)context;
    lock(c);
    if (c->sl.slBufQ) {
        (*c->sl.slBufQ)->Clear(c->sl.slBufQ);
    }
    if (c->sl.slPlay) {
        (*c->sl.slPlay)->SetPlayState(c->sl.slPlay, SL_PLAYSTATE_STOPPED);
    }
    c->sl.slBufQ = NULL;
    unlock(c);

    if (c->sl.slPlayObj) {
        (*c->sl.slPlayObj)->Destroy(c->sl.slPlayObj);
        c->sl.slPlayObj = NULL;
    }

    if (c->sl.slMixObj) {
        (*c->sl.slMixObj)->Destroy(c->sl.slMixObj);
        c->sl.slMixObj = NULL;
    }

    if (c->sl.slEngObj) {
        (*c->sl.slEngObj)->Destroy(c->sl.slEngObj);
        c->sl.slEngObj = NULL;
    }

    while (c->active) {
        usleep(100);
    }
    pthread_mutex_destroy(&c->mt);
    if (c->buffer) {
        free(c->buffer);
    }
    free(c);
}

static int init_sl(struct SPU* c)
{
    SLresult res;
    const SLInterfaceID ids[1] = {SL_IID_ENVIRONMENTALREVERB};
    const SLboolean req[1] = {SL_BOOLEAN_FALSE};

    res = slCreateEngine(&c->sl.slEngObj, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slEngObj)->Realize(c->sl.slEngObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slEngObj)->GetInterface(c->sl.slEngObj, SL_IID_ENGINE, &c->sl.slEng);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slEng)->CreateOutputMix(c->sl.slEng, &c->sl.slMixObj, 1, ids, req);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slMixObj)->Realize(c->sl.slMixObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    if (init_sl2(c)) {
        return -1;
    }
    return 0;
}

static int init_sl2(struct SPU* c)
{
    SLresult res;
    SLDataFormat_PCM format_pcm = {
        SL_DATAFORMAT_PCM,           /* PCM */
        1,                           /* 1ch */
        SL_SAMPLINGRATE_22_05,       /* 22050Hz */
        SL_PCMSAMPLEFORMAT_FIXED_16, /* 16bit */
        SL_PCMSAMPLEFORMAT_FIXED_16, /* 16bit */
        SL_SPEAKER_FRONT_CENTER,     /* center */
        SL_BYTEORDER_LITTLEENDIAN    /* little-endian */
    };
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataSource aSrc = {&loc_bufq, &format_pcm};
    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, c->sl.slMixObj};
    SLDataSink aSnk = {&loc_outmix, NULL};
    const SLInterfaceID ids[2] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_EFFECTSEND};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    format_pcm.numChannels = c->ch;
    switch (c->sampling) {
        case 8000:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_8;
            break;
        case 11025:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_11_025;
            break;
        case 22050:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_22_05;
            break;
        case 24000:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_24;
            break;
        case 32000:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_32;
            break;
        case 44100:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
            break;
        case 48000:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_48;
            break;
        case 64000:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_64;
            break;
        case 88200:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_88_2;
            break;
        case 96000:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_96;
            break;
        case 192000:
            format_pcm.samplesPerSec = SL_SAMPLINGRATE_192;
            break;
        default:
            return -1;
    }

    switch (c->bit) {
        case 8:
            format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_8;
            format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_8;
            break;
        case 16:
            format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
            format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
            break;
        case 20:
            format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_20;
            format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
            break;
        case 24:
            format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_24;
            format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
            break;
        case 28:
            format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_28;
            format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
            break;
        case 32:
            format_pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_32;
            format_pcm.containerSize = SL_PCMSAMPLEFORMAT_FIXED_32;
            break;
        default:
            return -1;
    }

    res = (*c->sl.slEng)->CreateAudioPlayer(c->sl.slEng, &c->sl.slPlayObj, &aSrc, &aSnk, 2, ids, req);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slPlayObj)->Realize(c->sl.slPlayObj, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slPlayObj)->GetInterface(c->sl.slPlayObj, SL_IID_PLAY, &c->sl.slPlay);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slPlayObj)->GetInterface(c->sl.slPlayObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &c->sl.slBufQ);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slBufQ)->RegisterCallback(c->sl.slBufQ, cb, c);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slPlay)->SetPlayState(c->sl.slPlay, SL_PLAYSTATE_PLAYING);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    res = (*c->sl.slBufQ)->Enqueue(c->sl.slBufQ, c->buffer, c->size);
    if (SL_RESULT_SUCCESS != res) {
        return -1;
    }
    return 0;
}

static void cb(SLAndroidSimpleBufferQueueItf bq, void* context)
{
    struct SPU* c = (struct SPU*)context;
    c->active = 1;
    lock(c);
    if (c->sl.slBufQ) {
        c->callback(c->buffer, c->size);
        (*c->sl.slBufQ)->Enqueue(c->sl.slBufQ, c->buffer, c->size);
    }
    unlock(c);
    c->active = 0;
}

static void lock(struct SPU* c)
{
    pthread_mutex_lock(&c->mt);
}

static void unlock(struct SPU* c)
{
    pthread_mutex_unlock(&c->mt);
}
