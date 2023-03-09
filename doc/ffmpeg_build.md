# Build ffmpeg in linux

#### Download
```shell
mkdir -p ~/ffmpeg_sources && \
cd ~/ffmpeg_sources && \
wget -O ffmpeg-5.1.2.tar.bz2 https://ffmpeg.org/releases/ffmpeg-5.1.2.tar.bz2 && \
tar jxvf ffmpeg-5.1.2.tar.bz2
```

#### Build
Add `--enable-libpulse` to enable PulseAudio.

```shell
cd ~/ffmpeg_sources/ffmpeg-5.1.2 && \
PATH="$HOME/bin:$PATH" PKG_CONFIG_PATH="$HOME/ffmpeg_build/lib/pkgconfig" ./configure \
  --prefix="$HOME/ffmpeg_build" \
  --pkg-config-flags="--static" \
  --extra-cflags="-I$HOME/ffmpeg_build/include" \
  --extra-ldflags="-L$HOME/ffmpeg_build/lib" \
  --extra-libs="-lpthread -lm" \
  --ld="g++" \
  --bindir="$HOME/bin" \
  --enable-gpl \
  --enable-gnutls \
  --enable-libass \
  --enable-libfdk-aac \
  --enable-libfreetype \
  --enable-libmp3lame \
  --enable-libopus \
  --enable-libvorbis \
  --enable-libvpx \
  --enable-libx264 \
  --enable-libx265 \
  --enable-libpulse \
  --enable-pic \
  --enable-shared \
  --enable-nonfree && \
PATH="$HOME/bin:$PATH" make -j4 && \
make install && \
hash -r
```

Now re-login or run the following command for your current shell session to recognize the new ffmpeg location:
`source ~/.profile`

#### Reference
https://trac.ffmpeg.org/wiki/CompilationGuide/Ubuntu