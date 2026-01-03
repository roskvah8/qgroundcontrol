# QGroundControl Supported Video Protocols

## Overview

QGroundControl supports a variety of video streaming protocols and sources for displaying live video feeds from UAVs and other vehicles. The support depends on compile-time flags and the platform.

---

## Standard Video Protocols

### 1. **RTSP (Real-Time Streaming Protocol)**
**Display Name:** "RTSP Video Stream"

**Technical Details:**
- **GStreamer Element:** `rtspsrc`
- **URL Format:** `rtsp://[host]:[port]/[path]`
- **Example:** `rtsp://192.168.42.1:554/live`
- **Default Timeout:** 8 seconds (configurable)
- **Supported Codecs:**
  - Video: H.264, H.265 (HEVC)
  - Audio: AAC (MPEG4-GENERIC), MP3 (MPA) ✅ **NEW in audio implementation**

**Audio Support:** ✅ **YES** (with new implementation)

**Use Cases:**
- IP cameras
- Professional video encoders
- Ground station video servers
- VLC streaming server

**Pipeline Architecture:**
```
rtspsrc → RTP depayloader → Parser → Decoder → Sink
        ↓ (for audio support)
        Audio RTP depayloader → Audio Parser → Decoder → Audio Sink
```

**Notes:**
- Supports dynamic RTP stream handling
- Can handle both video-only and video+audio streams
- Low-latency mode available

---

### 2. **UDP H.264 Stream**
**Display Name:** "UDP h.264 Video Stream"

**Technical Details:**
- **GStreamer Element:** `udpsrc`
- **URL Format:** `[host]:[port]`
- **Example:** `0.0.0.0:5600` (default)
- **Codec:** H.264 only
- **Container:** Raw H.264 elementary stream

**Audio Support:** ❌ **NO** (raw H.264 stream, no audio channel)

**Use Cases:**
- MAVLink video streaming
- PX4/ArduPilot companion computers
- Custom embedded video transmitters
- Low-overhead streaming

**Pipeline Architecture:**
```
udpsrc → h264parse → parsebin → decodebin3 → video sink
```

**Notes:**
- Most common format for drone video streaming
- Lower overhead than RTSP
- Unidirectional (no back-channel)

---

### 3. **UDP H.265 Stream**
**Display Name:** "UDP h.265 Video Stream"

**Technical Details:**
- **GStreamer Element:** `udpsrc`
- **URL Format:** `[host]:[port]`
- **Example:** `0.0.0.0:5600`
- **Codec:** H.265 (HEVC) only
- **Container:** Raw H.265 elementary stream

**Audio Support:** ❌ **NO** (raw H.265 stream, no audio channel)

**Use Cases:**
- Higher quality video at lower bitrates
- Advanced drone video systems
- Modern IP cameras with HEVC support

**Pipeline Architecture:**
```
udpsrc → h265parse → parsebin → decodebin3 → video sink
```

**Notes:**
- Better compression than H.264
- Requires HEVC decoder support
- Higher CPU usage for decoding

---

### 4. **TCP-MPEG2 Stream**
**Display Name:** "TCP-MPEG2 Video Stream"

**Technical Details:**
- **GStreamer Element:** `tcpclientsrc`
- **URL Format:** `[host]:[port]`
- **Example:** `192.168.143.200:3001`
- **Codec:** MPEG2 video
- **Protocol:** TCP (reliable, connection-oriented)

**Audio Support:** ⚠️ **UNKNOWN** (depends on MPEG2 stream configuration)

**Use Cases:**
- Legacy video systems
- Reliable video transmission (TCP vs UDP)
- MPEG2 encoding hardware

**Pipeline Architecture:**
```
tcpclientsrc → mpegvideoparse → parsebin → decodebin3 → video sink
```

**Notes:**
- TCP provides reliability but adds latency
- Less common in modern drone systems

---

### 5. **MPEG-TS Stream (MPEG Transport Stream)**
**Display Name:** "MPEG-TS Video Stream"

**Technical Details:**
- **GStreamer Element:** `udpsrc`
- **URL Format:** `[host]:[port]`
- **Example:** `0.0.0.0:5600`
- **Container:** MPEG-TS (Transport Stream)
- **Codecs:** Can contain multiple streams (video, audio, data)

**Audio Support:** ⚠️ **POSSIBLY** (MPEG-TS supports audio, but parsebin handling unclear)

