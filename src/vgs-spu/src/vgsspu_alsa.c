/* (C)2016, SUZUKI PLAN.
 *----------------------------------------------------------------------------
 * Description: VGS - Sound Processing Unit for ALSA
 *    Platform: Linux
 *      Author: Yoji Suzuki (SUZUKI PLAN)
 *----------------------------------------------------------------------------
 */
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include "vgsspu.h"

struct SPU {
    volatile int alive;
    void* buffer;
    void (*callback)(void* buffer, size_t size);
    pthread_t tid;
    size_t size;
    int sampling;
    int bit;
    int ch;
};

static void* sound_thread(void* context);

void* vgsspu_start(void (*callback)(void* buffer, size_t size))
{
    return vgsspu_start2(22050, 16, 1, 4410, callback);
}

void* vgsspu_start2(int sampling, int bit, int ch, size_t size, void (*callback)(void* buffer, size_t size))
{
    struct SPU* result;
    result = (struct SPU*)malloc(sizeof(strcut SPU));
    if (NULL == result) {
        return NULL;
    }
    result->buffer = malloc(size);
    if (NULL == result->buffer) {
        free(result);
        return NULL;
    }
    result->sampling = sampling;
    result->bit = bit;
    result->ch = ch;
    result->callback = callback;
    result->size = size;
    result->alive = 1;
    if (pthread_create(&result->tid, NULL, sound_thread, NULL)) {
        free(result->buffer);
        free(result);
        return NULL;
    }
    return result;
}

void vgsspu_end(void* context)
{
    struct SPU* c = (struct SPU*)context;
    c->alive = 0;
    pthread_join(c->tid, NULL);
    free(c->buffer);
    free(c);
}

static void* sound_thread(void* context)
{
    struct SPU* c = (struct SPU*)context;
    int div;
    snd_pcm_t* snd;
    snd_pcm_hw_params_t* hwp;
    int rate = c->sampling;
    int periods = 3;
    snd_pcm_hw_params_alloca(&hwp);
    if (snd_pcm_open(&snd, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        return NULL;
    }
    if (snd_pcm_hw_params_any(snd, hwp) < 0) {
        return NULL;
    }
    div = c->bit / 8;
    switch (c->bit) {
        case 8:
            if (snd_pcm_hw_params_set_format(snd, hwp, SND_PCM_FORMAT_U8) < 0) {
                return NULL;
            }
            break;
        case 16:
            if (snd_pcm_hw_params_set_format(snd, hwp, SND_PCM_FORMAT_S16_LE) < 0) {
                return NULL;
            }
            break;
        case 24:
            if (snd_pcm_hw_params_set_format(snd, hwp, SND_PCM_FORMAT_S24_LE) < 0) {
                return NULL;
            }
            break;
        case 32:
            if (snd_pcm_hw_params_set_format(snd, hwp, SND_PCM_FORMAT_S32_LE) < 0) {
                return NULL;
            }
            break;
        default:
            return NULL;
    }
    if (snd_pcm_hw_params_set_rate_near(snd, hwp, &rate, 0) < 0) {
        return NULL;
    }
    if (rate != SAMPLE_RATE) {
        return NULL;
    }
    if (snd_pcm_hw_params_set_channels(snd, hwp, c->ch) < 0) {
        return NULL;
    }
    if (snd_pcm_hw_params_set_periods(snd, hwp, periods, 0) < 0) {
        return NULL;
    }
    if (snd_pcm_hw_params_set_buffer_size(snd, hwp, (periods * c->size) / 4) < 0) {
        return NULL;
    }
    if (snd_pcm_hw_params(snd, hwp) < 0) {
        return NULL;
    }
    while (c->alive) {
        c->callback(c->buffer, c->size);
        while (c->alive) {
            if (snd_pcm_writei(snd, c->buffer, c->size / div) < 0) {
                snd_pcm_prepare(snd);
            } else {
                break;
            }
        }
    }
    snd_pcm_drain(snd);
    snd_pcm_close(snd);
    return NULL;
}
