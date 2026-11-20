#pragma once
#include "AML/AML.hpp"
#include<SFML/System/Vector2.hpp>
#include<vector>
#include<cstdint>

struct Node
{
    AML::vec2 pos{0.f, 0.f};
    AML::vec2 prevPos{0.f, 0.f};
    AML::vec2 force{0.f, 0.f};
   float invMass{1.5f};   // 0.0f means pinned/immovable
    bool pinned{false};
};

struct Spring
{
    std::size_t nodeA{0};
    std::size_t nodeB{0};
    float restLen{0.f};
    float k{0.f};
    bool isActive{true};
    std::uint8_t type{0}; // 0: horiz, 1: vert, 2: visible diag (TL->BR), 3: hidden diag
};

struct ClothData
{
    int cols{20};
    int rows{15};
    float restArea{0.f}; // Target area for internal jelly pressure
    std::vector<Node> nodes;
    std::vector<Spring> springs;
};

struct RigidSphereData
{
    AML::vec2 pos{500.f, 370.f};
    AML::vec2 vel{0.f, 0.f};
    AML::vec2 force{0.f, 0.f};
    float radius{10.f};
    float invMass{0.1f};
};

struct FluidSimData
{
    AML::circle sphere{{500.f, 120.f}, 28.f}; 
    AML::vec2   vel{0.f, 0.f};
    AML::vec2   force{0.f, 0.f};
    float       invMass{0.05f};      // Mass = 20.0f

    AML::AABB2D tank{{250.f, 340.f}, {750.f, 720.f}}; // Viscous fluid rectangle
    float       viscosity{0.18f};        // eta ( 0.05 = water, 0.45 = honey)
    float       fluidDensity{0.0045f};   // rho
};

namespace Phys
{
    void buildClothGrid(ClothData& cloth, float worldCenterX, float startY);
    void stepCloth(ClothData& cloth, float dt, sf::Vector2f mousePos, bool rightDown, bool leftDown, bool gravityEnabled = true);

    void buildJellyCube(ClothData& jelly, RigidSphereData& sphere, float worldCenterX, float startY);
    void stepJellyAndSphere(ClothData& jelly, RigidSphereData& sphere, float dt, sf::Vector2f mousePos,
                            bool leftClicked, bool leftDown, bool rightDown, bool gravityEnabled, sf::Vector2f worldBounds);

    void buildFluidSim(FluidSimData& data, float worldCenterX); 
    void stepFluidSim(FluidSimData& data, float dt, sf::Vector2f mousePos, bool leftClicked, bool gravityEnabled);
}