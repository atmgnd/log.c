/**
 * Copyright (c) 2017 rxi
 * Copyright (c) 2019 atmgnd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <stdarg.h>

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

void log_set_fp(FILE *fp);
void log_set_level(int level);

void log_init(const char *path, unsigned int size);
void log_log(int level, const char *fmt, ...);

#endif
