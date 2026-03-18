#ifndef PNANA_FEATURES_TERMINAL_TERMINAL_KEY_MAP_H
#define PNANA_FEATURES_TERMINAL_TERMINAL_KEY_MAP_H

#include "features/terminal/terminal_session.h"
#include <string>

namespace pnana {
namespace features {
namespace terminal {

// 将 ftxui 按键字符串转为 KeyEvent（与 ftxui 解耦，便于复用）
KeyEvent ftxuiKeyToKeyEvent(const std::string& key);

// 将 ftxui 单字符转为 KeyEvent
KeyEvent ftxuiCharToKeyEvent(char c);

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_TERMINAL_KEY_MAP_H
