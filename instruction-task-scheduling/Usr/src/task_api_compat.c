/**
 * @file task_api_compat.c
 * @brief 任务池接口兼容层实现
 *
 * 将原有接口调用转发到新的多任务池实现，
 * 确保既有代码无需修改。
 */

#include "task_api_compat.h"
#include "task_pool.h"
#include <string.h>

/* ===== 默认任务池 ID ===== */

#define DEFAULT_POOL_ID 0

/* ===== 内部状态（兼容） ===== */

static uint16_t g_id_pool_buffer[256] = {0};
static task_pool_local_state_t g_pool_state_compat = {0};

/* ===== 接口兼容实现 ===== */

/**
 * @brief 设置任务池（兼容接口）
 */
void set_task_pool(uint16_t* id_pool, uint16_t len)
{
    if (id_pool == NULL || len == 0) {
        return;
    }

    /* 复制到内部缓冲区 */
    uint16_t copy_len = len;
    if (copy_len > 256) {
        copy_len = 256;
    }
    memcpy(g_id_pool_buffer, id_pool, copy_len * sizeof(uint16_t));

    /* 获取或创建默认任务池 */
    task_pool_instance_t* pool = task_pool_get_instance(DEFAULT_POOL_ID);

    if (pool == NULL) {
        pool = task_pool_create(DEFAULT_POOL_ID);
        if (pool == NULL) {
            g_pool_state_compat.task_pool_run_state = TASK_POOL_END;
            return;
        }
    }

    /* 重置并添加任务 */
    task_pool_reset(DEFAULT_POOL_ID);

    /* 转换结束标记 */
    uint16_t task_ids[256];
    uint16_t count = 0;
    for (uint16_t i = 0; i < copy_len; i++) {
        if (id_pool[i] == TASK_POOL_END) {
            break;
        }
        task_ids[count++] = id_pool[i];
    }

    task_pool_add_tasks(DEFAULT_POOL_ID, task_ids, count);
    task_pool_set_active(DEFAULT_POOL_ID);

    g_pool_state_compat.task_pool_run_state = TASK_ID_INFO_LOAD;
    g_pool_state_compat.id_pool = g_id_pool_buffer;
    g_pool_state_compat.pool_id_idx = 0;
}

/**
 * @brief 任务池状态机（兼容接口）
 */
void task_pool_fsm(void)
{
    task_pool_instance_t* pool = task_pool_get_active();

    if (pool == NULL) {
        g_pool_state_compat.task_pool_run_state = TASK_POOL_END;
        return;
    }

    /* 执行一步 */
    int result = task_pool_step();

    /* 更新兼容状态 */
    if (result > 0) {
        g_pool_state_compat.task_pool_run_state = TASK_ID_EXECUTE;
        g_pool_state_compat.pool_id_idx = (uint16_t)pool->task_index;
        g_pool_state_compat.abnormal_id = pool->abnormal_id;
    } else if (result == 0) {
        g_pool_state_compat.task_pool_run_state = TASK_POOL_END;
    } else {
        g_pool_state_compat.task_pool_run_state = TASK_ID_ABNORMAL;
    }
}
