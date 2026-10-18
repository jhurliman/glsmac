#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace task::intro {

// Owns the demux/decode worker and movie audio device. Tick and destruction run
// on the caller thread; no engine/UI objects ever enter the worker or callback.
class Movie {
public:
	struct Frame { int width = 0, height = 0; double seconds = 0; std::vector<uint8_t> rgba; };
	struct Status { size_t decoded_frames = 0, decoded_samples = 0, damaged_packets = 0; double clock = 0; bool finished = false, audio_device_opened = false; std::string error, audio_fallback; };
	Movie( const std::string& path, bool sound );
	~Movie();
	Movie( const Movie& ) = delete;
	Movie& operator=( const Movie& ) = delete;
	// Returns the newest due frame, dropping superseded frames after a slow tick.
	bool Tick( Frame& frame );
	Status GetStatus() const;
private:
	struct Impl;
	std::unique_ptr<Impl> m;
};

}
