#pragma once

namespace DialogueProgressRuntime
{
// Invalidates all pending UI deadlines; safe at native load/network boundaries.
void ResetSession() noexcept;
} // namespace DialogueProgressRuntime
