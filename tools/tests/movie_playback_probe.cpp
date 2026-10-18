// Optional installed-asset smoke; assets stay outside the source tree.
#include "task/intro/Movie.h"
#include <chrono>
#include <cstdio>
#include <thread>

int main(int argc,char** argv) {
	if(argc<2 || argc>3) {std::fprintf(stderr,"usage: movie_playback_probe FILE [cancel]\n");return 2;}
	if(argc==3) {
		// Let the decoder fill its bounded queues, then destroy without consuming.
		for(int i=0;i<4;++i) {task::intro::Movie movie(argv[1],false);std::this_thread::sleep_for(std::chrono::milliseconds(200));}
		return 0;
	}
	task::intro::Movie movie(argv[1],true);
	task::intro::Movie::Frame frame;
	const auto begin=std::chrono::steady_clock::now();size_t presented=0;
	for(;;) {
		if(movie.Tick(frame)) {++presented;}
		const auto status=movie.GetStatus();
		if(status.finished || !status.error.empty()) {
			std::printf("finished=%d frames=%zu presented=%zu samples=%zu clock=%.3f audio_device=%d damaged_packets=%zu fallback=%s error=%s\n",status.finished,status.decoded_frames,presented,status.decoded_samples,status.clock,status.audio_device_opened,status.damaged_packets,status.audio_fallback.c_str(),status.error.c_str());
			return status.error.empty()?0:1;
		}
		if(std::chrono::steady_clock::now()-begin>std::chrono::minutes(10)) {return 3;}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}
