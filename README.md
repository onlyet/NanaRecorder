# 基于Qt+FFmpeg的录屏软件NanaRecorder



## UI
![QQ截图20230119022010](https://user-images.githubusercontent.com/19988547/213262749-ed8811aa-294a-44bc-8ed7-ad3c139540ac.png)


## 录制流程
![image](https://user-images.githubusercontent.com/19988547/183014314-ab124ad5-4ee4-47ce-b19d-52d1c5f41ee1.png)  

主线程：UI线程，调用Recorder接口  
采集线程：采集到帧后->格式转换/重采样->写进FIFO  
编码线程：循环从FIFO读取帧->编码->写进文件


## 环境依赖

- VS2019
- Qt5.12.9 
- FFmpeg5.1（项目已包含）  
  
解决方案支持Debug/Release和Win32/x64

## TODO
- [ ] 平衡高画质高帧率低码率  
- [X] 使用dshow替代gdigrab录制桌面  
- [ ] flush编码器  
- [ ] 支持同时录制扬声器和麦克风  
- [ ] 支持硬编码
