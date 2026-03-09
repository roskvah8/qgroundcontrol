/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ExternalAudioPlayer.h"
#include "QGCLoggingCategory.h"

#include <QtCore/QMetaObject>

QGC_LOGGING_CATEGORY(ExternalAudioPlayerLog, "qgc.videomanager.externalaudioplayer")

ExternalAudioPlayer::ExternalAudioPlayer(QObject *parent)
    : QObject(parent)
{
    qCDebug(ExternalAudioPlayerLog) << "ExternalAudioPlayer created";
}

ExternalAudioPlayer::~ExternalAudioPlayer()
{
    stop();
    qCDebug(ExternalAudioPlayerLog) << "ExternalAudioPlayer destroyed";
}

void ExternalAudioPlayer::start(const QString &rtspUrl)
{
#ifdef QGC_GST_STREAMING
    if (_playing) {
        stop();
    }

    if (rtspUrl.trimmed().isEmpty()) {
        qCWarning(ExternalAudioPlayerLog) << "Cannot start with empty RTSP URL";
        return;
    }

    qCDebug(ExternalAudioPlayerLog) << "Starting external audio from:" << rtspUrl;
    _buildPipeline(rtspUrl);
#else
    Q_UNUSED(rtspUrl)
    qCWarning(ExternalAudioPlayerLog) << "GStreamer not available";
#endif
}

void ExternalAudioPlayer::stop()
{
#ifdef QGC_GST_STREAMING
    if (_pipeline) {
        qCDebug(ExternalAudioPlayerLog) << "Stopping external audio";
        _destroyPipeline();
        _setPlaying(false);
    }
#endif
}

void ExternalAudioPlayer::setVolume(int volumePercent)
{
    _volumePercent = qBound(0, volumePercent, 100);

#ifdef QGC_GST_STREAMING
    if (_volume) {
        const double vol = _volumePercent / 100.0;
        g_object_set(_volume, "volume", vol, nullptr);
        qCDebug(ExternalAudioPlayerLog) << "Volume set to" << vol;
    }
#endif
}

#ifdef QGC_GST_STREAMING

void ExternalAudioPlayer::_buildPipeline(const QString &rtspUrl)
{
    _pipeline = gst_pipeline_new("extaudio_pipeline");
    if (!_pipeline) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to create pipeline";
        emit errorOccurred(tr("Failed to create audio pipeline"));
        return;
    }

    // Create rtspsrc
    _rtspsrc = gst_element_factory_make("rtspsrc", "extaudio_rtspsrc");
    if (!_rtspsrc) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to create rtspsrc";
        _destroyPipeline();
        emit errorOccurred(tr("Failed to create RTSP source"));
        return;
    }

    g_object_set(_rtspsrc,
                 "location", rtspUrl.toUtf8().constData(),
                 "latency", 100,
                 "protocols", 0x4, // TCP only
                 nullptr);

    // Create audio processing chain
    _audioConvert = gst_element_factory_make("audioconvert", "extaudio_convert");
    _audioResample = gst_element_factory_make("audioresample", "extaudio_resample");
    _volume = gst_element_factory_make("volume", "extaudio_volume");

    if (!_audioConvert || !_audioResample || !_volume) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to create audio processing elements";
        _destroyPipeline();
        emit errorOccurred(tr("Failed to create audio processing elements"));
        return;
    }

    // Set initial volume
    const double vol = _volumePercent / 100.0;
    g_object_set(_volume, "volume", vol, nullptr);

    // Create platform-appropriate audio sink
#if defined(Q_OS_WIN)
    _audioSink = gst_element_factory_make("wasapisink", "extaudio_sink");
#elif defined(Q_OS_MACOS)
    _audioSink = gst_element_factory_make("osxaudiosink", "extaudio_sink");
#elif defined(Q_OS_LINUX)
    _audioSink = gst_element_factory_make("pulsesink", "extaudio_sink");
    if (!_audioSink) {
        _audioSink = gst_element_factory_make("alsasink", "extaudio_sink");
    }
#elif defined(Q_OS_ANDROID)
    _audioSink = gst_element_factory_make("openslessink", "extaudio_sink");
