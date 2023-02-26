#include "FFmpegHelper.h"
#include "FFmpegHeader.h"

#ifdef WIN32
#include <dshow.h>
#elif __linux__

#else
#error Unsupported platform
#endif

#include <QDebug>

#include <unordered_set>

#define CAPTURE_SPEAKER_NAME "virtual-audio-capturer"
#define CAPTURE_MICROPHONE_NAME1 "麦克风"
#define CAPTURE_MICROPHONE_NAME2 "Microphone"

using namespace std;

void FFmpegHelper::registerAll()
{
    static bool s_init = false;
    if (!s_init) {
        s_init = true;
        //av_register_all();
        avdevice_register_all();
        //avcodec_register_all();
        qInfo() << "av_version_info:" << av_version_info();
    }
}

#ifdef WIN32
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
#endif  // WIN32

std::string FFmpegHelper::getAudioDevice(AudioCaptureDeviceType type) {
    string ret;

#ifdef WIN32
    GUID guid;
    char   sName[256] = {0};
    unordered_set<string> audioDevSet;

#if 0
    if (1 == id) {
        guid = CLSID_AudioRendererCategory;
    } else if (2 == id) {
        guid = CLSID_AudioInputDeviceCategory;
    }
#else
    // 目前麦克风和virtual-audio-capturer(系统声音)都是用这个ID
    guid = CLSID_AudioInputDeviceCategory;
#endif

    if (AudioCaptureDevice_Speaker == type) {
        audioDevSet.emplace(CAPTURE_SPEAKER_NAME);
    } else if (AudioCaptureDevice_Microphone == type) {
        audioDevSet.emplace(CAPTURE_MICROPHONE_NAME1);
        audioDevSet.emplace(CAPTURE_MICROPHONE_NAME2);
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

    bool isFound = false;
    pEm->Reset();
    ULONG cFetched;
    IMoniker* pM;
    while (pEm->Next(1, &pM, &cFetched) == S_OK && !isFound) {
        IPropertyBag* pBag = NULL;
        hr                 = pM->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pBag);
        if (SUCCEEDED(hr)) {
            VARIANT var;
            var.vt = VT_BSTR;
            hr     = pBag->Read(L"FriendlyName", &var, NULL);  //还有其他属性，像描述信息等等
            if (hr == NOERROR) {
                //获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sName, 256, "", NULL);
                SysFreeString(var.bstrVal);
                // 注意：=前后没有空格，FFmpeg命令行需要对设备名加双引号，API则不用
                string tmpName = string("audio=") + sName;

                for (const auto& dev : audioDevSet) {
                    if (tmpName.find(dev) != string::npos) {
#if 0
                        tmpName = QString::fromLocal8Bit(tmpName.c_str()).toStdString();
                        qDebug() << "Audio device:" << QString::fromLocal8Bit(tmpName.c_str());
#else
                        // 包含中文需要转UTF8编码
                        tmpName = AnsiToUTF8(tmpName.c_str(), tmpName.length());
                        qInfo() << "Audio device:" << QString::fromStdString(tmpName);
#endif
                        ret     = tmpName;
                        isFound = true;
                        break;
                    }
                }
            }
            pBag->Release();
        }
        pM->Release();
    }

    if (!isFound) {
        qInfo() << QString("Cant't find %1 device").arg(AudioCaptureDevice_Speaker == type ? "speaker" : "microphone");
    }

    pCreateDevEnum->Release();
    pEm->Release();
    ::CoUninitialize();
#endif  // WIN32
    return ret;
}

QString FFmpegHelper::err2Str(int err) {
    char errbuf[1024] = {0};
    av_strerror(err, errbuf, sizeof(errbuf) - 1);
    qCritical() << QString("FFmpeg error:%1, code=%2").arg(errbuf).arg(err);
    return QString("%1(%2)").arg(errbuf).arg(err);
}
