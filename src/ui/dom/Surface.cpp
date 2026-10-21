#include "Surface.h"

#include <memory>

#include "types/mesh/Rectangle.h"
#include "scene/actor/Mesh.h"
#include "ui/geometry/Rectangle.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "loader/texture/SmacTurnFrames.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include "util/FS.h"
#include "loader/texture/SmacDialogFrame.h"
#include "ui/SmacDominanceRaster.h"
#include "gse/value/Array.h"
#include "gse/value/Object.h"
#include "gse/value/String.h"
#include "gse/value/Int.h"
#include "graphics/Graphics.h"
#include "types/texture/Texture.h"
#include "util/String.h"
#include "types/texture/Filter.h"
#include "ui/UI.h"
#include "gse/value/Float.h"
#include "gse/value/Bool.h"
#include "gse/value/Undefined.h"
#include "gse/callable/Native.h"
#include "task/intro/Movie.h"
#include "resource/ResourceManager.h"
#include "config/Config.h"
#include "audio/Audio.h"
#include "ui/dom/Container.h"
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace ui {
namespace dom {

Surface::Surface( DOM_ARGS_T )
	: Area( DOM_ARGS_PASS, tag ) {

	NEW( m_mesh, types::mesh::Rectangle );
	auto* g = m_geometry->AsRectangle();
	g->AddMesh( m_mesh );
	m_actor = new scene::actor::Mesh( "UI::Surface", m_mesh );
	Actor( m_actor );
	g->AddActor( m_actor );

    Method(GSE_CALL,"smacfactionportrait",NATIVE_CALL(this) {
        N_EXPECT_ARGS(1);N_GETVALUE(filename,0,String);
        // Same +10 modulo256 PCX rebasing, active common palette and raw255
        // (loaded index9) key as the original software indexed importer.
        const auto* atlas=g_engine->GetTextureLoader()->LoadCustomTexture(filename,types::texture::TF_SMAC_SOFTWARE_TERRAIN);
        if(atlas->GetWidth()!=1024 || atlas->GetHeight()!=768) {GSE_ERROR(gse::EC.LOADER_ERROR,"Faction portrait requires the original1024x768 sheet");}
        CreateTexture();m_background.texture->Resize(131,157);
        m_background.texture->AddFrom(atlas,types::texture::AM_DEFAULT,304,457,434,613);
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);
        m_geometry->AsRectangle()->SetStretched();
        return VALUE(gse::value::Undefined);
    }));
    Property(GSE_CALL,"smac_frame",gse::VT_ARRAY,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        std::vector<std::array<int,5>> calls;
        const auto& rows=static_cast<gse::value::Array*>(value)->value;
        if(rows.size()>128) {GSE_ERROR(gse::EC.UI_ERROR,"Too many dialog frame calls");}
        for(const auto* row:rows) {
            if(!row || row->type!=gse::VT_ARRAY || static_cast<const gse::value::Array*>(row)->value.size()!=5) {
                GSE_ERROR(gse::EC.UI_ERROR,"Dialog frame calls require [style,left,top,right,bottom]");
            }
            std::array<int,5> call{};size_t field=0;
            for(const auto* v:static_cast<const gse::value::Array*>(row)->value) {
                if(!v || v->type!=gse::VT_INT || static_cast<const gse::value::Int*>(v)->value<-4096 || static_cast<const gse::value::Int*>(v)->value>4096) {
                    GSE_ERROR(gse::EC.UI_ERROR,"Dialog frame fields require bounded integer pixels");
                }
                call[field++]=static_cast<const gse::value::Int*>(v)->value;
            }
            if(call[0]<0 || call[0]>=51) {GSE_ERROR(gse::EC.UI_ERROR,"Small dialog style must be0..50");}
            calls.push_back(call);
        }
        if(calls!=m_dialog_frames) {m_dialog_frames=std::move(calls);m_dialog_size={-1,-1};
            if(m_dialog_frames.empty()) {ClearTexture();} else {UpdateDialogFrames();}}
    },[this](GSE_CALLABLE){m_dialog_frames.clear();m_dialog_size={-1,-1};ClearTexture();});
    Property(GSE_CALL,"smac_frame_phase",gse::VT_ARRAY,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        const auto& fields=static_cast<gse::value::Array*>(value)->value;
        if(fields.size()!=2) {GSE_ERROR(gse::EC.UI_ERROR,"Dialog pattern phase requires [x,y]");}
        std::array<int,2> phase{};
        for(size_t i=0;i<2;++i) {
            if(!fields[i] || fields[i]->type!=gse::VT_INT || static_cast<const gse::value::Int*>(fields[i])->value<0 || static_cast<const gse::value::Int*>(fields[i])->value>4096) {
                GSE_ERROR(gse::EC.UI_ERROR,"Dialog pattern phase requires nonnegative bounded pixels");
            }
            phase[i]=static_cast<const gse::value::Int*>(fields[i])->value;
        }
        if(phase!=m_dialog_phase) {m_dialog_phase=phase;m_dialog_size={-1,-1};UpdateDialogFrames();}
    },[this](GSE_CALLABLE){m_dialog_phase={0,0};m_dialog_size={-1,-1};UpdateDialogFrames();});
    GeometryHandler(GH_ON_AREA_UPDATE,[this]{UpdateDialogFrames();});
	Method( GSE_CALL, "playmovie", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 1 ); N_GETVALUE( path, 0, String );
		StopMovie();
		const auto& resolved = g_engine->GetResourceManager()->TryGetCustomPath(path);
		if (resolved.empty()) { return VALUE(gse::value::Bool,,false); }
		g_engine->GetAudio()->SuspendForMovie();
		try { m_movie=std::make_unique<task::intro::Movie>(resolved,!g_engine->GetConfig()->HasLaunchFlag(config::Config::LF_NOSOUND)); }
		catch (...) {g_engine->GetAudio()->ResumeAfterMovie();throw;}
		return VALUE(gse::value::Bool,,true);
	} ) );
	Method( GSE_CALL, "tickmovie", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 0 );
		if (!m_movie) {return VALUE(gse::value::Bool,,false);}
		task::intro::Movie::Frame frame;
		if(m_movie->Tick(frame)) {
			PresentMovieFrame(frame.width,frame.height,frame.rgba);
		}
		const auto status=m_movie->GetStatus();
		if(!status.error.empty() || status.finished) {
			if(!status.error.empty()) {Log("Movie playback: "+status.error);}
			StopMovie();return VALUE(gse::value::Bool,,false);
		}
		return VALUE(gse::value::Bool,,true);
	} ) );
	Method( GSE_CALL, "stopmovie", NATIVE_CALL( this ) {
		N_EXPECT_ARGS( 0 );StopMovie();return VALUE(gse::value::Undefined);
	} ) );

    Method(GSE_CALL,"smacworkshoppreview",NATIVE_CALL(this) {
        N_EXPECT_ARGS(2);N_GETVALUE(armor,0,Int);N_GETVALUE(yaw,1,Float);
        if(armor<0 || armor>1 || !std::isfinite(yaw) || std::abs(yaw)>64.0) {throw std::invalid_argument("Unsupported Workshop preview draft");}
        if(!m_workshop_models[armor]) {
            const auto read=[](const char* name) {
                std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));
                return std::string(bytes.begin(),bytes.end());
            };
            auto renderer=loader::cvr::CvrScoutRenderer::LoadWorkshop(read("VI.cvr"),armor?read("vipta00.cvr"):std::string(),
                read("viptr00.cvr"),read("palette.pcx"),read("system.txt"));
            m_workshop_models[armor]=std::make_shared<const loader::cvr::CvrScoutRenderer>(std::move(renderer));
        }
        const float angle=static_cast<float>(yaw);uint32_t bits;std::memcpy(&bits,&angle,sizeof(bits));
        const auto image=m_workshop_models[armor]->RenderWorkshop(bits);
        CreateTexture();m_background.texture->Resize(256,256);
        std::memcpy(m_background.texture->GetBitmap(),image.rgba.data(),image.rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
        return VALUE(gse::value::Bool,,true);
    }));

    Method(GSE_CALL,"smacdominancegraph",NATIVE_CALL(this) {
        N_EXPECT_ARGS(1);N_GET(frame,0,Object);
        const auto field=[&](const char* name,gse::value_type_t type)->gse::Value* {
            const auto item=frame->value.find(name);
            if(item==frame->value.end() || !item->second || item->second->type!=type) {throw std::invalid_argument("Incomplete dominance graph frame");}
            return item->second;
        };
        if(static_cast<gse::value::String*>(field("kind",gse::VT_STRING))->value!="smac-native-dominance-v1") {throw std::invalid_argument("Unknown dominance graph frame");}
        const auto turn=static_cast<gse::value::Int*>(field("turn",gse::VT_INT))->value;
        const auto history=game::backend::faction::SmacDominanceHistory::Deserialize(static_cast<gse::value::String*>(field("history",gse::VT_STRING))->value);
        const auto& values=static_cast<gse::value::Array*>(field("scores",gse::VT_ARRAY))->value;
        const auto& names=static_cast<gse::value::Array*>(field("native_keys",gse::VT_ARRAY))->value;
        if(turn<0 || turn>=1000 || values.size()!=8 || names.size()!=8) {throw std::invalid_argument("Unsupported dominance graph frame size");}
        std::array<int32_t,8> scores{};std::array<std::string,8> keys{};
        for(size_t slot=0;slot<8;++slot) {
            if(!values[slot] || values[slot]->type!=gse::VT_INT || !names[slot] || names[slot]->type!=gse::VT_STRING) {throw std::invalid_argument("Invalid dominance graph slot");}
            const auto score=static_cast<gse::value::Int*>(values[slot])->value;
            if(score<INT32_MIN || score>INT32_MAX) {throw std::invalid_argument("Dominance score outside signed32");}
            scores[slot]=static_cast<int32_t>(score);keys[slot]=static_cast<gse::value::String*>(names[slot])->value;
        }
        if(!m_graph_palette) {m_graph_palette=g_engine->GetTextureLoader()->GetSmacGraphPalette();}
        if(!m_graph_palette) {throw std::runtime_error("Dominance graph palette unavailable");}
        const auto graph=game::backend::faction::SmacDominanceGraph::Build(static_cast<size_t>(turn),{501,76,721,162},scores,history,keys,m_graph_palette->normal,m_graph_palette->alternate);
        const auto rgba=ui::RasterSmacDominance(graph,*m_graph_palette);
        CreateTexture();m_background.texture->Resize(220,86);
        std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);
        m_geometry->AsRectangle()->SetStretched();return VALUE(gse::value::Bool,,true);
    }));

    Method( GSE_CALL, "smacturnframe", NATIVE_CALL( this ) {
        N_EXPECT_ARGS(1);N_GETVALUE(frame,0,Int);
        if(frame<0 || frame>19) {GSE_ERROR(gse::EC.UI_ERROR,"Turn Complete frame must be0..19");}
        if(!m_turn_frames) {m_turn_frames=g_engine->GetTextureLoader()->GetSmacTurnFrames();}
        if(!m_turn_frames) {GSE_ERROR(gse::EC.LOADER_ERROR,"Turn Complete source unavailable");}
        PresentTurnFrame(m_turn_frames->rgba[frame==19 ? 18 : frame]);
        return VALUE(gse::value::Bool,,true);
    } ) );

	Property(
		GSE_CALL, "opacity", gse::VT_FLOAT, VALUE( gse::value::Float, , 1.0f ), PF_NONE,
		[ this ]( GSE_CALLABLE, gse::Value* const v ) {
			m_actor->SetAlpha( ( (gse::value::Float*)v )->value );
		},
		[ this ]( GSE_CALLABLE ) {
			m_actor->SetAlpha( 1.0f );
		}
	);

	Property(
		GSE_CALL, "background", gse::VT_STRING, nullptr, PF_NONE,
		[ this ]( GSE_CALLABLE, gse::Value* const v ) {
			const auto& str = ( (gse::value::String*)v )->value;
			// Keep the previous owned texture alive until the actor uses its replacement.
			std::unique_ptr< types::texture::Texture > replaced_texture;
			std::unique_ptr< types::texture::Texture > generated_menu;
			if ( str.empty() ) {
				GSE_ERROR( gse::EC.INVALID_ASSIGNMENT, "Property 'background' expects color code or texture path" );
			}
			bool is_tiled = true;
			types::Color color = {};
			if ( TryParseColor( GSE_CALL, str, color ) ) {
				CreateTexture();
				m_background.texture->Resize( 1, 1 );
				m_background.texture->SetPixel( 0, 0, color );
				is_tiled = false;
			}
			else {
				// texture format examples:
				//   texture.pcx
				//   texture.pcx:filter1(arg1,arg2,...):filter2():...
				const auto params = util::String::Split( str, ':' );
				ASSERT( !params.empty(), "texture params empty" );
				const auto palette_flags = params.size() > 1 && params.at( 1 ) == "smacpalette()"
					? types::texture::TF_SMAC_PALETTE : params.size() > 1 && params.at( 1 ) == "smacsoftware()"
                    ? types::texture::TF_SMAC_SOFTWARE_TERRAIN : types::texture::TF_NONE;
				const bool is_smac_menu = params.size() > 1 && params.at( 1 ).find( "smacmenu(" ) == 0;
				types::texture::Texture* texture = nullptr;
				if ( is_smac_menu ) {
					const auto& operation = params.at( 1 );
					const auto arguments = util::String::Split( operation.substr( 9, operation.size() - 10 ), ',' );
					long int frame, rows;
					long int viewport_width = g_engine->GetGraphics()->GetViewportWidth(), viewport_height = g_engine->GetGraphics()->GetViewportHeight();
					if ( operation.back() != ')' || ( arguments.size() != 3 && arguments.size() != 5 ) ||
						!util::String::ParseInt( arguments.at( 1 ), frame ) || !util::String::ParseInt( arguments.at( 2 ), rows ) || frame < 0 || rows <= 0 ) {
						GSE_ERROR( gse::EC.UI_ERROR, "smacmenu expects background filename, nonnegative frame and positive row count" );
					}
					if ( arguments.size() == 5 && ( !util::String::ParseInt( arguments.at( 3 ), viewport_width ) ||
						!util::String::ParseInt( arguments.at( 4 ), viewport_height ) || viewport_width <= 0 || viewport_height <= 0 ) ) {
						GSE_ERROR( gse::EC.UI_ERROR, "smacmenu viewport dimensions must be positive integers" );
					}
					generated_menu.reset( g_engine->GetTextureLoader()->CreateSmacMenuTexture( params.front(), arguments.at( 0 ), frame, rows, viewport_width, viewport_height ) );
					texture = generated_menu.get();
				}
				else {
					texture = g_engine->GetTextureLoader()->TryLoadCustomTexture( params.front(), palette_flags );
				}
				if ( !texture ) {
					GSE_ERROR( gse::EC.LOADER_ERROR, "Could not load texture: " + params.front() );
				}
				types::texture::Filter filter( texture );
				std::vector< std::string > args = {};
				for ( auto it = params.begin() + 1 ; it != params.end() ; it++ ) {
					const auto brackets_begin = it->find( '(' );
					args = {};
					std::string f;
					if ( brackets_begin != std::string::npos ) {
						const auto brackets_end = it->rfind( ')' );
						if ( brackets_end == std::string::npos || brackets_end <= brackets_begin ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Texture filter syntax error (unclosed bracket): " + *it );
						}
						args = util::String::Split( it->substr( brackets_begin + 1, brackets_end - brackets_begin - 1 ), ',' );
						f = it->substr( 0, brackets_begin );
					}
					else {
						f = *it;
					}

					const auto f_expect_args_fromto = [ &ctx, &si, &ep, &f, &args, &it ]( const uint8_t args_min, const uint8_t args_max ) {
						if ( args.size() < args_min || args.size() > args_max ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Texture filter '" + f + "' expects " + ( args_min == args_max
								? std::to_string( args_min )
								: std::to_string( args_min ) + " to " + std::to_string( args_max )
							) + " parameters, got " + std::to_string( args.size() ) + ": " + *it );
						}
					};
					const auto f_expect_args = [ & f_expect_args_fromto ]( const uint8_t args ) {
						return f_expect_args_fromto( args, args );
					};

					if ( f == "smacpalette" || f == "smacsoftware" ) {
						f_expect_args( 0 );
						if ( it != params.begin() + 1 || palette_flags != (f == "smacpalette" ? types::texture::TF_SMAC_PALETTE : types::texture::TF_SMAC_SOFTWARE_TERRAIN) ) {
							GSE_ERROR( gse::EC.UI_ERROR, f + "() must be the first texture operation" );
						}
					}
					else if ( f == "smacmenu" ) {
						f_expect_args_fromto( 3, 5 );
						if ( args.size() == 4 ) { GSE_ERROR( gse::EC.UI_ERROR, "smacmenu expects both viewport dimensions" ); }
						if ( it != params.begin() + 1 || !is_smac_menu ) { GSE_ERROR( gse::EC.UI_ERROR, "smacmenu() must be the first texture operation" ); }
					}
					else if ( f == "stretch" ) {
						f_expect_args( 0 );
						is_tiled = false;
					}
					else if ( f == "tint" ) {
						f_expect_args( 2 );
						float intensity;
						ParseColor( GSE_CALL, args.at( 0 ), color );
						if ( !util::String::ParseFloat( args.at( 1 ), intensity ) ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid tint parameters (expected float, got " + args.at( 1 ) + "): " + *it );
						}
						if ( intensity < 0.0f || intensity > 1.0f ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid tint parameters (intensity must be between 0.0 and 1.0):" + *it );
						}
						filter.Tint( color, intensity );
					}
					else if ( f == "crop" ) {
						f_expect_args( 4 );
						long int x1, y1, x2, y2;
						if (
							!util::String::ParseInt( args.at( 0 ), x1 ) ||
								!util::String::ParseInt( args.at( 1 ), y1 ) ||
								!util::String::ParseInt( args.at( 2 ), x2 ) ||
								!util::String::ParseInt( args.at( 3 ), y2 )
							) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid crop parameters (expected x1,y1,x2,y2, got " + args.at( 0 ) + "," + args.at( 1 ) + "," + args.at( 2 ) + "," + args.at( 3 ) + "): " + *it );
						}
						if ( x1 < 0 ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid crop parameters (x1 must be positive): " + *it );
						}
						if ( y1 < 0 ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid crop parameters (y1 must be positive): " + *it );
						}
						if ( x2 < x1 ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid crop parameters (x2 can't be less than x1): " + *it );
						}
						if ( y2 < y1 ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid crop parameters (y1 can't be less than y1): " + *it );
						}
						if ( x2 >= filter.GetWidth() ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid crop parameters (x2 must be less than texture width (" + std::to_string( filter.GetWidth() ) + ")): " + *it );
						}
						if ( y2 >= filter.GetHeight() ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid crop parameters (y2 must be less than texture height (" + std::to_string( filter.GetHeight() ) + ")): " + *it );
						}
						filter.Crop( x1, y1, x2, y2 );
					}
					else if ( f == "rotate" ) {
						f_expect_args( 0 );
						filter.Rotate();
					}
					else if ( f == "flipv" ) {
						f_expect_args( 0 );
						filter.FlipV();
					}
					else if ( f == "fliph" ) {
						f_expect_args( 0 );
						filter.FlipH();
					}
					else if ( f == "contrast" ) {
						f_expect_args( 1 );
						float value;
						if ( !util::String::ParseFloat( args.at( 0 ), value ) ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid contrast value (expected float, got " + args.at( 0 ) + "): " + *it );
						}
						if ( value < 0.0f || value > 10.0f ) {
							GSE_ERROR( gse::EC.UI_ERROR, "Invalid contrast value (must be between 0.0 and 10.0):" + *it );
						}
						filter.Contrast( value );
					}
					else {
						GSE_ERROR( gse::EC.UI_ERROR, "Unknown texture filter: " + f );
					}
				}
				const auto result = filter.GetTexture();
				if ( !result.texture ) { GSE_ERROR( gse::EC.LOADER_ERROR, "Texture filter produced no output" ); }
				if ( m_background.is_texture_owned ) {
					replaced_texture.reset( m_background.texture );
				}
				m_background.texture = result.texture;
				m_background.is_texture_owned = result.is_owned || result.texture == generated_menu.get();
				if ( result.texture == generated_menu.get() ) { generated_menu.release(); }
			}
			m_background.texture->FullUpdate(); // TODO: optimize?
			m_actor->SetTexture( m_background.texture );
			if ( is_tiled ) {
				m_geometry->AsRectangle()->SetTiled(
					{
						m_background.texture->GetWidth(),
						m_background.texture->GetHeight()
					}
				);
			}
			else {
				m_geometry->AsRectangle()->SetStretched();
			}
		},
		[ this ]( GSE_CALLABLE ) {
			ClearTexture();
		}
	);

}

