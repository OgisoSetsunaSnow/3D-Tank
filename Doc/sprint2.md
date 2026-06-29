# 目标
双平台具有一个物理碰撞和摄像机跟随的坦克
- 项目架构
- 渲染
- 物体交互
- GamePlay
<!--///-->
# 项目架构
- 优化了主循环结构
- 将引擎层各个功能变成单例
- 创建World类来统一创建和销毁Actor
- Component增加一个Start生命周期来让Component之间可以互相调用
```C++

int WinMain(_In_ HINSTANCE /* hInstance */,
            _In_opt_ HINSTANCE /* hPrevInstance */,
            _In_ LPSTR /* lpCmdLine */,
            _In_ int /* nCmdShow */)
{
    auto tankGame = std::make_unique<Game::TankGame>();
    GameFramework::sGet().mainLoop(tankGame.get());
}

int GameFramework::mainLoop(Game::IGame* game)
{
    game_ = game;
    init();
    while (!m_rendererManager.shouldQuit())
    {
        Timestep ts = timer_.tick();
        update(ts);
        render();
    }
    shutdown();
    return 1;
}
```

<!--///-->
# 渲染
- 实现了天空盒的渲染
- 创建RenderManager和RenderPass来统一管理渲染的物体，对渲染的物体进行排序

<!--///-->
# 物体交互
- AABB数据结构
- AABB的重叠检测
- 离散SAT检测 用于坦克移动
- 射线检测
-------



<!--///-->
# GamePlay
- 相机杆组件实现了相机跟随玩家，和视角的转动
- 使用状态机来处理PlayerTankActor的状态切换
- 创建CharacterControllerComponent 实现了基本的运动学控制器，实现了移动，重力模拟，地面检测，跳跃，发射子弹等逻辑
- EnemyTankActor的随机生成，使用状态机实现了简单的敌人AI
<!--///-->