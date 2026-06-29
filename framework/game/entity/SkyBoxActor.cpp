#include "game/entity/SkyBoxActor.h"

#include "game/component/SkyComponent.h"
namespace Game
{
SkyBoxActor::SkyBoxActor()
{
    addComponent<SkyComponent>("assets/tex/skybox.tga");
}

void SkyBoxActor::init()
{
    Actor::init();
}
void SkyBoxActor::tick(const Engineframework::Timestep& time)
{
    //
    Actor::tick(time);
}
SkyBoxActor::~SkyBoxActor() {}
}  // namespace Game1. 