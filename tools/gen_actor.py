#!/usr/bin/env python3
"""
Actor Code Generator for Game Project
Usage:
    python gen_actor.py WallActor --components StaticMeshComponent CollisionComponent
    python gen_actor.py BulletActor --components StaticMeshComponent --namespace Game
    python gen_actor.py EnemyActor --components StaticMeshComponent CollisionComponent CharacterControllerComponent --output src/game/entity
"""

import argparse
import os
from datetime import date

# ─────────────────────────────────────────────
# 已知 Component → include 路径 映射表
# 如有新 Component 直接在此扩展
# ─────────────────────────────────────────────
COMPONENT_INCLUDE_MAP = {
    "StaticMeshComponent":          "game/component/StaticMeshComponent.h",
    "CollisionComponent":           "game/component/CollisionComponent.h",
    "CharacterControllerComponent": "game/component/CharacterControllerComponent.h",
    "SpringArmComponent":           "game/component/SpringArmComponent.h",
    "Camera":                       "game/component/Camera.h",
}

# ─────────────────────────────────────────────
# 模板：.h
# ─────────────────────────────────────────────
HEADER_TEMPLATE = '''\
/**
 * @file {actor_name}.h
 * @brief Auto-generated Actor header
 * @date {date}
 */
#pragma once
#include "game/entity/Actor.h"
#include "gameframework/GameFramework.h"
{component_includes}
namespace {namespace}
{{
class {actor_name} : public Actor
{{
public:
    {actor_name}();
    void init() override;
    void tick(const Engineframework::Timestep& time) override;
    ~{actor_name}() override;

private:
{member_declarations}
}};
}}  // namespace {namespace}
'''

# ─────────────────────────────────────────────
# 模板：.cpp
# ─────────────────────────────────────────────
SOURCE_TEMPLATE = '''\
#include "game/entity/{actor_name}.h"

#include "game/World.h"

namespace {namespace}
{{
{actor_name}::{actor_name}()
{{
{add_component_calls}
    name = "{actor_name}";
}}

void {actor_name}::init()
{{
    Actor::init();
    // TODO: initialize {actor_name} logic
}}

void {actor_name}::tick(const Engineframework::Timestep& time)
{{
    Actor::tick(time);
    // TODO: per-frame logic
}}

{actor_name}::~{actor_name}() {{}}

}}  // namespace {namespace}
'''


def resolve_includes(components: list[str]) -> str:
    """生成 #include 行，未知组件给出 TODO 提示"""
    lines = []
    for comp in components:
        if comp in COMPONENT_INCLUDE_MAP:
            lines.append(f'#include "{COMPONENT_INCLUDE_MAP[comp]}"')
        else:
            lines.append(f'// TODO: add include for {comp}')
    return "\n".join(lines)


def resolve_members(components: list[str]) -> str:
    """生成私有成员指针声明"""
    lines = []
    for comp in components:
        # e.g. StaticMeshComponent → m_staticMeshComponent
        camel = comp[0].lower() + comp[1:]
        lines.append(f"    {comp}* m_{camel} = nullptr;")
    return "\n".join(lines) if lines else "    // No components"


def resolve_add_calls(components: list[str]) -> str:
    """生成构造函数中 addComponent 调用"""
    lines = []
    for comp in components:
        camel = comp[0].lower() + comp[1:]

        # StaticMeshComponent 需要额外参数
        if comp == "StaticMeshComponent":
            lines.append(
                f'    m_{camel} = addComponent<{comp}>("assets/mesh/TODO.OBJ", "assets/tex/TODO.tga");'
            )
        else:
            lines.append(f"    m_{camel} = addComponent<{comp}>();")
    return "\n".join(lines) if lines else "    // No components added"


def generate(actor_name: str, components: list[str], namespace: str, output_dir: str):
    today = date.today().isoformat()

    header_content = HEADER_TEMPLATE.format(
        actor_name=actor_name,
        date=today,
        namespace=namespace,
        component_includes=resolve_includes(components),
        member_declarations=resolve_members(components),
    )

    source_content = SOURCE_TEMPLATE.format(
        actor_name=actor_name,
        namespace=namespace,
        add_component_calls=resolve_add_calls(components),
    )

    os.makedirs(output_dir, exist_ok=True)
    header_path = os.path.join(output_dir, f"{actor_name}.h")
    source_path = os.path.join(output_dir, f"{actor_name}.cpp")

    with open(header_path, "w", encoding="utf-8") as f:
        f.write(header_content)
    with open(source_path, "w", encoding="utf-8") as f:
        f.write(source_content)

    print(f"✅ Generated:")
    print(f"   {header_path}")
    print(f"   {source_path}")
    print()
    print("─── Preview: Header ───────────────────────────")
    print(header_content)
    print("─── Preview: Source ───────────────────────────")
    print(source_content)


def main():
    parser = argparse.ArgumentParser(
        description="🎮 Virtuos Actor Code Generator",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("actor_name", help="Actor class name, e.g. WallActor")
    parser.add_argument(
        "--components", "-c",
        nargs="*",
        default=[],
        help="Components to mount, e.g. StaticMeshComponent CollisionComponent",
    )
    parser.add_argument(
        "--namespace", "-n",
        default="Game",
        help="C++ namespace (default: Game)",
    )
    parser.add_argument(
        "--output", "-o",
        default="src/game/entity",
        help="Output directory (default: src/game/entity)",
    )
    args = parser.parse_args()
    generate(args.actor_name, args.components, args.namespace, args.output)


if __name__ == "__main__":
    main()