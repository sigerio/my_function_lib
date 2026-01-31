/**
 * @file task_tdd_cases.c
 * @brief TDD 用例骨架实现
 */

#include "task_tdd_cases.h"
#include "task_pool.h"
#include "task_api_compat.h"
#include "task_test_api.h"
#include <stdio.h>
#include <string.h>

#ifdef TASK_SYNC_IMPL_PTHREAD
#include <unistd.h>
#include <fcntl.h>
#endif

/* ===== 断言宏 ===== */

#define TDD_ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("TDD断言失败: %s\n", msg); \
            return false; \
        } \
    } while (0)

/* ===== 日志捕获（仅在 pthread 环境） ===== */

typedef struct {
    int saved_fd;
    FILE* file;
    char path[128];
} tdd_log_ctx_t;

static bool tdd_log_begin(tdd_log_ctx_t* ctx, const char* path)
{
    if (ctx == NULL || path == NULL) {
        return false;
    }
    memset(ctx, 0, sizeof(*ctx));
    strncpy(ctx->path, path, sizeof(ctx->path) - 1);

#ifdef TASK_SYNC_IMPL_PTHREAD
    fflush(stdout);
    ctx->saved_fd = dup(fileno(stdout));
    if (ctx->saved_fd < 0) {
        return false;
    }
    ctx->file = freopen(path, "w+", stdout);
    if (ctx->file == NULL) {
        return false;
    }
#else
    (void)path;
#endif
    return true;
}

static bool tdd_log_end(tdd_log_ctx_t* ctx)
{
    if (ctx == NULL) {
        return false;
    }
#ifdef TASK_SYNC_IMPL_PTHREAD
    fflush(stdout);
    if (ctx->saved_fd >= 0) {
        dup2(ctx->saved_fd, fileno(stdout));
        close(ctx->saved_fd);
        ctx->saved_fd = -1;
    }
#endif
    return true;
}

static bool tdd_log_contains(const char* path, const char* expect)
{
    if (path == NULL || expect == NULL) {
        return false;
    }
#ifdef TASK_SYNC_IMPL_PTHREAD
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        return false;
    }
    char buf[512];
    size_t read_len = fread(buf, 1, sizeof(buf) - 1, fp);
    buf[read_len] = '\0';
    fclose(fp);
    return strstr(buf, expect) != NULL;
#else
    (void)path;
    (void)expect;
    return true;
#endif
}

/* ===== 用例实现 ===== */

/* ===== 测试任务 ID ===== */

#define TDD_TASK_TIMEOUT_ID 100
#define TDD_TASK_FEEDBACK_FAIL_ID 101
#define TDD_TASK_EXECUTE_FAIL_ID 102

/* ===== 超时失败任务 ===== */

static bool tdd_timeout_execute(void)
{
    printf("用例:超时失败 | 阶段:执行\n");
    return true;
}

static bool tdd_timeout_feedback(void)
{
    printf("用例:超时失败 | 阶段:反馈\n");
    return false;
}

static bool tdd_timeout_abnormal(void)
{
    printf("用例:超时失败 | 结果:异常\n");
    return true;
}

static uint64_t tdd_timeout_time(void)
{
    static uint64_t now = 0;
    now += 1;
    return now;
}

/* ===== 反馈失败任务 ===== */

static bool tdd_feedback_execute(void)
{
    printf("用例:反馈失败 | 阶段:执行\n");
    return true;
}

static bool tdd_feedback_fail(void)
{
    printf("用例:反馈失败 | 阶段:反馈\n");
    return false;
}

static bool tdd_feedback_abnormal(void)
{
    printf("用例:反馈失败 | 结果:异常\n");
    return true;
}

static uint64_t tdd_feedback_time(void)
{
    return 0;
}

/* ===== 执行失败任务 ===== */

static bool tdd_execute_fail(void)
{
    printf("用例:执行失败 | 阶段:执行\n");
    return false;
}

static bool tdd_execute_abnormal(void)
{
    printf("用例:执行失败 | 结果:异常\n");
    return true;
}

static uint64_t tdd_execute_time(void)
{
    static uint64_t now = 0;
    now += 1;
    return now;
}

