/**
 * @file task_test_main.c
 * @brief 任务池测试程序入口
 */

#include "task_test_api.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    printf("===========================================\n");
    printf("任务池测试程序\n");
    printf("===========================================\n");

    if (!task_test_init()) {
        return -1;
    }

    if (argc > 1) {
        int scenario = atoi(argv[1]);
        switch (scenario) {
        case 1:
            task_test_run_scenario_1();
            break;
        case 2:
            task_test_run_scenario_2();
            break;
        default:
            task_test_run_all();
            break;
        }
    } else {
        task_test_run_all();
    }

    printf("===========================================\n");
    printf("所有测试完成\n");
    printf("===========================================\n");

    return 0;
}
