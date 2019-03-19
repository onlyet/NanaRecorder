* 录屏功能支持：开始，暂停，结束。
* 使用Qt+C++封装FFmpeg API，没有使用废弃的FFmpeg API。 
###
* 主线程：Qt GUI线程，以后可接入录屏UI。
* MuxThreadProc：复用线程，启动音视频采集线程。打开输入/输出流，然后从fifoBuffer读取帧，编码生成各种格式视频。 
* ScreenRecordThreadProc：视频采集线程，从输入流采集帧，缩放后写入fifoBuffer。
* SoundRecordThreadProc：音频采集线程，从输入流采集样本，重采样后写入fifoBuffer。

  
