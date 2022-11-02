#include "video_loader.hpp"

#include <iostream>

VideoLoader::~VideoLoader()
{
	sws_freeContext(swsContext);
	avformat_close_input(&avFormatContext);
	avformat_free_context(avFormatContext);
	av_packet_free(&avPacket);
	av_frame_free(&avFrame);
	avcodec_free_context(&avCodecContext);
}



VideoLoaderResult VideoLoader::init(const std::string& file)
{
	fileName = file;

	//open context and file
	avFormatContext = avformat_alloc_context();

	if(!avFormatContext)
	{
		return VideoLoaderResult::FailContextInit;
	}

	int c = 0;
	AVInputFormat* avInputFormat = nullptr;
	do
	{
		avInputFormat = av_input_video_device_next(avInputFormat);
		if(avInputFormat)
		{
			std::cout << avInputFormat->long_name << "\n";
		}
		if(c == 3) break;
		c++;
	}while(avInputFormat != nullptr);


	if(avformat_open_input(&avFormatContext, fileName.data(), nullptr, nullptr) != 0)
	//if(avformat_open_input(&avFormatContext, "desktop", avInputFormat, nullptr) != 0)
	{
		return VideoLoaderResult::FailOpenFile;
	}

	//find video stream
	const AVCodec* codec{};
	const AVCodecParameters* codecParams{};

	for(int i = 0; i < static_cast<int>(avFormatContext->nb_streams); i++)
	{
		const auto stream = avFormatContext->streams[i];
		const AVCodecParameters* p = stream->codecpar;

		if(const AVCodec* c = avcodec_find_decoder(p->codec_id); 
		   p->codec_type == AVMEDIA_TYPE_VIDEO && c)
		{
			streamIndex = i;
			codec = c;
			codecParams = p;
			break;
		}
	}

	if(!codec || streamIndex == -1)
	{
		return VideoLoaderResult::FailNoVideoStream;
	}

	avCodecContext = avcodec_alloc_context3(codec);

	if(!avCodecContext)
	{
		return VideoLoaderResult::FailContextInit;
	}

	if(avcodec_parameters_to_context(avCodecContext, codecParams) < 0)
	{
		return VideoLoaderResult::FailContextInit;
	}

	if(avcodec_open2(avCodecContext, codec, nullptr) < 0)
	{
		return VideoLoaderResult::FailOpenCodec;
	}

	avFrame = av_frame_alloc();
	avPacket = av_packet_alloc();

	return VideoLoaderResult::Success;
}



void VideoLoader::start()
{
	workThread = std::thread(&VideoLoader::work, this);
}



void VideoLoader::stop()
{
	stopThread = true;

	workThread.join();
}



void VideoLoader::nextFrame()
{
	currentFrame = (currentFrame + 1) % FRAME_BUFFER_SIZE;
}


bool VideoLoader::isReady() const
{
	return ready;
}


void VideoLoader::work()
{

	while(!stopThread)
	{

		while(int response = av_read_frame(avFormatContext, avPacket) >= 0 && !stopThread)
		{
			if(avPacket->stream_index != streamIndex)
				continue;

			response = avcodec_send_packet(avCodecContext, avPacket);
			if(response < 0)
			{
				continue;
			}

			response = avcodec_receive_frame(avCodecContext, avFrame);

			if(response == AVERROR(EAGAIN) || response == AVERROR_EOF)
			{
				continue;
			}

			av_packet_unref(avPacket);

			while(currentFrame == workingFrame){if (stopThread){return;}}

			if(!initBuffer)
			{
				currentFrame = 0;
				videoHeight = avFrame->height;
				videoWidth = avFrame->width;

				for(int i = 0; i < FRAME_BUFFER_SIZE; i++)
				{
					frameBuffer[i] = std::make_unique<std::uint8_t[]>(static_cast<size_t>(avFrame->width) * avFrame->height * 4);
				}
				initBuffer = true;
			}


			std::uint8_t* dest[4] = {frameBuffer[workingFrame].get(), nullptr, nullptr, nullptr};
			const int destLinesize[4] = {avFrame->width * 4, 0, 0, 0};

			if(!swsContext)
			{
				swsContext = sws_getContext(videoWidth,
											videoHeight, 
											avCodecContext->pix_fmt, 
											videoWidth, 
											videoHeight, 
											AV_PIX_FMT_RGB0,
											SWS_BILINEAR, 
											nullptr, 
											nullptr, 
											nullptr);

				if(!swsContext)
				{
				}
			}


			sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, avFrame->height, dest, destLinesize);
			av_frame_unref(avFrame);

			workingFrame = (workingFrame + 1) % FRAME_BUFFER_SIZE;
			ready = true;
		}

		avio_seek(avFormatContext->pb, 0, SEEK_SET);
		avformat_seek_file(avFormatContext, streamIndex, 0,0,avFormatContext->streams[streamIndex]->duration, 0);

	}


}



std::uint8_t* VideoLoader::getCurrentFrame() const
{
	return frameBuffer[currentFrame].get();
}



int VideoLoader::getVideoWidth() const
{
	return videoWidth;
}



int VideoLoader::getVideoHeight() const
{
	return videoHeight;
}