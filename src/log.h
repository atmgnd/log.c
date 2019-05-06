/**
 * Copyright (c) 2017 rxi
 * Copyright (c) 2019 atmgnd
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#ifndef LOG_H
#define LOG_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

enum { LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL };

#define LOGT(...) log_log(LOG_TRACE, __VA_ARGS__)
#define LOGD(...) log_log(LOG_DEBUG, __VA_ARGS__)
#define LOGI(...)  log_log(LOG_INFO, __VA_ARGS__)
#define LOGW(...)  log_log(LOG_WARN, __VA_ARGS__)
#define LOGE(...) log_log(LOG_ERROR, __VA_ARGS__)
#define LOGF(...) log_log(LOG_FATAL, __VA_ARGS__)

void log_set_level(int level);

void log_init(const char *path, int size);
void log_cleanup();
void log_log(int level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
