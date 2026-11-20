#include "DataTypes.hpp"
#include <SFML/System/Vector2.hpp>
#include <algorithm>

namespace Phys
{
    // Boundary helpers: translate between AML and SFML at the edge
    inline sf::Vector2f toSF(const AML::vec2& v)
    {
        return sf::Vector2f{v.x, v.y};
    }

    inline AML::vec2 toAML(const sf::Vector2f& v)
    {
        return AML::vec2{v.x, v.y};
    }

    namespace
    {
        // Chapter 5: Partial Derivatives -> Elastic Spring Force F = -grad(U)
        void accumulateSpringForces(ClothData& cloth, float maxTear = 2.3f)
        {
            for (auto& s : cloth.springs)
            {
                if (!s.isActive) continue;

                Node& a = cloth.nodes[s.nodeA];
                Node& b = cloth.nodes[s.nodeB];

                AML::vec2 delta = a.pos - b.pos;
                float distSq = AML::length_sqr(delta);
                if (distSq <= 1e-12f) continue;

                float dist = std::sqrt(distSq);

                // Automatic tearing when stretched beyond maxTear ratio
                if (dist > s.restLen * maxTear)
                {
                    s.isActive = false;
                    continue;
                }

                // U = 0.5 * k * (|x_a - x_b| - L0)^2
                // F_a = -dU/dx_a = -k * (dist - L0) * (delta / dist)
                AML::vec2 dir = delta / dist;
                AML::vec2 force = dir * (-s.k * (dist - s.restLen));

                if (!a.pinned) a.force += force;
                if (!b.pinned) b.force -= force;
            }
        }

        // Chapter 4: Taylor Series -> Position Verlet Integration
        void integrateVerlet(ClothData& cloth, float dt, float damping)
        {
            const float dtSq = dt * dt;

            for (auto& n : cloth.nodes)
            {
                if (n.pinned || n.invMass == 0.f) continue;

                AML::vec2 vel = (n.pos - n.prevPos) * damping;
                AML::vec2 acc = n.force * n.invMass;

                n.prevPos = n.pos;
                n.pos += vel + acc * dtSq;

                // Reset force accumulator for next substep
                n.force = {0.f, 0.f};
            }
        }

        // Cutting Cloth State
        AML::vec2 s_prevMousePos{0.f, 0.f};
        bool s_wasRightDown = false;

        // Shortest squared distance from point P to line segment AB using AML::dot
        inline float distToSegmentSq(const AML::vec2& p, const AML::vec2& a, const AML::vec2& b)
        {
            AML::vec2 ab = b - a;
            AML::vec2 ap = p - a;
            float abLenSq = AML::length_sqr(ab);
            if (abLenSq <= 1e-6f) return AML::length_sqr(ap);

            float t = std::clamp(AML::dot(ap, ab) / abLenSq, 0.f, 1.f);
            AML::vec2 closest = a + ab * t;
            return AML::length_sqr(p - closest);
        }

        void cutAtPoint(ClothData& cloth, const AML::vec2& pt, float cutRadSq)
        {
            for (auto& s : cloth.springs)
            {
                if (!s.isActive) continue;

                const AML::vec2& pA = cloth.nodes[s.nodeA].pos;
                const AML::vec2& pB = cloth.nodes[s.nodeB].pos;

                if (distToSegmentSq(pt, pA, pB) <= cutRadSq)
                {
                    s.isActive = false;
                }
            }
        }

        void cutSpringsAlongSwipe(ClothData& cloth, const AML::vec2& start, const AML::vec2& end, float cutRadius)
        {
            const float cutRadSq = cutRadius * cutRadius;
            constexpr int samples = 5;

            for (int i = 0; i <= samples; ++i)
            {
                float alpha = static_cast<float>(i) / static_cast<float>(samples);
                AML::vec2 pt = start + (end - start) * alpha;
                cutAtPoint(cloth, pt, cutRadSq);
            }
        }

        void pullNodesNear(ClothData& cloth, const AML::vec2& cursor, float radius, float pullStrength)
        {
            const float radSq = radius * radius;

            for (auto& n : cloth.nodes)
            {
                if (n.pinned || n.invMass == 0.f) continue;

                AML::vec2 toCursor = cursor - n.pos;
                float distSq = AML::length_sqr(toCursor);

                if (distSq < radSq && distSq > 1e-4f)
                {
                    float dist = std::sqrt(distSq);
                    float falloff = 1.f - (dist / radius);
                    n.force += toCursor * (pullStrength * falloff);
                }
            }
        }

