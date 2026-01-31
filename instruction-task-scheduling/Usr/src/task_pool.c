/**
 * @file task_pool.c
 * @brief 多任务池实例管理实现
 */

#include "task_pool.h"
#include <string.h>
#include <stdio.h>

/* ===== 静态内存分配 ===== */

/**
 * @brief 任务池管理器静态实例
 */
static task_pool_manager_t g_pool_manager = {0};

/**
 * @brief 任务注册表
 *
 * 最大支持的任务数量由配置宏决定
 */
#ifndef TASK_MAX_REGISTERED
#define TASK_MAX_REGISTERED 256
#endif

static task_registry_entry_t g_task_registry[TASK_MAX_REGISTERED] = {0};

/* ===== 内部辅助函数 ===== */

/**
 * @brief 查找任务池实例
 */
static task_pool_instance_t* find_pool(uint16_t pool_id)
{
    for (uint16_t i = 0; i < TASK_POOL_MAX_INSTANCES; i++) {
        if (g_pool_manager.pools[i].used &&
            g_pool_manager.pools[i].pool_id == pool_id) {
            return &g_pool_manager.pools[i];
        }
    }
    return NULL;
}

/**
 * @brief 查找空闲槽位
 */
static int find_free_slot(void)
{
    for (uint16_t i = 0; i < TASK_POOL_MAX_INSTANCES; i++) {
        if (!g_pool_manager.pools[i].used) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief 重置任务管理信息
 */
static void reset_task_manage_info(task_manage_info_t* info)
{
    if (info) {
        info->time_cnt = 0;
        info->retry_cnt = 0;
        info->result = false;
    }
}

/**
 * @brief 重置任务本地状态
 */
static void reset_task_local_state(task_local_state_t* state)
{
    if (state) {
        state->cur_execute_id = 0;
        state->cur_run_state = TASK_STATE_INFO_LOAD;
    }
}

/* ===== 任务池管理接口实现 ===== */

/**
 * @brief 获取任务池管理器实例
 */
task_pool_manager_t* task_pool_get_manager(void)
{
    return &g_pool_manager;
}

/**
 * @brief 初始化任务池管理器
 */
bool task_pool_manager_init(void)
{
    memset(&g_pool_manager, 0, sizeof(task_pool_manager_t));
    g_pool_manager.active_pool_id = TASK_POOL_END_ID;
    g_pool_manager.pool_count = 0;
    memset(g_task_registry, 0, sizeof(g_task_registry));
    return true;
}

/**
 * @brief 创建任务池实例
 */
task_pool_instance_t* task_pool_create(uint16_t pool_id)
{
    /* 检查是否已存在 */
    task_pool_instance_t* existing = find_pool(pool_id);
    if (existing != NULL) {
        printf("任务池%u已存在\n", pool_id);
        return NULL;
    }

    /* 查找空闲槽位 */
    int slot = find_free_slot();
    if (slot < 0) {
        printf("任务池槽位不足\n");
        return NULL;
    }

    /* 初始化任务池 */
    task_pool_instance_t* pool = &g_pool_manager.pools[slot];
    memset(pool, 0, sizeof(task_pool_instance_t));

    pool->used = true;
    pool->pool_id = pool_id;
    pool->pool_state = POOL_STATE_IDLE;
    pool->task_count = 0;
    pool->task_index = 0;
    pool->abnormal_id = TASK_POOL_END_ID;
    pool->mutex = NULL;
    pool->mutex_initialized = false;

    /* 初始化任务队列为结束标记 */
    for (uint16_t i = 0; i < TASK_POOL_MAX_TASKS_PER_POOL; i++) {
        pool->task_ids[i] = TASK_POOL_END_ID;
    }

    /* 初始化任务状态 */
    reset_task_local_state(&pool->task_state);
    reset_task_manage_info(&pool->task_manage);

    g_pool_manager.pool_count++;

    printf("任务池%u已创建\n", pool_id);
    return pool;
}

/**
 * @brief 销毁任务池实例
 */
bool task_pool_destroy(uint16_t pool_id)
{
    task_pool_instance_t* pool = find_pool(pool_id);
    if (pool == NULL) {
        return false;
    }

    /* 如果是活跃池，先清除 */
    if (g_pool_manager.active_pool_id == pool_id) {
        g_pool_manager.active_pool_id = TASK_POOL_END_ID;
    }

    /* 销毁互斥锁（如果已初始化） */
    if (pool->mutex_initialized && pool->mutex != NULL) {
        const task_sync_ops_t* ops = task_sync_get_ops();
        if (ops && ops->mutex_deinit) {
            ops->mutex_deinit(pool->mutex);
        }
        pool->mutex_initialized = false;
    }

    /* 清零 */
    memset(pool, 0, sizeof(task_pool_instance_t));
    if (g_pool_manager.pool_count > 0) {
        g_pool_manager.pool_count--;
    }

    printf("任务池%u已销毁\n", pool_id);
    return true;
}

/**
 * @brief 获取任务池实例
 */
task_pool_instance_t* task_pool_get_instance(uint16_t pool_id)
{
    return find_pool(pool_id);
}

/**
 * @brief 设置活跃任务池
 */
bool task_pool_set_active(uint16_t pool_id)
{
    task_pool_instance_t* pool = find_pool(pool_id);
    if (pool == NULL) {
        printf("任务池%u不存在\n", pool_id);
        return false;
    }

    /* 只允许一个活跃池 */
    if (g_pool_manager.active_pool_id != TASK_POOL_END_ID &&
        g_pool_manager.active_pool_id != pool_id) {
        /* 先暂停之前的活跃池 */
        task_pool_instance_t* prev = find_pool(g_pool_manager.active_pool_id);
        if (prev) {
            prev->pool_state = POOL_STATE_SUSPENDED;
        }
    }

    g_pool_manager.active_pool_id = pool_id;
    pool->pool_state = POOL_STATE_RUNNING;

    printf("任务池%u已激活\n", pool_id);
    return true;
}

/**
 * @brief 获取活跃任务池
 */
task_pool_instance_t* task_pool_get_active(void)
{
    if (g_pool_manager.active_pool_id == TASK_POOL_END_ID) {
        return NULL;
    }
    return find_pool(g_pool_manager.active_pool_id);
}

/**
 * @brief 向任务池添加任务
 */
bool task_pool_add_tasks(uint16_t pool_id, const uint16_t* task_ids, uint16_t count)
{
    if (task_ids == NULL || count == 0) {
        return false;
    }

    task_pool_instance_t* pool = find_pool(pool_id);
    if (pool == NULL) {
        return false;
    }

    /* 检查容量 */
    if (pool->task_count + count > TASK_POOL_MAX_TASKS_PER_POOL) {
        printf("任务池%u容量不足\n", pool_id);
        return false;
    }

    /* 添加任务 */
    for (uint16_t i = 0; i < count; i++) {
        pool->task_ids[pool->task_count + i] = task_ids[i];
    }
    pool->task_count += count;

    return true;
}

/**
 * @brief 执行活跃任务池的一步状态机
 */
int task_pool_step(void)
{
    task_pool_instance_t* pool = task_pool_get_active();
    if (pool == NULL) {
        return 0; /* 无活跃池，表示完成 */
    }

    if (pool->pool_state != POOL_STATE_RUNNING) {
        return 0;
    }

    /* 检查是否有任务 */
    if (pool->task_index >= pool->task_count) {
        pool->pool_state = POOL_STATE_IDLE;
        return 0; /* 所有任务完成 */
    }

    /* 获取当前任务 ID */
    uint16_t task_id = pool->task_ids[pool->task_index];
    if (task_id == TASK_POOL_END_ID) {
        pool->pool_state = POOL_STATE_IDLE;
        return 0;
    }

    /* 执行任务状态机 */
    int result = task_pool_execute_step(pool, task_id);

    if (result > 0) {
        /* 任务完成，推进到下一个任务 */
        pool->task_index++;
        reset_task_local_state(&pool->task_state);
        reset_task_manage_info(&pool->task_manage);
    } else if (result < 0) {
        /* 任务异常 */
        pool->abnormal_id = task_id;
        pool->pool_state = POOL_STATE_ERROR;
        return -1;
    }

    return 1; /* 继续执行 */
}

/**
 * @brief 重置任务池
 */
bool task_pool_reset(uint16_t pool_id)
{
    task_pool_instance_t* pool = find_pool(pool_id);
    if (pool == NULL) {
        return false;
    }

    for (uint16_t i = 0; i < TASK_POOL_MAX_TASKS_PER_POOL; i++) {
        pool->task_ids[i] = TASK_POOL_END_ID;
    }
    pool->task_count = 0;
    pool->task_index = 0;
    pool->pool_state = POOL_STATE_IDLE;
    pool->abnormal_id = TASK_POOL_END_ID;

    reset_task_local_state(&pool->task_state);
    reset_task_manage_info(&pool->task_manage);

    return true;
}

/**
 * @brief 获取任务池状态
 */
pool_state_t task_pool_get_state(uint16_t pool_id)
{
    task_pool_instance_t* pool = find_pool(pool_id);
    if (pool == NULL) {
        return POOL_STATE_ERROR;
    }
    return pool->pool_state;
}

/* ===== 原子任务执行状态机 ===== */

/**
 * @brief 执行单步任务状态机
 *
 * 这是一个辅助函数，需要在 task_pool.c 内部实现
 */
int task_pool_execute_step(task_pool_instance_t* pool, uint16_t task_id)
{
    task_local_state_t* state = &pool->task_state;
    task_manage_info_t* manage = &pool->task_manage;

    state->cur_execute_id = task_id;

    /* 重置时间计数（非等待状态时） */
    if (state->cur_run_state != TASK_STATE_TIME) {
        manage->time_cnt = 0;
    }

    int result = 0;
    task_state_t next_state = state->cur_run_state;

    switch (state->cur_run_state) {
    case TASK_STATE_INFO_LOAD:
        /* 加载任务属性 */
        next_state = TASK_STATE_EXECUTE;
        break;

    case TASK_STATE_EXECUTE:
        /* 执行任务 */
        {
            const task_property_t* prop = task_get_property(task_id);
            if (prop == NULL) {
                printf("任务%u未注册\n", task_id);
                next_state = TASK_STATE_ABNORMAL;
                break;
            }

            /* 检查重试次数 */
            if (manage->retry_cnt >= prop->init_info.retry_cnt) {
                next_state = TASK_STATE_ABNORMAL;
                break;
            }

            /* 执行 */
            if (prop->execute()) {
                manage->retry_cnt++;
                if (prop->time) {
                    manage->time_cnt = prop->time() + prop->init_info.time_cnt;
                } else {
                    manage->time_cnt = prop->init_info.time_cnt;
                }
                next_state = TASK_STATE_TIME;
            }
        }
        break;

    case TASK_STATE_FEEDBACK:
        /* 处理反馈 */
        {
            const task_property_t* prop = task_get_property(task_id);
            if (prop && prop->feedback) {
                manage->result = prop->feedback();
                if (manage->result) {
                    result = 1; /* 完成 */
                    next_state = TASK_STATE_DONE;
                } else {
                    next_state = TASK_STATE_ABNORMAL;
                }
            } else {
                result = 1; /* 无反馈函数，视为完成 */
                next_state = TASK_STATE_DONE;
            }
        }
        break;

    case TASK_STATE_TIME:
        /* 等待超时或事件 */
        {
            const task_property_t* prop = task_get_property(task_id);
            if (prop && prop->time) {
                uint64_t current_time = prop->time();
                if (manage->time_cnt == 0 || (current_time >= manage->time_cnt)) {
                    /* 超时，进入反馈处理 */
                    next_state = TASK_STATE_FEEDBACK;
                }
            } else {
                /* 无时间函数，直接进入反馈 */
                next_state = TASK_STATE_FEEDBACK;
            }
        }
        break;

    case TASK_STATE_ABNORMAL:
        /* 异常处理 */
        {
            const task_property_t* prop = task_get_property(task_id);
            if (prop && prop->abnormal) {
                prop->abnormal();
            }
            result = -1; /* 异常 */
            next_state = TASK_STATE_DONE;
        }
        break;

    case TASK_STATE_DONE:
        /* 任务完成 */
        result = 1;
        break;

    default:
        break;
    }

    state->cur_run_state = next_state;
    return result;
}

/* ===== 任务注册接口实现 ===== */

/**
 * @brief 获取任务注册表
 */
task_registry_entry_t* task_get_registry(void)
{
    return g_task_registry;
}

/**
 * @brief 注册原子任务
 */
bool task_register(uint16_t task_id, const task_property_t* property)
{
    if (property == NULL) {
        return false;
    }

    /* 查找空闲槽位或已存在项 */
    int free_slot = -1;
    for (int i = 0; i < TASK_MAX_REGISTERED; i++) {
        if (g_task_registry[i].registered) {
            if (g_task_registry[i].task_id == task_id) {
                /* 已存在，更新 */
                g_task_registry[i].property = *property;
                return true;
            }
        } else if (free_slot < 0) {
            free_slot = i;
        }
    }

    if (free_slot < 0) {
        printf("任务注册表已满\n");
        return false;
    }

    g_task_registry[free_slot].task_id = task_id;
    g_task_registry[free_slot].property = *property;
    g_task_registry[free_slot].registered = true;

    return true;
}

/**
 * @brief 获取注册的任务属性
 */
const task_property_t* task_get_property(uint16_t task_id)
{
    for (int i = 0; i < TASK_MAX_REGISTERED; i++) {
        if (g_task_registry[i].registered &&
            g_task_registry[i].task_id == task_id) {
            return &g_task_registry[i].property;
        }
    }
    return NULL;
}

/**
 * @brief 取消注册任务
 */
bool task_unregister(uint16_t task_id)
{
    for (int i = 0; i < TASK_MAX_REGISTERED; i++) {
        if (g_task_registry[i].registered &&
            g_task_registry[i].task_id == task_id) {
            g_task_registry[i].registered = false;
            memset(&g_task_registry[i].property, 0, sizeof(task_property_t));
            return true;
        }
    }
    return false;
}
