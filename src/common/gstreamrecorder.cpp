/*
* Copyright (C) 2019 ~ 2019 UnionTech Software Technology Co.,Ltd.
*
* Author:     liuyanga <liuyanga@uniontech.com>
*
* Maintainer: liuyanga <liuyanga@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "gstreamrecorder.h"
#include <QDebug>

static const QString mp3Encoder = "capsfilter caps=audio/x-raw,rate=44100,channels=2 ! lamemp3enc name=enc target=bitrate cbr=true bitrate=192 ! xingmux ! id3mux";

GstPadProbeReturn bufferProbe(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    Q_UNUSED(pad);
    GstreamRecorder *recorder = static_cast<GstreamRecorder *>(user_data);
    if (GstBuffer *buffer = gst_pad_probe_info_get_buffer(info))
        return recorder->doBufferProbe(buffer) ? GST_PAD_PROBE_OK : GST_PAD_PROBE_DROP;
    return GST_PAD_PROBE_OK;
}

gboolean GstBusMessageCb(GstBus *bus, GstMessage *msg, void *userdata)
{
    Q_UNUSED(bus);
    GstreamRecorder *recorder = reinterpret_cast<GstreamRecorder *>(userdata);
    return  recorder->doBusMessage(msg);
}


GstreamRecorder::GstreamRecorder(QObject *parent)
    :QObject(parent)
{
     gst_init(nullptr, nullptr);
}

bool GstreamRecorder::createPipe()
{
   GstElement *audioSrc = nullptr; //声音采集设备
   GstElement *audioResample = nullptr; //重采样
   GstElement *audioConvert = nullptr; //格式转换
   GstElement *audioQueue = nullptr; //数据缓存
   GstElement *audioEncoder = nullptr; //编码器
   GstElement *audioOutput = nullptr; //输出文件
//   回音消除与噪声抑制
//   GstElement *audiowebrtcdsp = nullptr;
//   GstElement *audiowebrtcechoprobe = nullptr;

   bool success = false;
   do {
       audioSrc =  gst_element_factory_make("pulsesrc","audioSrc");
       if(audioSrc == nullptr){
           qDebug() << "audioSrc make error";
           break;
       }
       if(!m_currentDevice.isEmpty()){
           g_object_set(reinterpret_cast<gpointer*>(audioSrc), "device", m_currentDevice.toLatin1().data(), nullptr);
       }

//       audiowebrtcdsp= gst_element_factory_make("webrtcdsp","audiowebrtcdsp");
//       if(audiowebrtcdsp == nullptr){
//           qDebug() << "audiowebrtcdsp make error";
//           break;
//       }
//       audiowebrtcechoprobe = gst_element_factory_make("webrtcechoprobe","webrtcechoprobe");
//       if(audiowebrtcechoprobe == nullptr){
//           qDebug() << "audiowebrtcechoprobe make error";
//           break;
//       }
//       g_object_set(audiowebrtcdsp, "probe", "webrtcechoprobe", nullptr);

       audioResample = gst_element_factory_make("audioresample", nullptr);
       if(audioResample == nullptr){
           qDebug() << "audioResample make error";
           break;
       }
       audioConvert = gst_element_factory_make("audioconvert", "audioconvert");
       if(audioConvert == nullptr){
           qDebug() << "audioConvert make error";
           break;
       }
       audioQueue = gst_element_factory_make("queue", "audioqueue");
       if(audioQueue == nullptr){
           qDebug() << "audioQueue make error";
           break;
       }
       audioEncoder = gst_parse_bin_from_description(mp3Encoder.toLatin1().constData(),
                                                                         true, nullptr);
       if(audioEncoder == nullptr){
           qDebug() << "audioEncoder make error";
           break;
       }
       GstPad *pad = gst_element_get_static_pad(audioEncoder, "sink");
       if(pad){
           gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, bufferProbe, this, nullptr);
           gst_object_unref(pad);
       }else {
           qDebug() << "sink pad make error";
           break;
       }
       audioOutput = gst_element_factory_make("filesink", "filesink");
       if(audioOutput == nullptr){
           qDebug() << "audioOutput make error";
           break;
       }
       if(!m_outputFile.isEmpty()){
           g_object_set(reinterpret_cast<gpointer*>(audioOutput), "location",m_outputFile.toLatin1().constData(), nullptr);
       }
       m_pipeline =  gst_pipeline_new("deepin-voice-note");
       GstBus *bus = gst_pipeline_get_bus(reinterpret_cast<GstPipeline*>(m_pipeline));
       gst_bus_add_watch(bus,GstBusMessageCb,this);
       gst_object_unref(bus);

       gst_bin_add_many(reinterpret_cast<GstBin *>(m_pipeline),audioSrc,
                        /*audiowebrtcdsp,audiowebrtcechoprobe,*/
                        audioResample, audioConvert, audioQueue,
                        audioEncoder, audioOutput, nullptr);
       if(!gst_element_link_many(audioSrc,
                                 /*audiowebrtcdsp,audiowebrtcechoprobe,*/
                                 audioResample,audioConvert,
                                 audioQueue,audioEncoder,
                                 audioOutput,nullptr)){
            objectUnref(m_pipeline);
            m_pipeline = nullptr;
            qDebug() << "gst_element_link_many error";
            return success;
       }
       success = true;
   }while (!success);
   if(!success){
       objectUnref(audioSrc);
       objectUnref(audioResample);
       objectUnref(audioConvert);
       objectUnref(audioQueue);
       objectUnref(audioEncoder);
       objectUnref(audioOutput);
//       objectUnref(audiowebrtcdsp);
//       objectUnref(audiowebrtcechoprobe);
   }
   return  success;
}