static bool task_tdd_case_pool_create(void)
{
    printf("\n===== TDD测试场景1：任务池创建 =====\n");
    tdd_log_ctx_t log_ctx;
    task_pool_manager_init();

    TDD_ASSERT_TRUE(tdd_log_begin(&log_ctx, "Usr/build/tdd_log_create.txt"), "日志捕获失败");

    task_pool_instance_t* pool = task_pool_create(1);
    TDD_ASSERT_TRUE(pool != NULL, "任务池创建失败");

    task_pool_instance_t* pool_dup = task_pool_create(1);
    TDD_ASSERT_TRUE(pool_dup == NULL, "重复创建应失败");

    tdd_log_end(&log_ctx);

    TDD_ASSERT_TRUE(tdd_log_contains("Usr/build/tdd_log_create.txt", "任务池1已创建"), "缺少创建日志");
    TDD_ASSERT_TRUE(tdd_log_contains("Usr/build/tdd_log_create.txt", "任务池1已存在"), "缺少重复创建日志");

    printf("===== TDD测试场景1完成 =====\n\n");
    return true;
}

static bool task_tdd_case_pool_switch(void)
{
    printf("\n===== TDD测试场景2：任务池切换 =====\n");
    tdd_log_ctx_t log_ctx;
    task_pool_manager_init();

    TDD_ASSERT_TRUE(task_pool_create(1) != NULL, "任务池1创建失败");
    TDD_ASSERT_TRUE(task_pool_create(2) != NULL, "任务池2创建失败");

    TDD_ASSERT_TRUE(tdd_log_begin(&log_ctx, "Usr/build/tdd_log_switch.txt"), "日志捕获失败");

    TDD_ASSERT_TRUE(task_pool_set_active(1), "激活任务池1失败");
    TDD_ASSERT_TRUE(task_pool_set_active(2), "切换到任务池2失败");

    tdd_log_end(&log_ctx);

    task_pool_instance_t* pool1 = task_pool_get_instance(1);
    task_pool_instance_t* pool2 = task_pool_get_instance(2);
    TDD_ASSERT_TRUE(pool1 != NULL && pool2 != NULL, "任务池实例为空");
    TDD_ASSERT_TRUE(pool2->pool_state == POOL_STATE_RUNNING, "任务池2未处于执行态");

    TDD_ASSERT_TRUE(tdd_log_contains("Usr/build/tdd_log_switch.txt", "任务池2已激活"), "缺少切换日志");

    printf("===== TDD测试场景2完成 =====\n\n");
    return true;
}

static bool task_tdd_case_task_timeout(void)
{
    printf("\n===== TDD测试场景3：超时失败 =====\n");
    tdd_log_ctx_t log_ctx;
    task_pool_manager_init();

    task_property_t prop;
    prop.task_id = TDD_TASK_TIMEOUT_ID;
    prop.init_info.time_cnt = 1;
    prop.init_info.retry_cnt = 1;
    prop.init_info.result = false;
    prop.execute = tdd_timeout_execute;
    prop.feedback = tdd_timeout_feedback;
    prop.abnormal = tdd_timeout_abnormal;
    prop.time = tdd_timeout_time;
    task_register(TDD_TASK_TIMEOUT_ID, &prop);

    task_pool_instance_t* pool = task_pool_create(1);
    TDD_ASSERT_TRUE(pool != NULL, "任务池创建失败");

    uint16_t tasks[] = {TDD_TASK_TIMEOUT_ID};
    TDD_ASSERT_TRUE(task_pool_add_tasks(1, tasks, 1), "添加任务失败");
    TDD_ASSERT_TRUE(task_pool_set_active(1), "激活任务池失败");

    TDD_ASSERT_TRUE(tdd_log_begin(&log_ctx, "Usr/build/tdd_log_timeout.txt"), "日志捕获失败");

    int max_steps = 10;
    while (max_steps-- > 0) {
        int result = task_pool_step();
        if (result < 0) {
            break;
        }
    }

    tdd_log_end(&log_ctx);

    TDD_ASSERT_TRUE(pool->pool_state == POOL_STATE_ERROR, "超时失败未进入异常");
    TDD_ASSERT_TRUE(tdd_log_contains("Usr/build/tdd_log_timeout.txt", "用例:超时失败 | 结果:异常"), "缺少超时异常日志");
    printf("===== TDD测试场景3完成 =====\n\n");
    return true;
}

static bool task_tdd_case_compat_api(void)
{
    printf("\n===== TDD测试场景4：兼容接口 =====\n");
    task_test_init();

    uint16_t pool_list[] = {TASK_TEST_0, TASK_TEST_1, TASK_POOL_END};
    set_task_pool(pool_list, 3);

    int max_steps = 20;
    while (max_steps-- > 0) {
        task_pool_fsm();
        task_pool_instance_t* pool = task_pool_get_active();
        if (pool == NULL) {
            break;
        }
        if (pool->pool_state == POOL_STATE_IDLE) {
            break;
        }
        if (pool->pool_state == POOL_STATE_ERROR) {
            return false;
        }
    }

    printf("===== TDD测试场景4完成 =====\n\n");
    return true;
}

