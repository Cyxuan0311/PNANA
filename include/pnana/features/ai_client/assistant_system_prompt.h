#ifndef PNANA_FEATURES_AI_CLIENT_ASSISTANT_SYSTEM_PROMPT_H
#define PNANA_FEATURES_AI_CLIENT_ASSISTANT_SYSTEM_PROMPT_H

#include <string>

#ifdef BUILD_AI_CLIENT_SUPPORT

namespace pnana {
namespace features {
namespace ai_client {

// 返回 AI 助手使用的完整系统提示词（含角色说明、工具使用规则与 Skill 清单）
std::string getAssistantSystemPrompt();

} // namespace ai_client
} // namespace features
} // namespace pnana

#endif // BUILD_AI_CLIENT_SUPPORT

#endif // PNANA_FEATURES_AI_CLIENT_ASSISTANT_SYSTEM_PROMPT_H
