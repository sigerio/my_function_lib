/**
 * @file task_sync_baremetal.c
 * @brief 同步机制裸机空实现
 *
 * 裸机环境下不需要真实的互斥锁和条件变量，
 * 提供空实现接口保证代码可编译。
 */

#include "task_sync.h"

/* ===== 裸机互斥锁结构 ===== */

struct task_mutex {
    int dummy; /**< 占位符 */
};

/* ===== 裸机条件变量结构 ===== */

struct task_cond {
    int dummy; /**< 占位符 */
};

/* ===== 互斥锁操作 ===== */

/**
 * @brief 初始化互斥锁（空实现）
 */
static bool baremetal_mutex_init(task_mutex_t* mutex)
{
    (void)mutex;
    return true;
}

/**
 * @brief 销毁互斥锁（空实现）
 */
static void baremetal_mutex_deinit(task_mutex_t* mutex)
{
    (void)mutex;
}

/**
 * @brief 加锁（空实现）
 */
static bool baremetal_mutex_lock(task_mutex_t* mutex)
{
    (void)mutex;
    return true;
}

/**
 * @brief 解锁（空实现）
 */
static bool baremetal_mutex_unlock(task_mutex_t* mutex)
{
    (void)mutex;
    return true;
}

/* ===== 条件变量操作 ===== */

/**
 * @brief 初始化条件变量（空实现）
 */
static bool baremetal_cond_init(task_cond_t* cond)
{
    (void)cond;
    return true;
}

/**
 * @brief 销毁条件变量（空实现）
 */
static void baremetal_cond_deinit(task_cond_t* cond)
{
    (void)cond;
}

/**
 * @brief 等待条件变量（空实现，直接返回超时）
 */
static bool baremetal_cond_wait(task_cond_t* cond, task_mutex_t* mutex, uint32_t timeout_ms)
{
    (void)cond;
    (void)mutex;
    (void)timeout_ms;
    return false;
}

/**
 * @brief 唤醒一个等待线程（空实现）
 */
static bool baremetal_cond_signal(task_cond_t* cond)
{
    (void)cond;
    return true;
}

/**
 * @brief 唤醒所有等待线程（空实现）
 */
static bool baremetal_cond_broadcast(task_cond_t* cond)
{
    (void)cond;
    return true;
}

/* ===== 操作表 ===== */

static const task_sync_ops_t baremetal_sync_ops = {
    .mutex_init = baremetal_mutex_init,
    .mutex_deinit = baremetal_mutex_deinit,
    .mutex_lock = baremetal_mutex_lock,
    .mutex_unlock = baremetal_mutex_unlock,
    .cond_init = baremetal_cond_init,
    .cond_deinit = baremetal_cond_deinit,
    .cond_wait = baremetal_cond_wait,
    .cond_signal = baremetal_cond_signal,
    .cond_broadcast = baremetal_cond_broadcast,
};

/**
 * @brief 获取裸机同步操作表
 */
const task_sync_ops_t* task_sync_baremetal_get_ops(void)
{
    return &baremetal_sync_ops;
}