        // Radial shockwave impulse for Jelly & Semi-Euler Rigid Sphere
        void applyRadialImpulse(ClothData& jelly, RigidSphereData& sphere, const AML::vec2& origin,
                                float blastRadius, float sphereImpulse, float jellyImpulse)
        {
            // 1. Direct velocity impulse on Semi-Implicit Euler Rigid Sphere
            if (sphere.invMass > 0.f)
            {
                AML::vec2 delta = sphere.pos - origin;
                float dist = AML::length(delta);

                if (dist < blastRadius)
                {
                    AML::vec2 normal = (dist > 1e-4f) ? (delta / dist) : AML::vec2{0.f, -1.f};
                    float falloff = 1.f - (dist / blastRadius);
                    float dv = sphereImpulse * (0.35f + 0.65f * falloff) * sphere.invMass;

                    sphere.vel += normal * dv;
                    sphere.vel.y -= 180.f; // Slight upward pop so it arcs nicely
                }
            }

            // 2. Shockwave ripple on Position Verlet Jelly nodes
            for (auto& n : jelly.nodes)
            {
                if (n.pinned || n.invMass == 0.f) continue;

                AML::vec2 delta = n.pos - origin;
                float dist = AML::length(delta);

                if (dist < blastRadius && dist > 1e-4f)
                {
                    AML::vec2 normal = delta / dist;
                    float falloff = 1.f - (dist / blastRadius);
                    n.prevPos -= normal * (jellyImpulse * falloff);
                }
            }
        }

        // Collects outer boundary node indices in clockwise screen order (Top -> Right -> Bottom -> Left)
        std::vector<std::size_t> getPerimeterIndices(int cols, int rows)
        {
            std::vector<std::size_t> loop;
            if (cols < 2 || rows < 2) return loop;

            for (int c = 0; c < cols; ++c)
                loop.push_back(static_cast<std::size_t>(c));

            for (int r = 1; r < rows; ++r)
                loop.push_back(static_cast<std::size_t>(r * cols + (cols - 1)));

            for (int c = cols - 2; c >= 0; --c)
                loop.push_back(static_cast<std::size_t>((rows - 1) * cols + c));

            for (int r = rows - 2; r >= 1; --r)
                loop.push_back(static_cast<std::size_t>(r * cols));

            return loop;
        }

        // Shoelace Area using AML::cross (2D determinant x1*y2 - y1*x2)
        float computePolygonArea(const ClothData& jelly, const std::vector<std::size_t>& loop)
        {
            float area = 0.f;
            const std::size_t n = loop.size();
            for (std::size_t i = 0; i < n; ++i)
            {
                const AML::vec2& p1 = jelly.nodes[loop[i]].pos;
                const AML::vec2& p2 = jelly.nodes[loop[(i + 1) % n]].pos;
                area += AML::cross(p1, p2);
            }
            return std::abs(area) * 0.5f;
        }

        // Internal pressure force: inflates the jelly back toward restArea when compressed
        void applyInternalPressure(ClothData& jelly, float pressureStrength = 4200.f)
        {
            if (jelly.restArea <= 1e-3f) return;

            std::vector<std::size_t> loop = getPerimeterIndices(jelly.cols, jelly.rows);
            if (loop.size() < 3) return;

            float currentArea = computePolygonArea(jelly, loop);
            if (currentArea <= 1e-3f) return;

            float strain = (jelly.restArea - currentArea) / jelly.restArea;
            float pressure = pressureStrength * strain;

            const std::size_t n = loop.size();
            for (std::size_t i = 0; i < n; ++i)
            {
                Node& a = jelly.nodes[loop[i]];
                Node& b = jelly.nodes[loop[(i + 1) % n]];

                AML::vec2 edge = b.pos - a.pos;
                float edgeLen = AML::length(edge);
                if (edgeLen <= 1e-5f) continue;

                // Outward normal for clockwise loop in screen space (+Y down)
                AML::vec2 outwardNormal{edge.y / edgeLen, -edge.x / edgeLen};
                AML::vec2 edgeForce = outwardNormal * (pressure * edgeLen * 0.5f);

                if (!a.pinned) a.force += edgeForce;
                if (!b.pinned) b.force += edgeForce;
            }
        }
    }