**Use Cases:**
- Broadcast video systems
- Multi-stream video sources
- DVB (Digital Video Broadcasting) systems

**Pipeline Architecture:**
```
udpsrc → tsparse → parsebin → decodebin3 → [video sink, audio sink?]
```

**Notes:**
- Container format (like MP4 or MKV)
- Can multiplex multiple streams
- More overhead than raw H.264/H.265
- **TODO:** Test audio support with MPEG-TS

---

## Device-Specific Video Sources

### 6. **UVC Cameras (USB Video Class)**
**Display Name:** Dynamically detected camera names

**Technical Details:**
- **Platform:** Windows, Linux, macOS
- **GStreamer Element:** Platform-dependent (v4l2src on Linux, ksvideosrc on Windows)
- **Detection:** Automatic (lists all connected UVC devices)
- **Codecs:** Depends on camera capabilities

**Audio Support:** ⚠️ **POSSIBLY** (depends on UVC device and implementation)

**Use Cases:**
- USB webcams
- GoPro cameras (in webcam mode)
- Capture cards (HDMI to USB)
- Built-in laptop cameras

**Notes:**
- Requires `QGC_DISABLE_UVC` NOT defined at compile time
- Device names appear dynamically in video source list
- Platform-dependent implementation

---

### 7. **3DR Solo**
**Display Name:** "3DR Solo (requires restart)"

**Technical Details:**
- **Device:** 3DR Solo drone
- **Protocol:** Custom video streaming protocol
- **Requires:** Application restart to activate

**Audio Support:** ❌ **NO**

**Use Cases:**
- 3DR Solo drone video feed

**Notes:**
- Legacy device support
- Special handling required

---

### 8. **Parrot Discovery**
**Display Name:** "Parrot Discovery"

**Technical Details:**
- **Device:** Parrot Discovery drone
- **Protocol:** Parrot-specific video protocol

**Audio Support:** ❌ **NO**

**Use Cases:**
- Parrot Discovery drone video feed

---

### 9. **Yuneec Mantis G**
**Display Name:** "Yuneec Mantis G"

**Technical Details:**
- **Device:** Yuneec Mantis G drone
- **Protocol:** Yuneec-specific video protocol

**Audio Support:** ❌ **NO**

**Use Cases:**
- Yuneec Mantis G drone video feed

---

### 10. **Herelink AirUnit / Herelink Hotspot**
**Display Name:** "Herelink AirUnit" or "Herelink Hotspot"

**Technical Details:**
- **Device:** Herelink video transmission system
- **Protocol:** Herelink-specific protocol
- **Compile-time flag:** `QGC_HERELINK_AIRUNIT_VIDEO` (determines which one is available)

**Audio Support:** ⚠️ **UNKNOWN**

**Use Cases:**
- Herelink radio controller video transmission
- Professional drone video links

**Notes:**
- Only one variant available per build (AirUnit XOR Hotspot)

---

## Special Modes

### 11. **No Video Available**
**Display Name:** "No Video Available"

**Description:** Shown when no video streaming support is compiled in or no devices are detected

---

### 12. **Video Stream Disabled**
**Display Name:** "Video Stream Disabled"

**Description:** User option to disable video streaming entirely

---

## Protocol Comparison Table

| Protocol | Transport | Codecs | Audio | Latency | Reliability | Use Case |
|----------|-----------|--------|-------|---------|-------------|----------|
| **RTSP** | TCP/UDP | H.264/H.265/AAC/MP3 | ✅ YES | Medium | High | IP cameras, professional |
| **UDP H.264** | UDP | H.264 only | ❌ NO | Low | Medium | Drone video (most common) |
| **UDP H.265** | UDP | H.265 only | ❌ NO | Low | Medium | High-quality drone video |
| **TCP-MPEG2** | TCP | MPEG2 | ⚠️ Maybe | High | Very High | Legacy systems |
| **MPEG-TS** | UDP | Various | ⚠️ Maybe | Medium | Medium | Broadcast systems |
| **UVC** | USB | Various | ⚠️ Maybe | Low | N/A | USB cameras |

---

## Audio Support Summary

### ✅ **Confirmed Audio Support:**
1. **RTSP** - Full audio support with new implementation
   - AAC (MPEG4-GENERIC)
   - MP3 (MPA)
   - H.264 + AAC tested and working
   - Volume control, mute, settings persistence

