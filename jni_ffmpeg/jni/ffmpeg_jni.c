#include <stdio.h>
#include "ffmpeg_jni.h"
#include <android/log.h>



//��װ��ʽ
#include "libavformat/avformat.h"
//����
#include "libavcodec/avcodec.h"
//�ز���
#include "libswresample/swresample.h"


#define LOG_TAG "wangjf"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

char* Jstring2CStr(JNIEnv* env, jstring jstr) {
	char* rtn = NULL;
	jclass clsstring = (*env)->FindClass(env, "java/lang/String");
	jstring strencode = (*env)->NewStringUTF(env, "GB2312");
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes",
			"(Ljava/lang/String;)[B");
	jbyteArray barr = (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid,
			strencode); // String .getByte("GB2312");
	jsize alen = (*env)->GetArrayLength(env, barr);
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);
	if (alen > 0) {
		rtn = (char*) malloc(alen + 1); //"\0"
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	(*env)->ReleaseByteArrayElements(env, barr, ba, 0); //
	return rtn;
}

JNIEXPORT void JNICALL Java_com_research_amr2mp3_FFmpegUtil_jniRun
  (JNIEnv * env, jclass cls, 
  jstring jinput, jstring joutput){
	char* input = Jstring2CStr(env,jinput) ;
	char* output = Jstring2CStr(env,joutput);

	av_register_all();
        AVFormatContext *pFormatCtx = avformat_alloc_context();
        //����Ƶ�ļ�
		int resultint = avformat_open_input(&pFormatCtx, input, NULL, NULL);
        if (resultint != 0) {
            LOGI("%s", "open avformat fail");
			LOGE(" resultint  %d", resultint);
            return;
        }
        //��ȡ�����ļ���Ϣ
        if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
            LOGI("%s", "open stream info fail");
            return;
        }
        //��ȡ��Ƶ������λ��
        int i = 0, audio_stream_idx = -1;
        for (; i < pFormatCtx->nb_streams; i++) {
            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
                audio_stream_idx = i;
                break;
            }
        }
        //��ȡ������
        AVCodecContext *codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
        AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
        //�򿪽�����
        if (avcodec_open2(codecCtx, codec, NULL) < 0) {
            LOGI("%s", "open avcodec fial");
            return;
        }
        //ѹ������
        AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
        //��ѹ������
        AVFrame *frame = av_frame_alloc();
        //frame->16bit 44100 PCM ͳһ��Ƶ������ʽ�������
        SwrContext *swrContext = swr_alloc();
        //��Ƶ��ʽ  �ز������ò���
        const enum AVSampleFormat in_sample = codecCtx->sample_fmt;//ԭ��Ƶ�Ĳ���λ��
        //���������ʽ
        const enum AVSampleFormat out_sample = AV_SAMPLE_FMT_S16;//16λ
        int in_sample_rate = codecCtx->sample_rate;// ���������
        int out_sample_rate = 16000;//�������
    
        //������������
        uint64_t in_ch_layout = codecCtx->channel_layout;
        //�����������
        uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;//2ͨ�� ������ AV_CH_LAYOUT_STEREO  AV_CH_LAYOUT_MONO
    
        /**
         * struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
          int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
          int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
          int log_offset, void *log_ctx);
         */
        swr_alloc_set_opts(swrContext, out_ch_layout, out_sample, out_sample_rate, in_ch_layout, in_sample,
                           in_sample_rate, 0, NULL);
        swr_init(swrContext);
        int got_frame = 0;
        int ret;
        int out_channerl_nb = av_get_channel_layout_nb_channels(out_ch_layout);
        LOGE("out_channerl_nb %d ", out_channerl_nb);
        int count = 0;
        //������Ƶ�������� 16bit   44100  PCM����
        uint8_t *out_buffer = (uint8_t *) av_malloc(2 * 44100);
        FILE *fp_pcm = fopen(output, "wb");//������ļ�
		int aaa = 1;
        while (av_read_frame(pFormatCtx, packet) >= 0) {
    
            ret = avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);
            //LOGE("decode ing %d", count++);
            if (ret < 0) {
                LOGE("decode finish");
            }
            //����һ֡
            if (got_frame > 0) {
                /**
                 * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count,
                                    const uint8_t **in , int in_count);
                 */
                swr_convert(swrContext, &out_buffer, 2 * 44100,
                            (const uint8_t **) frame->data, frame->nb_samples);
                /**
                 * int av_samples_get_buffer_size(int *linesize, int nb_channels, int nb_samples,
                                   enum AVSampleFormat sample_fmt, int align);
                 */
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channerl_nb, frame->nb_samples,
                                                                 out_sample, 1);
				
				//LOGE("out_buffer_size %hu\n", out_buffer_size);												 
                fwrite(out_buffer, 1, out_buffer_size, fp_pcm);//������ļ�
				
            }
        }
        fclose(fp_pcm);
        av_frame_free(&frame);
        av_free(out_buffer);
        swr_free(&swrContext);
        avcodec_close(codecCtx);
        avformat_close_input(&pFormatCtx);
}



