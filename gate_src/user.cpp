#include "user.h"

#include <stdio.h>
#include <stdlib.h>

user_t *user_new(uint64_t uid)
{
    user_t *user = new user_t;
    if (NULL == user) {
        fprintf(stderr, "user_t alloc failed!\n");
        return NULL;
    }

    user->id = uid;
    pthread_mutex_init(&user->lock, NULL);
    return user;
}

void user_free(user_t **user)
{
    delete *user;
    *user = NULL;
}

user_manager_t *user_manager_new()
{
    user_manager_t * user_mgr = new user_manager_t;
    if (NULL == user_mgr) {
        fprintf(stderr, "user_manager_t alloc failed!\n");
        return NULL;
    }

    pthread_rwlock_init(&user_mgr->rwlock, NULL);
    return user_mgr;
}

void user_manager_free(user_manager_t **user_mgr)
{
    delete *user_mgr;
    *user_mgr = NULL;
}