Surface::~Surface() {
	StopMovie();
	if ( m_background.texture && m_background.is_texture_owned ) {
		delete m_background.texture;
	}
}

void Surface::PresentMovieFrame(int width,int height,const std::vector<uint8_t>& rgba) {
	if(width<=0 || height<=0 || width>2048 || height>2048 || rgba.size()!=static_cast<size_t>(width)*height*4) {
		throw std::invalid_argument("Invalid movie RGBA frame");
	}
	CreateTexture();m_background.texture->Resize(width,height);
	std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
	m_background.texture->FullUpdate();
	// Refresh the actor revision too: UI caches otherwise retain the first frame.
	m_actor->SetTexture(m_background.texture);
	auto* geometry=m_geometry->AsRectangle();geometry->SetStretched();
	const auto* parent=m_parent->GetGeometry();
	// GetInnerWidth/Height describe child-content bounds, which can be zero
	// before the first frame. The root's actual box is the presentation stage.
	const auto& parent_area=parent->GetEffectiveArea();
	const float viewport_width=std::min(parent_area.width,parent_area.height*4.0f/3.0f);
	const float scale=std::min(viewport_width/width,parent_area.height/height);
	geometry->SetWidth(width*scale);geometry->SetHeight(height*scale);
}

void Surface::PresentTurnFrame(const std::vector<uint8_t>& rgba) {
    if(rgba.size()!=233*23*4) {throw std::invalid_argument("Invalid Turn Complete RGBA frame");}
    // Reuse one owned texture. No path resolution, decode or allocation on the
    // recurring timer; refresh the actor revision so cached UI sees each frame.
    CreateTexture();m_background.texture->Resize(233,23);
    std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
    m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);
    m_geometry->AsRectangle()->SetStretched();
}

