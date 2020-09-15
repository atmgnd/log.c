/**
 * Copyright (c) 2017 rxi
 * Copyright (c) 2019 atmgnd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOGC_H
#define LOGC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

#ifdef LOGC_LOG_LOCATION
#define LOGCSTRIY(x) #x
#define LOGC2STR(x) LOGCSTRIY(x)
#define LOGC_LOCATION __FILE__ ":" LOGC2STR(__LINE__) " "
#else
#define LOGC_LOCATION
#endif

enum { LOGC_TRACE, LOGC_DEBUG, LOGC_INFO, LOGC_WARN, LOGC_ERROR, LOGC_FATAL };

#define LOGCT(...) logc_log(LOGC_TRACE, LOGC_LOCATION __VA_ARGS__)
#define LOGCD(...) logc_log(LOGC_DEBUG, LOGC_LOCATION __VA_ARGS__)
#define LOGCI(...) logc_log(LOGC_INFO,  LOGC_LOCATION __VA_ARGS__)
#define LOGCW(...) logc_log(LOGC_WARN,  LOGC_LOCATION __VA_ARGS__)
#define LOGCE(...) logc_log(LOGC_ERROR, LOGC_LOCATION __VA_ARGS__)
#define LOGCF(...) logc_log(LOGC_FATAL, LOGC_LOCATION __VA_ARGS__)

void logc_set_level(int level);

void logc_init(const char *path, int size);
void logc_cleanup();
void logc_log(int level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
