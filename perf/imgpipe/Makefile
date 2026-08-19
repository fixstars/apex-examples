# Copyright (c) 2026 Fixstars Corporation
# SPDX-License-Identifier: Apache-2.0

CC      ?= gcc
CFLAGS  ?= -O2 -g -fno-omit-frame-pointer -fopenmp -Wall -Wextra
LDFLAGS ?= -fopenmp -pthread

imgpipe: imgpipe.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

run: imgpipe
	./imgpipe

clean:
	rm -f imgpipe

.PHONY: run clean