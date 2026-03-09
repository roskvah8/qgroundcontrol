/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>
#include <QtCore/QString>

#ifdef QGC_GST_STREAMING
#include <gst/gst.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(ExternalAudioPlayerLog)

/// Plays audio from an external RTSP source independently from the video pipeline.
/// Used when IP cameras lack built-in audio and a separate audio source
/// (e.g., Raspberry Pi with USB mic) provides audio via RTSP.
class ExternalAudioPlayer : public QObject
{
    Q_OBJECT

public:
    explicit ExternalAudioPlayer(QObject *parent = nullptr);
    ~ExternalAudioPlayer() override;

    void start(const QString &rtspUrl);
    void stop();
    void setVolume(int volumePercent);
    bool isPlaying() const { return _playing; }

signals:
    void playingChanged(bool playing);
    void errorOccurred(const QString &error);

private:
#ifdef QGC_GST_STREAMING
    void _buildPipeline(const QString &rtspUrl);
    void _destroyPipeline();
    void _setPlaying(bool playing);

    static void _onRtspPadAdded(GstElement *rtspsrc, GstPad *pad, gpointer userData);
    static gboolean _onBusMessage(GstBus *bus, GstMessage *message, gpointer userData);

    GstElement *_pipeline = nullptr;
    GstElement *_rtspsrc = nullptr;
    GstElement *_audioConvert = nullptr;
    GstElement *_audioResample = nullptr;
    GstElement *_volume = nullptr;
    GstElement *_audioSink = nullptr;
    guint _busWatchId = 0;
#endif

    bool _playing = false;
    int _volumePercent = 80;
};
