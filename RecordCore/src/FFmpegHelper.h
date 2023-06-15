#ifndef ONLYET_FFMPEGHELPER_H
#define ONLYET_FFMPEGHELPER_H

#include <string>

#include <QString>

enum class AudioCaptureDevice;

namespace FFmpegHelper {
    void registerAll();
    /**
     * ����������ҪתUTF8����, UTF8������ʾ <�ַ����е��ַ���Ч��>
     * @param type �豸����
     * @return �豸��
    */
    std::string getAudioDevice(AudioCaptureDevice type);

    QString err2Str(int err);
}

#endif  // !ONLYET_FFMPEGHELPER_H
