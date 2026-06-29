# 概述
当前的游戏框架中，RenderInterface的初始化，窗口的初始化的都放在MainWin函数中。引擎层和逻辑层的生命周期区分不够明显。并且当前框架中只有Render的概念，为了后续系统能够开发游戏变成一个小型引擎，需要对当前框架进行重构引入主循环和基础的Entity Component架构。实现了一下功能
1. 构建游戏主循环，对各种系统进行初始化。编写Timer类来获取稳定的帧间隔时间方便GameLoop使用
2. 基础Entiy Component系统的构建。创建World类来创建Entity，Actor类
3. 封装MeshComponent类来提供任意模型的渲染
4. 封装TransformComponent类来提供Static Mesh的移动旋转
<!-- 1. 边界范围
1. 为什么要构建这个系统
2. 依然哪些系统
3. 关联文档 -->
# 关键技术选择
## 组件模式和ECS
### ECS
采用Manager + Component的方式
```C++
Entity tank = EntityManager::sCreaeteEntity();
Transform* transform = TransformManager::sAddComponent(this, ...);
transform = TransformManager::sGetComponent(this);
```
对于ECS来说实现的困难主要有一下这些。如何在连续的内存中快速找到对应的Entity或者Component。具体逻辑不是写在类似Tank类中，而是由System来负责逻辑的编写，增加了程序编写的困难。此外这样的设计也会造成Component生命周期内存泄漏问题，在没有GC的情况下
### 组件模式
```C++
Entity tank = word->Spawn();
Transform* transform = tank->AddComponent<Transform>(...);
transform = tank->GetComponent();
```
综上该项目的基础设计还是参考UE的OOP进行设计，让Entity自身保存所有的Component。创建一个Word类来处理所有的Entity的生成和销毁。来解决Component生命周期的问题
# 实现细节
## 主循环
这个项目中将采用 WinApp -> GameFramework -> Game 这样的架构来构建主循环。**该架构已完全实现。**

WinApp（`main_PC.cpp`）负责初始化 GameFramework 并将 IGame 对象传入 GameFramework 中。GameFramework 作为引擎层工具集来给各种 Component 提供底层的渲染、音频、**输入** 能力调用。Game 对象则负责真正的游戏逻辑和 GameLoop 的循环。

**当前实现的主循环调用顺序（GameFramework::run() 内部）**：
```
1. inputSystem_->Update()       ← 读取硬件按键状态
2. inputMapper_->update(dt)     ← 计算 Action / Axis 状态
3. game_->onUpdate(this, ts)    ← 游戏逻辑（只读输入）
4. renderer_->BeginFrame()
5. game_->onRender(this)
6. renderer_->EndFrame()
```

**IGame 接口方法签名**：
```cpp
virtual void onInit(GameFramework* fw) = 0;
virtual void onUpdate(GameFramework* fw, Timestep ts) = 0;  // fw 提供输入/渲染等服务
```

**输入系统已集成到 GameFramework：**
- `GameFramework` 内部持有 `IInputSystem*` 和 `InputMapper*`
- 游戏层通过 `fw->getInputMapper()` 只读访问，不需要自己管理输入生命周期

<!-- ![alt text](mainloop.png) -->
## 组件系统

组件采用类似UE中的组件系统，Entity维护自身的Component。其中每个Component也会持有对应的Entity。Entity使用模板函数来构建出对应的Component


<!-- ![alt text](component.png) -->

### StaticMeshComponent
在这个阶段每个StaticMeshComponent都会保存一份资源
```C++
void loadObj()
void draw()
```
### TransformComponent
TransformComponent 用来出来MVP矩阵的变换和自身的变换。存放父节点的TransformComponent，和自身所有子节点的transformComponent
```C++
```
# 不足
1. MeshComponent对于同一个资源的重复加载，后续使用引入技术来让多个Component引入同一个底层资源
2. TransformComponent中Scene Node系统实现完善。支持父子关系层级
3. DX12相关的封装还不够完善