    // ========================================================================
    // Cloth Physics
    // ========================================================================
    void buildClothGrid(ClothData& cloth, float worldCenterX, float startY)
    {
        cloth.nodes.clear();
        cloth.springs.clear();

        cloth.cols = 19; // Divides evenly into 4 anchor pins (cols 0, 6, 12, 18)
        cloth.rows = 14;
        const float spacing = 50.f;
        const float startX  = worldCenterX - ((cloth.cols - 1) * spacing) * 0.5f;

        for (int r = 0; r < cloth.rows; ++r)
        {
            float rowRatio = static_cast<float>(r) / static_cast<float>(cloth.rows);

            for (int c = 0; c < cloth.cols; ++c)
            {
                Node n;
                n.pos     = {startX + c * spacing, startY + r * spacing};
                n.force   = {0.f, 0.f};
                n.pinned  = (r == 0 && (c % 6 == 0)); // Exactly 4 pins: 0, 6, 12, 18
                n.invMass = n.pinned ? 0.f : 1.0f;

                if (n.pinned)
                {
                    n.prevPos = n.pos;
                }
                else
                {
                        AML::vec2 kick{std::sin(static_cast<float>(c) * 0.4f) * 1.2f * rowRatio, 2.2f * rowRatio};
                        n.prevPos = n.pos - kick;
                 }
            cloth.nodes.push_back(n);
            }
        }
        const float diagLen = spacing * std::sqrt(2.f);

        auto id = [&](int c, int r) -> std::size_t 
        {
            return static_cast<std::size_t>(r * cloth.cols + c);
        };

        for (int r = 0; r < cloth.rows; ++r) 
        {
            for (int c = 0; c < cloth.cols; ++c) {
                if (c + 1 < cloth.cols)
                    cloth.springs.push_back({id(c, r), id(c + 1, r), spacing, 3200.f, true, 0});
                if (r + 1 < cloth.rows)
                    cloth.springs.push_back({id(c, r), id(c, r + 1), spacing, 3200.f, true, 1});
                if (c + 1 < cloth.cols && r + 1 < cloth.rows)
                    cloth.springs.push_back({id(c, r), id(c + 1, r + 1), diagLen, 2000.f, true, 2});
                if (c - 1 >= 0 && r + 1 < cloth.rows)
                    cloth.springs.push_back({id(c, r), id(c - 1, r + 1), diagLen, 2000.f, true, 2});
            }
         }
    }   

    void stepCloth(ClothData& cloth, float dt, sf::Vector2f mousePosSF, bool rightDown, bool leftDown, bool gravityEnabled) {
        if (cloth.nodes.empty()) return;    

        AML::vec2 mousePos = toAML(mousePosSF);
        if (rightDown) {
            AML::vec2 swipeStart = s_wasRightDown ? s_prevMousePos : mousePos;
            cutSpringsAlongSwipe(cloth, swipeStart, mousePos, 12.f);
        }
        s_prevMousePos = mousePos;
        s_wasRightDown = rightDown; 

        constexpr int substeps = 8;
        const float subDt = dt / static_cast<float>(substeps);
        const float gravityY = gravityEnabled ? 980.f : 0.f;
        const float damping = 0.9985f;  

        for (int step = 0; step < substeps; ++step) {
            if (gravityEnabled) {
                for (auto& n : cloth.nodes) {
                    if (!n.pinned && n.invMass > 0.f)
                        n.force.y += gravityY / n.invMass;
                }
            }
            if (leftDown) {
                pullNodesNear(cloth, mousePos, 70.f, 2400.f);
            }
            accumulateSpringForces(cloth, 2.3f);
            integrateVerlet(cloth, subDt, damping);
        }
    }   

    // ========================================================================
    // Jelly & Semi-Implicit Euler Rigid Sphere Physics
    // ========================================================================
    void buildJellyCube(ClothData& jelly, RigidSphereData& sphere, float worldCenterX, float startY) {
        jelly.nodes.clear();
        jelly.springs.clear();
        jelly.cols = 8;
        jelly.rows = 8;
        const float spacing = 32.f;
        const float startX = worldCenterX - ((jelly.cols - 1) * spacing) * 0.5f;
        jelly.restArea = ((jelly.cols - 1) * spacing) * ((jelly.rows - 1) * spacing);   

        for (int r = 0; r < jelly.rows; ++r) {
            for (int c = 0; c < jelly.cols; ++c) {
                Node n;
                n.pos = {startX + c * spacing, startY + r * spacing};
                n.prevPos = n.pos;
                n.force = {0.f, 0.f};
                n.pinned = false;
                n.invMass = 1.0f;
                jelly.nodes.push_back(n);
            }
        }   

        const float diagLen = spacing * std::sqrt(2.f);
        auto id = [&](int c, int r) -> std::size_t {
            return static_cast<std::size_t>(r * jelly.cols + c);
        };  

        for (int r = 0; r < jelly.rows; ++r) {
            for (int c = 0; c < jelly.cols; ++c) {
                if (c + 1 < jelly.cols)
                    jelly.springs.push_back({id(c, r), id(c + 1, r), spacing, 6500.f, true, 0});
                if (r + 1 < jelly.rows)
                    jelly.springs.push_back({id(c, r), id(c, r + 1), spacing, 6500.f, true, 1});
                if (c + 1 < jelly.cols && r + 1 < jelly.rows)
                    jelly.springs.push_back({id(c, r), id(c + 1, r + 1), diagLen, 5200.f, true, 2});
                if (c - 1 >= 0 && r + 1 < jelly.rows)
                    jelly.springs.push_back({id(c, r), id(c - 1, r + 1), diagLen, 5200.f, true, 2});
            }
        }   

        sphere.pos = {worldCenterX + 180.f, startY + 40.f};
        sphere.vel = {-140.f, 0.f};
        sphere.force = {0.f, 0.f};
        sphere.radius = 34.f;
        sphere.invMass = 0.08f;
    }   

