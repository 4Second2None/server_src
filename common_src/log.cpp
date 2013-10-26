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
    buf[1023] = '\0';

    if (NULL == (*f = fopen(buf, "a+"))) {
        fprintf(stderr, "fopen path:%s failed!\n", buf);
        return -1;
    }

    return 0;
}

static pthread_t thread;

static void do_log_item(log *l, LQ_ITEM *item)
{
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
                sleep(10);
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

    /* create thread for writing asynchronously */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int ret = pthread_create(&thread, &attr, log_thread_func, l);
    if (0 != ret) {
        fprintf(stderr, "can't create log thread:%s!\n", strerror(ret));
        return -1;
    }

    strncpy(l->pathname, path, 512);
    l->pathname[511] = '\0';
    l->f = NULL;
    l->level = lv;
    l->ctrl_stdout = ctrl & LOG_CTRL_STDOUT;
    l->ctrl_timestamp = ctrl & LOG_CTRL_TIMESTAMP;
    l->ctrl_thread = ctrl & LOG_CTRL_THREAD;
    l->ctrl_locate = ctrl & LOG_CTRL_LOCATE;
    time_t t = time(NULL);
    l->last_touch = t;
    if (0 != create_file(&l->f, l->pathname, t)) {
        l->run = 0;
        return -1;
    }

    l->run = 1;
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
            item->locate[255] = '\0';
        }
    } else {
        item->locate = NULL;
    }
    if (l->ctrl_thread) {
        item->thread = syscall(SYS_gettid);
    }
    item->context = (char *)malloc(1024);
    if (NULL == item->context) {
        fprintf(stderr, "item->context alloc failed!\n");
        return;
    } else {
        va_list va;
        va_start(va, format);
        vsnprintf(item->context, 1024, format, va);
        va_end(va);
        item->context[1023] = '\0';
    }

    lq_push(&slq, item);
}

log glog;
