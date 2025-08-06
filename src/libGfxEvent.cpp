#include "../include/libGfxEvent.h"
#include <SDL2/SDL.h>

namespace gfx {
namespace Event {
namespace internal {
    std::unordered_map<EventType, std::vector<EventCallback>> eventListeners;
    std::unordered_map<KeyCode, bool> keyStates;
    std::unordered_map<MouseButton, bool> mouseButtonStates;
    Point currentMousePosition;
    bool textInputActive = false;
}

// Convert SDL keycode to our KeyCode
static KeyCode ConvertSDLKeyCode(SDL_Keycode sdlKey) {
    switch (sdlKey) {
        // 字母 A-Z
        case SDLK_a: return KeyCode::A;
        case SDLK_b: return KeyCode::B;
        case SDLK_c: return KeyCode::C;
        case SDLK_d: return KeyCode::D;
        case SDLK_e: return KeyCode::E;
        case SDLK_f: return KeyCode::F;
        case SDLK_g: return KeyCode::G;
        case SDLK_h: return KeyCode::H;
        case SDLK_i: return KeyCode::I;
        case SDLK_j: return KeyCode::J;
        case SDLK_k: return KeyCode::K;
        case SDLK_l: return KeyCode::L;
        case SDLK_m: return KeyCode::M;
        case SDLK_n: return KeyCode::N;
        case SDLK_o: return KeyCode::O;
        case SDLK_p: return KeyCode::P;
        case SDLK_q: return KeyCode::Q;
        case SDLK_r: return KeyCode::R;
        case SDLK_s: return KeyCode::S;
        case SDLK_t: return KeyCode::T;
        case SDLK_u: return KeyCode::U;
        case SDLK_v: return KeyCode::V;
        case SDLK_w: return KeyCode::W;
        case SDLK_x: return KeyCode::X;
        case SDLK_y: return KeyCode::Y;
        case SDLK_z: return KeyCode::Z;

        // 数字键 0-9
        case SDLK_0: return KeyCode::Num0;
        case SDLK_1: return KeyCode::Num1;
        case SDLK_2: return KeyCode::Num2;
        case SDLK_3: return KeyCode::Num3;
        case SDLK_4: return KeyCode::Num4;
        case SDLK_5: return KeyCode::Num5;
        case SDLK_6: return KeyCode::Num6;
        case SDLK_7: return KeyCode::Num7;
        case SDLK_8: return KeyCode::Num8;
        case SDLK_9: return KeyCode::Num9;

        // 控制键
        case SDLK_RETURN: return KeyCode::Enter;
        case SDLK_ESCAPE: return KeyCode::Escape;
        case SDLK_BACKSPACE: return KeyCode::Backspace;
        case SDLK_TAB: return KeyCode::Tab;
        case SDLK_SPACE: return KeyCode::Space;
        case SDLK_LSHIFT: return KeyCode::LShift;
        case SDLK_RSHIFT: return KeyCode::RShift;
        case SDLK_LCTRL: return KeyCode::LControl;
        case SDLK_RCTRL: return KeyCode::RControl;
        case SDLK_LALT: return KeyCode::LAlt;
        case SDLK_RALT: return KeyCode::RAlt;

        // 方向键
        case SDLK_UP: return KeyCode::Up;
        case SDLK_DOWN: return KeyCode::Down;
        case SDLK_LEFT: return KeyCode::Left;
        case SDLK_RIGHT: return KeyCode::Right;

        // 功能键 F1-F12
        case SDLK_F1: return KeyCode::F1;
        case SDLK_F2: return KeyCode::F2;
        case SDLK_F3: return KeyCode::F3;
        case SDLK_F4: return KeyCode::F4;
        case SDLK_F5: return KeyCode::F5;
        case SDLK_F6: return KeyCode::F6;
        case SDLK_F7: return KeyCode::F7;
        case SDLK_F8: return KeyCode::F8;
        case SDLK_F9: return KeyCode::F9;
        case SDLK_F10: return KeyCode::F10;
        case SDLK_F11: return KeyCode::F11;
        case SDLK_F12: return KeyCode::F12;

       

        default: return KeyCode::Unknown;
    }
}

// Convert SDL mouse button to our MouseButton
static MouseButton ConvertSDLMouseButton(Uint8 button) {
    switch (button) {
        case SDL_BUTTON_LEFT: return MouseButton::Left;
        case SDL_BUTTON_MIDDLE: return MouseButton::Middle;
        case SDL_BUTTON_RIGHT: return MouseButton::Right;
        default: return MouseButton::Unknown;
    }
}

void PollEvents(SDL_Event &sdlEvent) {
        Event event{};
        switch (sdlEvent.type) {
            case SDL_MOUSEBUTTONDOWN:
                event.type = EventType::MouseButtonDown;
                event.mouse.button = ConvertSDLMouseButton(sdlEvent.button.button);
                event.mouse.position = {static_cast<float>(sdlEvent.button.x), 
                                      static_cast<float>(sdlEvent.button.y)};
                event.mouse.clicks = sdlEvent.button.clicks;
                internal::mouseButtonStates[event.mouse.button] = true;
                break;
                
            case SDL_MOUSEBUTTONUP:
                event.type = EventType::MouseButtonUp;
                event.mouse.button = ConvertSDLMouseButton(sdlEvent.button.button);
                event.mouse.position = {static_cast<float>(sdlEvent.button.x), 
                                      static_cast<float>(sdlEvent.button.y)};
                internal::mouseButtonStates[event.mouse.button] = false;
                break;
                
            case SDL_MOUSEMOTION:
                event.type = EventType::MouseMotion;
                event.mouse.position = {static_cast<float>(sdlEvent.motion.x), 
                                      static_cast<float>(sdlEvent.motion.y)};
                event.mouse.relative = {static_cast<float>(sdlEvent.motion.xrel), 
                                      static_cast<float>(sdlEvent.motion.yrel)};
                internal::currentMousePosition = event.mouse.position;
                break;
                
            case SDL_KEYDOWN:
                event.type = EventType::KeyDown;
                event.keyboard.keycode = ConvertSDLKeyCode(sdlEvent.key.keysym.sym);
                event.keyboard.repeat = sdlEvent.key.repeat;
                internal::keyStates[event.keyboard.keycode] = true;
                break;
                
            case SDL_KEYUP:
                event.type = EventType::KeyUp;
                event.keyboard.keycode = ConvertSDLKeyCode(sdlEvent.key.keysym.sym);
                internal::keyStates[event.keyboard.keycode] = false;
                break;
        }
        
        // Notify listeners
        if (event.type != EventType::None) {
            for (auto& callback : internal::eventListeners[event.type]) {
                callback(event);
            }
        }
    
}

void AddListener(EventType type, EventCallback callback) {
    internal::eventListeners[type].push_back(callback);
}

bool IsKeyPressed(KeyCode key) {
    return internal::keyStates[key];
}

bool IsMouseButtonPressed(MouseButton button) {
    return internal::mouseButtonStates[button];
}

Point GetMousePosition() {
    return internal::currentMousePosition;
}

void StartTextInput() {
    SDL_StartTextInput();
    internal::textInputActive = true;
}

void StopTextInput() {
    SDL_StopTextInput();
    internal::textInputActive = false;
}

void SetTextInputRect(const Rect& rect) {
    SDL_Rect sdlRect = {static_cast<int>(rect.x), static_cast<int>(rect.y),
                        static_cast<int>(rect.w), static_cast<int>(rect.h)};
    SDL_SetTextInputRect(&sdlRect);
}

} // namespace Event
} // namespace gfx