#include "Backend.hpp"
#include "renderer/Render.hpp"


namespace backend
{
    static void updateCameraView(sf::RenderWindow& window, unsigned int winW, unsigned int winH, sf::Vector2f clothCenter)
{
    if (winW == 0 || winH == 0) return;

    float w = static_cast<float>(winW);
    float h = static_cast<float>(winH);

    // Minimum space needed so the whole cloth + top pins stay visible with padding
    constexpr float minVisibleW = 880.f;
    constexpr float minVisibleH = 660.f;

    // If window is smaller than the cloth, zoom out (> 1.0) to fit; otherwise keep 1:1 scale
    float scale = std::max({1.f, minVisibleW / w, minVisibleH / h});

    sf::View view;
    view.setCenter(clothCenter);
    view.setSize({w * scale, h * scale});
    window.setView(view);
}

    void Backend::createWindow(unsigned int width, unsigned int height, const std::string& title)
    {
        _width  = width;
        _height = height;
        _title  = title;

        _worldCenter = {static_cast<float>(_width) * 0.5f, static_cast<float>(_height) * 0.5f};

        std::cout << "Initialising SetUp...\n";

       sf::ContextSettings settings;
        settings.antiAliasingLevel = 8;

        _window.create(sf::VideoMode({_width, _height}), _title, sf::Style::Default, sf::State::Windowed, settings);
        updateCameraView(_window, _width, _height, {_width * 0.5f, _height * 0.5f}); //resize
        _window.setMouseCursorVisible(false); //hide cursor
        _window.setFramerateLimit(60);

        //initialize renderer
        Phys::buildClothGrid(_cloth, _worldCenter.x, 75.f);
        Phys::buildJellyCube(_jelly, _spheres, _worldCenter.x, 140.f);
        Phys::buildFluidSim(_fluid, _worldCenter.x);
        Renderer::init(_window, _mode, _cloth, _jelly, _fluid, _spheres, _mousePos, _leftDown, _rightDown);

          if (!_window.isOpen())
        {
            std::cout << "Failed To open Window\n";
            Terminate();
            return;
        }

    }

    void Backend::Run()
    {
        if (!_window.isOpen())
            createWindow(_width, _height, _title);
             std::cout << "Simulation Running\n";
        
        constexpr float fixedDt = 1.f / 60.f;
        sf::Clock fpsClock;
        int frameCount = 0;

        while (_window.isOpen())
        {
            ProcessInput();
            Update(fixedDt);

           // _window.clear(sf::Color(22, 24, 30));
           _window.clear(sf::Color(12, 13, 16)); 

            //_window.render();
            Renderer::render();

            _window.display();

            frameCount++;
            if (fpsClock.getElapsedTime().asSeconds() >= 0.25f)
            {
            int fps = static_cast<int>(frameCount / fpsClock.restart().asSeconds());
            frameCount = 0;
            //My Title Menu Bar
            _window.setTitle(_title + " | " + std::to_string(_width) + "x" + std::to_string(_height) + " | " + std::to_string(fps) + " FPS" + " | " + (_pause ? " [PAUSED] " : "[LIVE]"));
            }
        }

        Cleanup();
        Terminate();
    }

    void Backend::Update(float dt)
    {
        (void)dt;

       if (_mode == SimMode::Cloth)
    {
        if (_reset)
            Phys::buildClothGrid(_cloth, _worldCenter.x, 75.f);

        if (_pause) return;
        Phys::stepCloth(_cloth, dt, _mousePos, _rightDown, _leftDown, _toggleGrav);
    }
    else if (_mode == SimMode::Jelly)
    {
        if (_reset)
            Phys::buildJellyCube(_jelly, _spheres, _worldCenter.x, 140.f);

        if (_pause) return;
        Phys::stepJellyAndSphere(_jelly, _spheres, dt, _mousePos, _leftClicked,_leftDown, _rightDown, _toggleGrav,
                                 {_worldCenter.x * 2.f, _worldCenter.y * 2.f});
    }
    else 
    {
        if (_reset)
            Phys::buildFluidSim(_fluid, _worldCenter.x);

        if (_pause) return;
        Phys::stepFluidSim(_fluid, dt, _mousePos, _leftClicked, _toggleGrav);
    }
    }

