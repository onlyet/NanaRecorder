#pragma once
#include <string>

#include <QString>

enum class AudioCaptureDevice {
    Speaker = 0,  // ������
    Microphone    // ��˷�
};

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
