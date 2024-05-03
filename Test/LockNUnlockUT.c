#include "BMBase.h"

typedef struct {
    pthread_mutex_t* mtx;
    int i, exitRequest;
} ThreadCtx_t, *ThreadCtx_pt;

void* thread(void* vpThCtx)
{
    ThreadCtx_pt ctx = vpThCtx;
    do {
        pthread_mutex_lock(ctx->mtx);
        printf("ctx->i = %d\n", ctx->i++);
        if (ctx->exitRequest) 
        {
            ctx->exitRequest = 0;
            break;
        }
        pthread_mutex_trylock(ctx->mtx);
    } while (1);
    return vpThCtx;    
}

int LockNUnlockUT()
{
    BMStatus_t status = BMSTATUS_SUCCESS;
    pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_t th = {0};
    ThreadCtx_t ctx = { &mtx, 0, 0 };
    void* thret;
    do {
        pthread_mutex_trylock(&mtx);
        pthread_create(&th, NULL, thread, &ctx);
        for (int i = 0; i < 5; i++)
        {
            sleep(1);
            pthread_mutex_unlock(&mtx);
        }
        ctx.exitRequest = 1;
        pthread_mutex_unlock(&mtx);
        pthread_join(th, &thret);
        printf("ctx.i = %d\n", ctx.i);
    } while (0);
    pthread_mutex_destroy(&mtx);
    BMEND_FUNCEX(status);
    return status ? EXIT_FAILURE : EXIT_SUCCESS;
}