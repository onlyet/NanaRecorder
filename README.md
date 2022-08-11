# NanaRecorder

之前的录屏项目ScreenCapture存在音视频同步问题，所以重写了第二个版本：NanaRecorder。  
录制桌面和系统声音（扬声器）。

### 录制流程
![image](https://user-images.githubusercontent.com/19988547/183014314-ab124ad5-4ee4-47ce-b19d-52d1c5f41ee1.png)  

主线程：UI线程，调用Recorder接口  
采集线程：采集到帧后->格式转换/重采样->写进FIFO  
编码线程：循环从FIFO读取帧->编码->写进文件


### 环境依赖

- VS2019
- Qt5.12.9 
- FFmpeg4.1（项目已包含）

### UI
![image](https://user-images.githubusercontent.com/19988547/184192822-e971253a-33ea-43f0-913f-9ef8754271c1.png)


开始：开始录制  
结束：结束录制

### TODO
- [ ] 平衡高画质高帧率低码率  
- [ ] 使用dshow替代gdigrab录制桌面  
- [ ] flush编码器  
- [ ] 支持同时录制扬声器和麦克风  
- [ ] 支持硬编码
