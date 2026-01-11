/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "VideoSettings.h"
#include "VideoManager.h"
#include "VideoStreamConfigurationList.h"
#include "VideoStreamConfiguration.h"

#include <QtCore/QVariantList>

#ifdef QGC_GST_STREAMING
#include "GStreamer.h"
#endif
#ifndef QGC_DISABLE_UVC
#include "UVCReceiver.h"
#endif

DECLARE_SETTINGGROUP(Video, "Video")
{
    // Setup enum values for videoSource settings into meta data
    QVariantList videoSourceList;
#if defined(QGC_GST_STREAMING) || defined(QGC_QT_STREAMING)
    videoSourceList.append(videoSourceRTSP);
    videoSourceList.append(videoSourceUDPH264);
    videoSourceList.append(videoSourceUDPH265);
    videoSourceList.append(videoSourceTCP);
    videoSourceList.append(videoSourceMPEGTS);
    videoSourceList.append(videoSource3DRSolo);
    videoSourceList.append(videoSourceParrotDiscovery);
    videoSourceList.append(videoSourceYuneecMantisG);

    #ifdef QGC_HERELINK_AIRUNIT_VIDEO
        videoSourceList.append(videoSourceHerelinkAirUnit);
    #else
        videoSourceList.append(videoSourceHerelinkHotspot);
    #endif
#endif
#ifndef QGC_DISABLE_UVC
    QStringList uvcDevices = UVCReceiver::getDeviceNameList();
    for (const QString& device : uvcDevices) {
        videoSourceList.append(device);
    }
#endif
    if (videoSourceList.count() == 0) {
        _noVideo = true;
        videoSourceList.append(videoSourceNoVideo);
        setVisible(false);
    } else {
        videoSourceList.insert(0, videoDisabled);
    }

    // make translated strings
    QStringList videoSourceCookedList;
    for (const QVariant& videoSource: videoSourceList) {
        videoSourceCookedList.append( VideoSettings::tr(videoSource.toString().toStdString().c_str()) );
    }

    _nameToMetaDataMap[videoSourceName]->setEnumInfo(videoSourceCookedList, videoSourceList);

    _setForceVideoDecodeList();

    // Set default value for videoSource
    _setDefaults();

    // Initialize stream configurations list
    _streamConfigurations = new VideoStreamConfigurationList(this);
    _streamConfigurations->loadFromSettings();
}

void VideoSettings::_setDefaults()
{
    if (_noVideo) {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoSourceNoVideo);
    } else {
        _nameToMetaDataMap[videoSourceName]->setRawDefaultValue(videoDisabled);
    }
}

