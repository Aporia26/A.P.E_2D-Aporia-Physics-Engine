#pragma once
#include<SFML/Graphics.hpp>
#include "physics/DataTypes.hpp"
#include "app/Backend.hpp"

namespace Renderer
{

    void init(sf::RenderWindow& window,
              const SimMode& mode,
              const ClothData& cloth,
              const ClothData& jelly,
              const FluidSimData& fluid,
              const RigidSphereData& spheres,
              const sf::Vector2f& mousePos,
              const bool& leftDown,
              const bool& rightDown);

    void render();

    void drawCloth();
    void drawJelly();
    void drawFluid();
    void drawSpheres();
    void drawCursor();

    void Clean();

}