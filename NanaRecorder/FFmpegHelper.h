#pragma once
#include <string>

#include <QString>

enum AudioCaptureDeviceType {
    AudioCaptureDevice_Speaker = 0,  // 扬声器
    AudioCaptureDevice_Microphone    // 麦克风
};

namespace FFmpegHelper {
    void registerAll();
    /**
     * 包含中文需要转UTF8编码, UTF8编码显示 <字符串中的字符无效。>
     * @param type 设备类型
     * @return 设备名
    */
    std::string getAudioDevice(AudioCaptureDeviceType type);

    QString err2Str(int err);
}
