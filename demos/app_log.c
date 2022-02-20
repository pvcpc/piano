#include <unistd.h>
#include <app.h>

int
demo()
{
	app_log_info_vvv("Hello, inf0!");
	app_log_info_vv("Hello, inf1!");
	app_log_info_v("Hello, inf2!");
	app_log_info("Hello, inf3!");
	app_log_warn("Hello, warning!");
	app_log_error("Hello, error!");
	app_log_critical("Hello, critical!");
	
	_app_dump_system_journal(STDOUT_FILENO);
	return 0;
}
