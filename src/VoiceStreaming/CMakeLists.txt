# ============================================================================
# Voice Streaming Manager Module
# Handles live voice streaming over WebRTC
# ============================================================================

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        VoiceStreamingManager.cc
        VoiceStreamingManager.h
        WebRTCSignaling.cc
        WebRTCSignaling.h
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# ----------------------------------------------------------------------------
# GStreamer Voice Streaming Support
# ----------------------------------------------------------------------------

# Check if GStreamer support is enabled (should be enabled with video support)
# Use the same variable that VideoManager uses
if(QGC_ENABLE_GST_VIDEOSTREAMING)
    # GStreamer is already found by VideoManager's CMakeLists with WebRTC and Sdp as optional components
    # Check if WebRTC and SDP targets exist (created by FindGStreamer.cmake)

    # Debug: Check what GStreamer components were found
    message(STATUS "VoiceStreaming: Checking for GStreamer WebRTC and SDP components...")
    if(TARGET GStreamer::WebRTC)
        message(STATUS "VoiceStreaming:   GStreamer::WebRTC target: FOUND")
    else()
        message(STATUS "VoiceStreaming:   GStreamer::WebRTC target: NOT FOUND")
    endif()
    if(TARGET GStreamer::Sdp)
        message(STATUS "VoiceStreaming:   GStreamer::Sdp target: FOUND")
    else()
        message(STATUS "VoiceStreaming:   GStreamer::Sdp target: NOT FOUND")
    endif()

    if(TARGET GStreamer::WebRTC AND TARGET GStreamer::Sdp)
        # WebRTC and SDP are available - enable voice streaming

        target_sources(${CMAKE_PROJECT_NAME}
            PRIVATE
                GstVoiceStreamer.cc
                GstVoiceStreamer.h
        )

        # Link GStreamer WebRTC and SDP libraries
        target_link_libraries(${CMAKE_PROJECT_NAME}
            PRIVATE
                GStreamer::WebRTC
                GStreamer::Sdp
        )

        # Define QGC_GST_STREAMING as a preprocessor macro so code can use #ifdef
        target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE QGC_GST_STREAMING)

        # Note: GST_USE_UNSTABLE_API is defined directly in source files before
        # including GStreamer headers to suppress #warning directives

        message(STATUS "VoiceStreaming: GStreamer voice streaming ENABLED")
    else()
        message(WARNING "VoiceStreaming: GStreamer WebRTC/SDP libraries not found. Voice streaming will be DISABLED.")
        message(WARNING "  Install gstreamer-plugins-bad development package if you need voice streaming.")
    endif()
else()
    message(STATUS "VoiceStreaming: GStreamer voice streaming disabled (video streaming not enabled)")
endif()
