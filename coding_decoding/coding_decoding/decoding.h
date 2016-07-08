// ffmpegdecodec.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#define __STDC_CONSTANT_MACROS     //定义宏，用于c++
extern "C"
{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
}
int decode_write_frame(FILE *fp_YUV, AVCodecContext *pCodecCtx,struct SwsContext *img_convert_ctx,
                    AVFrame *pFrameYUV,AVFrame *pFrame,int frame_cnt, AVPacket *packet, int size_picture)
{
	int got_picture;
	int	ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
	if (ret < 0) {
		printf("Decode Error.(解码错误)\n");
		return ret;
	}
	if (!got_picture)
		return 0;
	if (got_picture) {
		printf("Flush Decoder: Succeed to decode %d frame!\n",frame_cnt);
		sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, 
			pFrameYUV->data, pFrameYUV->linesize);

		fwrite(pFrameYUV->data[0],1,size_picture,fp_YUV);     //Y
		fwrite(pFrameYUV->data[1],1,size_picture/4,fp_YUV);   //U
		fwrite(pFrameYUV->data[2],1,size_picture/4,fp_YUV);   //V
	}
	//av_free_packet(packet);
	return got_picture;
}

//@ in_filename 输入文件名
//@ out_filename 输出文件名
int decoding(const char *in_filename,const char *out_filename)
{	
	av_register_all();           //注册所有组件
	avformat_network_init();   //初始化网络模块，不是必须的

	//封装格式上下文信息，统领全局结构体，保存了视频文件封装格式相关信息。
	AVFormatContext * pFormatCtx = avformat_alloc_context(); //开辟一块AVFormatContext结构体内存，使用avformat_free_context()释放内存
	if(avformat_open_input(&pFormatCtx,in_filename,NULL,NULL)!=0){  //打开输入流文件，最后使用avformat_close_input()关闭，返回0成功
		printf("Couldn't open input stream.\n");      
		return -1;
	}
	if(avformat_find_stream_info(pFormatCtx,NULL)<0){  //读取media文件包获取流文件，用于没有头文件的文件格式，成功返回大于0
		printf("Couldn't find stream information.\n");
		return -1;
	}

	//此处是判断是不是视频文件0,1分别表示视频和音频的索引
	int videoindex=-1;
	//printf("%d\n",pFormatCtx->nb_streams);

	for(int i=0; i<pFormatCtx->nb_streams; i++) 
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
			videoindex=i;
			break;
		}
	//printf("%d\n",videoindex);      //输出视频流序号

	if(videoindex==-1){
		printf("Didn't find a video stream.\n");
		return -1;
	}

	//查找匹配的解码器，如果有，返回解码器
	//编码器上下文结构体，保存了视频（音频）编解码相关信息。
	AVCodecContext *pCodecCtx=pFormatCtx->streams[videoindex]->codec;
	
	//每种视频（音频）编解码器(例如H.264解码器)对应一个该结构体。
	AVCodec *pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	//pCodec=avcodec_find_decoder_by_name("h264");   //按照解码器名称查找
	if(pCodec==NULL){
		printf("Codec not found.\n");
		return -1;
	}
	printf("%s\n",pCodec->name);      //输出解码器类型

	//打开解码器，返回0成功，负数失败
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}
	
	/*
	 * 在此处添加输出视频信息的代码
	 * 取自于pFormatCtx，使用fprintf()
	 */
	FILE *fp=fopen("xx.txt","wb+");
	fprintf(fp,"视频时长：%d\n",pFormatCtx->duration);                //输出视频长度，单位；微秒
	fprintf(fp,"封装格式：%s\n",pFormatCtx->iformat->long_name);      //输出视频封装格式
	fprintf(fp,"分辨率：%d*%d\n",pFormatCtx->streams[videoindex]->codec->width,pFormatCtx->streams[videoindex]->codec->height);      //输出视频的分辨率
	fprintf(fp,"帧率：%d\n",pFormatCtx->streams[videoindex]->r_frame_rate);      //输出视频的分辨率
	fclose(fp);

	//存储一帧解码后像素数据
	AVFrame *pFrame=av_frame_alloc();       //开辟AVFrame结构体内存，使用av_frame_free()释放内存
	AVFrame *pFrameYUV=av_frame_alloc();

	//开辟一帧数据的大小，按照设置的格式宽度和高度
	//建立图片基于具体的图像数据
	uint8_t *out_buffer=(uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));	
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);
	
	//开辟存储一帧压缩编码数据的内存
	//存储一帧压缩编码数据
	AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	////Output Info-----------------------------
	//printf("--------------- File Information ----------------\n");
	//av_dump_format(pFormatCtx,0,in_filename,0);                       //输出输入文件封装的详细信息
	//printf("-------------------------------------------------\n");
	
	//开辟和返回一个SwsContext，初始化一个SwsContext。包含在类库libswscale的源代码，用于图像处理，使用sws_freeContext()释放
	//用于处理图片像素数据的类库。可以完成图片像素格式的转换，图片的拉伸等工作
	struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, 
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 

	int frame_cnt=0;
	int size_picture=pCodecCtx->width*pCodecCtx->height;
	FILE *fp_YUV=fopen(out_filename,"wb+");

	while(av_read_frame(pFormatCtx, packet)>=0){      //从输入文件读入一帧压缩编码数据，返回0正确
		if(packet->stream_index==videoindex){
		/*
			* 在此处添加输出H264码流的代码
			* 取自于packet，使用fwrite()
			*/
			decode_write_frame(fp_YUV, pCodecCtx,img_convert_ctx,
							 pFrameYUV,pFrame,frame_cnt,packet,size_picture);    //解码并写数据
			frame_cnt++;
		}
		av_free_packet(packet);
	}

	//Flush Decoder
	packet->data = NULL;
    packet->size = 0;
	int flag=1;
	while(flag){
		flag=decode_write_frame(fp_YUV, pCodecCtx,img_convert_ctx,
							 pFrameYUV,pFrame,frame_cnt,packet,size_picture);
		av_free_packet(packet);
	}

	fclose(fp_YUV);    //关闭文件

	//free
	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrame);          
	av_frame_free(&pFrameYUV);
	avcodec_close(pCodecCtx);      //关闭解码器

	avformat_free_context(pFormatCtx);
 	return 0;
}