void Surface::StopMovie() {
	if(m_movie) {
		const auto s=m_movie->GetStatus();
		Log("Movie stopped: frames="+std::to_string(s.decoded_frames)+
			" samples="+std::to_string(s.decoded_samples)+" clock="+std::to_string(s.clock)+
			" finished="+std::to_string(s.finished)+" audio_device="+std::to_string(s.audio_device_opened)+
			" damaged_packets="+std::to_string(s.damaged_packets)+
			(s.audio_fallback.empty() ? "" : " silent_fallback="+s.audio_fallback));
		m_movie.reset();g_engine->GetAudio()->ResumeAfterMovie();
	}
}

void Surface::Destroy( GSE_CALLABLE ) { StopMovie(); Area::Destroy(GSE_CALL); }

void Surface::CreateTexture() {
	if ( !m_background.texture || !m_background.is_texture_owned ) {
		m_background.texture = new types::texture::Texture();
		m_background.is_texture_owned = true;
		m_actor->SetTexture( m_background.texture );
	}
}

void Surface::ClearTexture() {
	m_actor->SetTexture( nullptr );
	if ( m_background.texture && m_background.is_texture_owned ) {
		delete m_background.texture;
	}
	m_background.texture = nullptr;
	m_background.is_texture_owned = false;
}

