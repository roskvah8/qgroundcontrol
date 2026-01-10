/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VoiceStreamingSettings.h"

DECLARE_SETTINGGROUP(VoiceStreaming, "VoiceStreaming")
{
}

DECLARE_SETTINGSFACT(VoiceStreamingSettings, voiceStreamingEnabled)
DECLARE_SETTINGSFACT(VoiceStreamingSettings, voicePeerAddress)
DECLARE_SETTINGSFACT(VoiceStreamingSettings, voiceVolume)
