#include <stdint.h>
#include <stdbool.h>


typedef enum
{
    TASK_ID_INFO_LOAD = 0,
    TASK_ID_EXECUTE,
    TASK_ID_FEEDBACK,
    TASK_ID_ABNORMAL,
    TASK_ID_TIME,
}TASK_ID_RUN_FSM;


typedef enum
{
    TASK_TEST_0 = 0,
    TASK_TEST_1,
    TASK_POOL_END,
}TASK_ID_NUM;


typedef struct
{
    uint64_t time_cnt;
    uint16_t retry_cnt;
    bool result;
}task_id_manage_t;


typedef struct
{
    uint16_t task_id;
    task_id_manage_t init_info;
    bool (*execute)(void);  //任务的执行函数--例如发送消息
    bool (*feedback)(void); //任务执行后返回信息的处理--例如指令解析
    bool (*abnormal)(void); //任务的异常处理函数--例如等待超时的处理
    uint64_t (*time)(void);     //任务等待及局内信号调度--例如超时后再次调用执行

    

}task_id_property_t;    //原子任务的初始化属性


typedef struct
{
    uint16_t cur_execute_id;    //当前执行的id
    uint16_t cur_task_run_state;//当前id的运行状态--执行完成？超时？解析？异常处理？

}task_id_local_state_t; //原子任务的执行状态管理



typedef struct
{
    uint16_t task_pool_run_state;
    uint16_t* id_pool;
    uint16_t pool_id_idx; //任务池索引
    uint16_t abnormal_id; //节点池异常点

}task_pool_local_state_t; //任务节点的执行状态管理


void set_task_pool(uint16_t* id_pool, uint16_t len);

void task_pool_fsm(void);


