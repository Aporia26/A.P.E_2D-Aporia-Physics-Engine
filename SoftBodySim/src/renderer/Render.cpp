#include "Render.hpp"
#include <iostream>
#include<algorithm>
#include<cmath>

namespace Renderer
{

     namespace    // an unnamed namespace makes those variables private to Renderer
    {
        sf::RenderWindow*      s_window    = nullptr;
        const SimMode*         s_mode      = nullptr;
        const ClothData*       s_cloth     = nullptr;
        const ClothData*       s_jelly     = nullptr;
        const FluidSimData*    s_fluid     = nullptr;
        const RigidSphereData* s_spheres   = nullptr;
        const sf::Vector2f*    s_mousePos  = nullptr;
        const bool*            s_leftDown  = nullptr;
        const bool*            s_rightDown = nullptr;

        sf::VertexArray s_fabric(sf::PrimitiveType::Triangles);
        sf::VertexArray s_weave(sf::PrimitiveType::Lines);

        // Shockwave pulse state for Jelly mode left-click blast
        sf::Clock    s_pulseClock;
        sf::Vector2f s_pulseOrigin{0.f, 0.f};
        bool         s_pulseActive = false;
        bool         s_wasLeftDown = false;
    }

    void init(sf::RenderWindow& window, const SimMode& mode, const ClothData& cloth, const ClothData& jelly, const FluidSimData& fluid, const RigidSphereData& spheres, const sf::Vector2f& mousePos, const bool& leftDown, const bool& rightDown)
    {
        s_window    = &window;
        s_mode      = &mode;
        s_jelly     = &jelly;
        s_cloth     = &cloth;
        s_fluid     = &fluid;
        s_spheres   = &spheres;
        s_mousePos  = &mousePos;
        s_leftDown  = &leftDown;
        s_rightDown = &rightDown;
    }

    void render()
    {
        if (!s_window)
         {
            std::cerr << "Renderer Error: s_window is null!\n";
            return;
        }   
        if (!s_window || !s_cloth || !s_spheres) return;

        if (!s_window || !s_mode) return;

        if (*s_mode == SimMode::Cloth)
        {
            drawCloth();
        }
        else if (*s_mode == SimMode::Jelly)
        {
            drawJelly();
            drawSpheres();
        }
        else if (*s_mode == SimMode::Fluid)
        {
            drawFluid();
        }
         drawCursor();
     }

    void drawCloth()
    {
        if (!s_window || !s_cloth) return;

        sf::RenderWindow& window = *s_window;
        const ClothData& cloth   = *s_cloth;

        if (cloth.nodes.empty()) return;

        s_weave.clear();

        // Render all structural & cross-diagonal threads 
        for (const auto& spring : cloth.springs)
        {
            if (!spring.isActive) continue;

            const AML::vec2& pA = cloth.nodes[spring.nodeA].pos;
            const AML::vec2& pB = cloth.nodes[spring.nodeB].pos;

            float dx = pA.x - pB.x;
            float dy = pA.y - pB.y;
            float len = std::sqrt(dx * dx + dy * dy);
            float strain = std::clamp((len - spring.restLen) / (spring.restLen * 0.35f), 0.f, 1.f);

            bool isDiag = (spring.type == 2);
            std::uint8_t baseAlpha = isDiag ? 38 : 175;
            std::uint8_t alpha = static_cast<std::uint8_t>(std::min(255.f, baseAlpha + 80.f * strain));

            // threads that brighten toward pure white under tension
            sf::Color threadCol(
                static_cast<std::uint8_t>(std::min(255.f, 210.f + 45.f * strain)),
                static_cast<std::uint8_t>(15 + 55 * strain),
                static_cast<std::uint8_t>(50 + 45 * strain),
                alpha
            );

            s_weave.append(sf::Vertex{sf::Vector2f{pA.x, pA.y}, threadCol});
            s_weave.append(sf::Vertex{sf::Vector2f{pB.x, pB.y}, threadCol});
        }
        window.draw(s_weave);

        //  Subtle micro-dots at grid intersections
        sf::CircleShape joint(1.6f);
        joint.setOrigin({1.6f, 1.6f});
        joint.setFillColor(sf::Color(210, 15, 50, 140));

        // 4 anchor pins along the top edge
        sf::CircleShape pin(4.8f);
        pin.setOrigin({4.8f, 4.8f});
        pin.setFillColor(sf::Color(255, 95, 31));
        pin.setOutlineThickness(1.5f);
        pin.setOutlineColor(sf::Color(255, 92, 0));

        for (const auto& node : cloth.nodes)
        {
            if (node.pinned || node.invMass == 0.f)
            {
                pin.setPosition({node.pos.x, node.pos.y});
                window.draw(pin);
            }
            else
            {
                joint.setPosition({node.pos.x, node.pos.y});
                window.draw(joint);
            }
        }
    }

