#include <stdio.h>
#include "task_property.h"
#include "task_api.h"

#include <time.h>
#include <pthread.h>
#include <unistd.h>



void* client_104(void* arg) {
    while(1) {
        task_pool_fsm();
        usleep(500*1000);
    }
    return NULL;
}


int main()
{
    pthread_t tid1, tid2;
    uint16_t pool[] = {TASK_TEST_0,TASK_TEST_1,TASK_TEST_1,TASK_TEST_0,TASK_POOL_END};
    set_task_pool(pool,5);
    int ret = 0;
    ret = pthread_create(&tid1, NULL, client_104, NULL);
    if(ret != 0) {
        printf("Create 104 client thread error!\n");
        return -1;
    }

    pthread_join(tid1, NULL);
    return 0;
}

