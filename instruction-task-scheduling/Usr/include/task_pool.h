/**
 * @file task_pool.h
 * @brief 多任务池实例管理接口
 *
 * 支持多个任务池实例，同一时刻仅允许一个活跃池执行。
 * 全部使用静态内存，不进行动态分配。
 */

#ifndef TASK_POOL_H
#define TASK_POOL_H

#include <stdint.h>
#include <stdbool.h>
#include "task_sync.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===== 配置宏 ===== */

/**
 * @brief 任务池最大实例数量
 */
#ifndef TASK_POOL_MAX_INSTANCES
#define TASK_POOL_MAX_INSTANCES 4
#endif

/**
 * @brief 每个任务池最大任务容量
 */
#ifndef TASK_POOL_MAX_TASKS_PER_POOL
#define TASK_POOL_MAX_TASKS_PER_POOL 64
#endif

/**
 * @brief 任务池结束标记
 */
#define TASK_POOL_END_ID 0xFFFFu

/* ===== 原子任务相关定义 ===== */

/**
 * @brief 任务 ID 运行状态机
 */
typedef enum {
    TASK_STATE_INFO_LOAD = 0,  /**< 加载任务信息 */
    TASK_STATE_EXECUTE,        /**< 执行任务 */
    TASK_STATE_FEEDBACK,       /**< 处理反馈 */
    TASK_STATE_ABNORMAL,       /**< 异常处理 */
    TASK_STATE_TIME,           /**< 等待超时 */
    TASK_STATE_DONE,           /**< 任务完成 */
} task_state_t;

/**
 * @brief 原子任务执行函数类型
 */
typedef bool (*task_execute_fn)(void);

/**
 * @brief 原子任务反馈处理函数类型
 */
typedef bool (*task_feedback_fn)(void);

/**
 * @brief 原子任务异常处理函数类型
 */
typedef bool (*task_abnormal_fn)(void);

/**
 * @brief 时间获取函数类型
 */
typedef uint64_t (*task_time_fn)(void);

/**
 * @brief 任务管理信息
 */
typedef struct {
    uint64_t time_cnt;    /**< 超时计数 */
    uint16_t retry_cnt;   /**< 重试计数 */
    bool result;          /**< 执行结果 */
} task_manage_info_t;

/**
 * @brief 原子任务属性
 */
typedef struct {
    uint16_t task_id;           /**< 任务 ID */
    task_manage_info_t init_info; /**< 初始化信息 */
    task_execute_fn execute;    /**< 执行函数 */
    task_feedback_fn feedback;  /**< 反馈函数 */
    task_abnormal_fn abnormal;  /**< 异常处理函数 */
    task_time_fn time;          /**< 时间函数 */
} task_property_t;

/**
 * @brief 原子任务执行状态
 */
typedef struct {
    uint16_t cur_execute_id;    /**< 当前执行的任务 ID */
    task_state_t cur_run_state; /**< 当前运行状态 */
} task_local_state_t;

/* ===== 任务池实例定义 ===== */

/**
 * @brief 任务池状态
 */
typedef enum {
    POOL_STATE_IDLE = 0,    /**< 空闲 */
    POOL_STATE_RUNNING,     /**< 运行中 */
    POOL_STATE_SUSPENDED,   /**< 暂停 */
    POOL_STATE_ERROR,       /**< 错误 */
} pool_state_t;

/**
 * @brief 任务池实例
 *
 * 所有内存均为静态分配，不进行动态内存操作
 */
typedef struct {
    bool used;                                 /**< 是否已占用 */
    uint16_t pool_id;                           /**< 任务池 ID */
    pool_state_t pool_state;                    /**< 任务池状态 */

    /* 任务队列 */
    uint16_t task_ids[TASK_POOL_MAX_TASKS_PER_POOL]; /**< 任务 ID 队列 */
    uint16_t task_count;                        /**< 任务数量 */
    uint16_t task_index;                        /**< 当前执行索引 */

    /* 当前任务状态 */
    task_local_state_t task_state;              /**< 当前任务执行状态 */
    task_manage_info_t task_manage;             /**< 当前任务管理信息 */

    /* 异常信息 */
    uint16_t abnormal_id;                       /**< 异常任务 ID */

    /* 同步原语（仅在需要同步时使用） */
    task_mutex_t* mutex;                        /**< 互斥锁指针 */
    bool mutex_initialized;                     /**< 互斥锁是否已初始化 */
} task_pool_instance_t;

