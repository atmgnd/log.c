
// #define LOGC_LOG_LOCATION
#include "log.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif


int main()
{
#ifdef _MSC_VER
	logc_init("D:\\ttmp\\xx.log", 5 * 1024);
#else
	logc_init("./xx.log", 5 * 1024);
#endif

	logc_log(LOGC_TRACE, "%s %s", __DATE__, __TIME__);
	logc_log(LOGC_TRACE, "Hello %s", "world");
	LOGCD("%s", "Hello world");

#ifdef _MSC_VER
	system("pause");
#endif

	return 0;
}
