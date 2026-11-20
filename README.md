# 2D Verlet Cloth, Soft-Body & Fluid Physics Sandbox

A real-time 2D physics simulation written in C++20 using **SFML 3** and **AML (Aporia Math Lib)**, my custom header-only math and collision library. 

The goal of this project was to implement core numerical integration methods and continuous mechanics formulas from scratch without relying on an external physics engine.

![Simulation Preview](assets/demo.gif)

---

## Pre-Built Release
A standalone Windows executable is available on **[PlayNow on Itch.com](https://aporiastudio.itch.io/softbody)**.

---

## Overview & Simulation Modes

Pressing **`TAB`** cycles between three independent simulation states:

### 1. Verlet Cloth
* **Integration:** Uses Position Verlet integration derived from 4th-order Taylor Series expansions to update particle positions without storing explicit velocity.
* **Spring Forces:** Connects nodes using structural (horizontal/vertical) and diagonal shear springs where elastic force is computed from potential energy gradients (\(\mathbf{F} = -\nabla U\)).
* **Tearing & Slicing:** Springs automatically snap when stretched past a maximum strain threshold, or can be sliced directly using right-click segment intersection checks.

### 2. Soft Jelly & Rigid-Body Coupling
* **Internal Pressure:** Computes the enclosed area of the outer perimeter every substep using the Shoelace formula (`AML::cross`) and applies outward normal forces proportional to area compression so the body retains its shape.
* **Rigid Sphere:** Simulated using Semi-Implicit (Symplectic) Euler integration (`vel` updated before `pos`) with wall/floor restitution and two-way momentum transfer against the soft-body nodes.
* **Impulse Interaction:** Left-clicking applies a radial velocity impulse to the rigid sphere and ripples the surrounding soft-body mesh.

### 3. Viscous Fluid Tank
* **Collision Detection:** Uses `AML::circle` and `AML::AABB2D` via `AML::overlap_circle_vs_aabb` to detect when the falling sphere enters the fluid volume.
* **Drag & Buoyancy:** Applies upward Archimedes' buoyancy (\(\mathbf{F}_b = -\rho A \mathbf{g}\)) and Stokes' viscous drag (\(\mathbf{F}_d = -6\pi\eta r \mathbf{v}\)), naturally slowing the sphere to terminal velocity.
* **Live Parameters:** Viscosity (η) can be adjusted in real time between water-like and honey-like densities using the arrow keys.

---
---

## Controls

### Mouse

| Action | Behavior |
| :--- | :--- |
| **Left Click + Drag** *(Cloth)* | Pull nearby cloth nodes |
| **Left Click** *(Jelly / Fluid)* | Apply radial impulse |
| **Right Click + Drag** | Slice springs along cursor path |
| **Middle Click** | Toggle pin on nearest node |

### Keyboard

| Key | Behavior |
| :--- | :--- |
| **`TAB`** | Switch mode (Cloth / Jelly / Fluid) |
| **`G`** | Toggle gravity on/off |
| **`Space`** | Pause / Resume simulation |
| **`R`** | Reset current simulation |
| **`Up` / `Down`** | Increase / Decrease fluid viscosity |
| **`Esc`** | Quit |

---

## Directory Structure

```text
├── AML/                # Custom header-only math library (Vector, Geometry, Collision, Matrix)
├── src/
    ├── app/                # SFML window setup, input handling, and adaptive camera view
        ├──Backend .cpp/hpp
    ├── physics/            # Simulation data structs and numerical solvers
        ├──Datatype.hpp
        ├──Solver.cpp
    ├── renderer/           # Stateless SFML 3 vertex batching and UI cursors
        ├──Render .cpp/hpp
    └── main.cpp            # Entry point
```

---

## Building from Source

### Requirements
* C++20 compiler (GCC, Clang, or MSVC)
* CMake 3.20+
* SFML 3.0+ (`Graphics`, `Window`, `System`)

### Build
```bash
git clone -b soft_body https://github.com
cd A.P.E_2D-Aporia-Physics-Engine/SoftBodySim

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
---

## License
This Project is Licensed Under the [GNU v3.0](https://github.com/Aporia26/A.P.E_2D-Aporia-Physics-Engine/blob/soft_body/LICENSE)
