/**
 * @file task_sync.h
 * @brief 同步机制抽象接口，用于适配裸机与 RTOS 环境
 *
 * 提供互斥锁和条件变量的抽象接口，支持：
 * - 裸机环境：空实现或轮询实现
 * - RTOS 环境：基于实际 OS 原语实现
 */

#ifndef TASK_SYNC_H
#define TASK_SYNC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 互斥锁句柄
 */
typedef struct task_mutex task_mutex_t;

/**
 * @brief 条件变量句柄
 */
typedef struct task_cond task_cond_t;

/**
 * @brief 同步接口操作表
 *
 * 通过函数指针抽象同步原语，实现层负责填充具体实现
 */
typedef struct {
    /**
     * @brief 创建互斥锁
     * @param mutex 互斥锁句柄指针
     * @return true 成功，false 失败
     */
    bool (*mutex_init)(task_mutex_t* mutex);

    /**
     * @brief 销毁互斥锁
     * @param mutex 互斥锁句柄指针
     */
    void (*mutex_deinit)(task_mutex_t* mutex);

    /**
     * @brief 加锁
     * @param mutex 互斥锁句柄指针
     * @return true 成功，false 失败
     */
    bool (*mutex_lock)(task_mutex_t* mutex);

    /**
     * @brief 解锁
     * @param mutex 互斥锁句柄指针
     * @return true 成功，false 失败
     */
    bool (*mutex_unlock)(task_mutex_t* mutex);

    /**
     * @brief 创建条件变量
     * @param cond 条件变量句柄指针
     * @return true 成功，false 失败
     */
    bool (*cond_init)(task_cond_t* cond);

    /**
     * @brief 销毁条件变量
     * @param cond 条件变量句柄指针
     */
    void (*cond_deinit)(task_cond_t* cond);

    /**
     * @brief 等待条件变量
     * @param cond 条件变量句柄指针
     * @param mutex 互斥锁句柄指针（已加锁）
     * @param timeout_ms 超时时间（毫秒），0 表示永久等待
     * @return true 收到信号，false 超时或失败
     */
    bool (*cond_wait)(task_cond_t* cond, task_mutex_t* mutex, uint32_t timeout_ms);

    /**
     * @brief 唤醒一个等待线程
     * @param cond 条件变量句柄指针
     * @return true 成功，false 失败
     */
    bool (*cond_signal)(task_cond_t* cond);

    /**
     * @brief 唤醒所有等待线程
     * @param cond 条件变量句柄指针
     * @return true 成功，false 失败
     */
    bool (*cond_broadcast)(task_cond_t* cond);
} task_sync_ops_t;

/**
 * @brief 获取当前同步接口操作表
 *
 * 通过配置宏 TASK_SYNC_IMPL 选择实现：
 * - TASK_SYNC_BAREMETAL: 裸机空实现
 * - TASK_SYNC_PTHREAD: pthread 实现
 *
 * @return 同步接口操作表指针
 */
const task_sync_ops_t* task_sync_get_ops(void);

/**
 * @brief 设置同步接口操作表
 * @param ops 操作表指针，NULL 使用默认实现
 * @return 之前的操作表指针
 */
const task_sync_ops_t* task_sync_set_ops(const task_sync_ops_t* ops);

/* ===== 便捷宏定义 ===== */

/**
 * @brief 初始化互斥锁
 */
#define TASK_MUTEX_INIT(mutex) \
    (task_sync_get_ops()->mutex_init(mutex))

/**
 * @brief 销毁互斥锁
 */
#define TASK_MUTEX_DEINIT(mutex) \
    (task_sync_get_ops()->mutex_deinit(mutex))

/**
 * @brief 加锁
 */
#define TASK_MUTEX_LOCK(mutex) \
    (task_sync_get_ops()->mutex_lock(mutex))

/**
 * @brief 解锁
 */
#define TASK_MUTEX_UNLOCK(mutex) \
    (task_sync_get_ops()->mutex_unlock(mutex))

/**
 * @brief 初始化条件变量
 */
#define TASK_COND_INIT(cond) \
    (task_sync_get_ops()->cond_init(cond))

/**
 * @brief 销毁条件变量
 */
#define TASK_COND_DEINIT(cond) \
    (task_sync_get_ops()->cond_deinit(cond))

/**
 * @brief 等待条件变量
 */
#define TASK_COND_WAIT(cond, mutex, timeout) \
    (task_sync_get_ops()->cond_wait(cond, mutex, timeout))

/**
 * @brief 唤醒一个等待线程
 */
#define TASK_COND_SIGNAL(cond) \
    (task_sync_get_ops()->cond_signal(cond))

/**
 * @brief 唤醒所有等待线程
 */
#define TASK_COND_BROADCAST(cond) \
    (task_sync_get_ops()->cond_broadcast(cond))

#ifdef __cplusplus
}
#endif

#endif /* TASK_SYNC_H */
