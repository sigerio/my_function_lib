/**
 * @file task_sync.c
 * @brief 同步机制默认实现与接口选择器
 *
 * 提供同步接口的默认实现和运行时选择机制。
 * 可通过编译宏 TASK_SYNC_IMPL 选择实现：
 * - TASK_SYNC_IMPL_BAREMETAL: 裸机空实现（默认）
 * - TASK_SYNC_IMPL_PTHREAD: pthread 实现
 * - 或在运行时通过 task_sync_set_ops 设置
 */

#include "task_sync.h"
#include <stddef.h>

/* ===== 外部实现声明 ===== */

extern const task_sync_ops_t* task_sync_baremetal_get_ops(void);
extern const task_sync_ops_t* task_sync_pthread_get_ops(void);

/* ===== 当前使用的操作表 ===== */

static const task_sync_ops_t* g_current_ops = NULL;

/* ===== 接口实现 ===== */

/**
 * @brief 获取当前同步接口操作表
 */
const task_sync_ops_t* task_sync_get_ops(void)
{
    if (g_current_ops == NULL) {
#if defined(TASK_SYNC_IMPL_PTHREAD)
        g_current_ops = task_sync_pthread_get_ops();
#else
        g_current_ops = task_sync_baremetal_get_ops();
#endif
    }
    return g_current_ops;
}

/**
 * @brief 设置同步接口操作表
 */
const task_sync_ops_t* task_sync_set_ops(const task_sync_ops_t* ops)
{
    const task_sync_ops_t* prev = g_current_ops;

    if (ops != NULL) {
        g_current_ops = ops;
    }

    return prev;
}
