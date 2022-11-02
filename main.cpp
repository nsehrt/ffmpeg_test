#define OLC_PGE_APPLICATION
#include <iostream>
#include "olcPixelGameEngine.h"
#include "video_loader.hpp"

constexpr float TIME60 = 1.0f/60.0f;

class VideoTest final : public olc::PixelGameEngine
{

public:

	explicit VideoTest(VideoLoader& vl) : videoLoader(vl)
	{
		sAppName = "ffmpeg test";
		timer = 0.0f;
	}


	bool OnUserCreate() override
	{
		return true;
	}


	bool OnUserUpdate(const float fElapsedTime) override
	{
		timer += fElapsedTime;

		if(timer >= TIME60)
		{
			timer -= TIME60;
			videoLoader.nextFrame();
		}

		int counter = 0;

		const auto data = videoLoader.getCurrentFrame();

		for(int y = 0; y < videoLoader.getVideoHeight(); y++)
		{
			for(int x = 0; x < videoLoader.getVideoWidth(); x++)
			{
				const auto p = olc::Pixel(data[counter], data[counter+1], data[counter+2]);
				counter += 4;
				Draw(x,y, p);
			}
		}

		

		return true;
	}


	VideoLoader& videoLoader;
	float timer;

};

int main()
{
	VideoLoader vid{};
	if(vid.init(R"(C:\Users\n_seh\source\repos\ffmpeg_test\test.mp4)") != VideoLoaderResult::Success)
	{
		std::cout << "ERROR\n";
		return -1;
	}

	vid.start();

	while(!vid.isReady()){}

	if(VideoTest handle{vid}; 
	   handle.Construct(vid.getVideoWidth(),vid.getVideoHeight(),1,1))
	{
		handle.Start();
	}
		
	vid.stop();

	return 0;

}


