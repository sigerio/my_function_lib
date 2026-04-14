## ADDED Requirements
### Requirement: AT 指令一问一答
系统 MUST 采用 AT 指令一问一答通信方式，请求与响应必须成对出现且按序号匹配。

#### Scenario: 请求响应匹配
- **WHEN** 主站发送一条 AT 请求
- **THEN** 从站返回对应序号的响应并以 OK/ERROR 结束

### Requirement: 双路 TCP 通道
系统 MUST 使用两路 TCP 通道模拟全双工：通道A仅主站→从站发送请求，通道B仅从站→主站返回响应与 URC。

#### Scenario: 通道分工
- **WHEN** 主站发送 AT 请求
- **THEN** 请求仅通过通道A发送，响应/URC 仅通过通道B返回

### Requirement: 通道独占
系统 MUST 保证发送线程独占通道A、接收线程独占通道B，避免并发写入破坏一问一答时序。

#### Scenario: 独占发送
- **WHEN** 多个线程产生发送请求
- **THEN** 仅发送线程写入通道A，其余线程只入队

### Requirement: AT 响应与错误码
系统 MUST 使用终止行 `OK` 或 `ERROR,<code>` 表达请求结果，并按约定的错误码语义处理重试或断开。

#### Scenario: 错误码处理
- **WHEN** 收到 `ERROR,<code>` 且错误码为可重试类型
- **THEN** 系统在限定次数内重试，否则进入断开阶段

### Requirement: AT+DATA 二进制 framing
系统 MUST 通过“头部行 + 二进制负载”的方式传输 APDU，并基于长度字段解析载荷边界。

#### Scenario: 解析二进制载荷
- **WHEN** 收到 `+DATA:<seq>,<result>,<len>\\r\\n` 头部
- **THEN** 系统读取紧随其后的 `len` 字节作为 APDU 负载，负载后的 `\\r\\n` 作为分隔，再解析终止行

#### Scenario: 请求负载分隔
- **WHEN** 主站发送 `AT+DATA=<seq>,<len>\\r\\n` 头部
- **THEN** 负载发送完成后必须追加 `\\r\\n` 作为下一条指令分隔

#### Scenario: URC 负载分隔
- **WHEN** 从站通过 `+DATA:<seq>,<len>\\r\\n` 异步上送
- **THEN** 负载发送完成后必须追加 `\\r\\n` 作为分隔

### Requirement: 三阶段通信状态机
系统 MUST 将通信划分为握手阶段、通信阶段与断开阶段，并按阶段顺序推进。

#### Scenario: 认证通过进入通信
- **WHEN** 握手认证结果为成功
- **THEN** 系统进入通信阶段

#### Scenario: 认证失败终止
- **WHEN** 握手认证结果为失败
- **THEN** 系统拒绝进入通信阶段并进入断开阶段

### Requirement: 握手阶段交换信息
系统 MUST 在握手阶段交换主机 ID、从机 ID、公钥等必要信息，并根据认证结果决定是否继续。

#### Scenario: 交换并确认身份
- **WHEN** 主站发起握手
- **THEN** 从站返回自身身份信息与认证结果

### Requirement: 握手字段格式
系统 MUST 使用定长 16 字节 ASCII 作为主机/从机 ID，不足右侧补字符 `0`；公钥使用 Base64 字符串；`auth_result` 为错误码数值（0=成功，其余失败）。

#### Scenario: 认证结果编码
- **WHEN** 握手响应返回 `auth_result`
- **THEN** `auth_result=0` 表示成功，非 0 表示失败并复用 ERROR 码表

### Requirement: 通信阶段承载 IEC104
系统 MUST 在通信阶段使用 IEC104 APDU 进行数据收发，并正确处理 I/S/U 帧与序列号。

#### Scenario: 传输 IEC104 数据包
- **WHEN** 需要发送 IEC104 数据
- **THEN** 通过 AT+DATA 以原始二进制封装 APDU 并在响应中确认结果

#### Scenario: 超长 APDU
- **WHEN** APDU 长度超过 IEC104 上限
- **THEN** 系统拒绝发送并返回 AT 错误