/* ===== 任务池管理接口 ===== */

/**
 * @brief 任务池管理器
 *
 * 管理多个任务池实例，确保同一时刻只有一个活跃池
 */
typedef struct {
    task_pool_instance_t pools[TASK_POOL_MAX_INSTANCES]; /**< 任务池实例数组 */
    uint16_t active_pool_id;                              /**< 当前活跃池 ID，0xFFFF 表示无 */
    uint16_t pool_count;                                  /**< 已创建池数量 */
} task_pool_manager_t;

/**
 * @brief 获取任务池管理器实例
 * @return 任务池管理器指针
 */
task_pool_manager_t* task_pool_get_manager(void);

/**
 * @brief 初始化任务池管理器
 * @return true 成功，false 失败
 */
bool task_pool_manager_init(void);

/**
 * @brief 创建任务池实例
 * @param pool_id 任务池 ID
 * @return 任务池实例指针，NULL 表示失败
 */
task_pool_instance_t* task_pool_create(uint16_t pool_id);

/**
 * @brief 销毁任务池实例
 * @param pool_id 任务池 ID
 * @return true 成功，false 失败
 */
bool task_pool_destroy(uint16_t pool_id);

/**
 * @brief 获取任务池实例
 * @param pool_id 任务池 ID
 * @return 任务池实例指针，NULL 表示不存在
 */
task_pool_instance_t* task_pool_get_instance(uint16_t pool_id);

/**
 * @brief 设置活跃任务池
 * @param pool_id 任务池 ID
 * @return true 成功，false 失败（池不存在或已活跃）
 */
bool task_pool_set_active(uint16_t pool_id);

/**
 * @brief 获取活跃任务池
 * @return 活跃任务池指针，NULL 表示无活跃池
 */
task_pool_instance_t* task_pool_get_active(void);

/**
 * @brief 向任务池添加任务
 * @param pool_id 任务池 ID
 * @param task_ids 任务 ID 数组
 * @param count 任务数量
 * @return true 成功，false 失败
 */
bool task_pool_add_tasks(uint16_t pool_id, const uint16_t* task_ids, uint16_t count);

/**
 * @brief 执行活跃任务池的一步状态机
 * @return >0: 继续执行，0: 完成，<0: 错误
 */
int task_pool_step(void);

/**
 * @brief 内部函数：执行单步任务状态机
 * @param pool 任务池实例
 * @param task_id 任务 ID
 * @return >0: 完成，0: 继续，<0: 异常
 */
int task_pool_execute_step(task_pool_instance_t* pool, uint16_t task_id);

/**
 * @brief 重置任务池
 * @param pool_id 任务池 ID
 * @return true 成功，false 失败
 */
bool task_pool_reset(uint16_t pool_id);

/**
 * @brief 获取任务池状态
 * @param pool_id 任务池 ID
 * @return 任务池状态
 */
pool_state_t task_pool_get_state(uint16_t pool_id);

/* ===== 原子任务注册接口 ===== */

/**
 * @brief 任务注册表项
 */
typedef struct {
    uint16_t task_id;        /**< 任务 ID */
    task_property_t property; /**< 任务属性 */
    bool registered;         /**< 是否已注册 */
} task_registry_entry_t;

/**
 * @brief 获取任务注册表
 * @return 任务注册表指针
 */
task_registry_entry_t* task_get_registry(void);

/**
 * @brief 注册原子任务
 * @param task_id 任务 ID
 * @param property 任务属性
 * @return true 成功，false 失败
 */
bool task_register(uint16_t task_id, const task_property_t* property);

/**
 * @brief 获取注册的任务属性
 * @param task_id 任务 ID
 * @return 任务属性指针，NULL 表示未注册
 */
const task_property_t* task_get_property(uint16_t task_id);

/**
 * @brief 取消注册任务
 * @param task_id 任务 ID
 * @return true 成功，false 失败
 */
bool task_unregister(uint16_t task_id);

#ifdef __cplusplus
}
#endif

#endif /* TASK_POOL_H */
