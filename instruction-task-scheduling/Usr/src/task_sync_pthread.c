/**
 * @file task_sync_pthread.c
 * @brief 同步机制 pthread 实现
 *
 * 基于 POSIX pthread 实现同步接口，
 * 适用于 Linux/RTOS 支持 pthread 的环境。
 */

#define _POSIX_C_SOURCE 200809L

#include "task_sync.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ===== pthread 互斥锁结构 ===== */

struct task_mutex {
    pthread_mutex_t handle; /**< pthread 互斥锁句柄 */
    bool initialized;       /**< 是否已初始化 */
};

/* ===== pthread 条件变量结构 ===== */

struct task_cond {
    pthread_cond_t handle;  /**< pthread 条件变量句柄 */
    bool initialized;       /**< 是否已初始化 */
};

/* ===== 互斥锁操作 ===== */

/**
 * @brief 初始化互斥锁
 */
static bool pthread_sync_mutex_init(task_mutex_t* mutex)
{
    if (!mutex) return false;

    memset(mutex, 0, sizeof(task_mutex_t));

    int ret = pthread_mutex_init(&mutex->handle, NULL);
    if (ret != 0) {
        printf("互斥锁初始化失败: %d\n", ret);
        return false;
    }

    mutex->initialized = true;
    return true;
}

/**
 * @brief 销毁互斥锁
 */
static void pthread_sync_mutex_deinit(task_mutex_t* mutex)
{
    if (!mutex || !mutex->initialized) return;

    pthread_mutex_destroy(&mutex->handle);
    mutex->initialized = false;
}

/**
 * @brief 加锁
 */
static bool pthread_sync_mutex_lock(task_mutex_t* mutex)
{
    if (!mutex || !mutex->initialized) return false;

    int ret = pthread_mutex_lock(&mutex->handle);
    return (ret == 0);
}

/**
 * @brief 解锁
 */
static bool pthread_sync_mutex_unlock(task_mutex_t* mutex)
{
    if (!mutex || !mutex->initialized) return false;

    int ret = pthread_mutex_unlock(&mutex->handle);
    return (ret == 0);
}

/* ===== 条件变量操作 ===== */

/**
 * @brief 初始化条件变量
 */
static bool pthread_sync_cond_init(task_cond_t* cond)
{
    if (!cond) return false;

    memset(cond, 0, sizeof(task_cond_t));

    int ret = pthread_cond_init(&cond->handle, NULL);
    if (ret != 0) {
        printf("条件变量初始化失败: %d\n", ret);
        return false;
    }

    cond->initialized = true;
    return true;
}

/**
 * @brief 销毁条件变量
 */
static void pthread_sync_cond_deinit(task_cond_t* cond)
{
    if (!cond || !cond->initialized) return;

    pthread_cond_destroy(&cond->handle);
    cond->initialized = false;
}

/**
 * @brief 等待条件变量
 */
static bool pthread_sync_cond_wait(task_cond_t* cond, task_mutex_t* mutex, uint32_t timeout_ms)
{
    if (!cond || !cond->initialized || !mutex || !mutex->initialized) {
        return false;
    }

    int ret;
    if (timeout_ms == 0) {
        /* 永久等待 */
        ret = pthread_cond_wait(&cond->handle, &mutex->handle);
    } else {
        /* 超时等待 */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        uint64_t nsec = ts.tv_nsec + (timeout_ms % 1000) * 1000000ULL;
        ts.tv_sec += timeout_ms / 1000 + nsec / 1000000000ULL;
        ts.tv_nsec = nsec % 1000000000ULL;

        ret = pthread_cond_timedwait(&cond->handle, &mutex->handle, &ts);
    }

    return (ret == 0);
}

/**
 * @brief 唤醒一个等待线程
 */
static bool pthread_sync_cond_signal(task_cond_t* cond)
{
    if (!cond || !cond->initialized) return false;

    int ret = pthread_cond_signal(&cond->handle);
    return (ret == 0);
}

/**
 * @brief 唤醒所有等待线程
 */
static bool pthread_sync_cond_broadcast(task_cond_t* cond)
{
    if (!cond || !cond->initialized) return false;

    int ret = pthread_cond_broadcast(&cond->handle);
    return (ret == 0);
}

/* ===== 操作表 ===== */

static const task_sync_ops_t pthread_sync_ops = {
    .mutex_init = pthread_sync_mutex_init,
    .mutex_deinit = pthread_sync_mutex_deinit,
    .mutex_lock = pthread_sync_mutex_lock,
    .mutex_unlock = pthread_sync_mutex_unlock,
    .cond_init = pthread_sync_cond_init,
    .cond_deinit = pthread_sync_cond_deinit,
    .cond_wait = pthread_sync_cond_wait,
    .cond_signal = pthread_sync_cond_signal,
    .cond_broadcast = pthread_sync_cond_broadcast,
};

/**
 * @brief 获取 pthread 同步操作表
 */
const task_sync_ops_t* task_sync_pthread_get_ops(void)
{
    return &pthread_sync_ops;
}
