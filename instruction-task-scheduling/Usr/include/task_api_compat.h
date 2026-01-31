/**
 * @file task_api_compat.h
 * @brief 任务池接口兼容层
 *
 * 保留原有接口声明，内部转发到新的多任务池实现。
 * 确保既有调用方无需修改代码。
 */

#ifndef TASK_API_COMPAT_H
#define TASK_API_COMPAT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===== 原有任务 ID 枚举（兼容） ===== */

/**
 * @brief 任务 ID 运行状态机（兼容旧代码）
 */
typedef enum {
    TASK_ID_INFO_LOAD = 0,
    TASK_ID_EXECUTE,
    TASK_ID_FEEDBACK,
    TASK_ID_ABNORMAL,
    TASK_ID_TIME,
} TASK_ID_RUN_FSM;

/**
 * @brief 任务 ID 枚举（兼容旧代码）
 */
typedef enum {
    TASK_TEST_0 = 0,
    TASK_TEST_1,
    TASK_POOL_END,
} TASK_ID_NUM;

/* ===== 原有结构体定义（兼容） ===== */

/**
 * @brief 任务 ID 管理信息（兼容旧代码）
 */
typedef struct {
    uint64_t time_cnt;
    uint16_t retry_cnt;
    bool result;
} task_id_manage_t;

/**
 * @brief 任务 ID 属性（兼容旧代码）
 */
typedef struct {
    uint16_t task_id;
    task_id_manage_t init_info;
    bool (*execute)(void);
    bool (*feedback)(void);
    bool (*abnormal)(void);
    uint64_t (*time)(void);
} task_id_property_t;

/**
 * @brief 任务 ID 本地状态（兼容旧代码）
 */
typedef struct {
    uint16_t cur_execute_id;
    uint16_t cur_task_run_state;
} task_id_local_state_t;

/**
 * @brief 任务池本地状态（兼容旧代码）
 */
typedef struct {
    uint16_t task_pool_run_state;
    uint16_t* id_pool;
    uint16_t pool_id_idx;
    uint16_t abnormal_id;
} task_pool_local_state_t;

/* ===== 原有接口声明（兼容） ===== */

/**
 * @brief 设置任务池（兼容接口）
 * @param id_pool 任务 ID 数组
 * @param len 数组长度
 *
 * @note 内部使用默认任务池（ID = 0）
 */
void set_task_pool(uint16_t* id_pool, uint16_t len);

/**
 * @brief 任务池状态机（兼容接口）
 *
 * @note 内部使用默认任务池（ID = 0）
 */
void task_pool_fsm(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_API_COMPAT_H */
