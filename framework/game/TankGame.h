#pragma once
#include "game/IGame.h"
#include "game/World.h"
namespace Engineframework
{
class Gameframework;
}  // namespace Engineframework
namespace Game
{
class PlayerTankActor;
class GroundActor;
class SkyBoxActor;
class WallActor;

class EnemyTankActor;
class TankGame : public IGame
{
public:
    TankGame() = default;
    ~TankGame();
    TankGame(const TankGame&) = delete;
    TankGame& operator=(const TankGame&) = delete;
    void onInit() override;
    void onUpdate(const Engineframework::Timestep& ts) override;
    void onShutdown() override;

private:
    void spawnEnemies();

    SkyBoxActor* skybox = nullptr;
    PlayerTankActor* tank = nullptr;
    GroundActor* ground = nullptr;
    WallActor* wall = nullptr;

    std::vector<EnemyTankActor*> m_enemies;
    bool isDrawAABB = false;

    int enemyCount = 20;
    void drawAllAABB();
};
}  // namespace Game
