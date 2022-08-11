#include "FFmpegHelper.h"
#include "FFmpegHeader.h"

#include <dshow.h>

#include <QDebug>

#define CAPTURE_SPEAKER_NAME "virtual-audio-capturer"
#define CAPTURE_MICROPHONE_NAME "��˷�"

using namespace std;

void FFmpegHelper::registerAll()
{
    //av_register_all();
    avdevice_register_all();
    //avcodec_register_all();
}

static std::string AnsiToUTF8(const char* _ansi, int _ansi_len) {
    std::string str_utf8("");
    wchar_t*    pUnicode = NULL;
    BYTE*       pUtfData = NULL;
    do {
        int unicodeNeed = MultiByteToWideChar(CP_ACP, 0, _ansi, _ansi_len, NULL, 0);
        pUnicode        = new wchar_t[unicodeNeed + 1];
        memset(pUnicode, 0, (unicodeNeed + 1) * sizeof(wchar_t));
        int unicodeDone = MultiByteToWideChar(CP_ACP, 0, _ansi, _ansi_len, (LPWSTR)pUnicode, unicodeNeed);

        if (unicodeDone != unicodeNeed) {
            break;
        }

        int utfNeed = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)pUnicode, unicodeDone, (char*)pUtfData, 0, NULL, NULL);
        pUtfData    = new BYTE[utfNeed + 1];
        memset(pUtfData, 0, utfNeed + 1);
        int utfDone = WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)pUnicode, unicodeDone, (char*)pUtfData, utfNeed, NULL, NULL);

        if (utfNeed != utfDone) {
            break;
        }
        str_utf8.assign((char*)pUtfData);
    } while (false);

    if (pUnicode) {
        delete[] pUnicode;
    }
    if (pUtfData) {
        delete[] pUtfData;
    }

    return str_utf8;
}

std::string FFmpegHelper::getAudioDevice(AudioCaptureDeviceType type) {
    string ret;
    GUID guid;
    char   sName[256] = {0};
#if 0
    if (1 == id) {
        guid = CLSID_AudioRendererCategory;
    } else if (2 == id) {
        guid = CLSID_AudioInputDeviceCategory;
    }
#else
    // Ŀǰ��˷��virtual-audio-capturer(ϵͳ����)���������ID
    guid = CLSID_AudioInputDeviceCategory;
#endif

    string captureDevice;
    if (AudioCaptureDevice_Speaker == type) {
        captureDevice = CAPTURE_SPEAKER_NAME;
    } else if (AudioCaptureDevice_Microphone == type) {
        captureDevice = CAPTURE_MICROPHONE_NAME;
    } else {
        return ret;
    }

    ::CoInitialize(NULL);

    ICreateDevEnum* pCreateDevEnum;  //enumrate all audio capture devices
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  (void**)&pCreateDevEnum);

    IEnumMoniker* pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(guid, &pEm, 0);
    if (hr != NOERROR) {
        ::CoUninitialize();
        //return "";
        return ret;
    }

    bool isRuning = true;
    pEm->Reset();
    ULONG cFetched;
    IMoniker* pM;
    while (pEm->Next(1, &pM, &cFetched) == S_OK && isRuning) {
        IPropertyBag* pBag = NULL;
        hr                 = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr     = pBag->Read(L"FriendlyName", &var, NULL);  //�����������ԣ���������Ϣ�ȵ�
            if (hr == NOERROR) {
                //��ȡ�豸����
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sName, 256, "", NULL);
                SysFreeString(var.bstrVal);
                // ע�⣺=ǰ��û�пո�FFmpeg��������Ҫ���豸����˫���ţ�API����
                ret = string("audio=") + sName;
                if (ret.find(captureDevice) != string::npos) {
                    isRuning = false;
                }

#if 0
                ret = QString::fromLocal8Bit(ret.c_str()).toStdString();
                qDebug() << "Audio device:" << QString::fromLocal8Bit(ret.c_str());
#else
                // ����������ҪתUTF8����
                ret = AnsiToUTF8(ret.c_str(), ret.length());
                qDebug() << "Audio device:" << QString::fromStdString(ret);
#endif
            }
            pBag->Release();
        }
        pM->Release();
    }

    pCreateDevEnum->Release();
    pEm->Release();
    ::CoUninitialize();
    return ret;
}

QString FFmpegHelper::err2Str(int err) {
    char errbuf[1024] = {0};
    av_strerror(err, errbuf, sizeof(errbuf) - 1);
    qDebug() << QString("FFmpeg error:%1, code=%2").arg(errbuf).arg(err);
    return QString(errbuf);
}