    void stepJellyAndSphere(ClothData& jelly, RigidSphereData& sphere, float dt, sf::Vector2f mousePosSF, bool leftClicked, bool leftDown, bool rightDown, bool gravityEnabled, sf::Vector2f worldBoundsSF) {
        (void)leftDown;
        AML::vec2 mousePos = toAML(mousePosSF);
        AML::vec2 worldBounds = toAML(worldBoundsSF);   

        if (leftClicked) {
            applyRadialImpulse(jelly, sphere, mousePos, 240.f, 9500.f, 5.5f);
        }
        if (rightDown) {
            AML::vec2 swipeStart = s_wasRightDown ? s_prevMousePos : mousePos;
            cutSpringsAlongSwipe(jelly, swipeStart, mousePos, 12.f);
        }
        s_prevMousePos = mousePos;
        s_wasRightDown = rightDown; 

        constexpr int substeps = 10;
        const float subDt = dt / static_cast<float>(substeps);
        const float gravityY = gravityEnabled ? 980.f : 0.f;
        const float damping = 0.998f;
        const float floorY = worldBounds.y - 40.f;  

        for (int step = 0; step < substeps; ++step) {
            if (gravityEnabled) {
                for (auto& n : jelly.nodes)
                    if (!n.pinned && n.invMass > 0.f)
                        n.force.y += gravityY / n.invMass;
            }   

            accumulateSpringForces(jelly, 3.2f);
            applyInternalPressure(jelly, 4200.f);
            integrateVerlet(jelly, subDt, damping); 

            // Floor & wall bounds for Jelly
            for (auto& n : jelly.nodes) {
                if (n.pinned) continue;
                if (n.pos.y > floorY) {
                    float vx = (n.pos.x - n.prevPos.x) * 0.85f;
                    n.pos.y = floorY;
                    n.prevPos.x = n.pos.x - vx;
                }
                if (n.pos.x < 40.f) n.pos.x = 40.f;
                if (n.pos.x > worldBounds.x - 40.f) n.pos.x = worldBounds.x - 40.f;
                if (n.pos.y < 30.f) n.pos.y = 30.f;
            }   

            // Semi-Implicit Euler integration for Rigid Sphere
            if (sphere.invMass > 0.f) {
                if (gravityEnabled) sphere.force.y += gravityY / sphere.invMass;
                // 1. Update velocity first, 2. Update position using new velocity
                sphere.vel += (sphere.force * sphere.invMass) * subDt;
                sphere.pos += sphere.vel * subDt;
                sphere.force = {0.f, 0.f};  

                constexpr float restitution = 0.80f;
                if (sphere.pos.y + sphere.radius > floorY) {
                    sphere.pos.y = floorY - sphere.radius;
                    sphere.vel.y = -sphere.vel.y * restitution;
                    sphere.vel.x *= 0.96f;
                }
                if (sphere.pos.x - sphere.radius < 40.f) {
                    sphere.pos.x = 40.f + sphere.radius;
                    sphere.vel.x = -sphere.vel.x * restitution;
                } else if (sphere.pos.x + sphere.radius > worldBounds.x - 40.f) {
                    sphere.pos.x = worldBounds.x - 40.f - sphere.radius;
                    sphere.vel.x = -sphere.vel.x * restitution;
                }
            }   

            // Two-way coupling: Rigid Sphere <-> Soft Jelly nodes
            for (auto& n : jelly.nodes) {
                AML::vec2 delta = n.pos - sphere.pos;
                float distSq = AML::length_sqr(delta);
                if (distSq < sphere.radius * sphere.radius && distSq > 1e-8f) {
                    float dist = std::sqrt(distSq);
                    AML::vec2 normal = delta / dist;
                    float pen = sphere.radius - dist;
                    float totalInv = n.invMass + sphere.invMass;
                    if (totalInv <= 0.f) continue;  

                    if (!n.pinned)
                        n.pos += normal * (pen * (n.invMass / totalInv));
                    sphere.pos -= normal * (pen * (sphere.invMass / totalInv));
                    sphere.vel -= normal * (pen * 18.f * (sphere.invMass / totalInv));
                }
            }
        }
    }