#### Scenario: 在途 I 帧限制
- **WHEN** AT 串行发送约束生效
- **THEN** 系统限制在途 I 帧数量为 1，即使 k 配置大于 1

### Requirement: URC 异步上送
系统 MUST 支持从站在通信阶段通过 URC 异步上送 `+DATA` 报文。

#### Scenario: 从站异步上送
- **WHEN** 从站有待上送的数据
- **THEN** 从站使用 `+DATA` 作为 URC 异步推送 APDU（原始二进制负载）

### Requirement: URC ACK
系统 MUST 在收到 URC 后通过通道A发送 `AT+URCACK=<seq>` 进行确认；失败通过 `ERROR,<code>` 返回。

#### Scenario: 队列满回传错误
- **WHEN** 收到 URC 且队列已满
- **THEN** 主站发送 `AT+URCACK=<seq>` 并返回 `ERROR,<code>` 作为失败确认

#### Scenario: URC ACK 超时
- **WHEN** 已发送 URCACK 但在超时内未收到确认
- **THEN** 系统仅记录错误，不进行重试

### Requirement: 通信阶段消息堆栈
系统 MUST 在通信阶段维护消息堆栈/队列，用于缓存未发送或未处理的 APDU，并为队列项标记发送/接收类型；队列容量按字节数统计，上限为 2M 静态池容量。

#### Scenario: 缓存待处理消息
- **WHEN** 同时存在待发送与待解析的 APDU
- **THEN** 系统将其放入堆栈/队列并按标记顺序处理

#### Scenario: 发送完成释放
- **WHEN** 标记为发送的消息完成发送
- **THEN** 系统自动释放该消息占用的内存

#### Scenario: 接收解析释放
- **WHEN** 标记为接收的消息解析完成
- **THEN** 系统自动释放该消息占用的内存

#### Scenario: 队列溢出
- **WHEN** 队列按字节数统计已达到容量上限
- **THEN** 新消息入队被拒绝并返回 AT 错误

### Requirement: 内存池上限
系统 MUST 提供 `my_malloc(size)` 内存分配接口，当前实现基于 2M 静态内存池并专用于 IEC104 队列/消息缓存；AT 解析与指令缓冲使用固定数组不占用内存池；超限申请必须丢弃并返回 AT 错误。

#### Scenario: 超限申请
- **WHEN** `my_malloc` 申请超过 2M 内存池上限
- **THEN** 申请被丢弃并返回 AT 错误

### Requirement: IEC104 地址长度
系统 MUST 支持 ASDU 地址 2 字节与 IOA 地址 3 字节的默认长度配置，并允许在配置中调整。

#### Scenario: 解析地址
- **WHEN** 接收包含信息对象的 ASDU
- **THEN** 系统按配置长度解析 CA 与 IOA

### Requirement: 断开阶段处理
系统 MUST 支持从站主动请求断开与主站请求从站断开，并在满足断开条件后完成断链。

#### Scenario: 从站请求断开
- **WHEN** 从站主动发起断开请求
- **THEN** 主站确认后进入断开阶段并结束通信

### Requirement: 超时与重试
系统 MUST 为 AT 请求定义超时策略，并在超时后按配置执行重试或断开。

#### Scenario: 请求超时
- **WHEN** AT 请求在超时时间内未收到响应
- **THEN** 系统按配置执行重试或进入断开阶段

#### Scenario: 线性重试
- **WHEN** 出现可重试错误且未超过重试次数
- **THEN** 系统按固定次数与线性间隔执行重试

### Requirement: 默认定时器与窗口
系统 MUST 提供默认且可配置的定时器与窗口参数，默认值为 IEC104 T1=15s、T2=10s、T3=20s、k=12、w=8；AT T_HS=5s、T_AT=3s、T_DISC=3s。

#### Scenario: 使用默认值
- **WHEN** 未提供自定义配置
- **THEN** 系统使用上述默认定时器与窗口参数

### Requirement: 多线程队列处理
系统 MUST 允许多线程协作处理通信阶段队列，任务池负责 AT 指令解析与 ACK 处理。

#### Scenario: 线程协作处理
- **WHEN** 通信阶段启动多线程处理
- **THEN** 队列中的发送/接收消息被并行处理且保持 AT 一问一答约束
