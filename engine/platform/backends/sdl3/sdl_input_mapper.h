#ifndef CH_SDL_INPUT_MAPPER_H
#define CH_SDL_INPUT_MAPPER_H

#include <SDL3/SDL.h>
#include "engine/core/key_codes.h"
#include "engine/core/mouse_codes.h"

namespace Chained
{
    class SdlInputMapper
    {
    public:
        static KeyCode MapKey(SDL_Scancode scancode)
        {
            switch (scancode)
            {
                case SDL_SCANCODE_SPACE: return KeyCode::Space;
                case SDL_SCANCODE_0: return KeyCode::D0;
                case SDL_SCANCODE_1: return KeyCode::D1;
                case SDL_SCANCODE_2: return KeyCode::D2;
                case SDL_SCANCODE_3: return KeyCode::D3;
                case SDL_SCANCODE_4: return KeyCode::D4;
                case SDL_SCANCODE_5: return KeyCode::D5;
                case SDL_SCANCODE_6: return KeyCode::D6;
                case SDL_SCANCODE_7: return KeyCode::D7;
                case SDL_SCANCODE_8: return KeyCode::D8;
                case SDL_SCANCODE_9: return KeyCode::D9;
                case SDL_SCANCODE_A: return KeyCode::A;
                case SDL_SCANCODE_B: return KeyCode::B;
                case SDL_SCANCODE_C: return KeyCode::C;
                case SDL_SCANCODE_D: return KeyCode::D;
                case SDL_SCANCODE_E: return KeyCode::E;
                case SDL_SCANCODE_F: return KeyCode::F;
                case SDL_SCANCODE_G: return KeyCode::G;
                case SDL_SCANCODE_H: return KeyCode::H;
                case SDL_SCANCODE_I: return KeyCode::I;
                case SDL_SCANCODE_J: return KeyCode::J;
                case SDL_SCANCODE_K: return KeyCode::K;
                case SDL_SCANCODE_L: return KeyCode::L;
                case SDL_SCANCODE_M: return KeyCode::M;
                case SDL_SCANCODE_N: return KeyCode::N;
                case SDL_SCANCODE_O: return KeyCode::O;
                case SDL_SCANCODE_P: return KeyCode::P;
                case SDL_SCANCODE_Q: return KeyCode::Q;
                case SDL_SCANCODE_R: return KeyCode::R;
                case SDL_SCANCODE_S: return KeyCode::S;
                case SDL_SCANCODE_T: return KeyCode::T;
                case SDL_SCANCODE_U: return KeyCode::U;
                case SDL_SCANCODE_V: return KeyCode::V;
                case SDL_SCANCODE_W: return KeyCode::W;
                case SDL_SCANCODE_X: return KeyCode::X;
                case SDL_SCANCODE_Y: return KeyCode::Y;
                case SDL_SCANCODE_Z: return KeyCode::Z;
                case SDL_SCANCODE_ESCAPE: return KeyCode::Escape;
                case SDL_SCANCODE_RETURN: return KeyCode::Enter;
                case SDL_SCANCODE_TAB: return KeyCode::Tab;
                case SDL_SCANCODE_BACKSPACE: return KeyCode::Backspace;
                case SDL_SCANCODE_INSERT: return KeyCode::Insert;
                case SDL_SCANCODE_DELETE: return KeyCode::Delete;
                case SDL_SCANCODE_RIGHT: return KeyCode::Right;
                case SDL_SCANCODE_LEFT: return KeyCode::Left;
                case SDL_SCANCODE_DOWN: return KeyCode::Down;
                case SDL_SCANCODE_UP: return KeyCode::Up;
                case SDL_SCANCODE_PAGEUP: return KeyCode::PageUp;
                case SDL_SCANCODE_PAGEDOWN: return KeyCode::PageDown;
                case SDL_SCANCODE_HOME: return KeyCode::Home;
                case SDL_SCANCODE_END: return KeyCode::End;
                case SDL_SCANCODE_CAPSLOCK: return KeyCode::CapsLock;
                case SDL_SCANCODE_SCROLLLOCK: return KeyCode::ScrollLock;
                case SDL_SCANCODE_NUMLOCKCLEAR: return KeyCode::NumLock;
                case SDL_SCANCODE_PRINTSCREEN: return KeyCode::PrintScreen;
                case SDL_SCANCODE_PAUSE: return KeyCode::Pause;
                case SDL_SCANCODE_F1: return KeyCode::F1;
                case SDL_SCANCODE_F2: return KeyCode::F2;
                case SDL_SCANCODE_F3: return KeyCode::F3;
                case SDL_SCANCODE_F4: return KeyCode::F4;
                case SDL_SCANCODE_F5: return KeyCode::F5;
                case SDL_SCANCODE_F6: return KeyCode::F6;
                case SDL_SCANCODE_F7: return KeyCode::F7;
                case SDL_SCANCODE_F8: return KeyCode::F8;
                case SDL_SCANCODE_F9: return KeyCode::F9;
                case SDL_SCANCODE_F10: return KeyCode::F10;
                case SDL_SCANCODE_F11: return KeyCode::F11;
                case SDL_SCANCODE_F12: return KeyCode::F12;
                case SDL_SCANCODE_LSHIFT: return KeyCode::LeftShift;
                case SDL_SCANCODE_LCTRL: return KeyCode::LeftControl;
                case SDL_SCANCODE_LALT: return KeyCode::LeftAlt;
                case SDL_SCANCODE_RSHIFT: return KeyCode::RightShift;
                case SDL_SCANCODE_RCTRL: return KeyCode::RightControl;
                case SDL_SCANCODE_RALT: return KeyCode::RightAlt;
                default: return KeyCode::Null;
            }
        }

        static MouseCode MapMouseButton(Uint8 sdlButton)
        {
            switch (sdlButton)
            {
                case SDL_BUTTON_LEFT: return MouseCode::ButtonLeft;
                case SDL_BUTTON_RIGHT: return MouseCode::ButtonRight;
                case SDL_BUTTON_MIDDLE: return MouseCode::ButtonMiddle;
                case SDL_BUTTON_X1: return MouseCode::ButtonSide;
                case SDL_BUTTON_X2: return MouseCode::ButtonExtra;
                default: return MouseCode::ButtonLeft;
            }
        }
    };
}

#endif
