#ifndef uSER_H_INCLUDED
#define uSER_H_INCLUDED

#include <map>

#include <pthread.h>
#include <inttypes.h>

typedef struct
{
    uint64_t id;
    char c;
    pthread_mutex_t lock;
} user_t;

typedef std::map<uint64_t, user_t*> user_map_t;

typedef struct
{
    user_map_t users_;
    pthread_rwlock_t rwlock;
} user_manager_t;

user_t *user_new(uint64_t uid);
void user_free(user_t **user);
user_manager_t *user_manager_new();
void user_manager_free(user_manager_t **user_mgr);

#endif /* uSER_H_INCLUDED */
