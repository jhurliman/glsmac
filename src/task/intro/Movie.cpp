#include "Movie.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <SDL.h>

#ifdef GLSMAC_HAVE_MOVIES
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#endif

namespace task::intro {

struct Movie::Impl {
	std::mutex mutex;
	std::condition_variable changed;
	std::thread worker;
	std::deque<Frame> video;
	std::deque<std::vector<uint8_t>> audio;
	size_t audio_bytes = 0;
	bool stop = false, eof = false, started = false, audio_started = false;
	Status status;
	SDL_AudioDeviceID device = 0;
	bool initialized_audio = false;
	uint64_t submitted = 0;
	double last_video_end = 0;
	std::chrono::steady_clock::time_point wall_start;
	static constexpr int RATE = 22050, CHANNELS = 2, BYTES_PER_SECOND = RATE * CHANNELS * 2;
	static constexpr size_t MAX_VIDEO = 16, MAX_AUDIO = BYTES_PER_SECOND * 2;

	~Impl() {
		{ std::lock_guard<std::mutex> lock(mutex); stop = true; }
		changed.notify_all();
		if (worker.joinable()) { worker.join(); }
		if (device) { SDL_ClearQueuedAudio(device); SDL_CloseAudioDevice(device); }
		if (initialized_audio) {SDL_QuitSubSystem(SDL_INIT_AUDIO);}
	}

#ifdef GLSMAC_HAVE_MOVIES
	void Decode( const std::string& path ) {
		AVFormatContext* format = avformat_alloc_context();
		if(format) {
			format->interrupt_callback = { [](void* opaque) -> int {
				auto* self=static_cast<Impl*>(opaque);
				std::lock_guard<std::mutex> lock(self->mutex);
				return self->stop ? 1 : 0;
			}, this };
		}
		AVCodecContext* v = nullptr;
		AVCodecContext* a = nullptr;
		AVPacket* packet = av_packet_alloc();
		AVFrame* frame = av_frame_alloc();
		SwsContext* scaler = nullptr;
		try {
			if (!format || !packet || !frame || avformat_open_input(&format,path.c_str(),nullptr,nullptr)<0 || avformat_find_stream_info(format,nullptr)<0) { throw std::runtime_error("Cannot open movie"); }
			// The supported original assets use EA's WVE demuxer, TQI video and
			// packed S16 EA ADPCM. Reject unexpected files before allocating frames.
			if (std::string(format->iformat->name) != "ea") { throw std::runtime_error("Unsupported movie container (expected EA WVE)"); }
			const int vi = av_find_best_stream(format,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
			const int ai = av_find_best_stream(format,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
			if (vi<0 || format->streams[vi]->codecpar->codec_id != AV_CODEC_ID_TQI) { throw std::runtime_error("Unsupported movie video codec (expected EA TQI)"); }
			const auto open = [&](int i) {
				auto* codec = avcodec_find_decoder(format->streams[i]->codecpar->codec_id);
				auto* ctx = avcodec_alloc_context3(codec);
				if (!ctx || avcodec_parameters_to_context(ctx,format->streams[i]->codecpar)<0 || avcodec_open2(ctx,codec,nullptr)<0) { avcodec_free_context(&ctx); throw std::runtime_error("Movie decoder unavailable"); }
				return ctx;
			};
			v = open(vi);
			if (v->width<=0 || v->height<=0 || v->width>2048 || v->height>2048) { throw std::runtime_error("Movie dimensions exceed playback bounds"); }
			if (ai>=0) {
				if (format->streams[ai]->codecpar->codec_id != AV_CODEC_ID_ADPCM_EA) { throw std::runtime_error("Unsupported movie audio codec (expected EA ADPCM)"); }
				a = open(ai);
				if (a->sample_rate != RATE || a->sample_fmt != AV_SAMPLE_FMT_S16
#if LIBAVUTIL_VERSION_MAJOR >= 57
					|| a->ch_layout.nb_channels != CHANNELS
#else
					|| a->channels != CHANNELS
#endif
				) { throw std::runtime_error("Unsupported movie audio format (expected 22050 Hz stereo S16)"); }
			}
			const double fps = av_q2d(av_guess_frame_rate(format,format->streams[vi],nullptr));
			if (!(fps>0 && fps<=120)) { throw std::runtime_error("Invalid movie frame rate"); }
			size_t video_index = 0;
			const auto damaged = [&] {
				std::lock_guard<std::mutex> lock(mutex);
				// The installed logo has a malformed trailing EA ADPCM block,
				// also rejected by FFmpeg 8. Continue past bounded packet damage.
				if (++status.damaged_packets>32) {throw std::runtime_error("Too many damaged movie packets");}
			};
			const auto receive = [&](AVCodecContext* codec, bool is_video) {
				for (;;) {
					const int result = avcodec_receive_frame(codec,frame);
					if (result==AVERROR(EAGAIN) || result==AVERROR_EOF) { break; }
					if (result<0) { damaged(); break; }
					if (is_video) {
						if (frame->width != v->width || frame->height != v->height) { throw std::runtime_error("Changing movie dimensions unsupported"); }
						Frame out; out.width=frame->width; out.height=frame->height;
						out.seconds = video_index++ / fps;
						out.rgba.resize(static_cast<size_t>(out.width)*out.height*4);
						scaler=sws_getCachedContext(scaler,out.width,out.height,static_cast<AVPixelFormat>(frame->format),out.width,out.height,AV_PIX_FMT_RGBA,SWS_BILINEAR,nullptr,nullptr,nullptr);
						if (!scaler) { throw std::runtime_error("Movie color conversion unavailable"); }
						uint8_t* dest[]={out.rgba.data()}; int stride[]={out.width*4};
						if (sws_scale(scaler,frame->data,frame->linesize,0,out.height,dest,stride)!=out.height) { throw std::runtime_error("Movie color conversion failed"); }
						std::unique_lock<std::mutex> lock(mutex);
						changed.wait(lock,[&]{return stop || video.size()<MAX_VIDEO;});
						if(stop) { av_frame_unref(frame); return; }
						last_video_end=out.seconds+1/fps; ++status.decoded_frames;
						video.push_back(std::move(out));
					} else {
						if (frame->format!=AV_SAMPLE_FMT_S16 || frame->nb_samples<0 || frame->nb_samples>RATE*2) { throw std::runtime_error("Invalid movie audio frame"); }
						const size_t size=static_cast<size_t>(frame->nb_samples)*CHANNELS*2;
						std::vector<uint8_t> out(size); std::memcpy(out.data(),frame->data[0],size);
						std::unique_lock<std::mutex> lock(mutex);
						changed.wait(lock,[&]{return stop || audio_bytes+size<=MAX_AUDIO;});
						if(stop) { av_frame_unref(frame); return; }
						audio_bytes+=size; status.decoded_samples+=frame->nb_samples;
						audio.push_back(std::move(out));
					}
					av_frame_unref(frame);
				}
			};
			for (;;) {
				{std::lock_guard<std::mutex> lock(mutex); if(stop) {break;}}
				const int result=av_read_frame(format,packet);
				if(result==AVERROR_EOF) {
					if(avcodec_send_packet(v,nullptr)>=0) {receive(v,true);}
					if(a && avcodec_send_packet(a,nullptr)>=0) {receive(a,false);}
					break;
				}
				if(result<0) {throw std::runtime_error("Cannot read movie packet");}
				auto* codec=packet->stream_index==vi?v:packet->stream_index==ai?a:nullptr;
				if(codec) {
					if(avcodec_send_packet(codec,packet)<0) {damaged();}
					else {receive(codec,codec==v);}
				}
				av_packet_unref(packet);
			}
		} catch(const std::exception& e) { std::lock_guard<std::mutex> lock(mutex); status.error=e.what(); }
		sws_freeContext(scaler); av_frame_free(&frame); av_packet_free(&packet);
		avcodec_free_context(&a); avcodec_free_context(&v); avformat_close_input(&format);
		{std::lock_guard<std::mutex> lock(mutex); eof=true;}
		changed.notify_all();
	}
#endif
};

Movie::Movie(const std::string& path,bool sound):m(new Impl) {
#ifdef GLSMAC_HAVE_MOVIES
	if(sound && (SDL_WasInit(SDL_INIT_AUDIO) || (m->initialized_audio=SDL_InitSubSystem(SDL_INIT_AUDIO)==0))) {
		SDL_AudioSpec spec{}; spec.freq=Impl::RATE; spec.format=AUDIO_S16SYS; spec.channels=Impl::CHANNELS; spec.samples=1024;
		m->device=SDL_OpenAudioDevice(nullptr,0,&spec,nullptr,0);
	}
	m->status.audio_device_opened=m->device!=0;
	if(sound && !m->device) {m->status.audio_fallback=SDL_GetError();}
	// Capture the worker-owned state, not Movie::m: libc++ clears the unique_ptr
	// before entering Impl's destructor, while its worker may not have started.
	auto* impl=m.get();
	m->worker=std::thread([impl,path]{impl->Decode(path);});
#else
	(void)path; (void)sound;
	m->eof=true; m->status.error="Movie playback was not enabled at build time";
#endif
}
Movie::~Movie() = default;

bool Movie::Tick(Frame& frame) {
	std::lock_guard<std::mutex> lock(m->mutex);
	if(!m->started && !m->video.empty() && (!m->audio.empty() || m->eof || m->video.size()>=Impl::MAX_VIDEO)) {
		m->started=true; m->wall_start=std::chrono::steady_clock::now();
	}
	if(!m->started) {m->status.finished=m->eof && m->video.empty(); return false;}
	if(m->device) {
		while(!m->audio.empty() && SDL_GetQueuedAudioSize(m->device)<Impl::BYTES_PER_SECOND) {
			const auto& bytes=m->audio.front();
			if(SDL_QueueAudio(m->device,bytes.data(),static_cast<Uint32>(bytes.size()))<0) {
				m->status.audio_fallback=SDL_GetError();
				SDL_CloseAudioDevice(m->device); m->device=0;
				// Retain elapsed playback time when falling back to silent output.
				m->wall_start=std::chrono::steady_clock::now()-std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(m->status.clock));
				break;
			}
			m->submitted+=bytes.size(); m->audio_bytes-=bytes.size(); m->audio.pop_front();
		}
		if(m->device && !m->audio_started && m->submitted) {SDL_PauseAudioDevice(m->device,0);m->audio_started=true;}
	}
	if(!m->device) {m->audio.clear();m->audio_bytes=0;}
	const double wall=std::chrono::duration<double>(std::chrono::steady_clock::now()-m->wall_start).count();
	if(m->device && m->audio_started) {
		m->status.clock=static_cast<double>(m->submitted-SDL_GetQueuedAudioSize(m->device))/Impl::BYTES_PER_SECOND;
		// A silent tail is legal. Once the decoder ends and audio drains, finish
		// the remaining video using a monotonic wall clock anchored to audio.
		if(m->eof && m->audio.empty() && SDL_GetQueuedAudioSize(m->device)==0) {
			SDL_CloseAudioDevice(m->device);m->device=0;
			m->wall_start=std::chrono::steady_clock::now()-std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(m->status.clock));
		}
	} else {m->status.clock=wall;}
	bool updated=false;
	while(!m->video.empty() && m->video.front().seconds<=m->status.clock) {frame=std::move(m->video.front());m->video.pop_front();updated=true;}
	m->status.finished=m->eof && m->video.empty() && m->audio.empty() && (!m->device || SDL_GetQueuedAudioSize(m->device)==0) && m->status.clock>=m->last_video_end;
	m->changed.notify_all();
	return updated;
}

Movie::Status Movie::GetStatus() const {std::lock_guard<std::mutex> lock(m->mutex);return m->status;}

}
