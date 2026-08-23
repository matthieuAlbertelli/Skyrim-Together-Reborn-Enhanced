#pragma once

namespace PapyrusFunctions
{

bool IsRemotePlayer(Actor* apActor);
bool IsPlayer(Actor* apActor);
bool IsConnected();
bool DidLaunchSkyrimTogether() { return true; };

} // namespace PapyrusFunctions
