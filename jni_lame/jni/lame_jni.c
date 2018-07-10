#include <stdio.h>
#include "lame_jni.h"
#include <android/log.h>
#include <lame.h>
#define LOG_TAG "System.out"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

int flag = 0;

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


JNIEXPORT void JNICALL Java_com_research_amr2mp3_LameUtil_jniConvertmp3
  (JNIEnv * env, jclass cls , 
  jstring jwav, jstring jmp3, 
  jint inSamplerate, jint outSamplerate, jint numChannels, jint brate, jint quality, jint vbrModel){
	char* cwav = Jstring2CStr(env,jwav) ;
	char* cmp3 = Jstring2CStr(env,jmp3);

	//1.�� wav,MP3�ļ�
	FILE* fwav = fopen(cwav,"rb");
	FILE* fmp3 = fopen(cmp3,"wb");

	short int wav_buffer[8192*2];
	unsigned char mp3_buffer[8192];

	//1.��ʼ��lame�ı�����
	lame_t lame =  lame_init();
	
	//2.����lame mp3����Ĳ���
	if(inSamplerate >= 0){
		lame_set_in_samplerate(lame , inSamplerate);
	}
	if(outSamplerate >= 0){
		lame_set_out_samplerate(lame, outSamplerate);
	}
	if(numChannels >= 0){
		lame_set_num_channels(lame, numChannels);
	}
	if(brate >= 0){
		lame_set_brate(lame, brate);
	}
	if(quality >= 0){
		lame_set_quality(lame, quality);
	}
	if(vbrModel >= 0){
		switch (vbrModel) {
			case 0:
				lame_set_VBR(lame, vbr_default);
				break;
			case 1:
				lame_set_VBR(lame, vbr_off);
				break;
			case 2:
				lame_set_VBR(lame, vbr_abr);
				break;
			case 3:
				lame_set_VBR(lame, vbr_mtrh);
				break;
			default:
				break;
		}
	}

	
	
	
	lame_init_params(lame);
	//3.��ʼд��
	int read ; int write; //������˶��ٸ��� ��д�˶��ٴ�
	int total=0; // ��ǰ����wav�ļ���byte��Ŀ
	do{
		if(flag==404){
			return;
		}
		read = fread(wav_buffer,sizeof(short int)*2, 8192,fwav);
		total +=  read* sizeof(short int)*2;
		if(read!=0){

			write = lame_encode_buffer_interleaved(lame,wav_buffer,read,mp3_buffer,8192);
			//��ת�����mp3����д���ļ���
			fwrite(mp3_buffer,sizeof(unsigned char),write,fmp3);
		}
		if(read==0){
			lame_encode_flush(lame,mp3_buffer,8192);
		}

	}while(read!=0);
	lame_mp3_tags_fid(lame, fmp3);
	lame_close(lame);
	fclose(fwav);
	fclose(fmp3);
}

