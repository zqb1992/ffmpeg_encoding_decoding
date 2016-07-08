//视频编码程序    

#include <stdio.h>
#define __STDC_CONSTANT_MACROS     //定义宏，用于c++

extern "C"
{
	#include "libavcodec/avcodec.h"
	#include "libavformat/avformat.h"
	#include "libswscale/swscale.h"
}


int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index)
{
	int ret;
	int got_frame;
	AVPacket enc_pkt;
	if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;
	while (1) {
		//printf("Flushing stream #%u encoder\n", stream_index);
		//ret = encode_write_frame(NULL, stream_index, &got_frame);
		enc_pkt.data = NULL;
		enc_pkt.size = 0;
		av_init_packet(&enc_pkt);
		ret = avcodec_encode_video2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,
			NULL, &got_frame);
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_frame)
		{ret=0;break;}
		printf("编码成功1帧！\n");
		/* mux encoded frame */
		ret = av_write_frame(fmt_ctx, &enc_pkt);
		if (ret < 0)
			break;
	}
	return ret;
}

AVCodecContext* setCodecContext(AVStream* video_st,AVOutputFormat* fmt,int in_w,int in_h)
{
	AVCodecContext* pCodecCtx;    //编码器上下文结构体，保存了视频（音频）编解码相关信息
	//此处是设置编码参数
	pCodecCtx = video_st->codec;                     //设置编码器环境
	pCodecCtx->codec_id = fmt->video_codec;          //设置编码器类型
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;      //编码器编码的数据类型
	
	//像素的格式，也就是说采用什么样的色彩空间来表明一个像素点 
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;            //设置像素格式
	
	//编码目标的视频帧大小，以像素为单位
	pCodecCtx->width = in_w;                         
	pCodecCtx->height = in_h;       

	//每秒25帧
	pCodecCtx->time_base.num = 1;                    
	pCodecCtx->time_base.den = 25;                   

	//平均码率，即采样的码率；显然，采样码率越大，视频大小越大
	pCodecCtx->bit_rate = 8000000; 
	//pCodecCtx->rc_max_rate = 400000; //max bit rate
	//pCodecCtx->rc_min_rate = 400000; //min bit rate
	
	//每250帧插入1个I帧，I帧越少，视频越小 
	pCodecCtx->gop_size=50;    //关键帧的最大间隔帧数
	pCodecCtx->keyint_min=10;  //关键帧的最小间隔帧数

	//B和P帧向前预测参考的帧数。取值范围1-16
	//pCodecCtx->refs=1;
	
	//H264
	pCodecCtx->me_range = 16;
	pCodecCtx->max_qdiff = 4;
	 
	//最大和最小量化系数  
	pCodecCtx->qmin = 10;     //最小的量化因子。取值范围1-51。建议在10-30之间。 
	pCodecCtx->qmax = 51;     //最大的量化因子。取值范围1-51。建议在10-30之间
	//因为我们的量化系数q是在qmin和qmax之间浮动的，  
	//qblur表示这种浮动变化的变化程度，取值范围0.0～1.0，取0表示不削减  
	pCodecCtx->qblur = 0.0;  

	//pCodecCtx->qcompress = 0.6;

	//两个非B帧之间允许出现多少个B帧数  
	//设置0表示不使用B帧  
	//b 帧越多，图片越小  
	//pCodecCtx->max_b_frames = 2;
	
	//b帧的生成策略，如果为true，则自动决定什么时候需要插入B帧，最高达到设置的最大B帧数；如果设置为false,那么最大的B帧数被使用。
	//pCodecCtx->b_frame_strategy = 0;  

	//运动估计  
	//pCodecCtx->pre_me = 2;  
  
	//设置最小和最大拉格朗日乘数  
	//拉格朗日乘数 是统计学用来检测瞬间平均值的一种方法  
	//pCodecCtx->lmin = 1;  
	//pCodecCtx->lmax = 5;  
    

  
	//空间复杂度的masking力度，取值范围 0.0-1.0  
	//pCodecCtx->spatial_cplx_masking = 0.0;  
  
	//运动场景预判功能的力度，数值越大编码时间越长  
	//pCodecCtx->me_pre_cmp = 0;  
  
	//采用（qmin/qmax的比值来控制码率，1表示局部采用此方法，）  
	//pCodecCtx->rc_qsquish = 1;  
  
	//设置 i帧、p帧与B帧之间的量化系数q比例因子，这个值越大，B帧越不清楚  
	//B帧量化系数 = 前一个P帧的量化系数q * b_quant_factor + b_quant_offset  
	//pCodecCtx->b_quant_factor = 1.25;  
  
	//i帧、p帧与B帧的量化系数便宜量，便宜越大，B帧越不清楚  
	//pCodecCtx->b_quant_offset = 1.25;  
  
	//p和i的量化系数比例因子，越接近1，P帧越清楚  
	//p的量化系数 = I帧的量化系数 * i_quant_factor + i_quant_offset  
	//pCodecCtx->i_quant_factor = 0.8;  
	//pCodecCtx->i_quant_offset = 0.0;  
  
	//码率控制测率，宏定义，查API  
	//pCodecCtx->rc_strategy = 2;  

	//DCT变换算法的设置，有7种设置，这个算法的设置是根据不同的CPU指令集来优化的取值范围在0-7之间  
	//pCodecCtx->dct_algo = 0;  
  
	//这两个参数表示对过亮或过暗的场景作masking的力度，0表示不作  
	//pCodecCtx->lumi_masking = 0.0;  
	//pCodecCtx->dark_masking = 0.0;  

	return pCodecCtx;
}