    void drawSpheres()
    {
      if (!s_window || !s_spheres) return;

        sf::RenderWindow& window       = *s_window;
        const RigidSphereData& spheres = *s_spheres;

        // Floor boundary line (matches floorY = 750 - 40 = 710)
        sf::RectangleShape floorLine({920.f, 1.5f});
        floorLine.setPosition({40.f, 710.f});
        floorLine.setFillColor(sf::Color(255, 255, 255, 50));
        window.draw(floorLine);

        // Semi-Implicit Euler rigid sphere
        sf::CircleShape sphere(spheres.radius);
        sphere.setOrigin({spheres.radius, spheres.radius});
        sphere.setPosition({spheres.pos.x, spheres.pos.y});
        sphere.setFillColor(sf::Color(235, 155, 55));
        sphere.setOutlineThickness(2.f);
        sphere.setOutlineColor(sf::Color(255, 220, 175));
        window.draw(sphere);
    }

    void drawJelly()
    {
        if (!s_window || !s_jelly) return;

        sf::RenderWindow& window = *s_window;
        const ClothData& jelly   = *s_jelly;
        if (jelly.nodes.empty()) return;

        s_fabric.clear();
        s_weave.clear();

        // Translucent gelatin fill
        const sf::Color jellyFill(25, 215, 165, 65);
        for (int r = 0; r < jelly.rows - 1; ++r)
        {
            for (int c = 0; c < jelly.cols - 1; ++c)
            {
                std::size_t tl = static_cast<std::size_t>(r * jelly.cols + c);
                std::size_t tr = tl + 1;
                std::size_t bl = static_cast<std::size_t>((r + 1) * jelly.cols + c);
                std::size_t br = bl + 1;

                sf::Vector2f pTL{jelly.nodes[tl].pos.x, jelly.nodes[tl].pos.y};
                sf::Vector2f pTR{jelly.nodes[tr].pos.x, jelly.nodes[tr].pos.y};
                sf::Vector2f pBL{jelly.nodes[bl].pos.x, jelly.nodes[bl].pos.y};
                sf::Vector2f pBR{jelly.nodes[br].pos.x, jelly.nodes[br].pos.y};

                s_fabric.append(sf::Vertex{pTL, jellyFill});
                s_fabric.append(sf::Vertex{pTR, jellyFill});
                s_fabric.append(sf::Vertex{pBR, jellyFill});

                s_fabric.append(sf::Vertex{pTL, jellyFill});
                s_fabric.append(sf::Vertex{pBR, jellyFill});
                s_fabric.append(sf::Vertex{pBL, jellyFill});
            }
        }
        window.draw(s_fabric);

        // Glowing internal spring lattice
        for (const auto& s : jelly.springs)
        {
            if (!s.isActive) continue;
            const AML::vec2& pA = jelly.nodes[s.nodeA].pos;
            const AML::vec2& pB = jelly.nodes[s.nodeB].pos;

            std::uint8_t alpha = (s.type == 2) ? 45 : 185;
            sf::Color edgeCol(60, 245, 195, alpha);

            s_weave.append(sf::Vertex{sf::Vector2f{pA.x, pA.y}, edgeCol});
            s_weave.append(sf::Vertex{sf::Vector2f{pB.x, pB.y}, edgeCol});
        }
        window.draw(s_weave);
    }

