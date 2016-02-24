#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include "debug.h"

static e_level module_level[MODULE_MAX];

static FILE *log_ptr = NULL;
static int gMaxLogFileSize = 0;
static const char *gFileName = NULL;

static const char *module_name[] = {
	"MODULE_NET",
	"MODULE_DB",
	"MODULE_COMMON",
	"MODULE_MAX"
};

static const char *level_name[] = {
	"DEBUG",
	"INFO",
	"ERROR"
};

static int check_reset_file(FILE *log_p) {
	long fileSize = 0;
	
	if ((fileSize = ftell(log_p)) == -1) {
		fclose(log_p);
		if ((log_p = fopen(gFileName, "a+")) == NULL) {
			return -1;
		}
	}
	if (fileSize >= gMaxLogFileSize) {
		char file_old[64] = { 0 };
		snprintf(file_old, sizeof(file_old)-1, "cp -f %s %s.old", gFileName, gFileName);
		system(file_old);
		if(truncate(gFileName, 0) != 0) {
			fclose(log_p);
			if ((log_p = fopen(gFileName, "a+")) == NULL) {
				return -1;
			}
		}
	}
	
	return 0;
}

e_level get_debug_level(const e_module mod) {
	return module_level[mod];
}
void set_debug_level(const e_module mod, const e_level level) {
	if(mod < MODULE_MIN || mod >= MODULE_MAX || level < DEBUG || level > ERROR) {
		fprintf(stderr, "set_debug_level invalid argument.\n");
		return;
	}

	module_level[mod] = level;

	return;
}
void close_debug(const e_module mod) {
	if(mod < MODULE_MIN || mod >= MODULE_MAX) {
		fprintf(stderr, "close_debug invalid argument.\n");
		return;
	}

	module_level[mod] = DEFAULT_LEVEL;
}

void log_init(const char *file, const int size, e_level level) {
	int mod;
	
	if (file == NULL || size < 0) {
		fprintf(stderr, "invalid argument.\n");
		return ;
	}
	
	gMaxLogFileSize = size;
	gFileName = file;
	if (log_ptr != NULL) {
		return;
	}else if ((log_ptr = fopen(gFileName, "a+")) == NULL) {
		fprintf(stderr, "Can't open log file[%s].\n", gFileName);
		return;
	}

	for (mod = MODULE_MIN; mod < MODULE_MAX; mod ++) {
		LOG_OPEN(((e_module)mod), level);
	}
	
	return;
}
void log_close() {
	int mod;

	if (log_ptr) {
		fclose(log_ptr);
		log_ptr = NULL;
	}

	for (mod = MODULE_MIN; mod < MODULE_MAX; mod ++) {
		LOG_CLOSE(((e_module)mod));
	}
}

void debug_log(const e_module mod, const e_level level, const char *file, const int line, const char *format, ...) {
    char buf[1024] = { 0 };
	char log_str[1024] = { 0 };
	char now_readable[32] = { 0 };
    va_list ap;
	pthread_t thread_id;
	time_t now;
	struct tm *now_tm;
	
	now = time(NULL);
	now_tm = localtime(&now);
	sprintf(now_readable, "%04d-%02d-%02d %02d:%02d:%02d",
		now_tm->tm_year+ 1900,
		now_tm->tm_mon + 1, 
		now_tm->tm_mday,
		now_tm->tm_hour,
		now_tm->tm_min, 
		now_tm->tm_sec);
	
	thread_id = pthread_self();
	snprintf(log_str, sizeof(log_str)-1, "%s [%13s:%5s]\tthread:%lu %20s:%d\t%s\n", now_readable, module_name[mod], level_name[level], thread_id, file, line, format);

    va_start(ap, format);
    if (vsnprintf(buf, sizeof(buf)-1, log_str, ap) == -1) {
        buf[sizeof(buf) - 1] = '\0';
    }
    va_end(ap);

	if (check_reset_file(log_ptr) != 0) {
		fprintf(stderr, "check_reset_file failed.\n");
		return;
	}

	if (log_ptr) {
		fwrite(buf, strlen(buf), 1, log_ptr);
		fflush(log_ptr);
        if (get_debug_level(mod) == DEBUG) {
            printf(buf);
        }
	}

	return;
}
