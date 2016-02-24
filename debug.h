
#ifndef __DEBUG_HEAD__
#define __DEBUG_HEAD__

#include <stdio.h>

typedef enum {
	MODULE_MIN = 0,
	MODULE_NET = MODULE_MIN,
	MODULE_DB,
	MODULE_COMMON,
	MODULE_MAX
}e_module;

typedef enum {
	DEBUG = 0,
	INFO,
	ERROR
}e_level;

#define CONFIG_DEBUG    (1)
#define DEFAULT_LEVEL   DEBUG

void log_init(const char *file, const int size, e_level level);
void log_close();
void debug_log(const e_module mod, const e_level level, const char *file, const int line, const char *format, ...);

e_level get_debug_level(const e_module mod);
void set_debug_level(const e_module mod, const e_level level);
void close_debug(const e_module mod);

#ifdef CONFIG_DEBUG
#define LOG_OPEN(module, level) do{\
	set_debug_level(module, level);\
}while(0);

		
#define LOG_CLOSE(module) do{\
	close_debug(module);\
}while(0);

#define LOG_DEBUG(module, format, args...) do{\
	e_level level;\
	if (module < MODULE_MIN || module >= MODULE_MAX) {\
		fprintf(stderr, "invalig arg.\n");\
	} else if ((level = get_debug_level(module)) == DEBUG) {\
		debug_log(module, DEBUG, __FILE__, __LINE__, format, ##args);\
	}\
}while(0);

#define LOG_INFO(module, format, args...) do{\
	if (module < MODULE_MIN || module >= MODULE_MAX) {\
		fprintf(stderr, "invalig arg.\n");\
	} else {\
		e_level level = get_debug_level(module);\
		if ((level <= INFO) &&  (level >= DEBUG) ) {\
			debug_log(module, INFO, __FILE__, __LINE__, format, ##args);\
		}\
	}\
}while(0);

#define LOG_ERROR(module, format, args...) do{\
	if (module < MODULE_MIN || module >= MODULE_MAX) {\
		fprintf(stderr, "invalig arg.\n");\
	} else {\
		e_level level = get_debug_level(module);\
		if ((level <= ERROR )  && (level >= DEBUG) ) {\
			debug_log(module, ERROR, __FILE__, __LINE__, format, ##args);\
		}\
	}\
}while(0);

#else
#define LOG_OPEN(module, level)
#define LOG_CLOSE(module)
#define LOG_DEBUG(module, format, args...)
#define LOG_INFO(module, format, args...)
#define LOG_ERROR(module, format, args...)
#endif

#endif /*__DEBUG_HEAD__*/

