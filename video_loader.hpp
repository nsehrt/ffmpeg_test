#pragma once
#include <array>
#include <memory>
#include <string>
#include <thread>

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libswscale/swscale.h>
	#include <libavformat/avformat.h>
	#include <libavdevice/avdevice.h>
}


enum class VideoLoaderResult
{
	FailContextInit,
	FailOpenFile,
	FailNoVideoStream,
	FailOpenCodec,
	Success
};


class VideoLoader
{

public:

	VideoLoader()=default;

	~VideoLoader();

	VideoLoaderResult init(const std::string& file);

	void start();

	void stop();

	void nextFrame();

	bool isReady() const;

	[[nodiscard]] std::uint8_t* getCurrentFrame() const;
	[[nodiscard]] int getVideoWidth() const;
	[[nodiscard]] int getVideoHeight() const;

	

private:

	void work();

	std::string fileName{};

	int videoWidth{};
	int videoHeight{};

	AVFormatContext* avFormatContext = nullptr;
	AVCodecContext* avCodecContext = nullptr;
	SwsContext* swsContext = nullptr;
	int streamIndex = -1;

	AVFrame* avFrame = nullptr;
	AVPacket* avPacket = nullptr;

	std::thread workThread{};

	static constexpr int FRAME_BUFFER_SIZE = 5;
	bool initBuffer = false;

	std::array<std::unique_ptr<std::uint8_t[]>, FRAME_BUFFER_SIZE> frameBuffer;
	std::atomic_int currentFrame = -1;
	std::atomic_int workingFrame = 0;
	std::atomic_bool stopThread = false;
	std::atomic_bool ready = false;
	
};