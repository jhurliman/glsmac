#include "Surface.h"
#include "SmacWorkshopModalText.h"
#include "SmacPreferencesText.h"

#include <memory>

#include "types/mesh/Rectangle.h"
#include "scene/actor/Mesh.h"
#include "ui/geometry/Rectangle.h"
#include "engine/Engine.h"
#include "loader/texture/TextureLoader.h"
#include "loader/texture/SmacTurnFrames.h"
#include "loader/cvr/CvrScoutRenderer.h"
#include "loader/cvr/CvrActorMaterial.h"
#include "game/frontend/SmacDevelopedOverlay.h"
#include "loader/txt/TXTLoader.h"
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
#include "ui/SmacWorkshopCachedPreview.h"
#include <chrono>
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
namespace {
uint64_t WorkshopNow() {
    return uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}
}

Surface::Surface( DOM_ARGS_T )
	: Area( DOM_ARGS_PASS, tag ) {

    Method(GSE_CALL,"smacpreferencestext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);
        if(!m_preferences_text) {
            std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath("script.txt"));
            m_preferences_text=ReadSmacPreferencesText(std::string(bytes.begin(),bytes.end()));
        }
        gse::value::array_elements_t pages;
        for(const auto& page:*m_preferences_text) {
            gse::value::array_elements_t rows;
            for(const auto& row:page.rows) {rows.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                {"id",VALUE(gse::value::Int,,row.id)}, {"label",VALUE(gse::value::String,,row.label)}
            }));}
            pages.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                {"title",VALUE(gse::value::String,,page.title)}, {"rows",VALUE(gse::value::Array,,rows)}
            }));
        }
        return VALUE(gse::value::Array,,pages);
    }));
    Method(GSE_CALL,"smacpreferencescaptions",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);
        std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath("labels.txt"));
        const auto labels=loader::txt::TXTLoader::ReadUniqueSection(std::string(bytes.begin(),bytes.end()),"LABELS");
        // The text reader retains the count at0. These are original native
        // zero-based token slots, not physical source lines or English keys.
        const std::array<size_t,20> slots{{750,751,752,753,754,1077,1078,1079,101,102,1088,664,1080,1081,1082,1083,1084,1085,1086,1087}};
        gse::value::array_elements_t words;
        for(const auto slot:slots) {
            if(slot+1>=labels.size() || labels[slot+1].empty() || labels[slot+1].size()>256) {GSE_ERROR(gse::EC.LOADER_ERROR,"Incomplete Preferences caption source");}
            words.push_back(VALUE(gse::value::String,,labels[slot+1]));
        }
        return VALUE(gse::value::Array,,words);
    }));

    Method(GSE_CALL,"smacworkshopparenttext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);return WorkshopParentText(GSE_CALL);
    }));
    Method(GSE_CALL,"smacworkshopabilitycardtext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(2);N_GETVALUE(name,0,String);N_GETVALUE(description,1,String);
        if(name.size()>256 || description.size()>4096 || name.find('\0')!=std::string::npos || description.find('\0')!=std::string::npos) {
            throw std::invalid_argument("Workshop ability card text exceeds bounds");
        }
        gse::value::array_elements_t tokens;
        for(const auto& word:SmacWorkshopModalWords("{"+name+":  }"+description)) {
            gse::value::array_elements_t spans;
            for(const auto& span:word.spans) {
                spans.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                    {"text",VALUE(gse::value::String,,span.text)}, {"color",VALUE(gse::value::Int,,span.emphasis ? 224:222)}
                }));
            }
            tokens.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
                {"text",VALUE(gse::value::String,,word.text)}, {"spans",VALUE(gse::value::Array,,spans)}
            }));
        }
        return VALUE(gse::value::Array,,tokens);
    }));
    Method(GSE_CALL,"smacworkshoprenametext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);return WorkshopRenameText(GSE_CALL);
    }));
    Method(GSE_CALL,"smacworkshopupgradetext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(6);N_GETVALUE(kind,0,Int);N_GET(source,1,Object);N_GET(target,2,Object);N_GETVALUE(count,3,Int);N_GETVALUE(total,4,Int);N_GETVALUE(available,5,Int);
        if(kind<0 || kind>6 || count<0 || count>255 || total<0 || total>INT32_MAX || available<INT32_MIN || available>INT32_MAX) {throw std::invalid_argument("Invalid Workshop Upgrade text context");}
        const auto unpack=[](gse::value::Object* object) {
            const auto field=[&](const char* key,gse::value_type_t type) {const auto* value=object->Get(key)->Deref();
                if(value->type!=type) {throw std::invalid_argument("Incomplete stored Workshop text record");}return value;};
            const auto string=[&](const char* key){return static_cast<const gse::value::String*>(field(key,gse::VT_STRING))->value;};
            const auto integer=[&](const char* key){return static_cast<const gse::value::Int*>(field(key,gse::VT_INT))->value;};
            const auto domain=integer("domain"),reactor=integer("reactor"),mask=integer("abilities");
            if(domain<0 || domain>2 || reactor<0 || reactor>255 || mask<0 || mask>UINT32_MAX) {throw std::invalid_argument("Invalid stored Workshop text metadata");}
            return SmacWorkshopModalDesign{string("name"),string("stats"),string("ability_tokens"),int32_t(domain),int32_t(reactor),uint32_t(mask)};
        };
        const auto a=unpack(source),b=unpack(target);
        auto* picker=static_cast<gse::value::Object*>(WorkshopPickerText(GSE_CALL));
        const auto& domains=static_cast<gse::value::Array*>(picker->Get("domains"))->value;
        const std::array<std::string,3> words{{static_cast<gse::value::String*>(picker->Get("psi"))->value,
            static_cast<gse::value::String*>(domains.at(1))->value,static_cast<gse::value::String*>(domains.at(2))->value}};
        return WorkshopUpgradeText(GSE_CALL,int32_t(kind),{kind==0 ? a.name:SmacWorkshopModalDesignLabel(a,words),SmacWorkshopModalDesignLabel(b,words)},
            {int32_t(total),int32_t(available),int32_t(count)});
    }));
    Method(GSE_CALL,"smacworkshoppresettext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);return WorkshopUpgradeText(GSE_CALL,10,{"",""},{0,0,0});
    }));
    Method(GSE_CALL,"smacworkshopintrinsictext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(2);N_GETVALUE(kind,0,Int);N_GETVALUE(name,1,String);
        if(kind<1 || kind>2 || name.size()>31 || name.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop intrinsic warning context");}
        return WorkshopUpgradeText(GSE_CALL,int32_t(kind)+7,{name,""},{0,0,0});
    }));
    Method(GSE_CALL,"smacworkshopredesigntext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(2);N_GETVALUE(name,0,String);N_GETVALUE(count,1,Int);
        if(name.size()>31 || name.find('\0')!=std::string::npos || count<0 || count>255) {throw std::invalid_argument("Invalid Workshop redesign text context");}
        return WorkshopUpgradeText(GSE_CALL,7,{name,""},{int32_t(count),0,0});
    }));
    Method(GSE_CALL,"smacworkshopretiretext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(2);N_GETVALUE(name,0,String);N_GETVALUE(count,1,Int);
        if(name.size()>31 || name.find('\0')!=std::string::npos || count<0 || count>255) {throw std::invalid_argument("Invalid Workshop Retire text context");}
        return WorkshopRetireText(GSE_CALL,name,int32_t(count));
    }));
    Method(GSE_CALL,"smacworkshopconsolebacking",NATIVE_CALL(this) {
        N_EXPECT_ARGS(0);
        if(!m_workshop_console_backing) {
            if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
            if(!m_dialog_source) {throw std::runtime_error("Original Workshop console palette unavailable");}
            // The console uses the same native7b01e0 scanline tile as its
            // strip. Local top50 is even, preserving the whole-console phase.
            std::vector<uint8_t> indexed(1024*207);
            for(size_t y=0;y<207;++y) {std::fill_n(indexed.begin()+y*1024,1024,y%2 ? 65:198);}
            const auto rgba=loader::texture::smac_dialog_frame::Rgba(indexed,*m_dialog_source);
            CreateTexture();m_background.texture->Resize(1024,207);
            std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
            m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);
            m_geometry->AsRectangle()->SetStretched();
            m_workshop_console_backing=true;
        }
        return VALUE(gse::value::Undefined);
    }));

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
    Property(GSE_CALL,"smac_checkbox",gse::VT_BOOL,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
        if(!m_dialog_source) {GSE_ERROR(gse::EC.LOADER_ERROR,"Original checkbox atlas unavailable");}
        const auto indexed=loader::texture::smac_dialog_frame::Checkbox(*m_dialog_source,static_cast<const gse::value::Bool*>(value)->value);
        const auto rgba=loader::texture::smac_dialog_frame::Rgba(indexed,*m_dialog_source);
        CreateTexture();m_background.texture->Resize(18,18);
        std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
    },[this](GSE_CALLABLE){ClearTexture();});
    Property(GSE_CALL,"smac_button_sprite",gse::VT_ARRAY,nullptr,PF_NONE,[this](GSE_CALLABLE,gse::Value* const value) {
        const auto& fields=static_cast<gse::value::Array*>(value)->value;
        if(fields.size()!=2 || !fields[0] || !fields[1] || fields[0]->type!=gse::VT_INT || fields[1]->type!=gse::VT_INT) {GSE_ERROR(gse::EC.UI_ERROR,"Modal button sprite requires [state,width]");}
        const auto state=static_cast<const gse::value::Int*>(fields[0])->value,width=static_cast<const gse::value::Int*>(fields[1])->value;
        if(state<0 || state>2 || width<6 || width>2048) {GSE_ERROR(gse::EC.UI_ERROR,"Modal button sprite requires state0..2 and width6..2048");}
        if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
        if(!m_dialog_source || m_dialog_source->indexed.size()!=size_t(750*900)) {throw std::runtime_error("Original modal button sprite source unavailable");}
        // Original460720 installs interface.pcx rows6/7/8 for generic
        // normal/hover/pressed buttons. They are three sprites, not frame27.
        std::vector<uint8_t> indexed(size_t(width)*20,9);const int top=127+21*int(state);
        for(int y=0;y<20;++y) {for(int x=0;x<width;++x) {
            const int sx=x<3 ? 64+x:x>=width-3 ? 136+x-int(width)+3:68+(x-3)%67;
            const auto index=m_dialog_source->indexed[size_t(top+y)*750+size_t(sx)];
            indexed[size_t(y)*size_t(width)+size_t(x)]=(index==6 || index==7)?9:index;
        }}
        const auto rgba=loader::texture::smac_dialog_frame::Rgba(indexed,*m_dialog_source);
        CreateTexture();m_background.texture->Resize(width,20);std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
    },[this](GSE_CALLABLE){ClearTexture();});
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

    Method(GSE_CALL,"smacworkshopabilitytext",NATIVE_CALL(this) {
        N_EXPECT_ARGS(1);N_GETVALUE(technology,0,String);
        if(technology.size()>128 || technology.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid ability guard technology text");}
        return WorkshopAbilityText(GSE_CALL,technology);
    }));

    Method(GSE_CALL,"smacworkshopcomponents",NATIVE_CALL(this) {
        N_EXPECT_ARGS_MIN_MAX(1,2);N_GETVALUE(armor,0,Int);
        bool armor_only=false;if(arguments.size()==2) {N_GETVALUE(only,1,Bool);armor_only=only;}
        if(armor<0 || armor>9) {throw std::invalid_argument("Unsupported Workshop side-card draft");}
        EnsureWorkshopComponents();
        // One original painter pass shares centering state in component order.
        std::array<uint32_t,3> position{};std::vector<uint8_t> rgba(623*368*4,0);
        const std::array<std::array<int,4>,4> clips{{{30,7,129,92},{30,101,129,186},{30,195,129,280},{462,7,561,92}}};
        for(int kind=0;kind<4;++kind) {
            const auto image=m_workshop_components->Render(kind,kind==2 ? int(armor):0,false,true,position);
            if(armor_only && kind!=2) {continue;}
            if(!image) {continue;}const auto& clip=clips[kind];
            for(int y=clip[1];y<clip[3];++y) {for(int x=clip[0];x<clip[2];++x) {
                const int sx=x-image->target_anchor[0]+128,sy=y-image->target_anchor[1]+128;
                if(sx<0 || sx>=256 || sy<0 || sy>=256) {throw std::invalid_argument("Workshop component clip exceeds internal image");}
                const size_t source=(size_t(sy)*256+sx)*4,destination=(size_t(y)*623+x)*4;
                if(kind==2 && armor==5) {
                    // Probability Sheath stores a shade operand, not a palette
                    // index. Blend against each original deswin backdrop pixel.
                    const auto color=m_workshop_components->CompositeBackdrop(image->indexed.color[size_t(sy)*256+sx],size_t(x),size_t(y));
                    const auto& rgb=m_workshop_components->Colors().at(color);
                    for(size_t channel=0;channel<3;++channel) {rgba[destination+channel]=rgb[channel];}rgba[destination+3]=255;
                } else {std::copy_n(image->rgba.begin()+source,4,rgba.begin()+destination);}
            }}
        }
        CreateTexture();m_background.texture->Resize(623,368);std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
        return VALUE(gse::value::Bool,,true);
    }));

    Method(GSE_CALL,"smacworkshoparmorpicker",NATIVE_CALL(this) {
        N_EXPECT_ARGS(2);N_GETVALUE(selected,0,Int);N_GET(rows,1,Array);
        if(selected< -1 || selected>9 || rows->value.size()>20) {throw std::invalid_argument("Unsupported Workshop armor picker page");}
        std::vector<std::pair<int,bool>> choices;std::array<bool,256> seen{};
        for(const auto* value:rows->value) {
            if(!value || value->type!=gse::VT_OBJECT) {throw std::invalid_argument("Invalid Workshop armor card");}
            const auto& row=static_cast<const gse::value::Object*>(value)->value;
            const auto id=row.find("id"),known=row.find("technology_available");
            if(id==row.end() || known==row.end() || !id->second || !known->second || id->second->type!=gse::VT_INT || known->second->type!=gse::VT_BOOL) {throw std::invalid_argument("Incomplete Workshop armor card");}
            const auto component=static_cast<const gse::value::Int*>(id->second)->value;
            if(component<0 || component>255 || seen[component]) {throw std::invalid_argument("Unsupported or duplicate Workshop armor card");}
            seen[component]=true;choices.emplace_back(int(component),static_cast<const gse::value::Bool*>(known->second)->value);
        }
        EnsureWorkshopComponents();
        if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
        if(!m_dialog_source) {throw std::runtime_error("Original Workshop frame source unavailable");}
        std::vector<uint8_t> indexed(623*387,9);std::array<uint32_t,3> position{};
        for(size_t ordinal=0;ordinal<choices.size();++ordinal) {
            const int id=choices[ordinal].first;const bool known=choices[ordinal].second;const int x=27+108*int(ordinal%5),y=4+94*int(ordinal/5);
            loader::texture::smac_dialog_frame::Paint(indexed,623,387,*m_dialog_source,{id==selected?2:0,x,y,x+105,y+91},{0,0,623,387});
            // Expansion-only models await their own native composition controls.
            if(id>9) {continue;}
            if(id==0) {continue;}
            const std::array<int,8> key{2,id,0,0,0,0,0,0};const auto before=position;
            const auto found=std::find_if(m_workshop_picker_images.begin(),m_workshop_picker_images.end(),[&](const auto& entry){return entry.key==key && entry.known==known && entry.before==before;});
            std::shared_ptr<const std::vector<uint8_t>> colors;
            if(found!=m_workshop_picker_images.end()) {colors=found->colors;position=found->after;}
            else {
                auto image=m_workshop_components->Render(2,id,true,known,position,int(ordinal));
                colors=std::make_shared<const std::vector<uint8_t>>(std::move(image->indexed.color));
                if(m_workshop_picker_images.size()==64) {m_workshop_picker_images.erase(m_workshop_picker_images.begin());}
                m_workshop_picker_images.push_back({key,known,before,position,colors});
            }
            for(int py=y;py<y+91;++py) {for(int px=x;px<x+105;++px) {
                const int sx=px-(x+52)+128,sy=py-(y+45)+128;
                const auto source=colors->at(size_t(sy)*256+sx);auto& destination=indexed[size_t(py)*623+px];
                if(id==5 && known) {destination=m_workshop_components->Composite(source,destination);}
                else if(source!=9) {destination=source;}
            }}
        }
        const auto rgba=loader::texture::smac_dialog_frame::Rgba(indexed,*m_dialog_source);
        CreateTexture();m_background.texture->Resize(623,387);std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
        return WorkshopPickerText(GSE_CALL);
    }));

    Method(GSE_CALL,"smacworkshopcomponentpicker",NATIVE_CALL(this) {
        N_EXPECT_ARGS(10);N_GETVALUE(kind,0,Int);N_GETVALUE(selected,1,Int);N_GET(rows,2,Array);
        N_GETVALUE(chassis,3,Int);N_GETVALUE(weapon,4,Int);N_GETVALUE(reactor,5,Int);N_GETVALUE(faction,6,String);
        N_GETVALUE(power,7,Int);N_GETVALUE(mode,8,Int);N_GETVALUE(strength,9,Int);
        const int maximum=kind==0?8:kind==1?20:4;
        if((kind!=0 && kind!=1 && kind!=3) || (selected!=-1 && selected<(kind==3?1:0)) || selected>maximum || rows->value.size()>size_t(kind==0?9:kind==1?20:4)
            || chassis<0 || chassis>8 || weapon<0 || weapon>20 || reactor<1 || reactor>4
            || power< -127 || power>127 || mode<0 || mode>12 || strength< -127 || strength>127
            || faction.empty() || faction.size()>256 || faction.find_first_of("/\\:\0",0,4)!=std::string::npos) {throw std::invalid_argument("Invalid Workshop component picker request");}
        struct Choice {int id;bool known;int power,mode,sprite;};
        std::vector<Choice> choices;std::array<bool,21> seen{};
        for(const auto* value:rows->value) {
            if(!value || value->type!=gse::VT_OBJECT) {throw std::invalid_argument("Invalid Workshop component card");}
            const auto& row=static_cast<const gse::value::Object*>(value)->value;
            const auto id=row.find("id"),known=row.find("technology_available");
            if(id==row.end() || known==row.end() || !id->second || !known->second || id->second->type!=gse::VT_INT || known->second->type!=gse::VT_BOOL) {throw std::invalid_argument("Incomplete Workshop component card");}
            const auto component=static_cast<const gse::value::Int*>(id->second)->value;
            if(component<(kind==3?1:0) || component>maximum || seen[size_t(component)]) {throw std::invalid_argument("Invalid or repeated Workshop component identity");}
            Choice choice{int(component),static_cast<const gse::value::Bool*>(known->second)->value,0,0,-1};
            if(kind==1) {
                const auto integer=[&](const char* key) {const auto item=row.find(key);
                    if(item==row.end() || !item->second || item->second->type!=gse::VT_INT) {throw std::invalid_argument("Incomplete Workshop weapon metadata");}
                    return static_cast<const gse::value::Int*>(item->second)->value;};
                const auto card_power=integer("power"),card_mode=integer("mode"),sprite=integer("sprite");
                if(card_power< -128 || card_power>127 || !((card_mode>=0 && card_mode<=2) || (card_mode>=7 && card_mode<=12))
                    || sprite< -128 || sprite>127 || ((card_mode==8 || card_mode==9 || card_mode==11) && sprite>9)) {
                    throw std::invalid_argument("Unsupported Workshop weapon definition or sprite selector");
                }
                choice.power=int(card_power);choice.mode=int(card_mode);choice.sprite=int(sprite);
            }
            seen[size_t(component)]=true;choices.push_back(choice);
        }
        if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
        if(!m_dialog_source) {throw std::runtime_error("Original Workshop frame source unavailable");}
        if(faction!=m_workshop_picker_faction) {m_workshop_picker_images.clear();m_workshop_picker_faction=faction;}
        const auto read=[](const std::string& name) {std::vector<unsigned char> bytes;
            util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
        std::vector<uint8_t> indexed(623*387,9);std::array<uint32_t,3> position{};
        for(size_t ordinal=0;ordinal<choices.size();++ordinal) {
            const auto& choice=choices[ordinal];const int id=choice.id;const bool known=choice.known;
            const int x=27+108*int(ordinal%5),y=4+94*int(ordinal/5);
            loader::texture::smac_dialog_frame::Paint(indexed,623,387,*m_dialog_source,{id==selected?2:0,x,y,x+105,y+91},{0,0,623,387});
            if(kind==1 && (choice.mode==8 || choice.mode==9 || choice.mode==11) && choice.sprite>=0) {
                if(!m_workshop_weapon_sprites) {m_workshop_weapon_sprites=loader::cvr::CvrActorMaterial::ReadWorkshopWeaponSprites(read("units.pcx"));}
                loader::cvr::CvrActorMaterial::PaintWorkshopWeaponSprite(indexed,623,387,m_workshop_weapon_sprites->at(size_t(choice.sprite)),int(ordinal));
            }
            const std::array<int,8> key{int(kind),kind==0?id:kind==1?0:int(chassis),kind==0?0:kind==1?id:int(weapon),kind==0?int(reactor):kind==1?1:id,
                kind==0?1:kind==1?choice.power:int(power),kind==0?0:kind==1?choice.mode:int(mode),kind==0 || kind==1?1:int(strength),0};
            const auto before=position;
            auto found=std::find_if(m_workshop_picker_images.begin(),m_workshop_picker_images.end(),[&](const auto& entry){return entry.key==key && entry.known==known && entry.before==before;});
            std::shared_ptr<const std::vector<uint8_t>> colors;
            if(found!=m_workshop_picker_images.end()) {colors=found->colors;position=found->after;}
            else {
                auto renderer=kind==1 ? loader::cvr::CvrScoutRenderer::LoadWorkshopPickerWeapon(read,read(faction),id,{key[4],key[5],1})
                    :loader::cvr::CvrScoutRenderer::LoadWorkshopDesign(read,read(faction),key[1],key[2],0,key[3],int(kind),{key[4],key[5],key[6]});
                auto image=renderer.RenderWorkshopPicker(int(kind),int(ordinal),known,position);
                colors=std::make_shared<const std::vector<uint8_t>>(std::move(image.indexed.color));
                if(m_workshop_picker_images.size()==64) {m_workshop_picker_images.erase(m_workshop_picker_images.begin());}
                m_workshop_picker_images.push_back({key,known,before,position,colors});
            }
            // The destination anchor changes metadata, not the256-square planes.
            for(int py=y;py<y+91;++py) {for(int px=x;px<x+105;++px) {
                const int sx=px-(x+52)+128,sy=py-(y+45)+128;
                const auto source=colors->at(size_t(sy)*256+sx);if(source!=9) {indexed[size_t(py)*623+px]=source;}
            }}
        }
        const auto rgba=loader::texture::smac_dialog_frame::Rgba(indexed,*m_dialog_source);
        CreateTexture();m_background.texture->Resize(623,387);std::memcpy(m_background.texture->GetBitmap(),rgba.data(),rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
        return WorkshopPickerText(GSE_CALL);
    }));

    Method(GSE_CALL,"smacworkshopclock",NATIVE_CALL(this) {
        N_EXPECT_ARGS(1);N_GETVALUE(operation,0,Int);
        if(operation<0 || operation>2) {throw std::invalid_argument("Invalid Workshop clock operation");}
        if(operation==0) {m_workshop_preview.Stop();}
        if(operation==2) {m_workshop_preview.Reset();}
        if(operation==1) {
            m_workshop_preview.Start(WorkshopNow());
            if(!m_workshop_preview_iterable) {
                Iterable([this]() {
                    if(m_workshop_preview.Due(WorkshopNow())) {
                        m_ui->WithGSE([this](GSE_CALLABLE) {Trigger(GSE_CALL,"workshopframe",gse::f_args_t{},gse::VT_BOOL);});
                    }
                });m_workshop_preview_iterable=true;
            }
        }
        return VALUE(gse::value::Bool,,true);
    }));
    Method(GSE_CALL,"smacworkshopcachedpreview",NATIVE_CALL(this) {
        N_EXPECT_ARGS(2);N_GETVALUE(icon,0,Int);N_GETVALUE(variant,1,Int);
        if(icon<0 || icon>9 || variant<0 || variant>(icon==3 ? 6:0)) {throw std::invalid_argument("Invalid Workshop cached preview");}
        m_workshop_preview.BeginPaint(true);
        const std::array<int,2> key{int(icon),int(variant)};
        if(key!=m_workshop_cached_key) {
            if(!m_workshop_cached) {
                const auto read=[](const std::string& name) {std::vector<unsigned char> bytes;
                    util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
                m_workshop_cached=std::make_shared<const ui::SmacWorkshopCachedPreview>(ui::SmacWorkshopCachedPreview::Load(read));
            }
            const auto image=m_workshop_cached->Render(key[0],key[1]);
            CreateTexture();m_background.texture->Resize(311,311);std::memcpy(m_background.texture->GetBitmap(),image.rgba.data(),image.rgba.size());
            m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();m_workshop_cached_key=key;
        }
        return VALUE(gse::value::Bool,,true);
    }));

    Method(GSE_CALL,"smacworkshopdesign",NATIVE_CALL(this) {
        N_EXPECT_ARGS_MIN_MAX(10,12);N_GETVALUE(chassis,0,Int);N_GETVALUE(weapon,1,Int);N_GETVALUE(armor,2,Int);N_GETVALUE(reactor,3,Int);
        N_GETVALUE(kind,4,Int);N_GETVALUE(faction,5,String);N_GETVALUE(yaw,6,Float);
        N_GETVALUE(power,7,Int);N_GETVALUE(mode,8,Int);N_GETVALUE(strength,9,Int);
        if(chassis<0 || chassis>8 || weapon<0 || weapon>20 || armor<0 || armor>9 || reactor<1 || reactor>4 || kind< -1 || kind>3
            || power< -127 || power>127 || mode<0 || mode>12 || strength< -127 || strength>127
            || faction.empty() || faction.size()>256 || faction.find_first_of("/\\:\0",0,4)!=std::string::npos
            || !std::isfinite(yaw) || !std::isfinite(static_cast<float>(yaw))) {throw std::invalid_argument("Unsupported Workshop composition request");}
        bool visible=false;if(arguments.size()>=11) {N_GETVALUE(show,10,Bool);visible=show;}
        const bool animated=arguments.size()>=11;
        uint32_t abilities=0;if(arguments.size()==12) {N_GETVALUE(mask,11,Int);
            if(mask<0 || uint64_t(mask)>UINT32_MAX) {throw std::invalid_argument("Invalid Workshop ability mask");}abilities=uint32_t(mask);
        }
        const std::array<int,8> key{int(chassis),int(weapon),int(armor),int(reactor),int(kind),int(power),int(mode),int(strength)};
        if(!m_workshop_design || key!=m_workshop_design_key || faction!=m_workshop_design_faction || abilities!=m_workshop_design_abilities) {
            const auto read=[](const std::string& name) {std::vector<unsigned char> bytes;
                util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
            const auto faction_bytes=read(faction);
            auto renderer=loader::cvr::CvrScoutRenderer::LoadWorkshopDesign(read,faction_bytes,key[0],key[1],key[2],key[3],key[4],{key[5],key[6],key[7],kind==1 && weapon==18,abilities});
            auto next=std::make_shared<const loader::cvr::CvrScoutRenderer>(std::move(renderer));
            m_workshop_design=std::move(next);m_workshop_design_key=key;m_workshop_design_faction=faction;m_workshop_design_abilities=abilities;
        }
        const float angle=static_cast<float>(yaw);uint32_t bits;std::memcpy(&bits,&angle,sizeof(bits));std::array<uint32_t,3> position{};
        if(animated && kind<0) {bits=m_workshop_preview.BeginPaint(false);}
        loader::cvr::CvrScoutRenderer::WorkshopAnimation animation{m_ui->ModelAnimationPhase(),visible};
        const auto image=kind<0 ? m_workshop_design->RenderWorkshop(bits,animated ? &animation:nullptr):m_workshop_design->RenderWorkshopCard(int(kind),position,animated ? &animation:nullptr);
        if(animated) {m_ui->ModelAnimationPhase()=animation.phase;}
        m_workshop_cached_key={-1,-1};
        CreateTexture();m_background.texture->Resize(256,256);std::memcpy(m_background.texture->GetBitmap(),image.rgba.data(),image.rgba.size());
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
        return VALUE(gse::value::Bool,,true);
    }));

    Method(GSE_CALL,"smacworkshopstrip",NATIVE_CALL(this) {
        N_EXPECT_ARGS(4);N_GETVALUE(rows,0,Array);N_GETVALUE(faction,1,String);N_GETVALUE(selected,2,Int);N_GETVALUE(hovered,3,Int);
        if(rows.size()>10 || faction.empty() || faction.size()>256 || faction.find_first_of("/\\:\0",0,4)!=std::string::npos) {
            throw std::invalid_argument("Invalid Workshop strip projection");
        }
        if(selected< -1 || selected>511 || hovered< -1 || hovered>511) {throw std::invalid_argument("Invalid Workshop strip selection");}
        std::vector<std::optional<std::array<int64_t,8>>> keys;std::vector<int32_t> ids;
        for(auto* value:rows) {
            if(!value || value->Deref()->type!=gse::VT_OBJECT) {throw std::invalid_argument("Invalid Workshop strip row");}
            auto* row=static_cast<gse::value::Object*>(value->Deref());const auto* active=row->Get("active")->Deref();
            const auto* id=row->Get("id")->Deref();if(id->type!=gse::VT_INT || static_cast<const gse::value::Int*>(id)->value<0
                || static_cast<const gse::value::Int*>(id)->value>511) {throw std::invalid_argument("Invalid Workshop strip raw ID");}
            ids.push_back(int32_t(static_cast<const gse::value::Int*>(id)->value));
            if(active->type!=gse::VT_BOOL) {throw std::invalid_argument("Invalid Workshop strip activity");}
            if(!static_cast<const gse::value::Bool*>(active)->value) {keys.push_back(std::nullopt);continue;}
            auto* components=row->Get("components")->Deref();if(components->type!=gse::VT_OBJECT) {throw std::invalid_argument("Missing Workshop strip components");}
            std::array<int64_t,8> key;size_t at=0;
            for(const auto* name:{"chassis","weapon","armor","reactor","offense","weapon_mode","strength","abilities"}) {
                const auto* field=static_cast<gse::value::Object*>(components)->Get(name)->Deref();
                if(field->type!=gse::VT_INT) {throw std::invalid_argument("Invalid Workshop strip component");}
                const auto number=static_cast<const gse::value::Int*>(field)->value;
                if(at==7 ? (number<0 || uint64_t(number)>UINT32_MAX):(number<INT32_MIN || number>INT32_MAX)) {throw std::invalid_argument("Workshop strip component exceeds signed32");}key[at++]=number;
            }
            if(key[0]<0 || key[0]>8 || key[1]<0 || key[1]>20 || key[2]<0 || key[2]>9 || key[3]<1 || key[3]>4) {
                throw std::invalid_argument("Workshop strip component identity outside stock catalog");
            }keys.push_back(key);
        }
        const auto read=[](const std::string& name) {std::vector<unsigned char> bytes;
            util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
        if(!m_workshop_strip_colors) {
            auto colors=loader::cvr::CvrActorMaterial::ReadGamma100Colors(read("palette.pcx"));
            const auto labels=loader::txt::TXTLoader::ReadUniqueSection(read("labels.txt"),"LABELS");
            if(labels.size()<=913 || labels[111].empty() || labels[913].empty()) {throw std::invalid_argument("Incomplete Workshop strip labels");}
            m_workshop_strip_labels={labels[111],labels[913]};m_workshop_strip_shadow=game::frontend::smac_developed_overlay::ShadowLookup(colors);
            m_workshop_strip_colors=std::move(colors);
        }
        const auto& colors=*m_workshop_strip_colors;const auto& lookup=m_workshop_strip_shadow;
        if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
        if(!m_dialog_source) {throw std::runtime_error("Workshop strip frame source unavailable");}
        if(faction!=m_workshop_strip_faction) {m_workshop_strip_images.clear();m_workshop_strip_faction=faction;}
        std::vector<uint8_t> indexed(756*56);
        for(size_t y=0;y<56;++y) {std::fill_n(indexed.begin()+y*756,756,y%2?65:198);}
        loader::texture::smac_dialog_frame::Paint(indexed,756,56,*m_dialog_source,{3,0,0,756,56},{0,0,756,56});
        for(size_t i=0;i<keys.size();++i) {
            const int x=int(72*i)+19;const int style=ids[i]==selected?2:ids[i]==hovered?1:0;
            loader::texture::smac_dialog_frame::Paint(indexed,756,56,*m_dialog_source,{style,x,3,x+70,53},{0,0,756,56},{8,192});
        }
        for(size_t i=0;i<keys.size();++i) {if(keys[i]) {
            const auto key=*keys[i];auto found=std::find_if(m_workshop_strip_images.begin(),m_workshop_strip_images.end(),[&](const auto& item){return item.key==key;});
            std::shared_ptr<const loader::cvr::CvrStripImage> image;
            if(found!=m_workshop_strip_images.end()) {image=found->image;}
            else {
                const auto renderer=loader::cvr::CvrScoutRenderer::LoadStripDesign(read,read(faction),key[0],key[1],key[2],key[3],{int(key[4]),int(key[5]),int(key[6]),false,uint32_t(key[7])});
                image=std::make_shared<const loader::cvr::CvrStripImage>(renderer.RenderStrip());
                if(m_workshop_strip_images.size()==16) {m_workshop_strip_images.erase(m_workshop_strip_images.begin());}
                m_workshop_strip_images.push_back({key,image});
            }
            const auto x=int32_t(72*i);image->Composite(indexed,756,56,756,lookup,{x-76,-95},{x+21,5,x+87,51});
        }}
        CreateTexture();m_background.texture->Resize(756,56);auto* target=m_background.texture->GetBitmap();
        for(size_t i=0;i<indexed.size();++i) {const auto& rgb=colors[indexed[i]];target[4*i]=rgb[0];target[4*i+1]=rgb[1];target[4*i+2]=rgb[2];target[4*i+3]=255;}
        m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
        const auto color=[&](uint8_t index) {const auto& rgb=colors[index];return "rgb("+std::to_string(rgb[0])+","+std::to_string(rgb[1])+","+std::to_string(rgb[2])+")";};
        return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
            {"normal",VALUE(gse::value::String,,color(159))},{"unused",VALUE(gse::value::String,,color(248))},{"obsolete",VALUE(gse::value::String,,color(255))},
            {"empty_label",VALUE(gse::value::String,,m_workshop_strip_labels[0])},{"obsolete_label",VALUE(gse::value::String,,m_workshop_strip_labels[1])}
        });
    }));
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
                            // LLVM15's analyzer crashes on the nested rvalue concatenation here.
                            auto range=std::to_string(args_min);
                            if(args_min!=args_max) {range+=" to ";range+=std::to_string(args_max);}
                            std::string message="Texture filter '";message+=f;message+="' expects ";message+=range;
                            message+=" parameters, got ";message+=std::to_string(args.size());message+=": ";message+=*it;
                            GSE_ERROR(gse::EC.UI_ERROR,message);
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
void Surface::PresentSolidColor(const types::Color& color) {
    CreateTexture();m_background.texture->Resize(1,1);m_background.texture->SetPixel(0,0,color);
    m_background.texture->FullUpdate();m_actor->SetTexture(m_background.texture);m_geometry->AsRectangle()->SetStretched();
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

gse::Value* Surface::WorkshopParentText(GSE_CALLABLE) {
    if(!m_workshop_parent_text) {
        const auto read=[](const std::string& name) {std::vector<unsigned char> bytes;
            util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
        const auto source=read("script.txt");if(source.size()>2*1024*1024) {throw std::invalid_argument("Workshop script exceeds source budget");}
        const auto design=loader::txt::TXTLoader::ReadUniqueSection(source,"DESIGNLABELS");
        const auto labels=loader::txt::TXTLoader::ReadUniqueSection(read("labels.txt"),"LABELS");
        if(design.size()!=16 || labels.size()<=926) {throw std::invalid_argument("Incomplete Workshop parent text source");}
        std::array<std::string,54> text;std::copy(design.begin(),design.end(),text.begin());text[16]=labels[123];text[24]=labels[757];text[25]=labels[126];text[26]=labels[926];text[27]=labels[758];
        text[28]=labels[919];text[29]=labels[271];text[30]=labels[103];
        const std::array<size_t,7> card_indices{81,127,80,139,26,668,122};
        for(size_t i=0;i<card_indices.size();++i) {text[31+i]=labels[card_indices[i]];}

        const auto presets=loader::txt::TXTLoader::ReadUniqueSection(source,"DESIGNAUTO");
        if(presets.size()!=14) {throw std::invalid_argument("Incomplete Workshop preset captions");}
        std::copy(presets.begin(),presets.end(),text.begin()+38);text[52]=labels[669];text[53]=labels[828];

        // LABELS includes its count at index0; offsets are four-byte entries.
        const std::array<size_t,7> status_indices{916,914,918,917,782,783,915};
        for(size_t i=0;i<status_indices.size();++i) {text[17+i]=labels[status_indices[i]];}
        for(size_t i=0;i<text.size();++i) {const auto& item=text[i];if(item.empty() || item.size()>(i==53 ? 256:44) || item.find('\0')!=std::string::npos) {throw std::invalid_argument("Unsupported Workshop parent label");}}
        m_workshop_parent_text=std::move(text);
    }
    if(!m_dialog_source) {m_dialog_source=g_engine->GetTextureLoader()->GetSmacDialogSource();}
    if(!m_dialog_source) {throw std::runtime_error("Original Workshop palette unavailable");}
    gse::value::array_elements_t labels,colors,status,presets;
    for(size_t i=0;i<16;++i) {labels.push_back(VALUE(gse::value::String,,m_workshop_parent_text->at(i)));}
    for(size_t i=18;i<23;++i) {status.push_back(VALUE(gse::value::String,,m_workshop_parent_text->at(i)));}
    for(const auto& rgb:m_dialog_source->palette) {
        colors.push_back(VALUE(gse::value::String,,"rgb("+std::to_string(rgb[0])+","+std::to_string(rgb[1])+","+std::to_string(rgb[2])+")"));
    }
    for(size_t i=38;i<52;++i) {presets.push_back(VALUE(gse::value::String,,m_workshop_parent_text->at(i)));}
    auto* picker=static_cast<gse::value::Object*>(WorkshopPickerText(GSE_CALL));
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"labels",VALUE(gse::value::Array,,labels)}, {"colors",VALUE(gse::value::Array,,colors)},
        {"presets",VALUE(gse::value::Array,,presets)}, {"presets_title",VALUE(gse::value::String,,m_workshop_parent_text->at(52))},
        {"preset_tooltip",VALUE(gse::value::String,,m_workshop_parent_text->at(53))},
        {"minimum",VALUE(gse::value::String,,m_workshop_parent_text->at(16))},
        {"status_title",VALUE(gse::value::String,,m_workshop_parent_text->at(17))},
        {"status_labels",VALUE(gse::value::Array,,status)}, {"slot",VALUE(gse::value::String,,m_workshop_parent_text->at(23))},
        {"obsolete",VALUE(gse::value::String,,m_workshop_parent_text->at(24))},
        {"rename",VALUE(gse::value::String,,m_workshop_parent_text->at(25))},
        {"retire",VALUE(gse::value::String,,m_workshop_parent_text->at(26))},
        {"upgrade",VALUE(gse::value::String,,m_workshop_parent_text->at(27))},
        {"apply",VALUE(gse::value::String,,m_workshop_parent_text->at(28))},
        {"done",VALUE(gse::value::String,,m_workshop_parent_text->at(29))},
        {"cancel",VALUE(gse::value::String,,m_workshop_parent_text->at(30))},
        {"chassis",VALUE(gse::value::String,,m_workshop_parent_text->at(31))},
        {"shielding",VALUE(gse::value::String,,m_workshop_parent_text->at(32))},
        {"reactor",VALUE(gse::value::String,,m_workshop_parent_text->at(33))},
        {"special_ability",VALUE(gse::value::String,,m_workshop_parent_text->at(34))},
        {"none",VALUE(gse::value::String,,m_workshop_parent_text->at(35))},
        {"level",VALUE(gse::value::String,,m_workshop_parent_text->at(36))},
        {"power",VALUE(gse::value::String,,m_workshop_parent_text->at(37))}, {"domains",picker->Get("domains")},
        {"weapon",picker->Get("weapon")}, {"equipment",picker->Get("equipment")}, {"psi",picker->Get("psi")}
    });
}

