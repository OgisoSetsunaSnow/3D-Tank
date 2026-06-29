# 1. 概述
1.能通过ImGui绘制窗口，将Log信息显示在窗口上
2.能够通过成员来进行Log的分区
3.能根据不同Log的级别来将信息显示为不同颜色
4.能够关闭和打开ImGui的Log调试窗口
5.因为使用ImGui和DX12，所以与框架Render相关联
# 2. Log系统需求
1. 支持按分区输出日志，每个人有独立的分区
2. 支持三种日志级别：`INFO/WARNING/ERROR`不同颜色的打印
3. 日志在ImGui窗口内实时显示
4. 支持按级别过滤显示
# 3. Log系统架构设计
Log系统设计目标是：在不破坏现有框架渲染流程的前提下，提供一套可在游戏窗口内实时查看的调试日志系统。调用方可以在任意模块中通过统一接口输出日志，LogSystem负责收集、分类、保存日志，并在每帧通过ImGui绘制到屏幕上。
## 3.1 整体结构
Log 系统整体采用“调用方->LogSystem->ImGui UI显示”的结构。
调用方不直接操作ImGui,也不直接负责绘制文字。  
调用方只需要通过宏接口提交日志，例如：
```CPP
LOG_INFO("HJL", "Hello");
```
日志提交后，由LogSystem统一管理。
LogSystem内部按照Channel对日志进行分区保存。
每个Channel中保存对应模块产生的日志记录。
# 4.Log窗口设计
DrawImGuiWindow()是Log窗口的总入口函数,设置窗口初始大小，开启窗口，绘制工具栏，绘制分区标签页，关闭窗口。
```CPP
ImGui::SetNextWindowSize();
该函数设置了窗口的大小，并且在用户拖拽调整了大小也不会被重置
DrawToolbar()；
这是绘制工具栏的函数，里面整理排列了按钮
DrawToggleButton();
这是过滤按钮函数，实现点击按钮查看还是过滤该状态类型的Log信息
DrawChannelTabs()；
这是Channel标签页绘制函数，创建Tab栏BeginTabBar，生成All总览标签页，遍历 m_Channels，为每个分区生成一个标签页，每个分区标签页内提供独立的Clear按钮
调用DrawChannelEntries() 绘制对应日志内容
```
# 5.游戏通用的Log设计方案
在游戏开发中，Log 系统通常不是单纯的`printf()`输出，而是一套用于调试、问题定位和运行状态观察的基础工具。  
一个较完整的游戏Log系统一般会包含日志级别、日志来源、日志存储、日志过滤以及多个输出目标。
本设计会参考通用 Log 系统的核心思想，但在实现上进行简化。
如学习，日志级别，日志来源，日志过滤，分区查看等功能。
# 6.API设计
使用宏定义简化调用方式，如下写日志接口的调用
```CPP
LogSystem::Get().AddLog("HJL", LogLevel::Info, "Hello");
简化为：
LOG_INFO ("HJL", "Hello");
Info为白色，Warning是黄色，Error是红色
```
绘制接口
```CPP
LogSystem::Get().DrawImGuiWindow();
```
过滤按钮接口
```CPP
void DrawToggleButton();
```
Log信息分区标签页接口
```CPP
void DrawChannelTabs();
```
# 7.验证
本章节用于说明Log系统开发完成后的验证方式。  
验证目标是确认Log系统能够正常接收日志、按分区保存日志、按级别显示不同颜色，并且能够通过ImGui窗口实时显示到游戏画面中，同时不影响现有 DX12 渲染流程。
## 7.1编译验证
确认新增的LogSystem、ImGui窗口绘制代码和宏接口能够正常编译。
## 7.2ImGui 窗口显示验证
确认Log窗口能够显示在游戏窗口内。
确认Log窗口能够显示正常的Log信息，颜色，字体和内容无任何错误。
## 7.3日志分区验证
确认Log系统支持按Channel分区保存和显示日志。
<!-- # 8.风险估算
本功能的主要风险是ImGui需要接入现有DX12 Render流程，因此会依赖Device、CommandList、DescriptorHeap等渲染资源，与Render模块存在一定耦合。若接入位置或资源复用不正确，可能导致ImGui窗口无法显示或影响原有渲染。为降低风险，LogSystem只负责日志管理和ImGui UI描述，DX12初始化与绘制提交放在Render/平台层处理 -->