void GstreamRecorder::deinit()
{
    stopRecord();
    objectUnref(m_pipeline);
    gst_deinit();
}

GstreamRecorder::~GstreamRecorder()
{
    deinit();
}

void GstreamRecorder::GetGstState(int *state, int *pending)
{
    *state = GST_STATE_NULL;
    *pending = GST_STATE_NULL;
    if (m_pipeline == nullptr) return;
    gst_element_get_state(m_pipeline, reinterpret_cast<GstState*>(state),
                          reinterpret_cast<GstState*>(pending), 0);
}

bool GstreamRecorder::startRecord()
{
    if(m_pipeline == nullptr && !createPipe()) return false;

    if(!m_format.isValid()){
        initFormat();
    }

    int state = -1;
    int pending = -1;
    GetGstState(&state, &pending);
    if (state == GST_STATE_PLAYING) {
        return true;
    } else if (state == GST_STATE_PAUSED) {
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        return true;
    }
    if (gst_element_set_state(m_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        qDebug() << "start error";
        return false;
    }
    return true;
}

void GstreamRecorder::stopRecord()
{
    if(m_pipeline != nullptr){
//        gst_element_send_event(m_pipeline, gst_event_new_eos());
//        g_usleep(50);
//        setStateToNull();
//        objectUnref(m_pipeline);
        int state = -1;
        int pending = -1;
        GetGstState(&state, &pending);
        if(state == GST_STATE_NULL) return;
        gst_element_send_event(m_pipeline, gst_event_new_eos());
        g_usleep(50);
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
    }
}

void GstreamRecorder::pauseRecord()
{
    if(m_pipeline != nullptr){
        int state = -1;
        int pending = -1;
        GetGstState(&state, &pending);

        if(state == GST_STATE_PLAYING){
            gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
        }
    }

}

void GstreamRecorder::setDevice(QString device)
{
    if(device != m_currentDevice){
        m_currentDevice = device;
        if(m_pipeline != nullptr){
            GstElement *audioSrc = gst_bin_get_by_name(reinterpret_cast<GstBin*>(m_pipeline), "audioSrc");
            g_object_set(reinterpret_cast<gpointer*>(audioSrc), "device", device.toLatin1().data(), nullptr);
        }
        qDebug() << device;
    }
}

void GstreamRecorder::setOutputFile(QString path)
{
    m_outputFile = path;
    if(m_pipeline != nullptr){
        GstElement *audioSink = gst_bin_get_by_name(reinterpret_cast<GstBin*>(m_pipeline), "filesink");
        g_object_set(reinterpret_cast<gpointer*>(audioSink), "location",m_outputFile.toLatin1().constData(), nullptr);
    }
}

bool GstreamRecorder::doBusMessage(GstMessage *message)
{
    if (!message) return true;
    switch (message->type){
    case GST_MESSAGE_ERROR:{
         GError *error = nullptr;
         gchar *dbg = nullptr;

         gst_message_parse_error(message, &error, &dbg);

         QString errMsg = error->message;
         emit errorMsg(errMsg);

         qDebug() << "Got pipeline error:"<< errMsg;

         if (dbg) {
             g_free(dbg);
         }

         if (error) {
             g_error_free(error);
         }
         break;
    }
    default:
        break;
    }
    return true;
}

bool GstreamRecorder::doBufferProbe(GstBuffer *buffer)
{
    if(buffer){
        qint64 position = static_cast<qint64>(buffer->pts);
        position = position >= 0
                ? position / (1000 * 1000) // 毫秒
                : -1;
        QByteArray data;
        GstMapInfo info;
        if (gst_buffer_map(buffer, &info, GST_MAP_READ)) {
            data = QByteArray(reinterpret_cast<const char *>(info.data),
                              static_cast<int>(info.size));
            gst_buffer_unmap(buffer, &info);
        } else {
            return true;
        }

        QMutexLocker locker(&m_bufferMutex);
        if (!m_pendingBuffer.isValid())
            QMetaObject::invokeMethod(this, "bufferProbed", Qt::QueuedConnection);
        m_pendingBuffer = QAudioBuffer(data, m_format, position);
    }
    return true;
}

void GstreamRecorder::bufferProbed()
{
    QAudioBuffer audioBuffer;
    {
        QMutexLocker locker(&m_bufferMutex);
        if (!m_pendingBuffer.isValid())
            return;
        audioBuffer = m_pendingBuffer;
        m_pendingBuffer = QAudioBuffer();
    }
    emit audioBufferProbed(audioBuffer);
}

void  GstreamRecorder::setStateToNull()
{
    GstState cur_state, pending;
    gst_element_get_state(m_pipeline, &cur_state, &pending, 0);
    if (cur_state == GST_STATE_NULL && pending == GST_STATE_VOID_PENDING)
        return;

    if (cur_state == GST_STATE_NULL && pending != GST_STATE_VOID_PENDING) {
        gst_element_set_state (m_pipeline, GST_STATE_NULL);
        return;
    }

    GstMessage *msg;
    GstBus *bus;
    gst_element_set_state(m_pipeline, GST_STATE_READY);
    gst_element_get_state(m_pipeline, nullptr, nullptr, 10);

    bus = gst_element_get_bus(m_pipeline);
    if (bus) {
        while ((msg = gst_bus_pop(bus))) {
            gst_bus_async_signal_func(bus, msg, nullptr);
            if (msg) {
                gst_message_unref(msg);
            }
        }
        gst_object_unref(bus);
    }
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
}

void GstreamRecorder::initFormat()
{
    //未压缩数据
    m_format.setCodec("audio/pcm");
    //通道，采样率
    m_format.setChannelCount(2);
    m_format.setSampleRate(44100);
    //lamemp3enc 编码器插件格式为S16LE
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);
    m_format.setSampleSize(16);
}

void GstreamRecorder::objectUnref(gpointer object)
{
    if(object != nullptr){
        gst_object_unref(object);
        object = nullptr;
    }
}