DECLARE_SETTINGSFACT(VideoSettings, aspectRatio)
DECLARE_SETTINGSFACT(VideoSettings, videoFit)
DECLARE_SETTINGSFACT(VideoSettings, gridLines)
DECLARE_SETTINGSFACT(VideoSettings, showRecControl)
DECLARE_SETTINGSFACT(VideoSettings, recordingFormat)
DECLARE_SETTINGSFACT(VideoSettings, maxVideoSize)
DECLARE_SETTINGSFACT(VideoSettings, enableStorageLimit)
DECLARE_SETTINGSFACT(VideoSettings, streamEnabled)
DECLARE_SETTINGSFACT(VideoSettings, disableWhenDisarmed)
DECLARE_SETTINGSFACT(VideoSettings, audioVolume)

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, videoSource)
{
    if (!_videoSourceFact) {
        _videoSourceFact = _createSettingsFact(videoSourceName);
        //-- Check for sources no longer available
        if(!_videoSourceFact->enumValues().contains(_videoSourceFact->rawValue().toString())) {
            if (_noVideo) {
                _videoSourceFact->setRawValue(videoSourceNoVideo);
            } else {
                _videoSourceFact->setRawValue(videoDisabled);
            }
        }
        connect(_videoSourceFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _videoSourceFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, forceVideoDecoder)
{
    if (!_forceVideoDecoderFact) {
        _forceVideoDecoderFact = _createSettingsFact(forceVideoDecoderName);

        _forceVideoDecoderFact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
        );

        connect(_forceVideoDecoderFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _forceVideoDecoderFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, lowLatencyMode)
{
    if (!_lowLatencyModeFact) {
        _lowLatencyModeFact = _createSettingsFact(lowLatencyModeName);

        _lowLatencyModeFact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
        );

        connect(_lowLatencyModeFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _lowLatencyModeFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspTimeout)
{
    if (!_rtspTimeoutFact) {
        _rtspTimeoutFact = _createSettingsFact(rtspTimeoutName);

        _rtspTimeoutFact->setVisible(
#ifdef QGC_GST_STREAMING
            true
#else
            false
#endif
        );

        connect(_rtspTimeoutFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspTimeoutFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, udpUrl)
{
    if (!_udpUrlFact) {
        _udpUrlFact = _createSettingsFact(udpUrlName);
        connect(_udpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _udpUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, rtspUrl)
{
    if (!_rtspUrlFact) {
        _rtspUrlFact = _createSettingsFact(rtspUrlName);
        connect(_rtspUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _rtspUrlFact;
}

DECLARE_SETTINGSFACT_NO_FUNC(VideoSettings, tcpUrl)
{
    if (!_tcpUrlFact) {
        _tcpUrlFact = _createSettingsFact(tcpUrlName);
        connect(_tcpUrlFact, &Fact::valueChanged, this, &VideoSettings::_configChanged);
    }
    return _tcpUrlFact;
}

bool VideoSettings::streamConfigured(void)
{
    //-- First, check if it's autoconfigured
    if(VideoManager::instance()->autoStreamConfigured()) {
        qCDebug(VideoManagerLog) << "Stream auto configured";
        return true;
    }

    //-- Check new multi-stream configuration list
    if (!_streamConfigurations) {
        qCDebug(VideoManagerLog) << "streamConfigured: _streamConfigurations is NULL";
        return false;
    }

    int streamCount = _streamConfigurations->count();
    qCDebug(VideoManagerLog) << "streamConfigured: checking" << streamCount << "streams";

    if (streamCount > 0) {
        // Check if there's at least one enabled stream
        for (int i = 0; i < streamCount; i++) {
            auto* stream = qobject_cast<VideoStreamConfiguration*>(_streamConfigurations->get(i));
            if (!stream) {
                qCWarning(VideoManagerLog) << "Stream" << i << "cast failed";
                continue;
            }

            if (stream->enabled() && stream->isValid()) {
                qCDebug(VideoManagerLog) << "Stream configured:" << stream->name();
                return true;
            }
        }
        qCDebug(VideoManagerLog) << "No enabled & valid streams found";
    }

    return false;
}

void VideoSettings::_configChanged(QVariant)
{
    emit streamConfiguredChanged(streamConfigured());
}

void VideoSettings::_setForceVideoDecodeList()
{
#ifdef QGC_GST_STREAMING
    static const QList<GStreamer::VideoDecoderOptions> removeForceVideoDecodeList{
#if defined(Q_OS_ANDROID)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderNVIDIA,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderIntel,
#elif defined(Q_OS_LINUX)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
#elif defined(Q_OS_WIN)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVideoToolbox,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVulkan,
#elif defined(Q_OS_MACOS)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
#elif defined(Q_OS_IOS)
    GStreamer::VideoDecoderOptions::ForceVideoDecoderDirectX3D,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderVAAPI,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderNVIDIA,
    GStreamer::VideoDecoderOptions::ForceVideoDecoderIntel,
#endif
    };

    for (const auto &value : removeForceVideoDecodeList) {
        _nameToMetaDataMap[forceVideoDecoderName]->removeEnumInfo(value);
    }
#endif
}
