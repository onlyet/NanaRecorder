* 录屏功能支持：开始，暂停，结束。
* 使用Qt+C++封装FFmpeg API，没有使用废弃的FFmpeg API。
* 主线程：Qt GUI线程，以后可接入录屏UI。
* 父线程（读）：ScreenRecordThreadProc() 打开输入/输出流，创建子线程，然后从fifoBuffer读取帧，编码生成各种格式视频。
* 子线程（写）：ScreenAcquireThreadProc() 从输入流采集帧，缩放后写入fifoBuffer。
* 父子线程通过AVFifoBuffer（环形缓冲区）通信。是生存者与消费者的关系。使用条件信号和互斥量提高CPU利用率。
* 目前没添加录音功能，纯视频。
