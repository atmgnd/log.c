#include "log.h"

#ifdef _MSC_VER
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <process.h>
#else
#include <pthread.h>
#endif

static int log_cmd_echo(int argc, char **args) {
	for (int i = 0; i < argc; i++) {
		LOGD("echo argv[%d]: %s", i, args[i]);
	}

	return 0;
}

static int log_cmd_test(int argc, char **args) {
	LOGT("test");
	LOGD("test");
	LOGI("test");
	LOGW("test");
	LOGE("test");
	LOGF("test");

	return 0;
}

#ifdef _MSC_VER
static void
#else
static void *
#endif
test_print_thread(void *c)
{
	static int count = 0;
	for(;;)
		LOGT("thread %d, count %d xx", *(int *)c, count++);
#ifndef _MSC_VER
	return NULL;
#endif
}

int main(int argc, char **argv)
{
#ifdef _MSC_VER
	WORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;

	if (WSAStartup(wVersionRequested, &wsaData) != 0 &&
		LOBYTE(wsaData.wVersion) != 2 &&
		HIBYTE(wsaData.wVersion) != 2)	{
		WSACleanup();
	}

	log_init("D:\\ttmp\\xx.log", 500 * 1024 * 1024);
#else
	log_init("./xx.log", 5 * 1024);
#endif

	log_log(LOG_TRACE, "Hello %s", "world");
	LOGD("%s", "Hello world2");

	log_register_cmd("echo", log_cmd_echo);
	log_register_cmd("test", log_cmd_test);

	int i = 0, j = 1, k = 2;
#ifdef _MSC_VER
	_beginthread(test_print_thread, 0, &i);
	_beginthread(test_print_thread, 0, &j);
	_beginthread(test_print_thread, 0, &k);
#else
	pthread_t pt0, pt1, pt2;
	pthread_create(&pt0, NULL, test_print_thread, &i);
	pthread_create(&pt1, NULL, test_print_thread, &j);
	pthread_create(&pt2, NULL, test_print_thread, &k);
#endif

	log_server_loop(5000);

	return 0;
}