gse::Value* Surface::WorkshopRenameText(GSE_CALLABLE) {
    if(!m_workshop_rename_text) {
        const auto read=[](const std::string& name){std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
        const auto source=read("script.txt");const auto labels=loader::txt::TXTLoader::ReadUniqueSection(read("labels.txt"),"LABELS");
        if(source.size()>2*1024*1024 || labels.size()<=103) {throw std::invalid_argument("Incomplete Workshop Rename text source");}
        std::array<std::string,4> text;bool inside=false,found=false;
        for(const auto& raw:util::String::Split(source,'\n')) {
            const auto line=util::String::TrimCopy(raw);if(line.empty() || line.front()==';') {continue;}
            if(line.front()=='#' && line.find_first_of(" \t")==std::string::npos) {
                inside=line=="#DESIGNNAME";if(inside) {if(found) {throw std::invalid_argument("Repeated DESIGNNAME section");}found=true;}
            }else if(inside) {
                if(line.compare(0,9,"#caption ")==0) {text[0]=line.substr(9);}
                else if(line.front()!='#') {if(!text[1].empty()) {text[1]+='\n';}text[1]+=line;}
            }
        }
        text[2]=labels[102];text[3]=labels[103];
        for(const auto& item:text) {if(item.empty() || item.size()>256 || item.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop Rename label");}}
        m_workshop_rename_text=std::move(text);
    }
    auto* parent=static_cast<gse::value::Object*>(WorkshopParentText(GSE_CALL));const auto& text=*m_workshop_rename_text;
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"title",VALUE(gse::value::String,,text[0])}, {"label",VALUE(gse::value::String,,text[1])},
        {"ok",VALUE(gse::value::String,,text[2])}, {"cancel",VALUE(gse::value::String,,text[3])}, {"colors",parent->Get("colors")}
    });
}

