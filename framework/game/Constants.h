#pragma once
#include <glm/glm.hpp>

// 游戏状态
enum class GameState
{
    MENU,
    PLAYING,
    GAME_OVER
};

// 武器类型
enum class WeaponType
{
    CANNON,  // 主炮（10能量）
};

// Actor类型
enum class ActorType
{
    PLAYER,
    GROUND,
    ENEMY,
    NONE,
    BULLET,
};
