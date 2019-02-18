//
//#ifdef	__cplusplus
//extern "C"
//{
//#endif
//
//#include <Windows.h>
//
//#include "libavcodec/avcodec.h"
//#include "libavformat/avformat.h"
//#include "libswscale/swscale.h"
//#include "libavdevice/avdevice.h"
//#include "libavutil/audio_fifo.h"
//
//#pragma comment(lib, "avcodec.lib")
//#pragma comment(lib, "avformat.lib")
//#pragma comment(lib, "avutil.lib")
//#pragma comment(lib, "avdevice.lib")
//#pragma comment(lib, "avfilter.lib")
//
//	//#pragma comment(lib, "avfilter.lib")
//	//#pragma comment(lib, "postproc.lib")
//	//#pragma comment(lib, "swresample.lib")
//#pragma comment(lib, "swscale.lib")
//#ifdef __cplusplus
//};
//#endif
//
//AVFormatContext	*pFormatCtx_Video = NULL, *pFormatCtx_Audio = NULL, *pFormatCtx_Out = NULL;
//AVCodecContext	*pCodecCtx_Video;
//AVCodec			*pCodec_Video;
//AVFifoBuffer	*fifo_video = NULL;
//AVAudioFifo		*fifo_audio = NULL;
//int VideoIndex, AudioIndex;
//
//CRITICAL_SECTION AudioSection, VideoSection;
//
//
//
//SwsContext *img_convert_ctx;
//int frame_size = 0;
//
//uint8_t *picture_buf = NULL, *frame_buf = NULL;
//
//bool bCap = true;
//
//DWORD WINAPI ScreenCapThreadProc(LPVOID lpParam);
//DWORD WINAPI AudioCapThreadProc(LPVOID lpParam);
//
//int OpenVideoCapture()
//{
//	AVInputFormat *ifmt = av_find_input_format("gdigrab");
//	//这里可以加参数打开，例如可以指定采集帧率
//	AVDictionary *options = NULL;
//	av_dict_set(&options, "framerate", "15", NULL);
//	if (avformat_open_input(&pFormatCtx_Video, "desktop", ifmt, &options) != 0)
//	{
//		printf("Couldn't open input stream.（无法打开视频输入流）\n");
//		return -1;
//	}
//	if (avformat_find_stream_info(pFormatCtx_Video, NULL)<0)
//	{
//		printf("Couldn't find stream information.（无法获取视频流信息）\n");
//		return -1;
//	}
//	if (pFormatCtx_Video->streams[0]->codec->codec_type != AVMEDIA_TYPE_VIDEO)
//	{
//		printf("Couldn't find video stream information.（无法获取视频流信息）\n");
//		return -1;
//	}
//	pCodecCtx_Video = pFormatCtx_Video->streams[0]->codec;
//	pCodec_Video = avcodec_find_decoder(pCodecCtx_Video->codec_id);
//	if (pCodec_Video == NULL)
//	{
//		printf("Codec not found.（没有找到解码器）\n");
//		return -1;
//	}
//	if (avcodec_open2(pCodecCtx_Video, pCodec_Video, NULL) < 0)
//	{
//		printf("Could not open codec.（无法打开解码器）\n");
//		return -1;
//	}
//
//
//
//	img_convert_ctx = sws_getContext(pCodecCtx_Video->width, pCodecCtx_Video->height, pCodecCtx_Video->pix_fmt,
//		pCodecCtx_Video->width, pCodecCtx_Video->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
//
//	frame_size = avpicture_get_size(pCodecCtx_Video->pix_fmt, pCodecCtx_Video->width, pCodecCtx_Video->height);
//	//申请30帧缓存
//	fifo_video = av_fifo_alloc(30 * avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx_Video->width, pCodecCtx_Video->height));
//
//	return 0;
//}
//
//static char *dup_wchar_to_utf8(wchar_t *w)
//{
//	char *s = NULL;
//	int l = WideCharToMultiByte(CP_UTF8, 0, w, -1, 0, 0, 0, 0);
//	s = (char *)av_malloc(l);
//	if (s)
//		WideCharToMultiByte(CP_UTF8, 0, w, -1, s, l, 0, 0);
//	return s;
//}
//
//int OpenAudioCapture()
//{
//	//查找输入方式
//	AVInputFormat *pAudioInputFmt = av_find_input_format("dshow");
//
//	//以Direct Show的方式打开设备，并将 输入方式 关联到格式上下文
//	char * psDevName = dup_wchar_to_utf8(L"audio=麦克风 (Realtek High Definition Au");
//
//	if (avformat_open_input(&pFormatCtx_Audio, psDevName, pAudioInputFmt, NULL) < 0)
//	{
//		printf("Couldn't open input stream.（无法打开音频输入流）\n");
//		return -1;
//	}
//
//	if (avformat_find_stream_info(pFormatCtx_Audio, NULL)<0)
//		return -1;
//
//	if (pFormatCtx_Audio->streams[0]->codec->codec_type != AVMEDIA_TYPE_AUDIO)
//	{
//		printf("Couldn't find video stream information.（无法获取音频流信息）\n");
//		return -1;
//	}
//
//	AVCodec *tmpCodec = avcodec_find_decoder(pFormatCtx_Audio->streams[0]->codec->codec_id);
//	if (0 > avcodec_open2(pFormatCtx_Audio->streams[0]->codec, tmpCodec, NULL))
//	{
//		printf("can not find or open audio decoder!\n");
//	}
//
//
//
//	return 0;
//}
//
//int OpenOutPut()
//{
//	AVStream *pVideoStream = NULL, *pAudioStream = NULL;
//	const char *outFileName = "test.mp4";
//	avformat_alloc_output_context2(&pFormatCtx_Out, NULL, NULL, outFileName);
//
//	if (pFormatCtx_Video->streams[0]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
//	{
//		AVCodecContext *videoCodecCtx;
//		VideoIndex = 0;
//		pVideoStream = avformat_new_stream(pFormatCtx_Out, NULL);
//
//		if (!pVideoStream)
//		{
//			printf("can not new stream for output!\n");
//			return -1;
//		}
//
//		//set codec context param
//		pVideoStream->codec->codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
//		pVideoStream->codec->height = pFormatCtx_Video->streams[0]->codec->height;
//		pVideoStream->codec->width = pFormatCtx_Video->streams[0]->codec->width;
//
//		pVideoStream->codec->time_base = pFormatCtx_Video->streams[0]->codec->time_base;
//		pVideoStream->codec->sample_aspect_ratio = pFormatCtx_Video->streams[0]->codec->sample_aspect_ratio;
//		// take first format from list of supported formats
//		pVideoStream->codec->pix_fmt = pFormatCtx_Out->streams[VideoIndex]->codec->codec->pix_fmts[0];
//
//		//open encoder
//		if (!pVideoStream->codec->codec)
//		{
//			printf("can not find the encoder!\n");
//			return -1;
//		}
//
//		if (pFormatCtx_Out->oformat->flags & AVFMT_GLOBALHEADER)
//			pVideoStream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//
//		if ((avcodec_open2(pVideoStream->codec, pVideoStream->codec->codec, NULL)) < 0)
//		{
//			printf("can not open the encoder\n");
//			return -1;
//		}
//	}
//
//	if (pFormatCtx_Audio->streams[0]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
//	{
//		AVCodecContext *pOutputCodecCtx;
//		AudioIndex = 1;
//		pAudioStream = avformat_new_stream(pFormatCtx_Out, NULL);
//
//		pAudioStream->codec->codec = avcodec_find_encoder(pFormatCtx_Out->oformat->audio_codec);
//
//		pOutputCodecCtx = pAudioStream->codec;
//
//		pOutputCodecCtx->sample_rate = pFormatCtx_Audio->streams[0]->codec->sample_rate;
//		pOutputCodecCtx->channel_layout = pFormatCtx_Out->streams[0]->codec->channel_layout;
//		pOutputCodecCtx->channels = av_get_channel_layout_nb_channels(pAudioStream->codec->channel_layout);
//		if (pOutputCodecCtx->channel_layout == 0)
//		{
//			pOutputCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
//			pOutputCodecCtx->channels = av_get_channel_layout_nb_channels(pOutputCodecCtx->channel_layout);
//
//		}
//		pOutputCodecCtx->sample_fmt = pAudioStream->codec->codec->sample_fmts[0];
//		AVRational time_base = { 1, pAudioStream->codec->sample_rate };
//		pAudioStream->time_base = time_base;
//		//audioCodecCtx->time_base = time_base;
//
//		pOutputCodecCtx->codec_tag = 0;
//		if (pFormatCtx_Out->oformat->flags & AVFMT_GLOBALHEADER)
//			pOutputCodecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;
//
//		if (avcodec_open2(pOutputCodecCtx, pOutputCodecCtx->codec, 0) < 0)
//		{
//			//编码器打开失败，退出程序
//			return -1;
//		}
//	}
//
//	if (!(pFormatCtx_Out->oformat->flags & AVFMT_NOFILE))
//	{
//		if (avio_open(&pFormatCtx_Out->pb, outFileName, AVIO_FLAG_WRITE) < 0)
//		{
//			printf("can not open output file handle!\n");
//			return -1;
//		}
//	}
//
//	if (avformat_write_header(pFormatCtx_Out, NULL) < 0)
//	{
//		printf("can not write the header of the output file!\n");
//		return -1;
//	}
//
//	return 0;
//}
//
//int _tmain(int argc, _TCHAR* argv[])
//{
//	av_register_all();
//	avdevice_register_all();
//	if (OpenVideoCapture() < 0)
//	{
//		return -1;
//	}
//	if (OpenAudioCapture() < 0)
//	{
//		return -1;
//	}
//	if (OpenOutPut() < 0)
//	{
//		return -1;
//	}
//
//	InitializeCriticalSection(&VideoSection);
//	InitializeCriticalSection(&AudioSection);
//
//	AVFrame *picture = av_frame_alloc();
//	int size = avpicture_get_size(pFormatCtx_Out->streams[VideoIndex]->codec->pix_fmt,
//		pFormatCtx_Out->streams[VideoIndex]->codec->width, pFormatCtx_Out->streams[VideoIndex]->codec->height);
//	picture_buf = new uint8_t[size];
//
//	avpicture_fill((AVPicture *)picture, picture_buf,
//		pFormatCtx_Out->streams[VideoIndex]->codec->pix_fmt,
//		pFormatCtx_Out->streams[VideoIndex]->codec->width,
//		pFormatCtx_Out->streams[VideoIndex]->codec->height);
//
//
//
//	//star cap screen thread
//	CreateThread(NULL, 0, ScreenCapThreadProc, 0, 0, NULL);
//	//star cap audio thread
//	CreateThread(NULL, 0, AudioCapThreadProc, 0, 0, NULL);
//	int64_t cur_pts_v = 0, cur_pts_a = 0;
//	int VideoFrameIndex = 0, AudioFrameIndex = 0;
//
//	while (1)
//	{
//		if (_kbhit() != 0 && bCap)
//		{
//			bCap = false;
//			Sleep(2000);//简单的用sleep等待采集线程关闭
//		}
//		if (fifo_audio && fifo_video)
//		{
//			int sizeAudio = av_audio_fifo_size(fifo_audio);
//			int sizeVideo = av_fifo_size(fifo_video);
//			//缓存数据写完就结束循环
//			if (av_audio_fifo_size(fifo_audio) <= pFormatCtx_Out->streams[AudioIndex]->codec->frame_size &&
//				av_fifo_size(fifo_video) <= frame_size && !bCap)
//			{
//				break;
//			}
//		}
//
//		if (av_compare_ts(cur_pts_v, pFormatCtx_Out->streams[VideoIndex]->time_base,
//			cur_pts_a, pFormatCtx_Out->streams[AudioIndex]->time_base) <= 0)
//		{
//			//read data from fifo
//			if (av_fifo_size(fifo_video) < frame_size && !bCap)
//			{
//				cur_pts_v = 0x7fffffffffffffff;
//			}
//			if (av_fifo_size(fifo_video) >= size)
//			{
//				EnterCriticalSection(&VideoSection);
//				av_fifo_generic_read(fifo_video, picture_buf, size, NULL);
//				LeaveCriticalSection(&VideoSection);
//
//				avpicture_fill((AVPicture *)picture, picture_buf,
//					pFormatCtx_Out->streams[VideoIndex]->codec->pix_fmt,
//					pFormatCtx_Out->streams[VideoIndex]->codec->width,
//					pFormatCtx_Out->streams[VideoIndex]->codec->height);
//
//				//pts = n * (（1 / timbase）/ fps);
//				picture->pts = VideoFrameIndex * ((pFormatCtx_Video->streams[0]->time_base.den / pFormatCtx_Video->streams[0]->time_base.num) / 15);
//
//				int got_picture = 0;
//				AVPacket pkt;
//				av_init_packet(&pkt);
//
//				pkt.data = NULL;
//				pkt.size = 0;
//				int ret = avcodec_encode_video2(pFormatCtx_Out->streams[VideoIndex]->codec, &pkt, picture, &got_picture);
//				if (ret < 0)
//				{
//					//编码错误,不理会此帧
//					continue;
//				}
//
//				if (got_picture == 1)
//				{
//					pkt.stream_index = VideoIndex;
//					pkt.pts = av_rescale_q_rnd(pkt.pts, pFormatCtx_Video->streams[0]->time_base,
//						pFormatCtx_Out->streams[VideoIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//					pkt.dts = av_rescale_q_rnd(pkt.dts, pFormatCtx_Video->streams[0]->time_base,
//						pFormatCtx_Out->streams[VideoIndex]->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
//
//					pkt.duration = ((pFormatCtx_Out->streams[0]->time_base.den / pFormatCtx_Out->streams[0]->time_base.num) / 15);
//
//					cur_pts_v = pkt.pts;
//
//					ret = av_interleaved_write_frame(pFormatCtx_Out, &pkt);
//					//delete[] pkt.data;
//					av_free_packet(&pkt);
//				}
//				VideoFrameIndex++;
//			}
//		}
//		else
//		{
//			if (NULL == fifo_audio)
//			{
//				continue;//还未初始化fifo
//			}
//			if (av_audio_fifo_size(fifo_audio) < pFormatCtx_Out->streams[AudioIndex]->codec->frame_size && !bCap)
//			{
//				cur_pts_a = 0x7fffffffffffffff;
//			}
//			if (av_audio_fifo_size(fifo_audio) >=
//				(pFormatCtx_Out->streams[AudioIndex]->codec->frame_size > 0 ? pFormatCtx_Out->streams[AudioIndex]->codec->frame_size : 1024))
//			{
//				AVFrame *frame;
//				frame = av_frame_alloc();
//				frame->nb_samples = pFormatCtx_Out->streams[AudioIndex]->codec->frame_size>0 ? pFormatCtx_Out->streams[AudioIndex]->codec->frame_size : 1024;
//				frame->channel_layout = pFormatCtx_Out->streams[AudioIndex]->codec->channel_layout;
//				frame->format = pFormatCtx_Out->streams[AudioIndex]->codec->sample_fmt;
//				frame->sample_rate = pFormatCtx_Out->streams[AudioIndex]->codec->sample_rate;
//				av_frame_get_buffer(frame, 0);
//
//				EnterCriticalSection(&AudioSection);
//				av_audio_fifo_read(fifo_audio, (void **)frame->data,
//					(pFormatCtx_Out->streams[AudioIndex]->codec->frame_size > 0 ? pFormatCtx_Out->streams[AudioIndex]->codec->frame_size : 1024));
//				LeaveCriticalSection(&AudioSection);
//
//				if (pFormatCtx_Out->streams[0]->codec->sample_fmt != pFormatCtx_Audio->streams[AudioIndex]->codec->sample_fmt
//					|| pFormatCtx_Out->streams[0]->codec->channels != pFormatCtx_Audio->streams[AudioIndex]->codec->channels
//					|| pFormatCtx_Out->streams[0]->codec->sample_rate != pFormatCtx_Audio->streams[AudioIndex]->codec->sample_rate)
//				{
//					//如果输入和输出的音频格式不一样 需要重采样，这里是一样的就没做
//				}
//
//				AVPacket pkt_out;
//				av_init_packet(&pkt_out);
//				int got_picture = -1;
//				pkt_out.data = NULL;
//				pkt_out.size = 0;
//
//				frame->pts = AudioFrameIndex * pFormatCtx_Out->streams[AudioIndex]->codec->frame_size;
//				if (avcodec_encode_audio2(pFormatCtx_Out->streams[AudioIndex]->codec, &pkt_out, frame, &got_picture) < 0)
//				{
//					printf("can not decoder a frame");
//				}
//				av_frame_free(&frame);
//				if (got_picture)
//				{
//					pkt_out.stream_index = AudioIndex;
//					pkt_out.pts = AudioFrameIndex * pFormatCtx_Out->streams[AudioIndex]->codec->frame_size;
//					pkt_out.dts = AudioFrameIndex * pFormatCtx_Out->streams[AudioIndex]->codec->frame_size;
//					pkt_out.duration = pFormatCtx_Out->streams[AudioIndex]->codec->frame_size;
//
//					cur_pts_a = pkt_out.pts;
//
//					int ret = av_interleaved_write_frame(pFormatCtx_Out, &pkt_out);
//					av_free_packet(&pkt_out);
//				}
//				AudioFrameIndex++;
//			}
//		}
//	}
//
//	delete[] picture_buf;
//
//	av_fifo_free(fifo_video);
//	av_audio_fifo_free(fifo_audio);
//
//	av_write_trailer(pFormatCtx_Out);
//
//	avio_close(pFormatCtx_Out->pb);
//	avformat_free_context(pFormatCtx_Out);
//
//	if (pFormatCtx_Video != NULL)
//	{
//		avformat_close_input(&pFormatCtx_Video);
//		pFormatCtx_Video = NULL;
//	}
//	if (pFormatCtx_Audio != NULL)
//	{
//		avformat_close_input(&pFormatCtx_Audio);
//		pFormatCtx_Audio = NULL;
//	}
//
//	return 0;
//}
//
//DWORD WINAPI ScreenCapThreadProc(LPVOID lpParam)
//{
//	AVPacket packet;/* = (AVPacket *)av_malloc(sizeof(AVPacket))*/;
//	int got_picture;
//	AVFrame	*pFrame;
//	pFrame = avcodec_alloc_frame();
//
//	AVFrame *picture = avcodec_alloc_frame();
//	int size = avpicture_get_size(pFormatCtx_Out->streams[VideoIndex]->codec->pix_fmt,
//		pFormatCtx_Out->streams[VideoIndex]->codec->width, pFormatCtx_Out->streams[VideoIndex]->codec->height);
//	//picture_buf = new uint8_t[size];
//
//	avpicture_fill((AVPicture *)picture, picture_buf,
//		pFormatCtx_Out->streams[VideoIndex]->codec->pix_fmt,
//		pFormatCtx_Out->streams[VideoIndex]->codec->width,
//		pFormatCtx_Out->streams[VideoIndex]->codec->height);
//
//	FILE *p = NULL;
//	p = fopen("proc_test.yuv", "wb+");
//	av_init_packet(&packet);
//	int height = pFormatCtx_Out->streams[VideoIndex]->codec->height;
//	int width = pFormatCtx_Out->streams[VideoIndex]->codec->width;
//	int y_size = height*width;
//	while (bCap)
//	{
//		packet.data = NULL;
//		packet.size = 0;
//		if (av_read_frame(pFormatCtx_Video, &packet) < 0)
//		{
//			continue;
//		}
//		if (packet.stream_index == 0)
//		{
//			if (avcodec_decode_video2(pCodecCtx_Video, pFrame, &got_picture, &packet) < 0)
//			{
//				printf("Decode Error.（解码错误）\n");
//				continue;
//			}
//			if (got_picture)
//			{
//				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
//					pFormatCtx_Out->streams[VideoIndex]->codec->height, picture->data, picture->linesize);
//
//				if (av_fifo_space(fifo_video) >= size)
//				{
//					EnterCriticalSection(&VideoSection);
//					av_fifo_generic_write(fifo_video, picture->data[0], y_size, NULL);
//					av_fifo_generic_write(fifo_video, picture->data[1], y_size / 4, NULL);
//					av_fifo_generic_write(fifo_video, picture->data[2], y_size / 4, NULL);
//					LeaveCriticalSection(&VideoSection);
//				}
//			}
//		}
//		av_free_packet(&packet);
//		//Sleep(50);
//	}
//	av_frame_free(&pFrame);
//	av_frame_free(&picture);
//	//delete[] picture_buf;
//	return 0;
//}
//
//DWORD WINAPI AudioCapThreadProc(LPVOID lpParam)
//{
//	AVPacket pkt;
//	AVFrame *frame;
//	frame = av_frame_alloc();
//	int gotframe;
//	while (bCap)
//	{
//		pkt.data = NULL;
//		pkt.size = 0;
//		if (av_read_frame(pFormatCtx_Audio, &pkt) < 0)
//		{
//			continue;
//		}
//
//		if (avcodec_decode_audio4(pFormatCtx_Audio->streams[0]->codec, frame, &gotframe, &pkt) < 0)
//		{
//			av_frame_free(&frame);
//			printf("can not decoder a frame");
//			break;
//		}
//		av_free_packet(&pkt);
//
//		if (!gotframe)
//		{
//			continue;//没有获取到数据，继续下一次
//		}
//
//		if (NULL == fifo_audio)
//		{
//			fifo_audio = av_audio_fifo_alloc(pFormatCtx_Audio->streams[0]->codec->sample_fmt,
//				pFormatCtx_Audio->streams[0]->codec->channels, 30 * frame->nb_samples);
//		}
//
//		int buf_space = av_audio_fifo_space(fifo_audio);
//		if (av_audio_fifo_space(fifo_audio) >= frame->nb_samples)
//		{
//			EnterCriticalSection(&AudioSection);
//			av_audio_fifo_write(fifo_audio, (void **)frame->data, frame->nb_samples);
//			LeaveCriticalSection(&AudioSection);
//		}
//	}
//	av_frame_free(&frame);
//	return 0;
//}
//char * EnumAudioDevice()
//{
//	UINT i = 0;
//	UINT uDevNum = waveInGetNumDevs();
//	for (; i<uDevNum; i++)
//	{
//		WAVEINCAPSW	wic = { 0 };
//		waveInGetDevCapsW(i, &wic, sizeof(wic));
//		printf("%s\n", wic.szPname);
//		char * tmpStr = dup_wchar_to_utf8(wic.szPname);
//		return tmpStr;
//	}
//}
//