### ❌ **No Audio Support:**
1. **UDP H.264** - Raw video stream only
2. **UDP H.265** - Raw video stream only
3. **Device-specific sources** - Legacy, video-only

### ⚠️ **Unknown/Untested:**
1. **TCP-MPEG2** - May support audio (needs testing)
2. **MPEG-TS** - Container supports audio, but parsebin handling unclear
3. **UVC Cameras** - Depends on device and implementation
4. **Herelink** - Unknown

---

## Technical Implementation Details

### GStreamer Source Elements Used

| QGC Video Source | GStreamer Element | Configuration |
|------------------|-------------------|---------------|
| RTSP | `rtspsrc` | `location=rtsp://...` |
| UDP H.264/H.265 | `udpsrc` | `address=X port=Y caps=application/x-rtp` |
| TCP-MPEG2 | `tcpclientsrc` | `host=X port=Y` |
| MPEG-TS | `udpsrc` | `address=X port=Y` |
| UVC (Linux) | `v4l2src` | `device=/dev/videoX` |
| UVC (Windows) | `ksvideosrc` | `device-index=X` |

### Audio Pipeline (RTSP Only)

```
RTSP Source (rtspsrc)
    ↓
RTP Depayloader (rtpmp4gdepay for AAC / rtpmpadepay for MP3)
    ↓
Parser (aacparse / mpegaudioparse)
    ↓
Audio Tee
    ↓
Audio Decoder Valve
    ↓
Decodebin3
    ↓
audioconvert → audioresample → volume → wasapisink/pulsesink
```

### Low Latency Mode

Available for RTSP streams:
- Removes `rtpjitterbuffer`
- Sets video sink to asynchronous mode
- Reduces latency by ~200ms
- May increase frame drops

---

## Compile-Time Configuration

Video support is controlled by these flags:

```cpp
#define QGC_GST_STREAMING          // Enable GStreamer-based video
#define QGC_QT_STREAMING           // Enable Qt Multimedia-based video
#define QGC_DISABLE_UVC            // Disable UVC camera support
#define QGC_HERELINK_AIRUNIT_VIDEO // Enable Herelink AirUnit (vs Hotspot)
```

Without `QGC_GST_STREAMING` or `QGC_QT_STREAMING`, only "No Video Available" is shown.

---

## Future Enhancement Opportunities

1. **Audio Recording** - Mux audio into recorded video files (currently video-only)
2. **MPEG-TS Audio** - Test and enable audio support for MPEG-TS streams
3. **Additional Codecs** - Add support for Opus, Vorbis, FLAC
4. **UDP Audio Streams** - Support separate UDP audio streams alongside video
5. **WebRTC** - Add WebRTC as a modern low-latency protocol
6. **SRT (Secure Reliable Transport)** - Better than RTSP for unreliable networks

---

## Testing Status

| Protocol | Video Tested | Audio Tested | Status |
|----------|--------------|--------------|--------|
| RTSP H.264 + AAC | ✅ | ✅ | Working |
| RTSP H.265 + AAC | ⚠️ | ⚠️ | Needs testing |
| RTSP H.264 + MP3 | ⚠️ | ⚠️ | Needs testing |
| RTSP Video-only | ✅ | N/A | Working |
| UDP H.264 | ✅ | N/A | Working (backward compat) |
| UDP H.265 | ⚠️ | N/A | Needs testing |
| TCP-MPEG2 | ⚠️ | ⚠️ | Needs testing |
| MPEG-TS | ⚠️ | ⚠️ | Needs testing |
| UVC Cameras | ⚠️ | ⚠️ | Platform-dependent |

---

## Configuration Examples

### RTSP from IP Camera
```
Video Source: RTSP Video Stream
RTSP URL: rtsp://192.168.1.100:554/stream1
Audio Enabled: Yes
```

### UDP from PX4 Companion Computer
```
Video Source: UDP h.264 Video Stream
UDP URL: 0.0.0.0:5600
```

### RTSP from VLC Server
```bash
# On streaming computer:
vlc video.mp4 --sout '#transcode{vcodec=h264,acodec=mp4a,ab=128}:rtp{sdp=rtsp://:8554/test}' --sout-keep

# In QGC:
Video Source: RTSP Video Stream
RTSP URL: rtsp://127.0.0.1:8554/test
Audio Enabled: Yes
```

---

This documentation reflects the current state of QGroundControl video protocol support as of the audio implementation enhancement.
