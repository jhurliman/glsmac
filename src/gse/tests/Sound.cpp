#include "Sound.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <vector>

#include "task/gsetests/GSETests.h"
#include "scene/actor/Sound.h"
#include "types/Sound.h"

namespace gse::tests {
namespace {
// Production callbacks use malloc-backed buffers, which ptr() also validates
// when the focused suite is run with --memorydebug.
struct CallbackBuffer {
	uint8_t* bytes = static_cast<uint8_t*>( malloc( 8 ) );
	~CallbackBuffer() { free( bytes ); }
	uint8_t* data() { return bytes; }
	size_t size() const { return 8; }
	uint8_t* begin() { return bytes; }
	uint8_t* end() { return bytes + size(); }
	uint8_t operator[]( size_t i ) const { return bytes[i]; }
	void fill( uint8_t value ) { std::fill( begin(), end(), value ); }
};
void SetSamples( types::Sound& sound, const std::vector<uint8_t>& samples ) {
	sound.m_buffer_size = samples.size();
	if ( !samples.empty() ) {
		sound.m_buffer = static_cast<unsigned char*>( malloc( samples.size() ) );
		std::memcpy( sound.m_buffer, samples.data(), samples.size() );
	}
}
}

void AddSoundTests( task::gsetests::GSETests* task ) {
	task->AddTest( "Sound repeating callbacks preserve every PCM byte across EOF", GT() {
		for ( bool autoplay : { false, true } ) {
			for ( size_t length : { 18u, 10u, 8u, 2u } ) {
				types::Sound pcm;
				std::vector<uint8_t> original( length );
				for ( size_t i = 0; i < length; ++i ) original[i] = static_cast<uint8_t>( i + 1 );
				SetSamples( pcm, original );
				scene::actor::Sound actor( "loop fixture", &pcm );
				actor.SetRepeatable( true );
				actor.SetAutoPlay( autoplay );
				actor.Play();
				CallbackBuffer output;
				for ( size_t callback = 0; callback < 9; ++callback ) {
					actor.GetNextBuffer( output.data(), output.size() );
					for ( size_t i = 0; i < output.size(); ++i ) {
						GT_ASSERT( output[i] == original[(callback * output.size() + i) % length], "loop inserted silence or skipped PCM bytes" );
					}
					GT_ASSERT( actor.IsActive() && !actor.IsFinished(), "repeatable actor stopped at EOF" );
					GT_ASSERT( actor.GetPos() == ((callback + 1) * output.size()) % length, "loop position is not the next PCM byte" );
				}
			}
		}
		GT_OK();
	} );
	task->AddTest( "Sound one shot callbacks pad only the tail and finish at EOF", GT() {
		for ( size_t length : { 2u, 8u, 10u, 18u } ) {
			types::Sound pcm;
			std::vector<uint8_t> original( length, 71 );
			SetSamples( pcm, original );
			scene::actor::Sound actor( "one shot fixture", &pcm );
			actor.Play();
			CallbackBuffer output;
			for ( size_t callback = 0; callback < 4; ++callback ) {
				output.fill( 99 );
				actor.GetNextBuffer( output.data(), output.size() );
				for ( size_t i = 0; i < output.size(); ++i ) {
					GT_ASSERT( output[i] == (callback * output.size() + i < length ? 71 : 0), "one shot tail or later callback is not silent" );
				}
				GT_ASSERT( actor.GetPos() == std::min( (callback + 1) * output.size(), length ), "one shot position escaped PCM buffer" );
				GT_ASSERT( actor.IsFinished() == ((callback + 1) * output.size() >= length), "EOF completion is delayed or premature" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "Sound muted paused empty and stopped callbacks preserve lifecycle", GT() {
		types::Sound pcm;
		SetSamples( pcm, { 1,2,3,4,5,6,7,8,9,10 } );
		scene::actor::Sound actor( "lifecycle fixture", &pcm );
		actor.SetRepeatable( true );
		actor.Play();
		actor.Mute();
		CallbackBuffer output;
		actor.GetNextBuffer( output.data(), output.size() );
		actor.GetNextBuffer( output.data(), output.size() );
		GT_ASSERT( actor.GetPos() == 6 && actor.IsActive(), "muted playback failed to advance across EOF" );
		GT_ASSERT( std::all_of(output.begin(), output.end(), [](uint8_t b){ return b == 0; }), "muted playback emitted PCM" );
		actor.Pause();
		output.fill( 99 );
		actor.GetNextBuffer( output.data(), output.size() );
		GT_ASSERT( actor.GetPos() == 6 && !actor.IsActive(), "paused playback advanced" );
		GT_ASSERT( std::all_of(output.begin(), output.end(), [](uint8_t b){ return b == 0; }), "paused playback emitted PCM" );
		actor.Play();
		actor.GetNextBuffer( output.data(), output.size() );
		GT_ASSERT( actor.GetPos() == 4 && actor.IsActive(), "resumed playback lost loop position" );
		actor.SetRepeatable( false );
		actor.Stop();
		output.fill( 99 );
		actor.GetNextBuffer( output.data(), output.size() );
		GT_ASSERT( actor.IsFinished() && !actor.IsActive() && actor.GetPos() == 0, "explicit stop resumed playback" );
		GT_ASSERT( std::all_of(output.begin(), output.end(), [](uint8_t b){ return b == 0; }), "stopped playback emitted PCM" );
		types::Sound empty;
		scene::actor::Sound empty_actor( "empty fixture", &empty );
		empty_actor.SetRepeatable( true );
		empty_actor.Play();
		output.fill( 99 );
		empty_actor.GetNextBuffer( output.data(), output.size() );
		GT_ASSERT( std::all_of(output.begin(), output.end(), [](uint8_t b){ return b == 0; }), "empty sound emitted PCM" );
		GT_ASSERT( empty_actor.GetPos() == 0, "empty sound advanced" );
		GT_OK();
	} );
}
}