    // ========================================================================
    // Viscosity Fluid Physics
    // ========================================================================
    void buildFluidSim(FluidSimData& fluid, float worldCenterX)
    {
        // Place the viscous fluid tank centered horizontally (500px wide, 380px deep)
        constexpr float halfTankW = 250.f;
        fluid.tank = AML::AABB2D{
            {worldCenterX - halfTankW, 340.f}, // Top-left corner (water surface at y = 340)
            {worldCenterX + halfTankW, 720.f}  // Bottom-right corner (tank floor at y = 720)
        };

        // Spawn the sphere in mid-air above the water surface
        fluid.sphere.center = {worldCenterX, 110.f};
        fluid.sphere.radius = 28.f;

        // Reset velocity & forces (prevents leftover momentum on Reset)
        fluid.vel   = {0.f, 0.f};
        fluid.force = {0.f, 0.f};

        // Physical properties
        fluid.invMass      = 0.05f;
        fluid.viscosity    = 0.22f;    // eta: higher = thicker honey, lower = thin water
        fluid.fluidDensity = 0.0045f;  // rho: upward buoyancy strength
    }
    void stepFluidSim(FluidSimData& fluid, float dt, sf::Vector2f mousePosSF, bool leftClicked, bool gravityEnabled)
    {
         AML::vec2 mousePos = toAML(mousePosSF);
        constexpr float pi = 3.14159265f;

        // Left-click launches the sphere upward/sideways so you can watch it splash back in
        if (leftClicked && fluid.invMass > 0.f)
        {
            AML::vec2 delta = fluid.sphere.center - mousePos;
            float dist = AML::length(delta);

            if (dist < 260.f)
            {
                AML::vec2 normal = (dist > 1e-4f) ? (delta / dist) : AML::vec2{0.f, -1.f};
                float falloff = 1.f - (dist / 260.f);
                float dv = 12000.f * (0.4f + 0.6f * falloff) * fluid.invMass;

                fluid.vel += normal * dv;
                fluid.vel.y -= 260.f; // Upward pop out of the tank
            }
        }

        constexpr int substeps = 8;
        const float subDt    = dt / static_cast<float>(substeps);
        const float gravityY = gravityEnabled ? 980.f : 0.f;

        for (int step = 0; step < substeps; ++step)
        {
            // 1. Gravity (F = m * g = g / invMass)
            if (gravityEnabled && fluid.invMass > 0.f)
            {
                fluid.force.y += gravityY / fluid.invMass;
            }

            // 2. Fluid forces when sphere overlaps the viscous AABB tank
            if (AML::overlap_circle_vs_aabb(fluid.sphere, fluid.tank))
            {
                float r    = fluid.sphere.radius;
                float area = pi * r * r;

                // Upward Archimedes buoyancy: F_b = -rho * Area * g
                if (gravityEnabled)
                {
                    fluid.force.y -= fluid.fluidDensity * area * gravityY;
                }

                // Stokes' viscous drag: F_d = -6 * pi * eta * r * v
                float dragCoeff = 6.f * pi * fluid.viscosity * r;
                fluid.force += fluid.vel * (-dragCoeff);
            }

            // 3. Semi-Implicit Euler integration
            fluid.vel += (fluid.force * fluid.invMass) * subDt;
            fluid.sphere.center += fluid.vel * subDt;
            fluid.force = {0.f, 0.f};

            // 4. Tank wall & floor collisions (top stays open for splashing)
            constexpr float restitution = 0.65f;
            float r = fluid.sphere.radius;

            if (fluid.sphere.center.y + r > fluid.tank.max_pt.y)
            {
                fluid.sphere.center.y = fluid.tank.max_pt.y - r;
                fluid.vel.y = -fluid.vel.y * restitution;
                fluid.vel.x *= 0.95f;
            }
            if (fluid.sphere.center.x - r < fluid.tank.min_pt.x)
            {
                fluid.sphere.center.x = fluid.tank.min_pt.x + r;
                fluid.vel.x = -fluid.vel.x * restitution;
            }
            else if (fluid.sphere.center.x + r > fluid.tank.max_pt.x)
            {
                fluid.sphere.center.x = fluid.tank.max_pt.x - r;
                fluid.vel.x = -fluid.vel.x * restitution;
            }
        }
    }
 }   