types::texture::Texture* Surface::GetOwnedTexturePtr() {
	if ( !m_background.texture ) {
		CreateTexture();
	}
	else {
		ASSERT( m_background.is_texture_owned, "GetOwnedTexturePtr on non-owned texture" );
	}
	return m_background.texture;
}

}
}

namespace ui::dom {
void Surface::UpdateDialogFrames() {
    if(m_dialog_frames.empty()) {return;}
    const auto& area=m_geometry->m_area;
    if(area.width<=0 || area.height<=0) {return;}
    if(!std::isfinite(area.width) || !std::isfinite(area.height) || area.width>2048 || area.height>2048) {
        throw std::invalid_argument("Dialog canvas exceeds supported logical size");
    }
    const int width=static_cast<int>(std::round(area.width)),height=static_cast<int>(std::round(area.height));
    if(width<=0 || height<=0 || m_dialog_size==std::array<int,2>{width,height}) {return;}
    if(width>2048 || height>2048) {throw std::invalid_argument("Dialog canvas exceeds supported logical size");}
    if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
    if(!m_dialog_source) {throw std::runtime_error("Original dialog frame source unavailable");}
    std::vector<uint8_t> indexed(size_t(width)*height,9);
    for(const auto& call:m_dialog_frames) {
        loader::texture::smac_dialog_frame::Paint(indexed,width,height,*m_dialog_source,call,{0,0,width,height},m_dialog_phase);
    }
    const auto rgba=loader::texture::smac_dialog_frame::Rgba(indexed,*m_dialog_source);
    m_dialog_size={width,height}; // geometry/actor updates may synchronously revisit this hook
    CreateTexture();m_background.texture->Resize(width,height);
    std::copy(rgba.begin(),rgba.end(),m_background.texture->GetBitmap());
    m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
}
}
