[English version](./README_en.md)

# NanaRecorder: A Screen Recording Application Based on Qt and FFmpeg

## UI
![QQ Screenshot 20230119022010](./screenshot/UI.png)

## Recording Workflow
![flowchart](./screenshot/flowchart.png)

Main Thread: UI thread; calls the Recorder interface.
Capture Thread: Captures frames -> Format conversion/Resampling -> Writes to FIFO.
Encoding/Muxing Thread: Loops to read frames from FIFO -> Encodes -> Writes to file.

## Environment Dependencies
### Windows
VS: VS2017 or later recommended.
Qt: Qt5.12 or later recommended.
FFmpeg 5.1 (Included in the project; DLLs are automatically copied to the executable directory after building).

My development environment:
- VS2022
- Qt5.12.9

The solution supports Debug/Release and Win32/x64 configurations.
</br>
[VS+Qt Development Environment Configuration](./doc/VS%2BQt%E5%BC%80%E5%8F%91%E7%8E%AF%E5%A2%83.pdf)

### Linux
My development environment:
- Qt6.2.4
- FFmpeg5.1.2

<font color=red>Note:</font>
Audio recording on Linux currently utilizes PulseAudio. Since FFmpeg does not support PulseAudio by default, you must compile FFmpeg manually. Add `--enable-libpulse` during the `./configure` step. Refer to the following document for compilation details:
[ffmpeg build reference](./doc/ffmpeg_build.md)

You need to modify the dependency paths for Qt and FFmpeg in `CMakeLists.txt`:
- QT_PATH
- FFMPEG_ROOT_DIR (The root directory of your compiled FFmpeg)

#### Build
```cpp
mkdir build && cd build
cmake ..
make -j4
```

#### Run App
1. Run directly via Qt Creator
2. Launch via command line: `../bin/NanaRecorder`

## TODO
- [ ] High image quality, small file size, low bitrate
- [ ] Flush encoder
- [X] Support simultaneous recording of speaker and microphone
- [ ] Support hardware encoding