    void drawFluid()
    {
        sf::RenderWindow& window  = *s_window;
        const FluidSimData& fluid = *s_fluid;

        sf::Vector2f tankPos{fluid.tank.min_pt.x, fluid.tank.min_pt.y};
        sf::Vector2f tankSize{
            fluid.tank.max_pt.x - fluid.tank.min_pt.x,
            fluid.tank.max_pt.y - fluid.tank.min_pt.y
        };

        // Blend color from clear cyan water (0.02) -> golden-amber honey (0.85)
        float t = std::clamp((fluid.viscosity - 0.02f) / 0.83f, 0.f, 1.f);

        sf::Color fillCol(
            static_cast<std::uint8_t>(0   + 235 * t),
            static_cast<std::uint8_t>(145 + 15  * t),
            static_cast<std::uint8_t>(255 - 225 * t),
            static_cast<std::uint8_t>(85  + 75  * t) // Thicker liquid looks slightly denser
        );

        sf::Color outlineCol(
            static_cast<std::uint8_t>(0   + 255 * t),
            static_cast<std::uint8_t>(210 - 15  * t),
            static_cast<std::uint8_t>(255 - 205 * t),
            220
        );

         // Viscous liquid 
        sf::RectangleShape liquid(tankSize);
        liquid.setPosition(tankPos);
        liquid.setFillColor(fillCol);
        liquid.setOutlineThickness(1.5f);
        liquid.setOutlineColor(outlineCol);
        window.draw(liquid);

        //container)
        sf::RectangleShape container({tankSize.x, tankSize.y + 40.f});
        container.setPosition({tankPos.x, tankPos.y - 40.f});
        container.setFillColor(sf::Color::Transparent);
        container.setOutlineThickness(3.f);
        container.setOutlineColor(sf::Color(180, 210, 240, 140));
        window.draw(container);

        // Dropping sphere 
        float r = fluid.sphere.radius;
        sf::CircleShape ball(r);
        ball.setOrigin({r, r});
        ball.setPosition({fluid.sphere.center.x, fluid.sphere.center.y});
        ball.setFillColor(sf::Color::Red);
        ball.setOutlineThickness(2.f);
        ball.setOutlineColor(sf::Color::White);
        window.draw(ball);

    }

