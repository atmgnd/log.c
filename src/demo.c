#include "log.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif


int main()
{
#ifdef _MSC_VER
	log_init("D:\\ttmp\\xx.log", 0);
#else
	log_init("./xx.log", 0);
#endif

	log_log(LOG_TRACE, "Hello %s", "world");
	LOGD("%s", "Hello world");

#ifdef _MSC_VER
	system("pause");
#endif

	return 0;
}