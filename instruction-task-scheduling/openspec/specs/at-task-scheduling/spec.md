# at-task-scheduling Specification

## Purpose
TBD - created by archiving change add-at-task-pool-portable. Update Purpose after archive.
## Requirements
### Requirement: 多任务池顺序执行
系统 MUST 支持多个任务池实例，但同一时刻只能有一个任务池处于执行态，实例数量由静态配置决定。

#### Scenario: 多任务池顺序执行
- **WHEN** 同时创建两个任务池实例
- **THEN** 同一时刻仅一个任务池执行，其余任务池保持等待

#### Scenario: 上层显式切换任务池
- **WHEN** 上层触发任务池切换接口
- **THEN** 系统将执行态切换到指定任务池

### Requirement: 同步 AT 指令流程
系统 MUST 支持同步 AT 指令流程，发送后进入等待并在反馈完成后结束该任务。

#### Scenario: 同步指令完成
- **WHEN** 任务执行发送并收到反馈
- **THEN** 任务状态进入完成并推进到下一任务

### Requirement: 无动态内存
系统 MUST 不使用动态内存分配，任务池与任务状态 MUST 使用静态或栈上内存。

#### Scenario: 初始化任务池
- **WHEN** 初始化任务池实例
- **THEN** 不发生任何动态内存分配

### Requirement: 同步机制可移植
系统 MUST 通过可替换的同步接口适配裸机与 RTOS。

#### Scenario: 裸机环境同步
- **WHEN** 在裸机环境运行
- **THEN** 同步接口使用空实现或轮询实现且不依赖 RTOS

### Requirement: 命名规范
新增接口与结构体名称 MUST 使用下划线命名，禁止驼峰。

#### Scenario: 新增接口命名
- **WHEN** 新增任务池或原子任务接口
- **THEN** 名称使用下划线风格且不出现驼峰

### Requirement: 兼容现有 API
系统 MUST 兼容现有对任务池的调用方式，即使内部结构体重划分也不破坏既有调用。

#### Scenario: 既有调用不变
- **WHEN** 继续使用现有任务池接口调用
- **THEN** 功能行为保持一致且不需要改动调用方

### Requirement: 核心逻辑不变
系统 MUST 保持现有任务执行的核心逻辑不变，仅允许结构与扩展能力调整。

#### Scenario: 保持既有任务流程
- **WHEN** 执行与现有版本相同的任务序列
- **THEN** 任务状态机与任务推进逻辑保持一致

### Requirement: 目录划分与接口隔离
系统 MUST 将 Usr 目录划分为 include/src/test，且测试接口必须与可移植接口独立。

#### Scenario: 测试接口隔离
- **WHEN** 编写调试用测试程序
- **THEN** 仅通过测试接口访问核心能力且不直接依赖可移植接口实现

### Requirement: 测试程序入口
系统 MUST 提供独立的测试程序入口文件，用于调试任务池流程。

#### Scenario: 调试任务池
- **WHEN** 编译并运行测试程序
- **THEN** 能执行预设任务池顺序并触发同步流程

### Requirement: 严格 TDD 流程
系统 MUST 采用严格 TDD 流程推进变更：先写测试用例并验证失败，再实现代码使测试通过，最后重构。

#### Scenario: TDD 工作流
- **WHEN** 开始实现某项变更
- **THEN** 先提交对应测试并出现失败，再提交实现使测试通过，最后进行重构

### Requirement: TDD 用例骨架
系统 MUST 提供最小 TDD 用例骨架，并通过测试入口统一触发。

#### Scenario: TDD 用例入口
- **WHEN** 执行测试入口
- **THEN** 会调用 TDD 用例骨架并输出失败或通过结果

