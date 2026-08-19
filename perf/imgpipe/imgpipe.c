// Copyright (c) 2026 Fixstars Corporation
// SPDX-License-Identifier: MIT

#define _GNU_SOURCE
#include <inttypes.h>
#include <omp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

static inline uint64_t xs64(uint64_t *s) {
    uint64_t x = *s; x ^= x << 13; x ^= x >> 7; x ^= x << 17; *s = x; return x;
}

static int threads = 4;
static int duration_s = 120;
static int dim = 4096;          /* image side; dim*dim pixels */

static int load_pipe[2];

static void *feeder(void *arg) {
    (void)arg;
    double t0 = now_sec();
    uint8_t tok = 1;
    while (now_sec() - t0 < duration_s) {
        struct timespec ts = {0, 40 * 1000 * 1000};
        nanosleep(&ts, NULL);
        if (write(load_pipe[1], &tok, 1) != 1) break;
    }
    uint8_t done = 0;
    (void)!write(load_pipe[1], &done, 1);
    return NULL;
}

static int load_tile(void) {
    uint8_t tok;
    ssize_t r = read(load_pipe[0], &tok, 1);
    if (r != 1) return 0;
    return tok;
}

static void transform(const uint32_t *src, uint32_t *dst) {
    #pragma omp parallel for schedule(static)
    for (int r = 0; r < dim; r++)
        for (int c = 0; c < dim; c++)
            dst[(size_t)c * dim + r] = src[(size_t)r * dim + c];
}

static uint64_t reduce(const uint32_t *img) {
    uint64_t acc = 0;
    #pragma omp parallel reduction(+ : acc)
    {
        uint64_t h = 0x9e3779b97f4a7c15ULL ^ (uint64_t)omp_get_thread_num();
        uint64_t local = 0;
        #pragma omp for schedule(static)
        for (size_t i = 0; i < (size_t)dim * dim; i++) {
            uint32_t v = img[i];
            if ((v & 0xff) >= 128) {
                h = (h ^ v) * 0x100000001b3ULL;
                h ^= h >> 23;
                local += h & 0x3f;
            } else {
                h = (h + v) * 0x9e3779b97f4a7c15ULL;
                h ^= h << 17;
                local += (h >> 7) & 0x1f;
            }
        }
        acc += local;
    }
    return acc;
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--threads") && i + 1 < argc) threads = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--duration") && i + 1 < argc) duration_s = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--dim") && i + 1 < argc) dim = atoi(argv[++i]);
        else { fprintf(stderr, "unknown arg: %s\n", argv[i]); return 2; }
    }

    omp_set_num_threads(threads);
    size_t npx = (size_t)dim * dim;
    fprintf(stderr, "config: threads=%d  dim=%d (%zu px)  duration=%ds\n",
            threads, dim, npx, duration_s);

    uint32_t *src = malloc(npx * sizeof(uint32_t));
    uint32_t *dst = malloc(npx * sizeof(uint32_t));
    if (!src || !dst) { perror("malloc"); return 1; }

    uint64_t seed = 0x243f6a8885a308d3ULL;
    for (size_t i = 0; i < npx; i++) src[i] = (uint32_t)xs64(&seed);

    if (pipe(load_pipe)) { perror("pipe"); return 1; }
    pthread_t ft;
    pthread_create(&ft, NULL, feeder, NULL);

    fprintf(stderr, "STEADY_STATE_BEGIN\n");
    fflush(stderr);

    double t_start = now_sec();
    uint64_t total = 0, frames = 0;
    while (now_sec() - t_start < duration_s) {
        if (!load_tile()) break;
        transform(src, dst);
        total += reduce(dst);
        frames++;
        fprintf(stderr, "frame %" PRIu64 ": %.1fs total=%" PRIu64 "\n",
                frames, now_sec() - t_start, total);
    }

    fprintf(stderr, "STEADY_STATE_END  frames=%" PRIu64 " total=%" PRIu64 "\n", frames, total);
    pthread_join(ft, NULL);
    free(src); free(dst);
    return 0;
}