//@ in_filename  输入文件名
//@ out_filename 输出文件名
//@ in_w         图像的宽
//@ in_h         图像的高	
int coding(const char* in_filename,const char* out_filename,int in_w,int in_h)
{
	AVFormatContext* pFormatCtx;    //封装格式上下文信息，统领全局结构体，保存了视频文件封装格式相关信息
	AVOutputFormat* fmt;            //数据输出格式

	FILE *in_file = fopen(in_filename, "rb");	//视频YUV源文件 
	av_register_all();         //注册所有组件

	//推测编码方式
	//方法1.组合使用几个函数
	//pFormatCtx = avformat_alloc_context();
	//fmt = av_guess_format(NULL, out_filename, NULL);
	//方法2
	//自动申请一块AVFormatContext内存，并根据输出文件后缀，判断输出封装格式，
	avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_filename);  
	fmt = pFormatCtx->oformat; 
	printf("%s\n",fmt->long_name);
	
	//创建和初始化AVIOContext上下文环境，用于访问out_file文件
	//读写模式时，out_file只能被写操作
	//AVIOContext是FFMPEG管理输入输出数据的结构体
	if (avio_open(&pFormatCtx->pb,out_filename, AVIO_FLAG_READ_WRITE) < 0)
	{
		printf("输出文件打开失败");
		return -1;
	}

	//创建输出码流的AVStream
	AVStream* video_st = avformat_new_stream(pFormatCtx, 0);    
	if (video_st==NULL)
	{
		return -1;
	}

	AVCodecContext* pCodecCtx = setCodecContext(video_st,fmt,in_w,in_h);    //编码器上下文结构体，保存了视频（音频）编解码相关信息
	
	//输出格式信息
	//av_dump_format(pFormatCtx, 0, out_file, 1);     //输出文件封装的详细信息
	
	//AVCodec* pCodec;              
	//查找匹配的编码器，如果有，返回编码器
	//每种视频（音频）编解码器(例如H.264解码器)对应一个该结构体 AV_CODEC_ID_H264
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	//printf("编码器名称：%s\n",pCodec->name);
	if (!pCodec)
	{
		printf("没有找到合适的编码器！\n");
		return -1;
	}	
	if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0)  //打开解码器，返回0成功，负数失败
	{
		printf("编码器打开失败！\n");
		return -1;
	}

	//存储一帧解码后像素数据
	AVFrame* picture = av_frame_alloc();      //开辟AVFrame结构体内存，使用av_frame_free()释放内存

	//按输出格式计算一幅图片的大小
	int size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	
	//开辟一帧数据的大小，按照设置的格式宽度和高度
	//建立图片基于具体的图像数据
	uint8_t* picture_buf = (uint8_t *)av_malloc(size);
	avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	//写文件头
	avformat_write_header(pFormatCtx,NULL);

	AVPacket pkt;      //存储一帧压缩后的数据
	int y_size = pCodecCtx->width * pCodecCtx->height;
	av_new_packet(&pkt,y_size*2);

	int i=0;
	while(fread(picture_buf, 1, y_size*3/2, in_file)){   //读入1.5倍数据

		picture->data[0] = picture_buf;          // 亮度Y
		picture->data[1] = picture_buf+ y_size;  // U 
		picture->data[2] = picture_buf+ y_size*5/4; // V
		
		
		//显示时间戳PTS
		picture->pts=i++;

		int got_picture=0;

		//编码
		int ret = avcodec_encode_video2(pCodecCtx, &pkt,picture, &got_picture); //编码一帧压缩数据，返回字节数
		if(ret < 0)
		{
			printf("编码错误！\n");
			return -1;
		}

		if (got_picture==1)
		{
			printf("编码成功第%d帧！\n",i-1);
			pkt.stream_index = video_st->index;
			ret = av_write_frame(pFormatCtx, &pkt);    //将包中数据写入到文件中
			av_free_packet(&pkt);                      //释放一个包
		}
	}


	//Flush Encoder
	int ret = flush_encoder(pFormatCtx,0);
	if (ret < 0) {
		printf("Flushing encoder failed\n");
		return -1;
	}

	//写文件尾
	av_write_trailer(pFormatCtx);

	//清理
	if (video_st)
	{
		avcodec_close(video_st->codec);
		av_free(picture);
		av_free(picture_buf);    
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	fclose(in_file);

	return 0;
}


