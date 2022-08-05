#include "FFmpegHelper.h"
#include "FFmpegHeader.h"

#include <dshow.h>

#include <QDebug>

using namespace std;

void FFmpegHelper::registerAll()
{
    //av_register_all();
    avdevice_register_all();
    //avcodec_register_all();
}

std::string FFmpegHelper::getAudioDevice(int id) {
    GUID guid;
    if (1 == id) {
        guid = CLSID_AudioRendererCategory;
    } else if (2 == id) {
        guid = CLSID_AudioInputDeviceCategory;
    }

    char sName[256] = {0};
    //QString capture = "";
    string ret;
    bool bRet       = false;
    ::CoInitialize(NULL);

    ICreateDevEnum* pCreateDevEnum;  //enumrate all audio capture devices
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ICreateDevEnum,
                                  (void**)&pCreateDevEnum);

    IEnumMoniker* pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(/*CLSID_AudioInputDeviceCategory*/ guid, &pEm, 0);
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
            hr     = pBag->Read(L"FriendlyName", &var, NULL);  //还有其他属性，像描述信息等等
            if (hr == NOERROR) {
                //获取设备名称
                WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1, sName, 256, "", NULL);
                SysFreeString(var.bstrVal);
                ret = string("audio=") + sName;
                if (ret.find(/*"扬声器"*/ "virtual-audio-capturer") != string::npos) {
                    isRuning = false;
                }
                qDebug() << "Audio device:" << /*QString::fromStdString(ret)*/ QString::fromLocal8Bit(ret.c_str());
            }
            pBag->Release();
        }
        pM->Release();
        bRet = true;
    }
    //pCreateDevEnum = NULL;
    //pEm            = NULL;
    pCreateDevEnum->Release();
    pEm->Release();
    ::CoUninitialize();
    //return capture;
    return ret;
}