gse::Value* Surface::WorkshopUpgradeText(GSE_CALLABLE,int32_t kind,const std::array<std::string,2>& names,const std::array<int32_t,3>& numbers) {
    if(!m_workshop_upgrade_text) {
        const std::array<std::string,11> tags{{"WHICHUPGRADE2","UPGRADEPROTO","UPGRADEPROTO2","UPGRADEPROTOCOST","UPGRADEPROTOCOST2","YESUPGRADE","NOUPGRADE","OVERWRITEPROTO","INTRINSICTYPE","INTRINSICNAME","NOPRESET"}};
        const auto read=[](const std::string& file){std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(file));return std::string(bytes.begin(),bytes.end());};
        const auto labels=loader::txt::TXTLoader::ReadUniqueSection(read("labels.txt"),"LABELS");
        if(labels.size()<=103) {throw std::invalid_argument("Incomplete Workshop Upgrade button labels");}
        const auto script=read("script.txt");std::array<std::string,46> text;
        for(size_t i=0;i<tags.size();++i) {const auto parsed=ReadSmacWorkshopModalTemplate(script,tags[i]);text[4*i]=parsed.caption;text[4*i+1]=parsed.body;
            for(size_t row=0;row<parsed.rows.size();++row) {text[4*i+2+row]=parsed.rows[row];}}
        text[44]=labels[102];text[45]=labels[103];m_workshop_upgrade_text=std::move(text);
    }
    const auto& text=*m_workshop_upgrade_text;const size_t first=4*size_t(kind);
    const auto body=SubstituteSmacWorkshopModal(text.at(first+1),names,numbers);
    gse::value::array_elements_t rows,tokens;
    for(size_t i=0;i<2;++i) {if(!text.at(first+2+i).empty()) {rows.push_back(VALUE(gse::value::String,,SubstituteSmacWorkshopModal(text.at(first+2+i),names,numbers)));}}
    for(const auto& word:SmacWorkshopModalWords(body)) {
        gse::value::array_elements_t spans;
        for(const auto& span:word.spans) {spans.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{
            {"text",VALUE(gse::value::String,,span.text)}, {"color",VALUE(gse::value::Int,,span.emphasis ? 157:226)}
        }));}
        tokens.push_back(VALUE(gse::value::Object,,GSE_CALL_NOGC,{{"text",VALUE(gse::value::String,,word.text)},{"spans",VALUE(gse::value::Array,,spans)}}));
    }
    auto* parent=static_cast<gse::value::Object*>(WorkshopParentText(GSE_CALL));
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"title",VALUE(gse::value::String,,text.at(first))}, {"body",VALUE(gse::value::String,,body)}, {"tokens",VALUE(gse::value::Array,,tokens)},
        {"rows",VALUE(gse::value::Array,,rows)}, {"ok",VALUE(gse::value::String,,text[44])},{"cancel",VALUE(gse::value::String,,text[45])}, {"colors",parent->Get("colors")}
    });
}
gse::Value* Surface::WorkshopRetireText(GSE_CALLABLE,const std::string& name,int32_t count) {
    if(!m_workshop_retire_text) {
        const auto read=[](const std::string& file){std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(file));return std::string(bytes.begin(),bytes.end());};
        const auto source=read("script.txt");const auto labels=loader::txt::TXTLoader::ReadUniqueSection(read("labels.txt"),"LABELS");
        if(source.size()>2*1024*1024 || labels.size()<=102) {throw std::invalid_argument("Incomplete Workshop Retire text source");}
        std::array<std::string,9> text;
        for(size_t variant=0;variant<2;++variant) {
            const std::string tag=variant ? "#RETIRETYPE":"#RETIRETYPE2";bool inside=false,found=false;size_t block=0;
            for(const auto& raw:util::String::Split(source,'\n')) {
                const auto line=util::String::TrimCopy(raw);
                if(line.empty()) {if(inside && block==0 && !text[variant*4+1].empty()) {block=1;}continue;}
                if(line.front()==';' || line.compare(0,2,"##")==0) {continue;}
                if(line.front()=='#' && line.find_first_of(" \t")==std::string::npos) {
                    inside=line==tag;if(inside) {if(found) {throw std::invalid_argument("Repeated Workshop Retire template");}found=true;}
                }else if(inside) {
                    if(line.compare(0,9,"#caption ")==0) {text[variant*4]=line.substr(9);}
                    else if(line.front()!='#') {
                        if(block>2) {throw std::invalid_argument("Unexpected Workshop Retire template rows");}
                        auto& part=text[variant*4+1+block];if(!part.empty()) {part+=' ';}part+=line;
                        if(block) {++block;}
                    }
                }
            }
        }
        // Original LABELS byte offset194 hex is entry101; the text loader
        // retains the count at index0, hence array102 (not decimal194/Ocean).
        text[8]=labels[102];
        for(const auto& item:text) {if(item.empty() || item.size()>1024 || item.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop Retire label");}}
        m_workshop_retire_text=std::move(text);
    }
    // Substitute original template tokens once; never reinterpret a user name
    // containing dollar signs as another template expression.
    const auto substitute=[&](const std::string& source) {
        std::string result;
        for(size_t i=0;i<source.size();) {
            if(source.compare(i,10,"$UNITTYPE0")==0) {result+=name;i+=10;}
            else if(source.compare(i,5,"$NUM0")==0) {result+=std::to_string(count);i+=5;}
            else if(source.compare(i,5,"$<#0:")==0) {
                const auto middle=source.find(':',i+5),end=source.find('>',i+5);
                if(middle==std::string::npos || end==std::string::npos || middle>end) {throw std::invalid_argument("Invalid Workshop Retire plural token");}
                result+=count==1 ? source.substr(i+5,middle-i-5):source.substr(middle+1,end-middle-1);i=end+1;
            }else {result+=source[i++];}
        }return result;
    };
    auto* parent=static_cast<gse::value::Object*>(WorkshopParentText(GSE_CALL));const auto& text=*m_workshop_retire_text;const size_t first=count ? 4:0;
    gse::value::array_elements_t rows{VALUE(gse::value::String,,substitute(text[first+2])),VALUE(gse::value::String,,substitute(text[first+3]))};
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"title",VALUE(gse::value::String,,text[first])}, {"body",VALUE(gse::value::String,,substitute(text[first+1]))},
        {"rows",VALUE(gse::value::Array,,rows)}, {"ok",VALUE(gse::value::String,,text[8])}, {"colors",parent->Get("colors")}
    });
}
gse::Value* Surface::WorkshopAbilityText(GSE_CALLABLE,const std::string& technology) {
    if(!m_workshop_ability_text) {
        const auto read=[](const std::string& name) {std::vector<unsigned char> bytes;
            util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
        const auto labels=loader::txt::TXTLoader::ReadUniqueSection(read("labels.txt"),"LABELS");
        const auto script=read("script.txt");if(script.size()>2*1024*1024 || labels.size()<711) {throw std::invalid_argument("Incomplete Workshop ability text source");}
        const auto section=[&](const std::string& tag) {
            std::array<std::string,2> result;bool inside=false,found=false;
            for(const auto& raw:util::String::Split(script,'\n')) {
                const auto line=util::String::TrimCopy(raw);if(line.empty() || line.front()==';') {continue;}
                if(line.front()=='#' && line.find_first_of(" \t")==std::string::npos) {
                    inside=line=="#"+tag;if(inside) {if(found) {throw std::invalid_argument("Repeated Workshop SCRIPT section");}found=true;}
                } else if(inside) {
                    if(line.compare(0,9,"#caption ")==0) {result[0]=line.substr(9);}
                    else if(line.front()!='#') {if(!result[1].empty()) {result[1]+='\n';}result[1]+=line;}
                }
            }
            if(!found || result[0].empty() || result[0].size()>256 || result[1].size()>4096) {throw std::invalid_argument("Invalid Workshop SCRIPT template");}
            return result;
        };
        const auto chooser=section("UNITSPECIAL"),second=section("TWOSPECIAL"),missile=section("MISSILESPECIAL");
        std::array<std::string,12> text{chooser[0],labels[137],labels[102],labels[103],labels[89],labels[125],labels[86],labels[710],second[0],second[1],missile[0],missile[1]};
        for(const auto& item:text) {if(item.empty() || item.find('\0')!=std::string::npos) {throw std::invalid_argument("Missing Workshop ability label");}}
        m_workshop_ability_text=std::move(text);
    }
    const auto& text=*m_workshop_ability_text;auto second=text[9];size_t at=0;
    while((at=second.find("$TECH0",at))!=std::string::npos) {second.replace(at,6,technology);at+=technology.size();}
    auto* picker=static_cast<gse::value::Object*>(WorkshopPickerText(GSE_CALL));
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"title",VALUE(gse::value::String,,text[0])}, {"none",VALUE(gse::value::String,,text[1])},
        {"ok",VALUE(gse::value::String,,text[2])}, {"cancel",VALUE(gse::value::String,,text[3])},
        {"cost",VALUE(gse::value::String,,text[4])}, {"attack",VALUE(gse::value::String,,text[5])},
        {"defense",VALUE(gse::value::String,,text[6])}, {"speed",VALUE(gse::value::String,,text[7])},
        {"second_title",VALUE(gse::value::String,,text[8])}, {"second_body",VALUE(gse::value::String,,second)},
        {"missile_title",VALUE(gse::value::String,,text[10])}, {"missile_body",VALUE(gse::value::String,,text[11])},
        {"domains",picker->Get("domains")}
    });
}

