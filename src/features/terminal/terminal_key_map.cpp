#include "features/terminal/terminal_key_map.h"

namespace pnana {
namespace features {
namespace terminal {

KeyEvent ftxuiKeyToKeyEvent(const std::string& key) {
    KeyEvent ev;
    ev.type = KeyEvent::Type::Char;
    ev.ch = 0;

    if (key == "return" || key == "ctrl_m")
        ev.type = KeyEvent::Type::Enter;
    else if (key == "Tab" || key == "tab")
        ev.type = KeyEvent::Type::Tab;
    else if (key == "Backspace")
        ev.type = KeyEvent::Type::Backspace;
    else if (key == "ctrl_h")
        ev.type = KeyEvent::Type::CtrlH;
    else if (key.size() == 1 && static_cast<unsigned char>(key[0]) == 0x7f)
        ev.type = KeyEvent::Type::Backspace;
    else if (key == "ArrowUp" || key == "arrow_up")
        ev.type = KeyEvent::Type::KeyUp;
    else if (key == "ArrowDown" || key == "arrow_down")
        ev.type = KeyEvent::Type::KeyDown;
    else if (key == "ArrowLeft" || key == "arrow_left")
        ev.type = KeyEvent::Type::KeyLeft;
    else if (key == "ArrowRight" || key == "arrow_right")
        ev.type = KeyEvent::Type::KeyRight;
    else if (key == "Home")
        ev.type = KeyEvent::Type::Home;
    else if (key == "End")
        ev.type = KeyEvent::Type::End;
    else if (key == "PageUp")
        ev.type = KeyEvent::Type::PageUp;
    else if (key == "PageDown")
        ev.type = KeyEvent::Type::PageDown;
    else if (key == "Delete")
        ev.type = KeyEvent::Type::Delete;
    else if (key == "ctrl_c")
        ev.type = KeyEvent::Type::CtrlC;
    else if (key == "ctrl_d")
        ev.type = KeyEvent::Type::CtrlD;
    else if (key == "ctrl_z")
        ev.type = KeyEvent::Type::CtrlZ;
    else if (key == "ctrl_l")
        ev.type = KeyEvent::Type::CtrlL;
    else if (key == "ctrl_u")
        ev.type = KeyEvent::Type::CtrlU;
    else if (key == "ctrl_k")
        ev.type = KeyEvent::Type::CtrlK;
    else if (key == "ctrl_a")
        ev.type = KeyEvent::Type::CtrlA;
    else if (key == "ctrl_e")
        ev.type = KeyEvent::Type::CtrlE;
    else if (key == "ctrl_w")
        ev.type = KeyEvent::Type::CtrlW;
    else if (key == "ctrl_x")
        ev.type = KeyEvent::Type::CtrlX;
    else if (key == "Escape")
        ev.type = KeyEvent::Type::Escape;
    else if (key.length() == 1) {
        ev.type = KeyEvent::Type::Char;
        ev.ch = key[0];
    }
    return ev;
}

KeyEvent ftxuiCharToKeyEvent(char c) {
    KeyEvent ev;
    ev.type = KeyEvent::Type::Char;
    ev.ch = c;
    return ev;
}

} // namespace terminal
} // namespace features
} // namespace pnana
