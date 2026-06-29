#include "game/TankGame.h"

#include "GameFramework.h"
#include "game/World.h"
#include "game/component/Camera.h"
#include "game/component/StaticMeshComponent.h"
#include "game/entity/EnemyTankActor.h"
#include "game/entity/GroundActor.h"
#include "game/entity/PlayerTankActor.h"
#include "game/entity/SkyBoxActor.h"
#include "game/entity/WallActor.h"
#include "gameframework/assets/AssetCommon.h"
#include "gameframework/log/LogSystem.h"

#include <cassert>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>

using namespace Engineframework;
namespace Game
{
TankGame::~TankGame() = default;

void TankGame::onInit()
{
    skybox = World::sMain.spawnActor<SkyBoxActor>(nullptr);
    ground = World::sMain.spawnActor<GroundActor>(nullptr);
    tank = World::sMain.spawnActor<PlayerTankActor>(nullptr);
    wall = World::sMain.spawnActor<WallActor>(nullptr);

    spawnEnemies();
}

void TankGame::onUpdate(const Engineframework::Timestep& ts)
{
    isDrawAABB = false;
    World::sMain.tickAllActor(ts);
    drawAllAABB();
    Camera::sMain.update(ts.getSeconds());
}

void TankGame::onShutdown()
{
    World::sMain.shutdown();
}

void TankGame::spawnEnemies()
{
    float halfX = 6000.f;
    float halfZ = 6000.f;
    glm::vec3 playerPos = tank->transform->getPosition();

    int spawned = 0;
    while (spawned < enemyCount)
    {
        float x   = ((float)rand() / RAND_MAX) * halfX * 2.f - halfX;
        float z   = ((float)rand() / RAND_MAX) * halfZ * 2.f - halfZ;
        float yaw = ((float)rand() / RAND_MAX) * 360.f;

        glm::vec3 pos = glm::vec3(x, 10.f, z);
        if (glm::length(pos - playerPos) < 1500.f)
            continue;

        EnemyTankActor* enemy = World::sMain.spawnActor<EnemyTankActor>(nullptr);
        enemy->transform->setPosition(pos);
        enemy->transform->setRotation(0.f, yaw, 0.f);
        spawned++;
    }
}

void TankGame::drawAllAABB()
{
    if (!isDrawAABB)
    {
        return;
    }
    auto colliderList = World::sMain.getPhysicsWorld().getAllColliders();
    for (const auto& collider : colliderList)
    {
        Engineframework::GameFramework::sGet().Renderer().drawAabb(collider->getWorldAABB());
    }
}

}  // namespace Game