gse::Value* Surface::WorkshopPickerText(GSE_CALLABLE) {
    if(!m_workshop_picker_text) {
        const auto read=[](const std::string& name) {std::vector<unsigned char> bytes;
            util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));return std::string(bytes.begin(),bytes.end());};
        const auto domains=loader::txt::TXTLoader::ReadUniqueSection(read("alpha.txt"),"TRIAD");
        const auto labels=loader::txt::TXTLoader::ReadUniqueSection(read("labels.txt"),"LABELS");
        if(domains.size()!=3 || labels.size()<198) {throw std::invalid_argument("Incomplete Workshop picker text source");}
        std::array<std::string,8> text{domains[0],domains[1],domains[2],labels[122],labels[127],labels[197],labels[130],labels[129]};
        for(const auto& item:text) {if(item.empty() || item.size()>128 || item.find('\0')!=std::string::npos) {throw std::invalid_argument("Invalid Workshop picker label");}}
        m_workshop_picker_text=std::move(text);
    }
    const auto& text=*m_workshop_picker_text;
    return VALUE(gse::value::Object,,GSE_CALL_NOGC,{
        {"domains",VALUE(gse::value::Array,,gse::value::array_elements_t{VALUE(gse::value::String,,text[0]),VALUE(gse::value::String,,text[1]),VALUE(gse::value::String,,text[2])})},
        {"power",VALUE(gse::value::String,,text[3])}, {"shielding",VALUE(gse::value::String,,text[4])}, {"psi",VALUE(gse::value::String,,text[5])},
        {"weapon",VALUE(gse::value::String,,text[6])}, {"equipment",VALUE(gse::value::String,,text[7])}
    });
}

void Surface::EnsureWorkshopComponents() {
        if(!m_workshop_components) {
            const auto read=[](const std::string& name) {
                std::vector<unsigned char> bytes;util::FS::ReadFile(bytes,g_engine->GetResourceManager()->GetCustomPath(name));
                return std::string(bytes.begin(),bytes.end());
            };
            m_workshop_components=std::make_shared<const loader::cvr::CvrWorkshopComponents>(loader::cvr::CvrWorkshopComponents::Load(read));
        }
 }

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
