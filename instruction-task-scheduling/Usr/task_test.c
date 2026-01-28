#include "task_property.h"
#include <stdio.h>
#include <string.h>
#include "task_api.h"

#include <time.h>

static task_id_local_state_t task_id_local_state;
static task_id_manage_t task_id_manage = {
    .retry_cnt = 0,
    .time_cnt = 0,
    .result = false,
};


task_id_property_t task_id_property[] = 
{
    [TASK_TEST_0] = {
        .task_id = TASK_TEST_0,
        .init_info = {
            .time_cnt = 3,
            .retry_cnt = 3,
        },
        .execute = task_id_0_execute,
        .abnormal = task_id_0_abnormal,
        .feedback = task_id_0_feedback,
        .time = task_id_0_time,
    },
    [TASK_TEST_1] = {
        .task_id = TASK_TEST_1,
        .init_info = {
            .time_cnt = 3,
            .retry_cnt = 3
        },
        .execute = task_id_1_execute,
        .abnormal = task_id_1_abnormal,
        .feedback = task_id_1_feedback,
        .time = task_id_1_time,
    },
    
};


task_id_property_t cur_run_task_id;

static void reset_task_id_time(void)
{
    task_id_manage.time_cnt = 0;
}

static void reset_task_id_info(void)
{
    memset(&task_id_manage,0,sizeof(task_id_manage_t));
    memset(&task_id_local_state,0,sizeof(task_id_local_state_t));
    
}

static void make_next_time(uint64_t _time)
{
    task_id_manage.time_cnt = time(NULL) + _time;
}



uint8_t task_id_run_fsm(uint16_t task_id)
{
    uint16_t run_state = task_id_local_state.cur_task_run_state;
    uint8_t result = 0;

    task_id_local_state.cur_execute_id = task_id;

    if(task_id_local_state.cur_task_run_state != TASK_ID_TIME)
    {
        reset_task_id_time();
    }
    
    switch (run_state) {
    case TASK_ID_INFO_LOAD:
        memcpy(&cur_run_task_id,&task_id_property[task_id],sizeof(task_id_property_t));
        run_state = TASK_ID_EXECUTE;
        break;
    case TASK_ID_EXECUTE:
        if(task_id_manage.retry_cnt >= cur_run_task_id.init_info.retry_cnt)
        {
            run_state = TASK_ID_ABNORMAL;
            break;
        }
            
        if(cur_run_task_id.execute())
        {
            task_id_manage.retry_cnt++;
            make_next_time(cur_run_task_id.init_info.time_cnt);
            run_state = TASK_ID_TIME;
        }
        break;
    case TASK_ID_FEEDBACK:
        task_id_manage.result = cur_run_task_id.feedback();
        if(task_id_manage.result)   
        {
            reset_task_id_info();
            result = 1;
            break;
        }
        else
        {
            run_state = TASK_ID_ABNORMAL;
            break;
        }
        break;
    case TASK_ID_TIME:
        if(1)
        {
            run_state = TASK_ID_FEEDBACK; //TODO
            break;
        } 
        task_id_manage.time_cnt = cur_run_task_id.time();
        if(task_id_manage.time_cnt <= (uint64_t)time(NULL))
            run_state = TASK_ID_EXECUTE;
        break;
    case TASK_ID_ABNORMAL:
        cur_run_task_id.abnormal();
        reset_task_id_info();
        result = 2;
        break;
    default:
        break;
    }

    task_id_local_state.cur_task_run_state = run_state;

    return result;
}


uint16_t id_pool[256] = {TASK_POOL_END};
task_pool_local_state_t task_pool_manage = {
    .task_pool_run_state = TASK_POOL_END,
    .id_pool = id_pool,
    .abnormal_id = TASK_POOL_END,
    .pool_id_idx = 0,
};




static void push_tail_pool(uint16_t* id_pool, uint16_t len)
{
    uint16_t id = 0;
    if(task_pool_manage.id_pool == NULL)    return;
    while(*(task_pool_manage.id_pool+id) != TASK_POOL_END)
    {
        id++;
    }
    for(uint16_t i = 0; i<len; i++)
    {
        *(task_pool_manage.id_pool+id+i) = id_pool[i];
    }
    
} 

static void task_pool_remove_done(void)
{
    if(task_pool_manage.id_pool == NULL)    return;
    uint16_t max = (uint16_t)(sizeof(id_pool) / sizeof(id_pool[0]));
    if(max == 0)    return;
    if(task_pool_manage.id_pool[0] == TASK_POOL_END)  return;
    for(uint16_t i = 0; i + 1 < max; i++)
    {
        task_pool_manage.id_pool[i] = task_pool_manage.id_pool[i + 1];
        if(task_pool_manage.id_pool[i] == TASK_POOL_END)  break;
    }
    task_pool_manage.id_pool[max - 1] = TASK_POOL_END;
    task_pool_manage.pool_id_idx = 0;
}

void set_task_pool(uint16_t* id_pool, uint16_t len)
{
    
    task_pool_manage.task_pool_run_state = TASK_ID_INFO_LOAD;
    push_tail_pool(id_pool, len);
    
}   


void task_pool_load_info(void)
{
    task_pool_manage.pool_id_idx = 0;
    reset_task_id_info();
}


uint8_t task_node_execute(void)
{
    
    if(task_pool_manage.id_pool == NULL)    return false;
    uint16_t cur_id = task_pool_manage.id_pool[task_pool_manage.pool_id_idx];
    if(cur_id == TASK_POOL_END)  return false;

    uint8_t result = task_id_run_fsm(cur_id);
    if(result == 1)
    {
        task_pool_remove_done();
        result = 1;
    }
    else if(result == 2)
    {
        task_pool_manage.abnormal_id = cur_id;
    }
    
    return result;
}


bool task_node_abnormal(void)
{
    printf("abnormal node  %d\n",task_pool_manage.abnormal_id);
    return true;
}

void task_pool_fsm(void)
{
    uint16_t run_state = task_pool_manage.task_pool_run_state;
    uint8_t result = 0;
    switch (run_state) {
    case TASK_ID_INFO_LOAD:
        task_pool_load_info();
        run_state = TASK_ID_EXECUTE;
        break;
    case TASK_ID_EXECUTE:  
        result = task_node_execute();
        if(result == 2) run_state = TASK_ID_ABNORMAL;
        break;
    case TASK_ID_ABNORMAL:
        task_node_abnormal();
        break;
    default:
        break;
    }

    task_pool_manage.task_pool_run_state = run_state;
}


