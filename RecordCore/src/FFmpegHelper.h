#pragma once
#include <string>

#include <QString>

enum AudioCaptureDeviceType {
    AudioCaptureDevice_Speaker = 0,  // ������
    AudioCaptureDevice_Microphone    // ��˷�
};

namespace FFmpegHelper {
    void registerAll();
    /**
     * ����������ҪתUTF8����, UTF8������ʾ <�ַ����е��ַ���Ч��>
     * @param type �豸����
     * @return �豸��
    */
    std::string getAudioDevice(AudioCaptureDeviceType type);

    QString err2Str(int err);
}
