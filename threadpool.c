
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "threadpool.h"

// _threadpool is the internal threadpool structure that is
// cast to type "threadpool" before it given out to callers

typedef struct _node{
    void * arg;
    struct _node* next;
    void (*thread_main) (void*);
    
} node;

// you should fill in this structure with whatever you need
typedef struct _threadpool_st {
    
    int threadNumber;
    
    pthread_t *threads;
    
    node* quehead;
    node* quetail;
    
    int quesize;
    
    pthread_mutex_t lockList;
    pthread_cond_t notEmpty;
    pthread_cond_t empty;
    
    int off;
    
} _threadpool;


/* Function executed by all threads */

void* thread_main(threadpool p) {
    
    _threadpool * pool = (_threadpool *) p;
    
    node* workStruct;
    
    while(1) {
        pool->quesize = pool->quesize;
        /* Acquire lock */
        pthread_mutex_lock(&(pool->lockList));
        
        
        while( pool->quesize == 0) {
            if(pool->off) {
                pthread_mutex_unlock(&(pool->lockList));
                pthread_exit(NULL);
            }
            /*get lock */
            pthread_mutex_unlock(&(pool->lockList));
            pthread_cond_wait(&(pool->notEmpty),&(pool->lockList));
            
            if(pool->off) {
                pthread_mutex_unlock(&(pool->lockList));
                pthread_exit(NULL);
            }
        }
        
        workStruct = pool->quehead;
        
        pool->quesize--;
        
        if(pool->quesize == 0) {
            pool->quehead = NULL;
            pool->quetail = NULL;
        }
        else {
            pool->quehead = workStruct->next;
        }
        
        if(pool->quesize == 0 && ! pool->off) {
            pthread_cond_signal(&(pool->empty));
        }
        pthread_mutex_unlock(&(pool->lockList));
        (workStruct->thread_main) (workStruct->arg);
        free(workStruct);
    }
}

threadpool create_threadpool(int num_threads_in_pool) {
    
    // add your code here to initialize the newly created threadpool
    
    
    _threadpool *pool;
    
    // sanity check the argument
    if ((num_threads_in_pool <= 0) || (num_threads_in_pool > MAXT_IN_POOL))
        return NULL;
    
    pool = (_threadpool *) malloc(sizeof(_threadpool));
    if (pool == NULL) {
        fprintf(stderr, "Out of memory creating a new threadpool!\n");
        return NULL;
    }
    
    
    /*Allocating memory for pool */
    pool->threads = (pthread_t*) malloc (sizeof(pthread_t) * num_threads_in_pool);
    
    
    
    /* Mutex and condition variable init. */
    pthread_cond_init(&(pool->notEmpty),NULL);
    
    pthread_cond_init(&(pool->empty),NULL);
    
    pthread_mutex_init(&pool->lockList,NULL);
    
    
    
    /*Initialize structure*/
    pool->threadNumber = num_threads_in_pool;
    pool->quesize = 0;
    pool->quehead = NULL;
    pool->quetail = NULL;
    pool->off = 0;
    
    
    /* Actual creation of threads */
    int i;
    
    for (i = 0; i < num_threads_in_pool;i++) {
        pthread_create(&(pool->threads[i]),NULL,thread_main,pool);
    }
    return (threadpool) pool;
}


void dispatch(threadpool from_me, dispatch_fn dispatch_to_here,
              void *arg) {
    
    // add your code here to dispatch a thread
    
    
    _threadpool *pool = (_threadpool *) from_me; //ya estaba ahi
    
    
    /*Structure Variable*/
    node * workStruct;
    workStruct = (node*) malloc(sizeof(node));
    
    workStruct->thread_main = dispatch_to_here;
    workStruct->arg = arg;
    workStruct->next = NULL;
    
    pthread_mutex_lock(&(pool->lockList));
    
    
    if(pool->quesize == 0) {
        pool->quehead = workStruct;
        pool->quetail = workStruct;
        pthread_cond_signal(&(pool->notEmpty));
        
    } else {
        pool->quetail->next = workStruct;
        pool->quetail = workStruct;
    }
    pool->quesize++;
    
    /* Unlock the queue */
    pthread_mutex_unlock(&(pool->lockList));
}

void destroy_threadpool(threadpool destroyme) {
    // add your code here to kill a threadpool
    
    _threadpool *pool = (_threadpool *) destroyme;
    
    free(pool->threads);
    
    pthread_mutex_destroy(&(pool->lockList));
    pthread_cond_destroy(&(pool->empty));
    pthread_cond_destroy(&(pool->notEmpty));
    return;
}
