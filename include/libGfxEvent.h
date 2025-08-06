#ifndef NEBULAXLIBGFXEVENT_H
#define NEBULAXLIBGFXEVENT_H
#include "libGfx.h"
namespace gfx{
namespace Event
{
    // ==================== Event Types ====================
    enum class EventType
    {
        None,
        MouseButtonDown,
        MouseButtonUp,
        MouseMotion,
        MouseWheel,
        KeyDown,
        KeyUp,
        TextInput
    };

    // ==================== Mouse Button Definitions ====================
    enum class MouseButton
    {
        Left,
        Middle,
        Right,
        X1,
        X2,
        Unknown
    };

    // ==================== Key Code Definitions ====================
    enum class KeyCode
    {
        Unknown,
        
        // Letters
        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
        
        // Numbers
        Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
        
        // Function Keys
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        
        // Special Keys
        Escape,
        Space,
        Enter,
        Tab,
        Backspace,
        Delete,
        Insert,
        Home,
        End,
        PageUp,
        PageDown,
        
        // Arrow Keys
        Left,
        Right,
        Up,
        Down,
        
        // Modifier Keys
        LShift,
        RShift,
        LCtrl,
        RCtrl,
        LAlt,
        RAlt,
        LGui,  // Windows/Command key
        RGui,
        
        // Keypad
        KP_0, KP_1, KP_2, KP_3, KP_4,
        KP_5, KP_6, KP_7, KP_8, KP_9,
        KP_Enter,
        KP_Plus,
        KP_Minus,
        KP_Multiply,
        KP_Divide,
        KP_Period,
        
        // Other
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        LControl,
        RControl
    };

    // ==================== Event Structure ====================
    struct Event
    {
        EventType type = EventType::None;
        
        // Mouse events data
        struct
        {
            Point position;         // Mouse position
            Point relative;         // Relative motion
            MouseButton button;     // Button involved
            int clicks;             // Number of clicks (1 for single, 2 for double, etc.)
            int wheelX;             // Horizontal scroll
            int wheelY;             // Vertical scroll
        } mouse;
        
        // Keyboard events data
        struct
        {
            KeyCode keycode;        // Physical key pressed
            bool repeat;            // Is this a key repeat?
            bool alt;              // Alt modifier
            bool ctrl;             // Control modifier
            bool shift;            // Shift modifier
            bool gui;             // Windows/Command modifier
        } keyboard;
        
        // Text input data
        struct
        {
            char text[32];          // UTF-8 text input
        } text;
    };

    // ==================== Event Listener Interface ====================
    using EventCallback = std::function<void(const Event&)>;

    // ==================== Event System API ====================
    void PollEvents(SDL_Event &sdlEvent);
    void AddListener(EventType type, EventCallback callback);
    
    //void RemoveListener(EventType type, EventCallback callback);
    
    // Helper functions
    bool IsKeyPressed(KeyCode key);
    bool IsMouseButtonPressed(MouseButton button);
    Point GetMousePosition();
    
    // Text input management
    void StartTextInput();
    void StopTextInput();
    void SetTextInputRect(const Rect& rect);
}
}
#endif