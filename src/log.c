/*
 * Copyright (c) 2017 rxi
 * Copyright (c) 2019 atmgnd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "log.h"
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <Windows.h>
#include <sys/timeb.h>  
#else
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sys/time.h>
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

static struct {
	int level;
	FILE *fp;
#ifdef _MSC_VER
	CRITICAL_SECTION lock;
#else
	pthread_mutex_t lock;
#endif
} L = { 0xFF };

static const char *level_names[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static void lock(void) {
#ifdef _MSC_VER
	EnterCriticalSection(&L.lock);
#else
	pthread_mutex_lock(&L.lock);
#endif
}

static void unlock(void) {
#ifdef _MSC_VER
	LeaveCriticalSection(&L.lock);
#else
	pthread_mutex_unlock(&L.lock);
#endif
}

void log_set_level(int level) {
	L.level = level;
}

static inline unsigned int thread_id() {
#ifdef _MSC_VER
	return GetCurrentThreadId();
#else
	return (unsigned int)(syscall(__NR_gettid));
#endif
}

static unsigned int file_size(const char *path) {
#ifdef _MSC_VER
	struct _stat buf;
	if (!_stat(path, &buf)) {
#else
	struct stat buf;
	if (!stat(path, &buf)) {
#endif
		return buf.st_size;
	}

	return 0;
}

void log_init(const char *path, unsigned int size) {
	if (size > 0 && file_size(path) > size) {
		L.fp = fopen(path, "wb+");
	}
	else {
		L.fp = fopen(path, "ab+");
	}

#ifdef _MSC_VER
	InitializeCriticalSection(&L.lock);
#else
	pthread_mutexattr_t attr;
	int type;

	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutexattr_gettype(&attr, &type);
	pthread_mutex_init(&L.lock, &attr);
	pthread_mutexattr_destroy(&attr);
#endif
	L.level = LOG_TRACE;
}

void log_cleanup() {
	if (L.fp) {
		fclose(L.fp);
	}

	if (L.level != 0xFF) {
#ifdef _MSC_VER
		DeleteCriticalSection(&L.lock);
#else
		pthread_mutex_destroy(&L.lock);
#endif
	}
}

void log_log(int level, const char *fmt, ...) {
	if (level < L.level) {
		return;
	}

	lock();

	int ms;
	struct tm lt;

#ifdef _MSC_VER
	struct timeb t;

	ftime(&t);
	localtime_s(&lt, &t.time);
	ms = t.millitm;
#else
	struct timeval tv;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &lt);
	ms = (int)(tv.tv_usec / 1000);
#endif
	FILE *log2 = L.fp ? L.fp : stderr;

	va_list args;
	fprintf(log2, "%04d-%02d-%02d %02d:%02d:%02d.%03d" " %-5s [%d]: ",
		lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, ms,
		level_names[level], thread_id());
	va_start(args, fmt);
	vfprintf(log2, fmt, args);
	va_end(args);
	fprintf(log2, "\n");
	fflush(log2);

	unlock();
}
