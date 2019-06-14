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
#ifdef _MSC_VER
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <sys/timeb.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#endif
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#define strtok_r strtok_s
#define _CLOSE_SOCKET(s) do{if(s != INVALID_SOCKET){shutdown(s, SD_BOTH);closesocket(s);s=INVALID_SOCKET;}}while(0)
#else
typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define _CLOSE_SOCKET(s) do{if(s != INVALID_SOCKET){shutdown(s, SHUT_RDWR);close(s);s=INVALID_SOCKET;}}while(0)
#endif

typedef int(*lsh_fn_inner) (void *, int argc, char **argv);

typedef struct _lsh_cmd {
	char command[16];
	void *fn;
}lsh_cmd;

static struct{
	int level;
	int tel_level;
	FILE *fp;
	int size;
	SOCKET tel_client;
	lsh_cmd *cmd_list;
	int cmd_count;
	lsh_cmd *cmd_list_predefined;
	int cmd_count_predefined;
#ifdef _MSC_VER
	CRITICAL_SECTION lock;
#else
	pthread_mutex_t lock;
#endif
}L = { 0xFF };

typedef struct _inner_cmd_context {
	int auth;
	int exit;
	SOCKET s;
}inner_cmd_context;

static const char *level_names[] = {
  "INV", "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static int log_register_cmd_inner(const char *command, void *fn, int prefined);

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

static int log_auth(const char *auth) {
	return auth && strcmp(auth, "123456") == 0;
}

static int log_cmd_exit(void *p, int argc, char **args) {
	inner_cmd_context *p2 = (inner_cmd_context *)p;
	p2->exit = 1;

	return 0;
}

static int log_cmd_telog(void *p, int argc, char **args) {
	char buf[64];
	lock();
	if (argc > 1) {
		L.tel_client = (atoi(args[1]) ? (((inner_cmd_context *)p)->s): INVALID_SOCKET);
	}
	send(((inner_cmd_context *)p)->s, buf, snprintf(buf, 64, "telog %d\r\n", L.tel_client != INVALID_SOCKET), 0);
	unlock();

	return 0;
}

static int log_cmd_level(void *p, int argc, char **args) {
	char buf[64];
	SOCKET s = ((inner_cmd_context *)p)->s;

	lock();
	if (argc == 2) {
		int level = atoi(args[1]);
		if (level >= LOG_TRACE && level <= LOG_FATAL) {
			L.level = level;
		}
	} else if (argc == 3 && strcmp(args[1], "tel") == 0) {
		int level = atoi(args[2]);
		if (level >= LOG_TRACE && level <= LOG_FATAL) {
			L.tel_level = level;
		}
	}
	send(s, buf, snprintf(buf, 64, "current level: %s/%s, avaiable level:",
		level_names[L.level], level_names[L.tel_level]), 0);
	for (int i = LOG_TRACE; i <= LOG_FATAL; i++) {
		send(s, buf, snprintf(buf, 64, " %s(%d)", level_names[i], i), 0);
	}
	send(s, "\r\n", 2, 0);
	unlock();

	return 0;
}

static int log_cmd_help(void *p, int argc, char **args) {
	inner_cmd_context *p2 = (inner_cmd_context *)p;

	lock();
	send(p2->s, "command list:", 14, 0);
	for (int i = 0; i < L.cmd_count_predefined; ++i) {
		send(p2->s, " ", 1, 0);
		send(p2->s, L.cmd_list_predefined[i].command, strlen(L.cmd_list_predefined[i].command), 0);
	}
	for (int i = 0; i < L.cmd_count; ++i) {
		send(p2->s, " ", 1, 0);
		send(p2->s, L.cmd_list[i].command, strlen(L.cmd_list[i].command), 0);
	}
	send(p2->s, "\r\n", 2, 0);
	unlock();

	return 0;
}

void log_init(const char *path, int size) {
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
	L.fp = fopen(path, "ab+");
	L.size = size;
	L.level = LOG_DEBUG;
	L.tel_level = LOG_TRACE;
	L.tel_client = INVALID_SOCKET;
	L.cmd_list = L.cmd_list_predefined = NULL;
	L.cmd_count = L.cmd_count_predefined = 0;
	log_register_cmd_inner("exit", (void *)log_cmd_exit, 1);
	log_register_cmd_inner("telog", (void *)log_cmd_telog, 1);
	log_register_cmd_inner("level", (void *)log_cmd_level, 1);
	log_register_cmd_inner("help", (void *)log_cmd_help, 1);
}

void log_cleanup() {
	if (L.fp) {
		fclose(L.fp);
	}

	if (L.cmd_list) {
		free(L.cmd_list);
	}

	if (L.cmd_list_predefined) {
		free(L.cmd_list_predefined);
	}

	if (L.level != 0xFF) {
#ifdef _MSC_VER
		DeleteCriticalSection(&L.lock);
#else
		pthread_mutex_destroy(&L.lock);
#endif
	}
}

static int log_register_cmd_inner(const char *command, void *fn, int prefined) {
	lsh_cmd *cmd_list = prefined ? L.cmd_list_predefined : L.cmd_list;
	int cmd_count = prefined ? L.cmd_count_predefined : L.cmd_count;
	int upper_bound;

	if (strlen(command) >= 16) return -1;
	lock();
	cmd_list = (lsh_cmd *)realloc(cmd_list, sizeof(lsh_cmd) * (cmd_count + 1));
	if (cmd_list == NULL) {
		unlock();
		return -1;
	}

	for (upper_bound = 0; upper_bound < cmd_count; upper_bound++) {
		if (strcmp(cmd_list[upper_bound].command, command) < 0) {
			break;
		}
	}

	if (upper_bound < cmd_count) {
		memmove(&cmd_list[upper_bound + 1], &cmd_list[upper_bound], sizeof(lsh_cmd) * (cmd_count - upper_bound));
	}

	strcpy(cmd_list[upper_bound].command, command);
	cmd_list[upper_bound].fn = fn;

	if (prefined) {
		L.cmd_list_predefined = cmd_list;
		L.cmd_count_predefined++;
	} else {
		L.cmd_list = cmd_list;
		L.cmd_count++;
	}

	unlock();
	return 0;
}

int log_register_cmd(const char *command, lsh_fn fn) {
	return log_register_cmd_inner(command, (void *)fn, 0);
}

static void *log_find_cmd(const char *command, int predefined) {
	lsh_cmd *cmd_list = predefined ? L.cmd_list_predefined : L.cmd_list;
	int cmd_count = predefined ? L.cmd_count_predefined : L.cmd_count;
	int left = 0, right = cmd_count - 1;

	while (left <= right) {
		int mid = (left + right) / 2;
		int cr = strcmp(cmd_list[mid].command, command);
		if (cr == 0) {
			return cmd_list[mid].fn;
		} else if (cr > 0) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	return NULL;
}

void log_log(int level, const char *fmt, ...) {
#ifdef _MSC_VER
	struct timeb t;
#else
	struct timeval tv;
#endif
#define _LOG_BUF_SIZE 128
	int ms, log_size, buf_size = _LOG_BUF_SIZE;
	struct tm lt;
	FILE *log2;
	char logbuf[_LOG_BUF_SIZE], *logbuf2 = NULL, *logbuf3 = logbuf;
	va_list args;

	if (level < L.level && (L.tel_client == INVALID_SOCKET || level < L.tel_level)) return;

	lock();
	if (L.size > 0 && L.fp && ftell(L.fp) > L.size) {
#ifdef _MSC_VER
		_chsize_s(_fileno(L.fp), 0);
#else
		ftruncate(fileno(L.fp), 0);
#endif
	}

#ifdef _MSC_VER
	ftime(&t);
	localtime_s(&lt, &t.time);
	ms = t.millitm;
#else
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &lt);
	ms = (int)(tv.tv_usec / 1000);
#endif

	for (;;) {
		log_size = snprintf(logbuf3, buf_size, "%04d-%02d-%02d %02d:%02d:%02d.%03d" " %-5s [%d]: ",
			lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, lt.tm_hour, lt.tm_min, lt.tm_sec, ms,
			level_names[level], thread_id());
		va_start(args, fmt);
		log_size += vsnprintf(logbuf3 + log_size, buf_size - log_size, fmt, args);
		va_end(args);

		if (log_size >= buf_size && logbuf2 == NULL && (logbuf2 = (char *)malloc(log_size + 1)) != NULL) {
			logbuf3 = logbuf2;
			buf_size = log_size + 1;
			continue;
		}

		break;
	}

	if (level >= L.level ) {
		log2 = L.fp ? L.fp : stderr;
		fwrite(logbuf3, 1, log_size, log2);
		fprintf(log2, "\n");
		fflush(log2);
	}

	if (level >= L.tel_level && L.tel_client != INVALID_SOCKET) {
		send(L.tel_client, logbuf3, log_size, 0);
		send(L.tel_client, "\r\n", 2, 0);
	}

	if (logbuf2) {
		free(logbuf2);
	}

	unlock();
}

// Rewrites a C character buffer in-place to remove any telnet command-codes from it
// @param buf Pointer to a buffer of data bytes just recv()'d from the telnet client
// @param buflen The number of valid bytes that (buf) is pointing to
// @returns the number of valid data bytes that (buf) is pointing to after control codes were removed 
static int clean_telnet_buffer(char * buf, int buflen) {
	// persistent state variables, for the case where a telnet command gets split
	// up across several incoming data buffers
	static int _in_subnegotiation = 0;
	static int _command_bytes_left = 0;

	// Based on the document at http://support.microsoft.com/kb/231866
#define TELNET_CHAR_IAC (255)
#define TELNET_CHAR_SB  (250)
#define TELNET_CHAR_SE  (240) 

	char * output = buf;
	for (int i = 0; i < buflen; i++) {
		unsigned char c = buf[i];
		int keep_char = ((c & 0x80) == 0);
		switch (c) {
		case '\0': keep_char = 0;											   break;
		case TELNET_CHAR_IAC: _command_bytes_left = 3;                         break;
		case TELNET_CHAR_SB:  _in_subnegotiation = 1;                          break;
		case TELNET_CHAR_SE:  _in_subnegotiation = 0; _command_bytes_left = 0; break;
		}
		if (_command_bytes_left > 0) { --_command_bytes_left; keep_char = 0; }
		if (_in_subnegotiation) keep_char = 0;
		if (keep_char) *output++ = c;  // strip out any telnet control/escape codes
	}

	return (output - buf);  // return new (possibly shorter) data-buffer length
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/**
   @brief Split a line into tokens (very naively).
   @param line The line.
   @return Null-terminated array of tokens.
 */
static char **lsh_split_line(char *line, int *pargc) {
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char **tokens = (char **)malloc(bufsize * sizeof(char*));
	char *token, *saveptr, **tokens_backup;

	if (!tokens) {
		return NULL;
	}

	token = strtok_r(line, LSH_TOK_DELIM, &saveptr);
	while (token != NULL) {
		tokens[position] = token;
		position++;

		if (position >= bufsize) {
			bufsize += LSH_TOK_BUFSIZE;
			tokens_backup = tokens;
			tokens = (char **)realloc(tokens, bufsize * sizeof(char*));
			if (!tokens) {
				free(tokens_backup);
				return NULL;
			}
		}

		token = strtok_r(NULL, LSH_TOK_DELIM, &saveptr);
	}
	tokens[position] = NULL;

	*pargc = position;
	return tokens;
}

int log_server_loop(int port) {
	SOCKET sock = INVALID_SOCKET, sock2 = INVALID_SOCKET;
	struct sockaddr_in addr;
	int hr = 0, opt = 1;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		return -1;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&opt, sizeof(opt));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0 || listen(sock, 0)) {
		_CLOSE_SOCKET(sock);
		return -1;
	}

	do {
		inner_cmd_context icc = { 0 };
#define BUF_SIZE 256
		char linebuf[BUF_SIZE];
		int buf_len = 0, post_nl = 0, no_nl_len, argc;
		static const char *tel_options =
			"\xFF\xFB\x03"  // WILL Suppress Go Ahead
			"\xFF\xFB\x01"  // WILL Echo
			"\xFF\xFD\x03"  // DO Suppress Go Ahead
			"\xFF\xFE\x01"; // DON't Echo

		sock2 = accept(sock, NULL, 0);
		if (sock2 == INVALID_SOCKET) {
#ifdef _MSC_VER
			int serrno = WSAGetLastError();
			if (serrno == WSAEINTR /*|| serrno == WSAEWOULDBLOCK*/) {
#else
			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
#endif
				continue;
			}
			hr = -1;
			break;
		}

		icc.s = sock2;
		send(sock2, tel_options, strlen(tel_options), 0);
		for (;!icc.exit;) {
			int rcv_len = recv(sock2, linebuf + buf_len, BUF_SIZE - buf_len - 1, 0);
			if (rcv_len == 0) {
				break; // closed
			} else if (rcv_len < 0) {
#ifdef _MSC_VER
				int serrno = WSAGetLastError();
				if (serrno == WSAEINTR || serrno == WSAEWOULDBLOCK) {
#else
				if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
#endif
					continue;
				}
				break;
			}

			rcv_len = clean_telnet_buffer(linebuf + buf_len, rcv_len);

			while (rcv_len) {
				linebuf[buf_len + rcv_len] = '\0';
				if (buf_len == 0 && post_nl) {// remove any prefix '\r' '\n'
					int nls = strspn(linebuf, "\r\n");
					memmove(linebuf, linebuf + nls, rcv_len - nls);
					rcv_len -= nls;
					post_nl = 0;
					continue;
				}

				no_nl_len = strcspn(linebuf + buf_len, "\r\n");
				if (no_nl_len < rcv_len) { // new line
					lock();
					send(sock2, linebuf + buf_len, no_nl_len, 0);  // echo
					send(sock2, "\r\n", 2, 0);
					unlock();
					linebuf[buf_len + no_nl_len] = '\0';

					char **args = lsh_split_line(linebuf, &argc);

					if (!icc.auth) {
						icc.auth = args ? log_auth(args[0]) : 0;
					} else if (args && args[0]) {
						void *f;
						int predefined = 1;
						lock();
						if ((f = log_find_cmd(args[0], 1)) == NULL) {
							predefined = 0;
							f = log_find_cmd(args[0], 0);
						}
						unlock();

						if (f) {
							predefined ? (((lsh_fn_inner)(f))(&icc, argc, args)) : (((lsh_fn)(f))(argc, args));
						} else {
							lock();
							send(sock2, "sunny sunny\r\n", 13, 0);
							unlock();
						}
					}

					free(args);
					memmove(linebuf, linebuf + buf_len + no_nl_len + 1, rcv_len - no_nl_len - 1);
					buf_len = 0;
					rcv_len = rcv_len - no_nl_len - 1;
					post_nl = 1;
				} else {
					lock();
					send(sock2, linebuf + buf_len, rcv_len, 0);  // echo
					unlock();
					buf_len += rcv_len;
					break;
				}
			}
		}

		lock();
		L.tel_client = INVALID_SOCKET;
		unlock();
		_CLOSE_SOCKET(sock2);
	} while (1);

	lock();
	L.tel_client = INVALID_SOCKET;
	unlock();

	_CLOSE_SOCKET(sock2);
	_CLOSE_SOCKET(sock);

	return hr;
}