static bool task_tdd_case_feedback_fail(void)
{
    printf("\n===== TDD测试场景5：反馈失败 =====\n");
    tdd_log_ctx_t log_ctx;
    task_pool_manager_init();

    task_property_t prop;
    prop.task_id = TDD_TASK_FEEDBACK_FAIL_ID;
    prop.init_info.time_cnt = 0;
    prop.init_info.retry_cnt = 1;
    prop.init_info.result = false;
    prop.execute = tdd_feedback_execute;
    prop.feedback = tdd_feedback_fail;
    prop.abnormal = tdd_feedback_abnormal;
    prop.time = tdd_feedback_time;
    task_register(TDD_TASK_FEEDBACK_FAIL_ID, &prop);

    task_pool_instance_t* pool = task_pool_create(1);
    TDD_ASSERT_TRUE(pool != NULL, "任务池创建失败");

    uint16_t tasks[] = {TDD_TASK_FEEDBACK_FAIL_ID};
    TDD_ASSERT_TRUE(task_pool_add_tasks(1, tasks, 1), "添加任务失败");
    TDD_ASSERT_TRUE(task_pool_set_active(1), "激活任务池失败");

    TDD_ASSERT_TRUE(tdd_log_begin(&log_ctx, "Usr/build/tdd_log_feedback_fail.txt"), "日志捕获失败");

    int max_steps = 10;
    while (max_steps-- > 0) {
        int result = task_pool_step();
        if (result < 0) {
            break;
        }
    }

    tdd_log_end(&log_ctx);

    TDD_ASSERT_TRUE(pool->pool_state == POOL_STATE_ERROR, "反馈失败未进入异常");
    TDD_ASSERT_TRUE(tdd_log_contains("Usr/build/tdd_log_feedback_fail.txt", "用例:反馈失败 | 结果:异常"), "缺少反馈异常日志");
    printf("===== TDD测试场景5完成 =====\n\n");
    return true;
}

static bool task_tdd_case_execute_fail(void)
{
    printf("\n===== TDD测试场景6：执行失败 =====\n");
    task_pool_manager_init();

    task_property_t prop;
    prop.task_id = TDD_TASK_EXECUTE_FAIL_ID;
    prop.init_info.time_cnt = 1;
    prop.init_info.retry_cnt = 1;
    prop.init_info.result = false;
    prop.execute = tdd_execute_fail;
    prop.feedback = NULL;
    prop.abnormal = tdd_execute_abnormal;
    prop.time = tdd_execute_time;
    task_register(TDD_TASK_EXECUTE_FAIL_ID, &prop);

    task_pool_instance_t* pool = task_pool_create(1);
    TDD_ASSERT_TRUE(pool != NULL, "任务池创建失败");

    uint16_t tasks[] = {TDD_TASK_EXECUTE_FAIL_ID};
    TDD_ASSERT_TRUE(task_pool_add_tasks(1, tasks, 1), "添加任务失败");
    TDD_ASSERT_TRUE(task_pool_set_active(1), "激活任务池失败");

    int max_steps = 5;
    while (max_steps-- > 0) {
        int result = task_pool_step();
        if (result < 0) {
            break;
        }
    }

    TDD_ASSERT_TRUE(pool->task_index == 0, "执行失败不应推进任务");
    TDD_ASSERT_TRUE(pool->pool_state == POOL_STATE_RUNNING, "执行失败不应直接完成");
    printf("===== TDD测试场景6完成 =====\n\n");
    return true;
}

bool task_tdd_run_all_cases(void)
{
    int fail_count = 0;

    if (!task_tdd_case_pool_create()) {
        fail_count++;
    }
    if (!task_tdd_case_pool_switch()) {
        fail_count++;
    }
    if (!task_tdd_case_task_timeout()) {
        fail_count++;
    }
    if (!task_tdd_case_compat_api()) {
        fail_count++;
    }
    if (!task_tdd_case_feedback_fail()) {
        fail_count++;
    }
    if (!task_tdd_case_execute_fail()) {
        fail_count++;
    }

    if (fail_count > 0) {
        printf("TDD用例失败数: %d\n", fail_count);
        return false;
    }

    printf("TDD用例全部通过\n");
    return true;
}
