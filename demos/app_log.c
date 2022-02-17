#include <unistd.h>
#include <app.h>

int
demo()
{
	app_log_info("Hello, info!");
	app_log_warn("Hello, warning!");
	app_log_error("Hello, error!");
	
	_app_dump_system_journal(STDOUT_FILENO);
	return 0;
}