#elif defined(Q_OS_IOS)
    _audioSink = gst_element_factory_make("osxaudiosink", "extaudio_sink");
#else
    _audioSink = gst_element_factory_make("autoaudiosink", "extaudio_sink");
#endif

    if (!_audioSink) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to create audio sink";
        _destroyPipeline();
        emit errorOccurred(tr("Failed to create audio output"));
        return;
    }

    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(_pipeline), _rtspsrc, _audioConvert, _audioResample, _volume, _audioSink, nullptr);

    // Link the audio processing chain (rtspsrc will be linked dynamically via pad-added)
    if (!gst_element_link_many(_audioConvert, _audioResample, _volume, _audioSink, nullptr)) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to link audio processing chain";
        _destroyPipeline();
        emit errorOccurred(tr("Failed to link audio elements"));
        return;
    }

    // Connect pad-added signal for dynamic RTSP pad linking
    g_signal_connect(_rtspsrc, "pad-added", G_CALLBACK(_onRtspPadAdded), this);

    // Set up bus watch for error handling
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(_pipeline));
    if (bus) {
        _busWatchId = gst_bus_add_watch(bus, _onBusMessage, this);
        gst_object_unref(bus);
    }

    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to start pipeline";
        _destroyPipeline();
        emit errorOccurred(tr("Failed to start audio pipeline"));
        return;
    }

    _setPlaying(true);
    qCDebug(ExternalAudioPlayerLog) << "External audio pipeline started";
}

void ExternalAudioPlayer::_destroyPipeline()
{
    if (_busWatchId > 0) {
        g_source_remove(_busWatchId);
        _busWatchId = 0;
    }

    if (_pipeline) {
        gst_element_set_state(_pipeline, GST_STATE_NULL);
        gst_object_unref(_pipeline);
        _pipeline = nullptr;
    }

    // Elements are owned by the pipeline, so they are freed when the pipeline is destroyed
    _rtspsrc = nullptr;
    _audioConvert = nullptr;
    _audioResample = nullptr;
    _volume = nullptr;
    _audioSink = nullptr;
}

void ExternalAudioPlayer::_setPlaying(bool playing)
{
    if (_playing != playing) {
        _playing = playing;
        emit playingChanged(_playing);
    }
}

