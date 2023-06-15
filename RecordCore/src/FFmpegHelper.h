#ifndef ONLYET_FFMPEGHELPER_H
#define ONLYET_FFMPEGHELPER_H

#include <string>

#include <QString>

namespace onlyet {

enum class AudioCaptureDevice;

namespace FFmpegHelper {
void registerAll();
/**
     * 包含中文需要转UTF8编码, UTF8编码显示 <字符串中的字符无效。>
     * @param type 设备类型
     * @return 设备名
    */
std::string getAudioDevice(AudioCaptureDevice type);

QString err2Str(int err);
}  // namespace FFmpegHelper

}  // namespace onlyet

#endif  // !ONLYET_FFMPEGHELPER_H
