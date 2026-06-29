#include "PlayerHUDView.h"

#include "game/World.h"
#include "game/entity/TankActor.h"
#include "imgui.h"

namespace Game
{
void PlayerHUDView::draw()
{
    Game::Actor* playerActor = Game::World::sMain.findPlayerActor();
    if (!playerActor)
        return;

    Game::TankActor* player = dynamic_cast<Game::TankActor*>(playerActor);
    if (!player)
        return;

    float current = player->getHp();
    float max = player->getMaxHp();
    float ratio = (max > 0.f) ? (current / max) : 0.f;
    ImGui::SetNextWindowPos(ImVec2(20, 80), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(250, 60), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("HUD",
                 nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::Text("HP:%.0f/%.0f", current, max);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float w = 200.f, h = 16.f;
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), IM_COL32(80, 80, 80, 200));
    ImU32 color = (ratio > 0.5f) ? IM_COL32(50, 200, 50, 230) : IM_COL32(200, 50, 50, 230);
    dl->AddRectFilled(pos, ImVec2(pos.x + w * ratio, pos.y + h), color);
    ImGui::End();
    ImGui::PopStyleVar();
}
}  // namespace Game