void ExternalAudioPlayer::_onRtspPadAdded(GstElement *rtspsrc, GstPad *pad, gpointer userData)
{
    Q_UNUSED(rtspsrc)
    auto *player = static_cast<ExternalAudioPlayer *>(userData);

    // Check if this is an audio pad
    GstCaps *caps = gst_pad_get_current_caps(pad);
    if (!caps) {
        caps = gst_pad_query_caps(pad, nullptr);
    }

    if (!caps || gst_caps_is_any(caps) || gst_caps_get_size(caps) == 0) {
        if (caps) {
            gst_caps_unref(caps);
        }
        return;
    }

    const GstStructure *structure = gst_caps_get_structure(caps, 0);
    const gchar *media = gst_structure_get_string(structure, "media");

    qCDebug(ExternalAudioPlayerLog) << "RTSP pad added, media:" << (media ? media : "unknown");

    if (!media || g_strcmp0(media, "audio") != 0) {
        // Ignore non-audio pads (video, etc.)
        gst_caps_unref(caps);
        qCDebug(ExternalAudioPlayerLog) << "Ignoring non-audio pad";
        return;
    }

    const gchar *encodingName = gst_structure_get_string(structure, "encoding-name");
    qCDebug(ExternalAudioPlayerLog) << "Audio encoding:" << (encodingName ? encodingName : "unknown");
    gst_caps_unref(caps);

    // Create appropriate depayloader and decoder based on encoding
    GstElement *depay = nullptr;
    GstElement *decoder = nullptr;

    if (g_strcmp0(encodingName, "OPUS") == 0) {
        depay = gst_element_factory_make("rtpopusdepay", nullptr);
        decoder = gst_element_factory_make("opusdec", nullptr);
    } else if (g_strcmp0(encodingName, "MPEG4-GENERIC") == 0) {
        depay = gst_element_factory_make("rtpmp4gdepay", nullptr);
        decoder = gst_element_factory_make("decodebin", nullptr);
    } else if (g_strcmp0(encodingName, "PCMU") == 0) {
        depay = gst_element_factory_make("rtppcmudepay", nullptr);
        decoder = gst_element_factory_make("mulawdec", nullptr);
    } else if (g_strcmp0(encodingName, "PCMA") == 0) {
        depay = gst_element_factory_make("rtppcmadepay", nullptr);
        decoder = gst_element_factory_make("alawdec", nullptr);
    } else if (g_strcmp0(encodingName, "MPA") == 0 || g_strcmp0(encodingName, "MPEG1") == 0) {
        depay = gst_element_factory_make("rtpmpadepay", nullptr);
        decoder = gst_element_factory_make("decodebin", nullptr);
    } else {
        qCWarning(ExternalAudioPlayerLog) << "Unsupported audio encoding:" << encodingName;
        QMetaObject::invokeMethod(player, [player, encodingName]() {
            emit player->errorOccurred(tr("Unsupported audio encoding: %1").arg(encodingName));
        }, Qt::QueuedConnection);
        return;
    }

    if (!depay || !decoder) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to create depayloader/decoder for" << encodingName;
        gst_clear_object(&depay);
        gst_clear_object(&decoder);
        return;
    }

    // Add to pipeline and link
    gst_bin_add_many(GST_BIN(player->_pipeline), depay, decoder, nullptr);

    // Link: rtspsrc pad -> depay
    GstPad *depaySink = gst_element_get_static_pad(depay, "sink");
    if (!depaySink || gst_pad_link(pad, depaySink) != GST_PAD_LINK_OK) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to link RTSP pad to depayloader";
        if (depaySink) {
            gst_object_unref(depaySink);
        }
        return;
    }
    gst_object_unref(depaySink);

    // Link: depay -> decoder -> audioconvert
    if (!gst_element_link(depay, decoder)) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to link depay to decoder";
        return;
    }

    if (!gst_element_link(decoder, player->_audioConvert)) {
        qCCritical(ExternalAudioPlayerLog) << "Failed to link decoder to audioconvert";
        return;
    }

    gst_element_sync_state_with_parent(depay);
    gst_element_sync_state_with_parent(decoder);

    qCDebug(ExternalAudioPlayerLog) << "Audio pad linked successfully, encoding:" << encodingName;
}

gboolean ExternalAudioPlayer::_onBusMessage(GstBus *bus, GstMessage *message, gpointer userData)
{
    Q_UNUSED(bus)
    auto *player = static_cast<ExternalAudioPlayer *>(userData);

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR: {
        GError *err = nullptr;
        gchar *debug = nullptr;
        gst_message_parse_error(message, &err, &debug);

        const QString errorMsg = QStringLiteral("External audio error: %1 (%2)")
                                     .arg(err->message)
                                     .arg(debug ? debug : "no debug info");

        qCWarning(ExternalAudioPlayerLog) << errorMsg;

        QMetaObject::invokeMethod(player, [player, errorMsg]() {
            emit player->errorOccurred(errorMsg);
            player->stop();
        }, Qt::QueuedConnection);

        g_error_free(err);
        g_free(debug);
        break;
    }

    case GST_MESSAGE_EOS:
        qCDebug(ExternalAudioPlayerLog) << "End of stream";
        QMetaObject::invokeMethod(player, [player]() {
            player->stop();
        }, Qt::QueuedConnection);
        break;

    case GST_MESSAGE_STATE_CHANGED:
        if (GST_MESSAGE_SRC(message) == GST_OBJECT(player->_pipeline)) {
            GstState oldState, newState, pending;
            gst_message_parse_state_changed(message, &oldState, &newState, &pending);
            qCDebug(ExternalAudioPlayerLog) << "Pipeline state:"
                                             << gst_element_state_get_name(oldState) << "->"
                                             << gst_element_state_get_name(newState);
        }
        break;

    default:
        break;
    }

    return TRUE;
}

#endif // QGC_GST_STREAMING
