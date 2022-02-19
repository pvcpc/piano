#include <unistd.h>
#include <app.h>

int
demo()
{
	app_log_info_vvv("Hello, inf0!", 0);
	app_log_info_vv("Hello, inf1!", 0);
	app_log_info_v("Hello, inf2!", 0);
	app_log_info("Hello, inf3!", 0);
	app_log_warn("Hello, warning!", 0);
	app_log_error("Hello, error!", 0);
	app_log_critical("Hello, critical!", 0);
	
	_app_dump_system_journal(STDOUT_FILENO);
	return 0;
}
