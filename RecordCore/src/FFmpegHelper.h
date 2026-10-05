#ifndef ONLYET_FFMPEGHELPER_H
#define ONLYET_FFMPEGHELPER_H

#include <string>

#include <QString>

namespace onlyet {

enum class AudioCaptureDevice;

namespace FFmpegHelper {
void registerAll();
/**
* Strings containing Chinese characters require UTF-8 encoding; otherwise, they display as "<Invalid characters in string>".
* @param type Device type
* @return Device name
*/
std::string getAudioDevice(AudioCaptureDevice type);

QString err2Str(int err);
}  // namespace FFmpegHelper

}  // namespace onlyet

#endif  // !ONLYET_FFMPEGHELPER_H