        void drawCursor()
    {
        if (!s_window || !s_mousePos || !s_leftDown || !s_rightDown || !s_mode) return;

        sf::RenderWindow& window = *s_window;
        const sf::Vector2f pos   = *s_mousePos;
        const bool leftDown      = *s_leftDown;
        const bool cutting       = *s_rightDown;
        const bool isJellyMode   = (*s_mode == SimMode::Jelly);

        // Detect rising edge of left-click in Jelly mode to trigger a shockwave pulse
        if (isJellyMode && leftDown && !s_wasLeftDown)
        {
            s_pulseOrigin = pos;
            s_pulseActive = true;
            s_pulseClock.restart();
        }
        s_wasLeftDown = leftDown;

        // 1. Expanding shockwave ring (Jelly mode blast feedback)
        if (s_pulseActive)
        {
            constexpr float duration = 0.35f;
            float elapsed = s_pulseClock.getElapsedTime().asSeconds();

            if (elapsed >= duration)
            {
                s_pulseActive = false;
            }
            else
            {
                float t = elapsed / duration;
                float easeOut = 1.f - (1.f - t) * (1.f - t); // Smooth deceleration
                float radius = 15.f + easeOut * 225.f;        // Expands out to 240px blastRadius
                std::uint8_t alpha = static_cast<std::uint8_t>((1.f - t) * 200.f);

                sf::CircleShape shockwave(radius);
                shockwave.setOrigin({radius, radius});
                shockwave.setPosition(s_pulseOrigin);
                shockwave.setFillColor(sf::Color(60, 245, 195, static_cast<std::uint8_t>(alpha * 0.12f)));
                shockwave.setOutlineThickness(2.2f * (1.f - t * 0.5f));
                shockwave.setOutlineColor(sf::Color(60, 245, 195, alpha));
                window.draw(shockwave);
            }
        }

        // 2. Continuous field / grab ring while holding left-click
        if (leftDown)
        {
            if (isJellyMode)
            {
                // Repulsive force-field halo in Jelly mode
                constexpr float fieldRadius = 160.f;
                sf::CircleShape fieldRing(fieldRadius);
                fieldRing.setOrigin({fieldRadius, fieldRadius});
                fieldRing.setPosition(pos);
                fieldRing.setFillColor(sf::Color(60, 245, 195, 12));
                fieldRing.setOutlineThickness(1.f);
                fieldRing.setOutlineColor(sf::Color(60, 245, 195, 55));
                window.draw(fieldRing);
            }
            else
            {
                // Grab radius ring in Cloth mode
                constexpr float grabRadius = 70.f;
                sf::CircleShape grabRing(grabRadius);
                grabRing.setOrigin({grabRadius, grabRadius});
                grabRing.setPosition(pos);
                grabRing.setFillColor(sf::Color(255, 255, 255, 14));
                grabRing.setOutlineThickness(1.2f);
                grabRing.setOutlineColor(sf::Color(255, 255, 255, 75));
                window.draw(grabRing);
            }
        }

        // 3. Vector scissor cursor (always drawn on top)
        const float bladeAngle    = cutting ? 10.f : 26.f;
        const sf::Color bladeCol  = cutting ? sf::Color(255, 75, 95) : sf::Color(220, 228, 240);
        const sf::Color handleCol = cutting ? sf::Color(210, 15, 50) : sf::Color(150, 162, 180);

        auto drawHalfScissor = [&](float angleDeg, float handleOffsetY)
        {
            sf::RectangleShape blade({18.f, 2.f});
            blade.setOrigin({3.f, 1.f});
            blade.setPosition(pos);
            blade.setRotation(sf::degrees(angleDeg));
            blade.setFillColor(bladeCol);
            window.draw(blade);

            float rad = (angleDeg + 180.f) * 3.14159265f / 180.f;
            sf::Vector2f ringPos = pos + sf::Vector2f{std::cos(rad) * 10.f, std::sin(rad) * 10.f + handleOffsetY};

            sf::CircleShape ring(4.2f);
            ring.setOrigin({4.2f, 4.2f});
            ring.setPosition(ringPos);
            ring.setFillColor(sf::Color::Transparent);
            ring.setOutlineThickness(1.6f);
            ring.setOutlineColor(handleCol);
            window.draw(ring);
        };

        drawHalfScissor(-bladeAngle, -1.5f);
        drawHalfScissor(bladeAngle,   1.5f);

        sf::CircleShape pivot(1.8f);
        pivot.setOrigin({1.8f, 1.8f});
        pivot.setPosition(pos);
        pivot.setFillColor(sf::Color::White);
        window.draw(pivot);
    }

    void Clean()
    {
        s_fabric.clear();
        s_weave.clear();

        s_window    = nullptr;
        s_cloth     = nullptr;
        s_spheres   = nullptr;
        s_mousePos  = nullptr;
        s_leftDown  = nullptr;
        s_rightDown = nullptr;
        s_jelly = nullptr;
        s_mode = nullptr;
        s_pulseActive = false;
        s_wasLeftDown = false;
    }

}