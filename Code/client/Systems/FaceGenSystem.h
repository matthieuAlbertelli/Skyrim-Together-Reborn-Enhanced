#pragma once

struct World;
struct NiTriBasedGeom;
struct Actor;
struct FaceGenComponent;
struct Tints;

/**
 * @brief Manages the face gen of remote players.
 */
struct FaceGenSystem
{
    static NiTriBasedGeom* GetHeadGeometry(Actor* apActor) noexcept;
    static void Update(World& aWorld, Actor* apActor, FaceGenComponent& aFaceGenComponent) noexcept;
    static void Setup(World& aWorld, entt::entity aEntity, const Tints& acTints) noexcept;
};
