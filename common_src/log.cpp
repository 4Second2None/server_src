#include "log.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>


/************************************* log item queue *************************************/

typedef struct log_queue_item LQ_ITEM;
struct log_queue_item {
    int level;
    time_t tm;
    char *locate;
    pthread_t thread;
    char *context;
    size_t sz;
    LQ_ITEM *next;
};

typedef struct log_queue LQ;
struct log_queue {
    LQ_ITEM *head;
    LQ_ITEM *tail;
    pthread_mutex_t lock;
}; 

static LQ slq;

static void lq_init(LQ *lq) {
    pthread_mutex_init(&lq->lock, NULL);
    lq->head = NULL;
    lq->tail = NULL;
}

static void lq_push(LQ *lq, LQ_ITEM *item) {
    item->next = NULL;

    pthread_mutex_lock(&lq->lock);
    if (NULL == lq->tail)
        lq->head = item;
    else
        lq->tail->next = item;
    lq->tail = item;
    pthread_mutex_unlock(&lq->lock);
}

static LQ_ITEM *lq_pop(LQ *lq) {
    LQ_ITEM *item;

    pthread_mutex_lock(&lq->lock);
    item = lq->head;
    if (NULL != item) {
        lq->head = item->next;
        if (NULL == lq->head)
            lq->tail = NULL;
    }
    pthread_mutex_unlock(&lq->lock);

    return item;
}

/************************************* log impl *************************************/

log glog;

static int create_file(FILE **f, const char* path, time_t t)
{
    if (NULL != *f) {
        fclose(*f);
        *f = NULL;
    }

    char buf[1024];
    struct tm *ptm = localtime(&t);
    char timestamp[128];
    strftime(timestamp, 128, "%Y%m%d.log", ptm);
    snprintf(buf, 1024, "%s%s", path, timestamp);

    if (NULL == (*f = fopen(buf, "at+"))) {
        fprintf(stderr, "fopen path:%s failed!\n", buf);
        return -1;
    }

    return 0;
}

static pthread_t thread;

static void do_log_item(log *l, LQ_ITEM *item)
{
    const char *clr = "\033[0m";

    /* color clear */
    const char *lv_color = NULL;
    if (item->level == LOG_LEVEL_FATAL)
        lv_color = "\033[35m";
    else if (item->level == LOG_LEVEL_ERROR)
        lv_color = "\033[31m";
    else if (item->level == LOG_LEVEL_WARN)
        lv_color = "\033[36m";
    else if (item->level == LOG_LEVEL_INFO)
        lv_color = "\033[33m";
    else if (item->level == LOG_LEVEL_DEBUG)
        lv_color = "\033[0m";

    /* time */
    char timestamp[32];
    struct tm *ptm = localtime(&(item->tm));
    strftime(timestamp, 32, "%H:%M:%S ", ptm);

    /* level text */
    const char *lv_text = NULL;
    if (item->level == LOG_LEVEL_FATAL)
        lv_text = "[fatal] ";
    else if (item->level == LOG_LEVEL_ERROR)
        lv_text = "[error] ";
    else if (item->level == LOG_LEVEL_WARN)
        lv_text = "[warn ] ";
    else if (item->level == LOG_LEVEL_INFO)
        lv_text = "[info ] ";
    else if (item->level == LOG_LEVEL_DEBUG)
        lv_text = "[debug] ";

    /* thread */
    char thread_info[32];
    snprintf(thread_info, 32, "[tid=%lu] ", item->thread);

    if (l->ctrl_stdout) {
        if (l->ctrl_locate) {
            char buf[1024];
            snprintf(buf, 1024, "%s%s%s%s%s%s%s%s\n",
                    clr, lv_color, timestamp, lv_text, thread_info, item->locate, item->context, clr);
            printf(buf);
        } else {
            char buf[1024];
            snprintf(buf, 1024, "%s%s%s%s%s%s%s\n",
                    clr, lv_color, timestamp, lv_text, thread_info, item->context, clr);
            printf(buf);
        }
    }

    if (l->ctrl_locate) {
        char buf[1024];
        snprintf(buf, 1024, "%s%s%s%s%s\n",
                timestamp, lv_text, thread_info, item->locate, item->context);
        fwrite(buf, strlen(buf), 1, l->f);
    } else {
        char buf[1024];
        snprintf(buf, 1024, "%s%s%s%s\n",
                timestamp, lv_text, thread_info, item->context);
        fwrite(buf, strlen(buf), 1, l->f);
    }

    /* free */
    if (item->locate)
        free(item->locate);
    if (item->context)
        free(item->context);
    free(item);
}

static void *log_thread_func(void *arg)
{
    log* l = (log *)arg;

    while (l)
    {
        LQ_ITEM *item = lq_pop(&slq);
        if (NULL == item) {
            if (l->run)
                sleep(1);
            else
                break;
        } else {
            do_log_item(l, item);
        }
    }

    return NULL;
}

int log_open(log *l, const char *path, int lv, int ctrl)
{
    if (strlen(path) >= 512) {
        fprintf(stderr, "path too long!\n");
        return -1;
    }

    lq_init(&slq);

    strncpy(l->pathname, path, 512);
    l->f = NULL;
    l->level = lv;
    l->ctrl_stdout = ctrl & LOG_CTRL_STDOUT;
    l->ctrl_locate = ctrl & LOG_CTRL_LOCATE;
    time_t t = time(NULL);
    l->last_touch = t;
    if (0 != create_file(&l->f, l->pathname, t)) {
        l->run = 0;
        return -1;
    }

    l->run = 1;

    /* create thread for writing asynchronously */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int ret = pthread_create(&thread, &attr, log_thread_func, l);
    if (0 != ret) {
        fprintf(stderr, "can't create log thread:%s!\n", strerror(ret));
        return -1;
    }

    return 0;
}

void log_close(log *l)
{
    l->run = 0;
    pthread_join(thread, NULL);
}

void log_write(log *l, int lv, const char *file, int line, const char *func, const char *format, ...)
{
    if (0 == l->run)
        return;

    if (lv < l->level)
        return;

    LQ_ITEM *item = (LQ_ITEM *)malloc(sizeof(LQ_ITEM));
    if (NULL == item) {
        fprintf(stderr, "LQ_ITEM alloc failed!\n");
        return;
    }
    
    item->level = lv;
    item->tm = time(NULL);
    if (l->ctrl_locate) {
        item->locate = (char *)malloc(256);
        if (NULL == item->locate) {
            fprintf(stderr, "item->locate alloc failed!\n");
            free(item);
            return;
        } else {
            snprintf(item->locate, 256, "[%s:%d:%s] ", file, line, func);
        }
    } else {
        item->locate = NULL;
    }
    item->thread = syscall(SYS_gettid);
    item->context = (char *)malloc(1024);
    if (NULL == item->context) {
        fprintf(stderr, "item->context alloc failed!\n");
        return;
    } else {
        va_list va;
        va_start(va, format);
        vsnprintf(item->context, 1024, format, va);
        va_end(va);
    }

    lq_push(&slq, item);
}