    void Backend::Cleanup()
    {
        //clear the buffers
        Renderer::Clean();
    }

    void Backend::Terminate()
    {
        if (_window.isOpen())
        {
        _window.close();
        std::cout << "Simulation is Terminated!\n";
        }
    }

    void Backend::ProcessInput()
    {
        _reset = false;

        while (const std::optional event = _window.pollEvent())
        {
            _leftClicked = false;

            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                _width  = resized->size.x;
                _height = resized->size.y;
               
                updateCameraView(_window, _width, _height, _worldCenter);
            
                // Update SFML's camera so shapesd never stretch
               // sf::FloatRect visibleArea({0.f, 0.f}, {static_cast<float>(_width), static_cast<float>(_height)});
               // _window.setView(sf::View(visibleArea));
            }

            if (event->is<sf::Event::Closed>())
                Terminate();

            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::R)
                {
                    _reset = true;
                    std::cout << "Simulation Reset\n";
                }
                if (key->code == sf::Keyboard::Key::Escape)
                    Terminate();
                if (key->code == sf::Keyboard::Key::G)
                {
                _toggleGrav = !_toggleGrav;
                std::cout << "Gravity: " << (_toggleGrav ? "ON\n" : "OFF\n");
                }
                if (key->code == sf::Keyboard::Key::Space)
                {
                 _pause = !_pause;
                 std::cout << (_pause ? "Simulation Paused\n" : "Simulation Resumed\n");
                }
                 if (key->code == sf::Keyboard::Key::Tab)
                {
                if (_mode == SimMode::Cloth)      _mode = SimMode::Jelly;
                else if (_mode == SimMode::Jelly) _mode = SimMode::Fluid;
                else                              _mode = SimMode::Cloth;
                _reset = true;
                if (_mode == SimMode::Cloth)      std::cout << "[Mode]: Verlet Cloth\n";
                else if (_mode == SimMode::Jelly) std::cout << "[Mode]: Jelly Cube & Rigid Sphere\n";
                else                              std::cout << "[Mode]: Viscosity Fluid\n";                
                }

                if (_mode == SimMode::Fluid)
                {
                    if (key->code == sf::Keyboard::Key::Up)
                    {
                     _fluid.viscosity = std::min(0.85f, _fluid.viscosity + 0.06f);
                     std::cout << "Viscosity Increased (Thicker): " << _fluid.viscosity << "\n";
                    }
                    if (key->code == sf::Keyboard::Key::Down)
                    {
                     _fluid.viscosity = std::max(0.02f, _fluid.viscosity - 0.06f);
                     std::cout << "Viscosity Decreased (Thinner): " << _fluid.viscosity << "\n";
                    }
                }
            }

            if (const auto* mouseBtn = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseBtn->button == sf::Mouse::Button::Left)
                    _leftClicked = true;
            
            }

        }

        _mousePos  = _window.mapPixelToCoords(sf::Mouse::getPosition(_window));
        _leftDown  = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        _rightDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
    }

     void Backend::PrintLog()
     {
        std::cout << R"(
        ====================================================================
                           2D Verlet Cloth & Jelly Sim
                     Using: AML (Aporia Math Lib) & SFML3
        ====================================================================
          MOUSE CONTROLS:
            [Left Click + Drag]   Grab and pull cloth nodes
            [Right Click + Drag]  Cut / tear springs 
            [Middle Click]        Pin / unpin nearest node

          KEYBOARD CONTROLS:
            [Space]               Pause / Resume 
            [R]                   Reset cloth to initial state
            [G]                   Toggle gravity ON / OFF
            [Up / Down]           Increase / Decrease fluid viscosity (Water -> Honey)
            [TAB]                 Toggle between Cloth & Jelly Sim
            [Esc]                 Exit simulation
        ====================================================================
        )" << std::endl;

     }
    
}