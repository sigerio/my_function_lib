/**
 * @file task_test_api.c
 * @brief 任务池测试接口实现
 */

#include "task_test_api.h"
#include "task_pool.h"
#include "task_api_compat.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef TASK_SYNC_IMPL_PTHREAD
#include <unistd.h>
#endif

#include "task_tdd_cases.h"
static void task_test_sleep_us(unsigned int us)
{
#ifdef TASK_SYNC_IMPL_PTHREAD
    usleep(us);
#else
    (void)us;
#endif
}

/* ===== 测试用原子任务实现 ===== */

static bool task_test_0_execute(void)
{
    printf("[任务0] 执行中...\n");
    return true;
}

static bool task_test_0_feedback(void)
{
    printf("[任务0] 反馈处理中...\n");
    return true;
}

static bool task_test_0_abnormal(void)
{
    printf("[任务0] 异常处理中...\n");
    return true;
}

static uint64_t task_test_0_time(void)
{
    static uint64_t counter = 0;
    return ++counter;
}

static bool task_test_1_execute(void)
{
    printf("[任务1] 执行中...\n");
    return true;
}

static bool task_test_1_feedback(void)
{
    printf("[任务1] 反馈处理中...\n");
    return true;
}

static bool task_test_1_abnormal(void)
{
    printf("[任务1] 异常处理中...\n");
    return true;
}

static uint64_t task_test_1_time(void)
{
    static uint64_t counter = 0;
    return ++counter;
}

/* ===== 测试初始化 ===== */

static void register_test_tasks(void)
{
    task_property_t prop;

    prop.task_id = TASK_TEST_0;
    prop.init_info.time_cnt = 3;
    prop.init_info.retry_cnt = 3;
    prop.init_info.result = false;
    prop.execute = task_test_0_execute;
    prop.feedback = task_test_0_feedback;
    prop.abnormal = task_test_0_abnormal;
    prop.time = task_test_0_time;
    task_register(TASK_TEST_0, &prop);

    prop.task_id = TASK_TEST_1;
    prop.init_info.time_cnt = 2;
    prop.init_info.retry_cnt = 2;
    prop.init_info.result = false;
    prop.execute = task_test_1_execute;
    prop.feedback = task_test_1_feedback;
    prop.abnormal = task_test_1_abnormal;
    prop.time = task_test_1_time;
    task_register(TASK_TEST_1, &prop);
}

bool task_test_init(void)
{
    if (!task_pool_manager_init()) {
        printf("初始化任务池管理器失败\n");
        return false;
    }

    register_test_tasks();
    return true;
}

/* ===== 测试场景 1 ===== */

void task_test_run_scenario_1(void)
{
    printf("\n===== 测试场景1：多任务池顺序执行 =====\n");

    task_pool_instance_t* pool1 = task_pool_create(1);
    if (pool1 == NULL) {
        printf("创建任务池1失败\n");
        return;
    }

    uint16_t tasks1[] = {TASK_TEST_0, TASK_TEST_1};
    task_pool_add_tasks(1, tasks1, 2);

    task_pool_set_active(1);

    printf("执行任务池1...\n");
    for (int i = 0; i < 10; i++) {
        int result = task_pool_step();
        if (result == 0) {
            printf("任务池1完成\n");
            break;
        }
        task_test_sleep_us(100000);
    }

    task_pool_instance_t* pool2 = task_pool_create(2);
    if (pool2 == NULL) {
        printf("创建任务池2失败\n");
        return;
    }

    uint16_t tasks2[] = {TASK_TEST_1, TASK_TEST_0, TASK_TEST_1};
    task_pool_add_tasks(2, tasks2, 3);

    printf("\n切换到任务池2...\n");
    task_pool_set_active(2);

    for (int i = 0; i < 15; i++) {
        int result = task_pool_step();
        if (result == 0) {
            printf("任务池2完成\n");
            break;
        }
        task_test_sleep_us(100000);
    }

    task_pool_destroy(1);
    task_pool_destroy(2);

    printf("===== 测试场景1完成 =====\n\n");
}

/* ===== 测试场景 2 ===== */

void task_test_run_scenario_2(void)
{
    printf("\n===== 测试场景2：兼容接口测试 =====\n");

    uint16_t pool[] = {TASK_TEST_0, TASK_TEST_1, TASK_TEST_1, TASK_TEST_0, TASK_POOL_END};
    set_task_pool(pool, 5);

    printf("执行任务池（使用兼容接口）...\n");
    for (int i = 0; i < 20; i++) {
        task_pool_fsm();
        task_test_sleep_us(100000);
    }

    printf("===== 测试场景2完成 =====\n\n");
}

void task_test_run_all(void)
{
    printf("运行所有测试...\n");
    task_test_run_scenario_1();
    task_test_run_scenario_2();
    task_test_run_tdd_cases();
}

void task_test_run_tdd_cases(void)
{
    task_tdd_run_all_cases();
}
