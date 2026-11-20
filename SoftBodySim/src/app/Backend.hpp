#pragma once
#include<SFML/Graphics.hpp>
#include "physics/DataTypes.hpp"
#include<iostream>


      enum class SimMode
        {
            Cloth,
            Jelly,
            Fluid
        };


namespace backend
{
    class Backend
    {
    public:

        void PrintLog();
        void createWindow(unsigned int width, unsigned int height, const std::string& title);

   
        void Run();
        void Update(float dt);
        void Cleanup();
        void Terminate();

        void ProcessInput();

    private:
       unsigned int _width = 1000;
       unsigned int _height = 780;
       sf::Vector2f _worldCenter{500.f, 375.f};
        std::string _title = "Cloth & Jelly Simulation";

        sf::RenderWindow _window;

        sf::Vector2f _mousePos{0.f, 0.f};
        bool _leftDown  = false;
        bool _leftClicked = false;
        bool _rightDown = false;
        bool _reset = false;
        bool _pause = false;
        bool _toggleSim = false;
        bool _toggleGrav = true;

        SimMode _mode = SimMode::Cloth;
        ClothData _cloth;
        ClothData _jelly;
        FluidSimData _fluid;
        RigidSphereData _spheres;
    };
}