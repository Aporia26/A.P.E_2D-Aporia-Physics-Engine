#include "app/Backend.hpp"

int main()
{
    backend::Backend app;
    app.PrintLog();
    app.Run();
}

/*
# 2D Soft-Body, Rigid-Body & Viscous Fluid Physics Sandbox

A real-time, Data-Oriented 2D physics simulation engine built from scratch in **C++20** using **SFML 3** and **AML (Aporia Math Lib)** — a custom header-only linear algebra and collision library.

![Simulation Demo](assets/demo.gif)
*(Tip: Replace this path with a short GIF or screenshot showing all three simulation modes!)*

---

## ✨ Features & Simulation Modes

Press **`TAB`** at any time to cycle seamlessly between three isolated physics sandboxes:

### 1. Verlet Cloth Simulation (`SimMode::Cloth`)
* **Position Verlet Integration:** Velocity-free soft-body integration derived from 4th-order Taylor Series expansions.
* **Elastic Spring Gradients (\(\mathbf{F} = -\nabla U\)):** Structural (horizontal/vertical) and cross-diagonal shear springs with real-time strain color shifting.
* **Interactive Slicing & Stress Tearing:** Right-click and drag with a custom vector scissor cursor to slice threads along fast mouse swipes, or yank the cloth with left-click until over-stretched springs snap automatically.

### 2. Soft Jelly Cube & Rigid Body Coupling (`SimMode::Jelly`)
* **Internal Area-Conservation Pressure:** Uses the **Shoelace Formula** (`AML::cross`) to calculate enclosed polygon area in real time and applies outward normal pressure forces (\(\mathbf{F}_p = P \cdot \hat{\mathbf{n}}\)) so the jelly regains its volume after compression or cutting.
* **Semi-Implicit Euler Rigid Sphere:** Symplectic integration (`vel` updated prior to `pos`) with restitution, wall friction, and two-way impulse transfer against soft-body jelly nodes.
* **Radial Shockwave Impulse:** Left-clicking triggers an expanding shockwave blast that launches the rigid sphere and ripples the jelly mesh.

### 3. Viscous Fluid & Stokes' Drag Tank (`SimMode::Fluid`)
* **Custom Geometry Collision (`AML::circle` vs. `AML::AABB2D`):** Detects fluid submersion via `AML::overlap_circle_vs_aabb`.
* **Stokes' Law & Archimedes' Buoyancy:** Simulates upward buoyant force (\(\mathbf{F}_b = -\rho A \mathbf{g}\)) and velocity-dependent viscous drag (\(\mathbf{F}_d = -6\pi\eta r \mathbf{v}\)), naturally decelerating the falling sphere to terminal velocity.
* **Live Viscosity Control:** Adjust fluid viscosity (η) in real time from thin cyan water to thick amber honey using the **Up / Down** arrow keys.

---

## 🧮 Mathematical & Engine Architecture

* **Data-Oriented Design (DoD):** Flat `std::vector` node and spring buffers eliminate pointer chasing and maximize CPU cache locality.
* **Sub-Stepped Stability:** Fixed `1/60s` render frames are subdivided into `8–10` physics sub-steps (`subDt = dt / substeps`) to keep stiff Hookean springs and high-viscosity drag well within numerical stability bounds.
* **Zero-Dependency Renderer:** `Backend`, `Physics`, and `Renderer` are strictly decoupled—physics operates purely in `AML::vec2` world space, while `Renderer` batch-draws `sf::VertexArray` geometry and maintains an auto-centering adaptive camera on window resize.

---

## 🎮 Controls

### Mouse Controls

| Input | Action |
| :--- | :--- |
| **Left Click + Drag** *(Cloth)* | Grab and pull nearby cloth nodes |
| **Left Click** *(Jelly / Fluid)* | Fire a radial shockwave impulse |
| **Right Click + Drag** | Slice/cut springs in real time with scissor tool |
| **Middle Click** | Pin or unpin the nearest node |

### Keyboard Controls

| Key | Action |
| :--- | :--- |
| **`TAB`** | Cycle modes: **Cloth → Jelly & Sphere → Viscous Fluid** |
| **`G`** | Toggle gravity **ON / OFF** (Zero-G mode) |
| **`Space`** | Pause / Resume simulation |
| **`R`** | Reset active simulation to initial state |
| **`Up` / `Down`** | Increase / Decrease fluid viscosity (η) in Fluid mode |
| **`Esc`** | Exit simulation |

---

## 📁 Project Structure

```text
├── AML/                     # Custom Aporia Math Lib (vec2, mat3/4, AABB2D, circle, SAT)
│   ├── Vector.hpp
│   ├── Geometry.hpp
│   ├── Collision.hpp
│   └── Matrix.hpp
├── app/                     # Window lifecycle, adaptive camera, input & FPS telemetry
│   ├── Backend.hpp
│   └── Backend.cpp
├── physics/                 # DoD data structs, Verlet, Semi-Euler, Pressure & Fluid solvers
│   ├── DataTypes.hpp
│   └── PhysicsSolvers.hpp / .cpp
├── renderer/                # Stateless SFML 3 batch vertex rendering & custom cursors
│   ├── Render.hpp
│   └── Render.cpp
└── main.cpp                 # Entry point
```

---

## 🛠️ Building from Source

### Prerequisites
* **C++20** compatible compiler (GCC / Clang / MSVC)
* **CMake** (3.20+)
* **SFML 3.0+** (`Graphics`, `Window`, `System`)

### Build Instructions
```bash
git clone https://github.com
cd verlet-cloth-jelly-sim
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

---

## 📦 Download Pre-Built Executable
Don't want to compile from source? Download the standalone Windows `.zip` build directly from **[itch.io](https://itch.io)**, extract it, and run the executable.

---

## 📜 License
This project is open-source under the [MIT License](LICENSE).

*/