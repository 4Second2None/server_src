#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <stdio.h>
#include <time.h>

#define LOG_LEVEL_FATAL 5
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_DEBUG 1

#define LOG_CTRL_STDOUT 1
#define LOG_CTRL_THREAD 2
#define LOG_CTRL_LOCATE 4

#define LOG_OPEN(path, lv, ctrl) log_open(&glog, path, lv, ctrl)
#define LOG_CLOSE() log_close(&glog)

#define mfatal(format, ...) log_write(&glog, LOG_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define merror(format, ...) log_write(&glog, LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define mwarn(format, ...) log_write(&glog, LOG_LEVEL_WARN, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define minfo(format, ...) log_write(&glog, LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
#define mdebug(format, ...) log_write(&glog, LOG_LEVEL_DEBUG, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)

typedef struct {
    volatile int run;
    char pathname[512];
    FILE *f;
    time_t last_touch;
    int level;
    unsigned ctrl_stdout:1;
    unsigned ctrl_locate:1;
} log;

int log_open(log *l, const char *path, int lv, int ctrl);
void log_close(log *l);
void log_write(log *l, int lv, const char *file, int line, const char *func, const char *format, ...);

extern log glog;

#endif /* LOG_H_INCLUDED */
