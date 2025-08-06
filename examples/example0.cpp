#include "../include/libGfx.h"
#include "../include/libGfxEvent.h"

#include <string> // 添加string头文件

int main(int argc, char* argv[]) {
    GFX_INIT();
        SDL_Window* window = SDL_CreateWindow(
        "libGfx 示例窗口",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        std::cerr << "CreateWindow Failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "GL Context Failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    

    if (!gfx::Renderer::Init(window)) {
        std::cerr << "Renderer Init Failed\n";
        return -1;
    }

    // 设置初始投影矩阵
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    s_projection = glm::ortho(0.0f, (float)w, (float)h, 0.0f);
    glViewport(0, 0, w, h);

    bool running = true;
    SDL_Event event;

    gfx::Font* font = gfx::Font::Load("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 24,gfx::White);
    gfx::FontA* fonta = gfx::FontA::Load("/usr/share/fonts/truetype/ubuntu/Ubuntu-B.ttf", 24);
    
    // 添加文本状态变量
    std::string displayText = "Hello,World!";
    bool spacePressed = false;

    // 注册空格键事件监听
    gfx::Event::AddListener(gfx::Event::EventType::KeyDown, [&](const gfx::Event::Event& e) {
        if (e.keyboard.keycode == gfx::Event::KeyCode::Space && !spacePressed) {
            displayText = "Press";
            spacePressed = true;
            
        }
    });

    gfx::Event::AddListener(gfx::Event::EventType::KeyUp, [&](const gfx::Event::Event& e) {
        if (e.keyboard.keycode == gfx::Event::KeyCode::Space) {
            displayText = "Hello,World!";
            spacePressed = false;
        }
    });
    gfx::Point click=gfx::Point(500, 400);
    gfx::Event::AddListener(gfx::Event::EventType::MouseButtonDown, [&](const gfx::Event::Event& event) {
        if (event.mouse.button == gfx::Event::MouseButton::Left) {
            // 处理左键点击
            std::cout << "Left mouse button clicked at position: "
                      << event.mouse.position.x << ", " << event.mouse.position.y << std::endl;
            click=gfx::Point(event.mouse.position.x , event.mouse.position.y );
        }
    });
    
    while (running) {
        

        // 保留原有的SDL事件处理用于窗口事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
                gfx::Renderer renderer;
                renderer.HandleWindowResize(event.window.data1, event.window.data2);
            }
            gfx::Event::PollEvents(event);
        }
        
        gfx::Renderer::Clear(gfx::Color(gfx::Red));

        // 画个矩形
        gfx::Renderer::DrawRect(gfx::Rect(100, 100, 200, 150), gfx::Red);
        gfx::Renderer::DrawRect(gfx::Rect(150, 100, 200, 150), gfx::White);
        // 画条线
        gfx::Renderer::DrawLine(gfx::Point(300, 300), click, gfx::Yellow, 3.0f);

        // 使用可变的文本内容
        gfx::Renderer::DrawText(displayText.c_str(), gfx::Point(50, 50), font);

        gfx::Renderer::DrawText(displayText.c_str(), gfx::Point(100, 100), fonta,gfx::Black,1.0f,0.0f);
        
        // 提交一帧
        gfx::Renderer::Present();

        SDL_Delay(16);  // 简单的限帧
    }
    gfx::Renderer::Shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}