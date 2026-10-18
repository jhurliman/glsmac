#include "Movie.h"
#include "task/intro/Movie.h"
#include "task/gsetests/GSETests.h"
#include <chrono>
#include <thread>

namespace gse::tests {
void AddMovieTests(task::gsetests::GSETests* task) {
	task->AddTest("movie missing asset terminates safely and worker cancellation joins",GT() {
		for(int i=0;i<32;++i) {
			task::intro::Movie movie("/__glsmac_missing_movie__/opening.wve",false);
			if(i%2==0) {continue;} // Destroy while opening/decoding is still pending.
			task::intro::Movie::Frame frame;
			const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
			for(;;) {
				movie.Tick(frame);
				const auto status=movie.GetStatus();
				if(status.finished) {
					GT_ASSERT(!status.error.empty() && status.decoded_frames==0 && status.decoded_samples==0 && !status.audio_device_opened,"missing movie must produce no media and a diagnostic");
					break;
				}
				GT_ASSERT(std::chrono::steady_clock::now()<deadline,"missing movie worker did not finish");
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
		GT_OK();
	});
}
}
