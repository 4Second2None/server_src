#include "user.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

static void create_thread(pthread_t *thread, void *(*func)(void *), void *);
static void* produce_user(void *);
static void* user_do(void *);
static void* consume_user(void *);

int main(int argc, char **argv)
{
    srand((int)time(NULL));

    user_manager_t *user_mgr = user_manager_new();
    if (NULL == user_mgr) {
        fprintf(stderr, "user_manager_new() failed!\n");
        return 1;
    }

    pthread_t produce_user_thread;
    create_thread(&produce_user_thread, produce_user, user_mgr);

    pthread_t user_do_thread;
    create_thread(&user_do_thread, user_do, user_mgr);

    pthread_t consume_user_thread;
    create_thread(&consume_user_thread, consume_user, user_mgr);

    pthread_join(produce_user_thread, NULL);
    pthread_join(user_do_thread, NULL);
    pthread_join(consume_user_thread, NULL);

    user_manager_free(&user_mgr);

    return 0;
}

static void create_thread(pthread_t *thread, void *(*func)(void *), void *arg)
{
    pthread_attr_t attr;
    int ret;

    pthread_attr_init(&attr);

    if ((ret = pthread_create(thread, &attr, func, arg)) != 0) {
        fprintf(stderr, "can't create thread:%s\n", strerror(ret));
        exit(1);
    }
}

static void* produce_user(void *arg)
{
    user_manager_t *user_mgr = (user_manager_t *)arg;

    while (1)
    {
        uint64_t uid = (uint64_t)rand() % 10000 + 1;

        pthread_rwlock_wrlock(&user_mgr->rwlock);
        user_map_t::iterator itr = user_mgr->users_.find(uid);
        if (itr == user_mgr->users_.end())
        {
            user_t *user = user_new(uid);
            if (NULL != user) {
                pthread_mutex_lock(&user->lock);
                user_mgr->users_.insert(user_map_t::value_type(uid, user));
                pthread_mutex_unlock(&user->lock);
            }
        }
        pthread_rwlock_unlock(&user_mgr->rwlock);
    }

    return NULL;
}

static void* user_do(void *arg)
{
    user_manager_t *user_mgr = (user_manager_t *)arg;

    while (1)
    {
        uint64_t uid = (uint64_t)rand() % 10000 + 1;

        pthread_rwlock_rdlock(&user_mgr->rwlock);
        user_map_t::iterator itr = user_mgr->users_.find(uid);
        if (itr != user_mgr->users_.end())
        {
            user_t *user = itr->second;
            pthread_mutex_lock(&user->lock);
            user->c = 'd';
            pthread_mutex_unlock(&user->lock);
        }
        pthread_rwlock_unlock(&user_mgr->rwlock);
    }

    return NULL;
}

static void* consume_user(void *arg)
{
    user_manager_t *user_mgr = (user_manager_t *)arg;

    while (1)
    {
        uint64_t uid = (uint64_t)rand() % 10000 + 1;

        pthread_rwlock_wrlock(&user_mgr->rwlock);
        user_map_t::iterator itr = user_mgr->users_.find(uid);
        if (itr != user_mgr->users_.end())
        {
            user_t *user = itr->second;
            pthread_mutex_lock(&user->lock);
            user_mgr->users_.erase(itr);
            pthread_mutex_unlock(&user->lock);
            user_free(&user);
        }
        pthread_rwlock_unlock(&user_mgr->rwlock);
    }

    return NULL;
}
