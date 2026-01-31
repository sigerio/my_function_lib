/**
 * @file task_test_api.h
 * @brief 任务池测试接口
 *
 * 测试接口与可移植接口独立，测试程序仅通过本头文件访问。
 */

#ifndef TASK_TEST_API_H
#define TASK_TEST_API_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化测试环境并注册测试任务
 * @return true 成功，false 失败
 */
bool task_test_init(void);

/**
 * @brief 运行测试场景1：多任务池顺序执行
 */
void task_test_run_scenario_1(void);

/**
 * @brief 运行测试场景2：兼容接口测试
 */
void task_test_run_scenario_2(void);

/**
 * @brief 运行所有测试场景
 */
void task_test_run_all(void);

/**
 * @brief 运行 TDD 用例
 */
void task_test_run_tdd_cases(void);

#ifdef __cplusplus
}
#endif

#endif /* TASK_TEST_API_H */
