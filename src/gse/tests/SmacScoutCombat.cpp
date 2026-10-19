#include "graphics/opengl/OpenGL.h"
#include "scene/actor/Actor.h"
#include "graphics/opengl/shader_program/Orthographic.h"
#include "util/FinallyGuard.h"
#include "SmacScoutCombat.h"
#include <cstdlib>
#include "loader/texture/SmacEffect.h"
#include "game/ScoutBattlePresentation.h"
#include "game/SmacHealthbar.h"
#include "game/SmacUnitOrder.h"
#include "game/frontend/base/SmacPopulationBadge.h"
#include "game/frontend/unit/BadgeDefs.h"
#include "game/frontend/unit/SmacOrderGlyph.h"
#include "game/backend/map/Consts.h"
#include "game/frontend/sprite/InstancedSpriteManager.h"
#include "scene/Scene.h"
#include "scene/actor/Instanced.h"
#include "scene/actor/Sprite.h"
#include "types/texture/Texture.h"
#include "game/FrontendRequest.h"

#include "game/backend/unit/SmacScoutCombat.h"
#include "fixtures/SmacScoutCombat.h"
#include "fixtures/SmacEffectEscape.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <limits>
#include <stdexcept>
#include <string>

namespace gse::tests {
namespace {
namespace model = game::backend::unit::smac_scout;
namespace fixture = fixtures::smac_scout;
using game::backend::SmacGameplayRandomState;
constexpr model::UnitId ATTACKER = UINT64_C( 0xfedcba9876543210 );
constexpr model::UnitId DEFENDER = UINT64_C( 0x123456789abcdef0 );

model::Participant Participant( const fixture::Input& i, const size_t side, const model::UnitId id ) {
	return { id, i.reactors[ side ], i.health[ side ], i.morale[ side ], i.morale[ side ], side == 0 ? i.spent : 0 };
}
model::AttackResult Resolve( const fixture::Input& i ) {
	return model::ResolveNeutralFlatAttack( Participant( i, 0, ATTACKER ), Participant( i, 1, DEFENDER ), i.allowance, { i.shared, i.crt }, { ATTACKER, DEFENDER } );
}
bool Rejects( const std::function< void() >& f ) {
	try { f(); } catch ( const std::invalid_argument& ) { return true; }
	return false;
}
std::optional< model::UnitId > SurvivingReference( const int original_index, const model::UnitId survivor ) {
	// The original compacted roster has exactly one survivor. Translate the
	// original reference through that roster, never through a stale index or
	// the separate continuation reference. Invalid fixture indices fail loudly.
	if ( original_index == -1 ) { return std::nullopt; }
	if ( original_index != 0 ) { throw std::logic_error( "invalid singleton fixture index" ); }
	return survivor;
}
bool Singleton( const model::Survivor& s ) { return !s.stack_links[ 0 ] && !s.stack_links[ 1 ]; }
bool StepMatches( const fixture::Step& e, const model::Survivor& s, const model::References& refs, const model::UnitId continuation, const SmacGameplayRandomState& rng, const std::array< bool, 2 >& present, const int original_return ) {
	return s.identity == ATTACKER && s.health == e.health && s.stored_morale == e.morale && s.spent == e.spent &&
		s.position == ( e.position == 0 ? model::Position::Source : model::Position::Destination ) && Singleton( s ) &&
		refs.selected == SurvivingReference( e.selected_index, ATTACKER ) && refs.secondary == SurvivingReference( e.secondary_index, ATTACKER ) &&
		continuation == *SurvivingReference( e.continuation_index, ATTACKER ) && rng.Shared() == e.shared && rng.Crt() == e.crt && present == e.present && original_return == e.wrapper_return;
}
}

void AddSmacScoutCombatTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC order glyph retains native center baseline and neighboring badge pixels",GT(){
        GT_ASSERT(game::SmacOrderForegroundIndex("SPARTANS")==44 && game::SmacOrderForegroundIndex("UNIV")==0
            && game::SmacOrderForegroundIndex("spartans")==0,"Order foreground used faction slot or a case-folded filename");
        types::Font font("original order metrics");
        for(const char letter:std::string("H-fRFSM")) {
            auto& glyph=font.m_symbols[static_cast<unsigned char>(letter)];
            glyph={letter=='-'?4:8,0,1,1,0,9,static_cast<unsigned char*>(malloc(1))};if(!glyph.data){throw std::bad_alloc();}glyph.data[0]=255;
        }
        const auto white=types::Color::FromRGB(255,255,255),black=types::Color::FromRGB(0,0,0),green=types::Color::FromRGB(0,255,0);
        types::texture::Texture badge(23,30);badge.Fill(0,0,22,29,white);badge.SetPixel(6,6,green);
        game::frontend::unit::PaintSmacOrderGlyph(badge,font,'H',black);
        GT_ASSERT(badge.GetPixel(11,7)==black.GetRGBA() && badge.GetPixel(6,6)==green.GetRGBA() && badge.GetPixel(10,7)==white.GetRGBA(),"H lost measured x11/baseline16 or altered health backing");
        game::frontend::unit::PaintSmacOrderGlyph(badge,font,'-',black);
        GT_ASSERT(badge.GetPixel(13,7)==black.GetRGBA() && badge.GetWidth()==23 && badge.GetHeight()==30,"Dash centering or flag geometry changed");
        auto& glyph=font.m_symbols['H'];glyph.width=129;
        GT_ASSERT(Rejects([&]{game::frontend::unit::PaintSmacOrderGlyph(badge,font,'H',black);}),"Unbounded order glyph bitmap accepted");
        glyph.width=1;glyph.left=INT32_MIN;glyph.top=INT32_MAX;
        game::frontend::unit::PaintSmacOrderGlyph(badge,font,'H',black);
        GT_ASSERT(badge.GetPixel(6,6)==green.GetRGBA(),"Extreme bearings wrote outside glyph bounds");GT_OK();
    });
    task->AddTest("SMAC population badge admits resolved stock metadata and preserves mod colors",GT(){
        std::array<types::Color,256> palette{};
        palette[241]=types::Color::FromRGB(0,252,0);palette[32]=types::Color::FromRGB(45,72,41);
        palette[242]=types::Color::FromRGB(16,228,0);palette[255]=types::Color::FromRGB(255,255,255);
        types::texture::Texture asset(162,764);
        asset.SetPixel(4,739,palette[241]);asset.SetPixel(4,747,palette[32]);asset.SetPixel(4,755,palette[242]);
        asset.SetPixel(4,763,palette[32]);asset.SetPixel(161,749,palette[241]);
        const std::array<types::Color,3> configured{types::Color::FromRGBA(asset.GetPixel(4,755)),
            types::Color::FromRGBA(asset.GetPixel(4,763)),types::Color::FromRGBA(asset.GetPixel(161,749))};
        const auto admitted=[&]{return game::frontend::base::MatchesSmacPopulationMetadata(asset,palette,241,32,242,configured);};
        GT_ASSERT(admitted(),"Measured stock metadata was rejected");
        for(const auto y:{739,747,755}) {
            const auto saved=asset.GetPixel(4,y);asset.SetPixel(4,y,types::Color::FromRGB(123,45,67));
            GT_ASSERT(!admitted(),"Replacement faction color was mistaken for stock metadata");asset.SetPixel(4,y,saved);
        }
        for(size_t field=0;field<configured.size();++field) {
            auto changed=configured;changed[field]=types::Color::FromRGB(123,45,67);
            GT_ASSERT(!game::frontend::base::MatchesSmacPopulationMetadata(asset,palette,241,32,242,changed),
                "Scripted faction colors were replaced by stock styling");
        }
        types::texture::Texture short_asset(5,755);
        GT_ASSERT(!game::frontend::base::MatchesSmacPopulationMetadata(short_asset,palette,241,32,242,configured),
            "Small custom artwork admitted out-of-bounds color metadata");
        GT_ASSERT(game::frontend::base::SmacPopulationDisplayColor(palette,241).GetRGBA()==types::Color::FromRGB(0,235,0).GetRGBA()
            && game::frontend::base::SmacPopulationDisplayColor(palette,255).GetRGBA()==palette[255].GetRGBA(),
            "Software common-color cap changed reserved white or stock green");
        for(int channel=0;channel<=255;++channel) {
            palette[155]=types::Color::FromRGB(channel,channel,channel);
            const auto expected=types::Color::RGBA(std::min(channel,235),std::min(channel,235),std::min(channel,235),255);
            GT_ASSERT(game::frontend::base::SmacPopulationDisplayColor(palette,155).GetRGBA()==expected,
                "Common palette clamp lost an unchanged channel byte");
        }
        GT_OK();
    });
    task->AddTest("SMAC population badge preserves original one and two digit layout",GT(){
        types::Font font("population layout fixture");
        // Pin the measured original advance and baseline independently of host fonts.
        for(const char digit:std::string("0189")) {
            auto& glyph=font.m_symbols[static_cast<unsigned char>(digit)];
            glyph={9,0,1,1,0,12,static_cast<unsigned char*>(malloc(1))};
            if(!glyph.data) {throw std::bad_alloc();}glyph.data[0]=255;
        }
        const auto black=types::Color::FromRGB(0,0,0),white=types::Color::FromRGB(255,255,255);
        const auto clear=types::Color::FromRGBA(0,0,0,0);
        auto nine=game::frontend::base::MakeSmacPopulationBadge(9,font,clear,white,black);
        auto ten=game::frontend::base::MakeSmacPopulationBadge(10,font,white,black,black);
        GT_ASSERT(nine->GetWidth()==14 && ten->GetWidth()==23 && nine->GetHeight()==16 && ten->GetHeight()==16,
            "Original placeholder8/88 width or fixed16 pixel height changed");
        GT_ASSERT(nine->GetPixel(3,2)==white.GetRGBA() && ten->GetPixel(3,2)==black.GetRGBA()
            && ten->GetPixel(12,2)==black.GetRGBA(),"Digits lost left padding baseline or glyph advance");
        GT_ASSERT(nine->GetPixel(2,2)==clear.GetRGBA() && nine->GetPixel(4,3)==clear.GetRGBA()
            && ten->GetPixel(1,1)==white.GetRGBA(),"Transparent/filled interior or absent shadow changed");
        for(auto* texture:{nine.get(),ten.get()}) {
            for(size_t x=0;x<texture->GetWidth();++x) {GT_ASSERT(texture->GetPixel(x,0)==black.GetRGBA()
                && texture->GetPixel(x,15)==black.GetRGBA(),"Population horizontal border is not one pixel");}
            for(size_t y=0;y<16;++y) {GT_ASSERT(texture->GetPixel(0,y)==black.GetRGBA()
                && texture->GetPixel(texture->GetWidth()-1,y)==black.GetRGBA(),"Population vertical border is not one pixel");}
        }
        auto& zero=font.m_symbols['0'];free(zero.data);zero.width=9;zero.data=static_cast<unsigned char*>(malloc(9));
        if(!zero.data) {throw std::bad_alloc();}std::fill_n(zero.data,9,255);
        auto hundred=game::frontend::base::MakeSmacPopulationBadge(100,font,white,black,black);
        GT_ASSERT(hundred->GetWidth()==30 && hundred->GetPixel(22,1)==black.GetRGBA()
            && hundred->GetPixel(23,1)==clear.GetRGBA() && hundred->GetPixel(29,2)==black.GetRGBA(),
            "Third population digit was clipped or native two-digit frame expanded");
        GT_OK();
    });
    task->AddTest("SMAC health bar renderer preserves indexed colors geometry cache and retirement",GT(){
        scene::Scene scene("Health bar fixture",scene::SCENE_TYPE_WORLD);
        game::frontend::sprite::InstancedSpriteManager sprites(&scene);
        {
            game::frontend::unit::BadgeDefs badges(&sprites);
            auto* yellow=badges.GetBadgeHealthbarSprite(0.7f,10);
            auto* same=badges.GetBadgeHealthbarSprite(0.7f,20);
            auto* green=badges.GetBadgeHealthbarSprite(0.7f,30);
            types::texture::Texture flag_pixels(23,30);
            const auto& map=game::backend::map::s_consts;
            const float flag_width=map.tile.scale.x*0.25f;
            const float flag_height=map.tile.scale.y*map.sprite.y_scale*0.5f;
            auto* flag=sprites.GetInstancedSprite("Health alignment flag",&flag_pixels,{0,0},{23,30},{11,15},
                {flag_width,flag_height},game::frontend::ZL_UNITS);
            const auto flag_center=flag->actor->GetSpriteActor()->NormalizePosition(
                game::frontend::unit::BadgeDefs::GetBadgeCoords({0,0,0}));
            const auto* bar_actor=yellow->instanced_sprite->actor->GetSpriteActor();
            const auto bar_center=bar_actor->NormalizePosition(
                game::frontend::unit::BadgeDefs::GetBadgeHealthbarCoords({0,0,0},true));
            const auto bar_size=bar_actor->GetDimensions();
            const float dx=(bar_center.x-bar_size.x/2-(flag_center.x-flag_width/2))*23/flag_width;
            const float dy=(bar_center.y-bar_size.y/2-(flag_center.y-flag_height/2))*30/flag_height;
            GT_ASSERT(std::abs(dx-6)<0.001f && std::abs(dy-6)<0.001f,
                "Actual actor rectangles do not place the bar at flag source pixel6,6");
            sprites.RemoveInstancedSpriteByKey(flag->key);
            GT_ASSERT(yellow==same && yellow!=green,"Equal heights conflated distinct damage-threshold colors");
            const auto check=[&](game::frontend::sprite::Sprite* sprite,uint32_t ink,int filled)->bool {
                const auto* texture=sprite->instanced_sprite->actor->GetSpriteActor()->GetTexture();
                if(texture->GetWidth()!=2 || texture->GetHeight()!=21) {return false;}
                for(size_t y=0;y<21;++y) {for(size_t x=0;x<2;++x) {
                    if(texture->GetPixel(x,y)!=(y<size_t(filled) ? ink:types::Color::FromRGB(0,0,0).GetRGBA())) {return false;}
                }}return true;
            };
            GT_ASSERT(check(yellow,types::Color::FromRGB(255,255,0).GetRGBA(),14)
                && check(green,types::Color::FromRGB(0,255,0).GetRGBA(),14)
                && check(badges.GetBadgeHealthbarSprite(0.0f,10),0,0),
                "Actual health textures changed source width height fill direction opacity or palette colors");
            GT_ASSERT(badges.GetBadgeHealthbarSprite(0.7f)!=yellow,"Legacy ratio cache aliased a canonical bar");
        }
        bool empty=false;scene.WithActors([&](const auto& actors){empty=actors.empty();});
        GT_ASSERT(empty,"Retiring health badges left scene actors with retired textures");GT_OK();
    });
    task->AddTest("SMAC health bar matches original Fission Fusion and clamped reactor draws",GT(){
        using Color=game::SmacHealthbar::Color;
        struct Case {int32_t maximum,damage;uint8_t filled;Color color;};
        // RE-SMAC-MAP-OVERLAYS-001: original fill rectangles at y106.
        const Case cases[]={{10,0,21,Color::Green},{10,2,16,Color::Green},{10,3,14,Color::Yellow},
            {10,5,10,Color::Yellow},{10,6,8,Color::Red},{10,9,2,Color::Red},{10,10,0,Color::Red},
            {20,5,15,Color::Green},{20,6,14,Color::Yellow},{20,12,8,Color::Yellow},
            {20,13,7,Color::Red},{20,19,1,Color::Red},{1000,254,15,Color::Green}};
        for(const auto& row:cases) {
            const auto actual=game::CalculateSmacHealthbar(float(row.maximum-row.damage)/row.maximum,row.maximum);
            GT_ASSERT(actual.filled==row.filled && actual.color==row.color,"Original health fill/threshold differs");
        }
        GT_ASSERT(Rejects([]{game::CalculateSmacHealthbar(0.5f,0);}) &&
            Rejects([]{game::CalculateSmacHealthbar(std::numeric_limits<float>::quiet_NaN(),10);}),
            "Health renderer silently accepted missing or nonfinite authority");
        GT_OK();
    });
    if(std::getenv("GLSMAC_TEST_SCOUT_GPU")) {task->AddTest("SMAC Scout GPU indexed compositor matches every ordinary background and source row",GT(){
        namespace fx=loader::texture::smac_effect;
        GT_ASSERT(SDL_InitSubSystem(SDL_INIT_VIDEO)==0,"GPU fixture could not initialize video");
        util::FinallyGuard video([]{SDL_QuitSubSystem(SDL_INIT_VIDEO);});
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
        auto* window=SDL_CreateWindow("Scout GPU control",0,0,256,17,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
        GT_ASSERT(window,"GPU fixture could not create a hidden window");util::FinallyGuard destroy([&]{SDL_DestroyWindow(window);});
        const auto context=SDL_GL_CreateContext(window);GT_ASSERT(context,"GPU fixture could not create GL3.3 context");
        util::FinallyGuard destroy_context([&]{SDL_GL_DeleteContext(context);});glewExperimental=GL_TRUE;
        GT_ASSERT(glewInit()==GLEW_OK,"GPU fixture could not initialize GL loader");while(glGetError()!=GL_NO_ERROR) {}
        class Probe:public graphics::opengl::shader_program::Orthographic {public:GLuint Program() const {return m_gl_shader_program;}} shader;
        shader.Start();util::FinallyGuard stop([&]{GLint current=0;glGetIntegerv(GL_CURRENT_PROGRAM,&current);if(current) {glUseProgram(0);}shader.Stop();});const auto program=shader.Program();
        GLuint vao=0,vbo=0,textures[2]{};glGenVertexArrays(1,&vao);glBindVertexArray(vao);glGenBuffers(1,&vbo);glBindBuffer(GL_ARRAY_BUFFER,vbo);for(auto& texture:textures) {glGenTextures(1,&texture);}
        util::FinallyGuard buffers([&]{GLint current=0;glGetIntegerv(GL_CURRENT_PROGRAM,&current);if(current) {glUseProgram(0);}glActiveTexture(GL_TEXTURE0);GLint bound=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);if(bound) {glBindTexture(GL_TEXTURE_2D,0);}glBindBuffer(GL_ARRAY_BUFFER,0);glBindVertexArray(0);for(auto& texture:textures) {glDeleteTextures(1,&texture);}glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);});
        const float vertices[]{-1,-1,0,0,0,1,-1,0,1,0,-1,1,0,0,17.0f/35,1,1,0,1,17.0f/35};
        glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices,GL_STATIC_DRAW);glUseProgram(program);
        const auto coordinate=glGetAttribLocation(program,"aCoord"),uv=glGetAttribLocation(program,"aTexCoord");
        glEnableVertexAttribArray(coordinate);glVertexAttribPointer(coordinate,3,GL_FLOAT,GL_FALSE,5*sizeof(float),nullptr);
        glEnableVertexAttribArray(uv);glVertexAttribPointer(uv,2,GL_FLOAT,GL_FALSE,5*sizeof(float),reinterpret_cast<void*>(3*sizeof(float)));
        glVertexAttrib4f(glGetAttribLocation(program,"aTintColor"),1,1,1,1);glVertexAttrib3f(glGetAttribLocation(program,"aNormal"),0,0,1);
        glUniform1i(glGetUniformLocation(program,"uPageColumns"),0);glViewport(0,0,256,17);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        graphics::opengl::OpenGL capture("GPU control",256,17,false,false);util::FinallyGuard release([&]{capture.ReleaseEffectBackground();});
        fx::Palette palette{};for(size_t i=0;i<256;++i) {palette[i]={uint8_t(i),uint8_t(i),uint8_t(i)};}
        fx::ApplySmacSoftwareReservedColors(palette);
        fx::Rows rows{};for(size_t i=0;i<16;++i) {rows[i]={49,int(i*100/15)};}const auto bank=fx::Generate(palette,rows);
        std::vector<uint8_t> background(256*17*4),atlas(256*35*4),output(256*17*4);
        for(size_t y=0;y<17;++y) {for(size_t x=0;x<256;++x) {
            for(size_t c=0;c<3;++c) {background[(y*256+x)*4+c]=palette[x][c];}background[(y*256+x)*4+3]=255;
            atlas[(y*256+x)*4]=y==16 ? 252:y;atlas[(y*256+x)*4+3]=(x==15 || x==253) ? 0:255;
            atlas[((y+17)*256+x)*4]=(y==16 ? fx::SmacSoftwareStartupEscape(x):bank[y*256+x]);atlas[((y+17)*256+x)*4+3]=255;
        }}
        for(size_t x=0;x<256;++x) {for(size_t c=0;c<3;++c) {atlas[(34*256+x)*4+c]=palette[x][c];}atlas[(34*256+x)*4+3]=255;}
        const auto upload=[&](GLuint texture,int height,const uint8_t* pixels) {
            GLint bound=0;glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);if(bound) {glBindTexture(GL_TEXTURE_2D,0);}
            glBindTexture(GL_TEXTURE_2D,texture);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,256,height,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        };
        glActiveTexture(GL_TEXTURE0);upload(textures[0],17,background.data());
        glUniform1ui(glGetUniformLocation(program,"uFlags"),scene::actor::Actor::RF_IGNORE_CAMERA|scene::actor::Actor::RF_IGNORE_LIGHTING);glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        const auto origin=capture.PrepareEffectBackground();glUniform2i(glGetUniformLocation(program,"uEffectOrigin"),origin.x,origin.y);
        glActiveTexture(GL_TEXTURE0);upload(textures[1],35,atlas.data());
        glUniform1ui(glGetUniformLocation(program,"uFlags"),scene::actor::Actor::RF_IGNORE_CAMERA|scene::actor::Actor::RF_IGNORE_LIGHTING|scene::actor::Actor::RF_SMAC_EFFECT);glDrawArrays(GL_TRIANGLE_STRIP,0,4);
        glReadPixels(0,0,256,17,GL_RGBA,GL_UNSIGNED_BYTE,output.data());GT_ASSERT(glGetError()==GL_NO_ERROR,"GPU compositor/capture generated a GL error");
        for(size_t y=0;y<17;++y) {for(size_t x=10;x<=245;++x) {
            const auto expected=x==15 ? uint8_t(x):(y==16 ? fixtures::smac_effect_escape[x]:bank[y*256+x]);
            GT_ASSERT(output[(y*256+x)*4]==palette[expected][0] && output[(y*256+x)*4+1]==palette[expected][1] && output[(y*256+x)*4+2]==palette[expected][2] && output[(y*256+x)*4+3]==255,
                "GPU effect differs from indexed source/background lookup at "+std::to_string(y)+"/"+std::to_string(x));
        }}GT_OK();
    });}

    task->AddTest("SMAC Scout indexed effect compositor matches original numeric CLUT controls",GT(){
        namespace fx=loader::texture::smac_effect;
        fx::Palette palette{};for(size_t i=0;i<256;++i) {palette[i]={uint8_t(i),uint8_t(i),uint8_t(i)};}
        fx::Rows rows{};rows.fill({49,0});rows[0]={49,100};rows[1]={49,40};
        const auto bank=fx::Generate(palette,rows);
        const std::array<uint8_t,5> source{{0,1,2,253,255}},background{{44,100,200,249,255}},expected{{59,83,200,249,255}};
        for(size_t i=0;i<source.size();++i) {GT_ASSERT(fx::Composite(bank,source[i],background[i])==expected[i],"Indexed blend differs from the original asset-free compositor");}
        palette[200]={199,199,199};const auto duplicate=fx::Generate(palette,rows);
        GT_ASSERT(fx::Composite(duplicate,2,200)==199,"Zero opacity bypassed original nearest-color tie quantization");
        const auto protected_bank=fx::Generate(palette,rows,{{59,59}});
        GT_ASSERT(fx::Composite(protected_bank,0,59)==59 && fx::Composite(protected_bank,0,44)==58,"Protected effect colors were altered or considered nearest candidates");
        for(int row=0;row<16;++row) {for(int i=0;i<10;++i) {GT_ASSERT(bank[row*256+i]==i && bank[row*256+246+i]==246+i,"Generated CLUT rewrote reserved destination indices");}}
        for(size_t i=0;i<256;++i) {GT_ASSERT(fx::SmacSoftwareStartupEscape(i)==fixtures::smac_effect_escape[i],"SMAC startup escape differs from original execution");}
        auto reserved=palette;fx::ApplySmacSoftwareReservedColors(reserved);
        GT_ASSERT((reserved[0]==std::array<uint8_t,3>{0,0,0} && reserved[1]==std::array<uint8_t,3>{128,0,0}
            && reserved[6]==std::array<uint8_t,3>{0,128,128} && reserved[255]==std::array<uint8_t,3>{255,255,255}
            && reserved[44]==palette[44]),"Software reserved colors changed ordinary colors or opaque escape RGB");
        GT_ASSERT(Rejects([&]{fx::Composite(bank,252,44);}),"Unknown original out-of-bank source was silently given a color");
        std::string system="; diagnostic\r\n#EXPLOSIONS3\r\n";
        for(int i=0;i<16;++i) {system+="49, 100, ; row\r\n";}
        const auto parsed=fx::ReadRows(system,3);
        GT_ASSERT(parsed[0].color==49 && parsed[15].opacity==100,"Installed CRLF/comment explosion rows did not bind");
        GT_ASSERT(Rejects([&]{fx::ReadRows(system,0);}) && Rejects([&]{fx::ReadRows(system+"49,100\n",3);}),"Missing or oversized explosion bank accepted");GT_OK();
    });

    task->AddTest("SMAC Scout playback matches original grouped effects and preserves late frames",GT(){
        namespace av=game::scout_presentation;
        const model::Participant a(ATTACKER,1,10,1,1,0),d(DEFENDER,1,10,1,1,0);
        const auto result=model::ResolveNeutralFlatAttack(a,d,3,{3,3},{ATTACKER,DEFENDER});
        const auto plan=av::Prepare(result);
        const std::array<std::array<int32_t,2>,19> damage{{{{0,1}},{{1,1}},{{1,2}},{{2,2}},{{2,3}},{{2,4}},{{3,4}},{{4,4}},{{5,4}},{{5,5}},{{5,6}},{{6,6}},{{7,6}},{{7,7}},{{7,8}},{{8,8}},{{9,8}},{{9,9}},{{9,10}}}};
        const std::array<size_t,7> effect_rounds{{5,7,11,12,17,18,19}},targets{{1,0,1,0,0,1,1}};
        GT_ASSERT(plan.steps.size()==19 && plan.units[0]==ATTACKER && plan.units[1]==DEFENDER,"Playback lost stable participants or original rounds");
        size_t effect=0;
        for(size_t i=0;i<damage.size();++i) {
            GT_ASSERT((plan.steps[i].health==std::array<int32_t,2>{{10-damage[i][0],10-damage[i][1]}}),"Playback health differs from original timeline");
            if(effect<effect_rounds.size() && i+1==effect_rounds[effect]) {
                GT_ASSERT(plan.steps[i].target==targets[effect] && plan.steps[i].effect==(effect==6 ? av::Effect::Death:av::Effect::Hit),"Playback grouped effect differs from original call");++effect;
            } else {GT_ASSERT(plan.steps[i].effect==av::Effect::None,"Playback invented an effect between original grouped hits");}
        }
        for(const bool fast:{false,true}) {for(const uint64_t tick:{uint64_t(25),uint64_t(100)}) {
            av::Playback playback(plan,fast);size_t health=0,frames=0,clears=0;bool finished=false;uint64_t ended=0;
            for(uint64_t now=0;now<20000 && !finished;now+=tick) {while(const auto action=playback.Next(now)) {
                if(action->kind==av::Playback::Kind::Health) {++health;}
                if(action->kind==av::Playback::Kind::Frame) {GT_ASSERT(action->frame==frames%16,"Late clock skipped or reordered an original frame");++frames;}
                if(action->kind==av::Playback::Kind::Clear) {++clears;}
                if(action->kind==av::Playback::Kind::Finished) {finished=true;ended=now;}
            }}
            GT_ASSERT(finished && health==19 && frames==112 && clears==7 && !playback.Next(30000),"Playback lost health, frame, tail or terminal actions");
            GT_ASSERT(ended==(fast ? (tick==25 ? 800u:1600u):(tick==25 ? 3550u:11900u)),"Playback changed original minimum waits or caught up late frames");
        }}
        game::FrontendRequest request(game::FrontendRequest::FR_SCOUT_BATTLE);request.data.scout_battle.plan=new av::Plan(plan);
        game::FrontendRequest copy(request);request.data.scout_battle.plan->steps[0].health[0]=0;
        GT_ASSERT(copy.data.scout_battle.plan!=request.data.scout_battle.plan && copy.data.scout_battle.plan->steps[0].health[0]==10,"Frontend transport aliased the owning battle timeline");
        GT_OK();
    });

	task->AddTest("SMAC Scout Base garrison matches original17 rounds and sequential bonuses",GT(){
        const model::Participant a(ATTACKER,1,10,1,1,0),d(DEFENDER,1,10,1,1,0);
        const auto result=model::ResolveFlatScoutAttack(a,d,3,{3,4},{ATTACKER,DEFENDER},false,{25,25,25});
        // Original execution receipt: damage before, two raw draws, their
        // modulo280 values, then CRT state after the pair.
        const std::array<std::array<uint32_t,7>,17> rounds{{
			{{0,0,51,17945,51,25,3323539102}},
			{{0,1,27159,386,279,106,2172805640}},
			{{0,2,17345,27504,265,64,1802518722}},
			{{0,3,20815,20576,95,136,1348511052}},
			{{1,3,10960,6020,40,140,394542630}},
			{{2,3,5261,28277,221,277,4000656848}},
			{{3,3,24162,930,82,90,2208475850}},
			{{4,3,11002,19737,82,137,1293524884}},
			{{5,3,17913,1682,273,2,110250670}},
			{{5,4,10319,10474,239,114,686481048}},
			{{5,5,1102,28279,262,279,1853301202}},
			{{6,5,16179,21197,219,197,1389170396}},
			{{6,6,11333,12790,133,190,838235190}},
			{{7,6,15811,2607,131,87,170890336}},
			{{7,7,16929,15514,129,114,1016776154}},
			{{7,8,14450,19601,170,1,3432059684}},
			{{7,9,23796,23159,276,199,3665287870}}
        }};
        GT_ASSERT(result.rounds.size()==17 && result.draws.size()==34 && (result.strengths==std::array<int32_t,2>{280,280}),"Base battle strength/draw count differs");
        for(size_t i=0;i<rounds.size();++i) {
            const auto& r=rounds[i];const auto& got=result.rounds[i];
            GT_ASSERT(got.health_before[0]==10-int(r[0]) && got.health_before[1]==10-int(r[1])
                && result.draws[2*i].raw==r[2] && result.draws[2*i+1].raw==r[3]
                && got.draws[0]==int(r[4]) && got.draws[1]==int(r[5]) && result.draws[2*i+1].state_after==r[6],"original Base round changed");
        }
        GT_ASSERT(result.victory==model::Victory::Attacker && result.survivor.health==3 && result.survivor.stored_morale==2
            && result.survivor.spent==3 && result.survivor.position==model::Position::Source && !result.references.secondary
            && result.promotion.winner_weight==84 && result.promotion.loser_weight==280 && result.random.Crt()==3665287870u
            && result.random.Shared()==3 && result.original_returns.wrapper_return==0,"Base aftermath or promotion differs");
        const auto fanatic=model::ResolveFlatScoutAttack(a,d,3,{3,4},{ATTACKER,DEFENDER},true,{25,25,25});
        GT_ASSERT((fanatic.strengths==std::array<int32_t,2>{350,280}),"FANATIC and Base stages were added instead of multiplied");
        const auto fractional=model::ResolveFlatScoutAttack(a,d,3,{3,4},{ATTACKER,DEFENDER},true,{17,19,23});
        GT_ASSERT((fractional.strengths==std::array<int32_t,2>{311,275}),"modded stages lost independent integer truncation");GT_OK();
    });
	task->AddTest( "SMAC Scout combat matches every original round draw and promotion", GT() {
		size_t round_total = 0, draw_total = 0, admission_draws = 0, parity_draws = 0;
		for ( const auto& c : fixture::CASES ) {
			const auto result = Resolve( c.input );
			GT_ASSERT( result.strengths == c.strengths && result.round_widths == c.widths, "strength/partial movement differs: " + std::string( c.id ) );
			GT_ASSERT( result.health_after == c.health_after, "final hit/health differs: " + std::string( c.id ) );
			GT_ASSERT( result.rounds.size() == c.round_count && result.draws.size() == c.draw_count, "draw suppression or lethal stop differs: " + std::string( c.id ) );
			for ( size_t i = 0; i < result.rounds.size(); ++i ) {
				const auto& actual = result.rounds[ i ]; const auto& expected = fixture::ROUNDS[ c.first_round + i ];
				GT_ASSERT( actual.health_before == expected.health_before && actual.draws == expected.draws, "round order/modulo/tie differs: " + std::string( c.id ) );
			}
			for ( size_t i = 0; i < result.draws.size(); ++i ) {
				const auto& actual = result.draws[ i ]; const auto& expected = fixture::DRAWS[ c.first_draw + i ];
				GT_ASSERT( static_cast< int >( actual.phase ) == expected.phase && actual.state_before == expected.before && actual.state_after == expected.after && actual.raw == expected.raw, "CRT phase/state/raw differs: " + std::string( c.id ) );
				admission_draws += actual.phase == model::DrawPhase::PromotionAdmission; parity_draws += actual.phase == model::DrawPhase::PromotionParity;
			}
			const auto& p = result.promotion;
			GT_ASSERT( p.winner_weight == c.weights[ 0 ] && p.loser_weight == c.weights[ 1 ] && p.admitted == c.admitted && p.stored_before == c.stored_before && p.stored_after == c.stored_after, "promotion differs: " + std::string( c.id ) );
			GT_ASSERT( result.random.Shared() == c.shared_after && result.random.Crt() == c.crt_after, "final random states differ: " + std::string( c.id ) );
			round_total += result.rounds.size(); draw_total += result.draws.size();
		}
		GT_ASSERT( round_total == 3073 && draw_total == 6194 && admission_draws == 67 && parity_draws == 31, "original transcript coverage changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC Scout normal returns preserve victory and stable reference meaning", GT() {
		for ( const auto& c : fixture::CASES ) {
			const auto result = Resolve( c.input );
			const auto expected_id = c.attacker_won ? ATTACKER : DEFENDER;
			const auto expected_dead = c.attacker_won ? DEFENDER : ATTACKER;
			const size_t side = c.attacker_won ? 0 : 1;
			GT_ASSERT( result.victory == ( c.attacker_won ? model::Victory::Attacker : model::Victory::Defender ) && result.survivor.identity == expected_id && result.deleted_identity == expected_dead, "winner identity changed after compaction: " + std::string( c.id ) );
			const auto& s = result.survivor;
			GT_ASSERT( s.reactor == c.input.reactors[ side ] && s.health == c.health_after[ side ] && s.stored_morale == c.stored_after && s.spent == c.survivor_spent, "survivor state lost at normal return: " + std::string( c.id ) );
			GT_ASSERT( s.position == ( c.attacker_won ? model::Position::Source : model::Position::Destination ) && Singleton( s ) && result.unit_present == c.present, "post-kill position/stack/occupancy differs: " + std::string( c.id ) );
			GT_ASSERT( result.references.selected == SurvivingReference( c.selected_index, expected_id ) && result.references.secondary == SurvivingReference( c.secondary_index, expected_id ), "selection did not follow participant identity: " + std::string( c.id ) );
			GT_ASSERT( result.continuation_identity == *SurvivingReference( c.continuation_index, expected_id ), "continuation did not name compacted survivor: " + std::string( c.id ) );
			GT_ASSERT( result.original_returns.core_return == c.core_return && result.original_returns.wrapper_return == c.wrapper_return, "original return evidence differs: " + std::string( c.id ) );
			GT_ASSERT( result.random.Shared() == c.shared_after && result.random.Crt() == c.crt_after, "cleanup added random draws: " + std::string( c.id ) );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout attack then empty-flat continuation matches all three histories", GT() {
		for ( const auto& c : fixture::SEQUENCES ) {
			const auto first = Resolve( c.input );
			GT_ASSERT( StepMatches( c.first, first.survivor, first.references, first.continuation_identity, first.random, first.unit_present, first.original_returns.wrapper_return ), "first normal-return state differs: " + std::string( c.id ) );
			const auto second = model::ContinueEmptyFlatWithoutLottery( first.survivor, c.second_allowance, first.random, first.references );
			GT_ASSERT( StepMatches( c.second, second.survivor, second.references, second.continuation_identity, second.random, second.unit_present, second.original_wrapper_return ), "second normal-return state differs: " + std::string( c.id ) );
			GT_ASSERT( second.random == first.random && second.outcome == ( c.second.wrapper_return == 1 ? model::EmptyMove::Moved : model::EmptyMove::Exhausted ), "empty-flat result or draw suppression differs" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout reference retirement never substitutes continuation selection", GT() {
		const model::Participant attacker( 0, 1, 1, 1, 1, 0 ), defender( UINT64_MAX, 1, 1, 1, 1, 255 );
		const SmacGameplayRandomState rng( 0xabcdef01u, 308 ); // Original tied first round kills attacker.
		for ( const auto selected : { std::optional< model::UnitId >{}, std::optional< model::UnitId >( 0 ), std::optional< model::UnitId >( UINT64_MAX ) } ) {
			for ( const auto secondary : { std::optional< model::UnitId >{}, std::optional< model::UnitId >( 0 ), std::optional< model::UnitId >( UINT64_MAX ) } ) {
				const auto r = model::ResolveNeutralFlatAttack( attacker, defender, 3, rng, { selected, secondary } );
				GT_ASSERT( r.victory == model::Victory::Defender && r.survivor.identity == UINT64_MAX && r.survivor.spent == 255, "stable identity/spending endpoint rejected" );
				GT_ASSERT( r.references.selected == ( selected == 0 ? std::nullopt : selected ) && r.references.secondary == ( secondary == 0 ? std::nullopt : secondary ), "dead reference retained or survivor reference lost" );
				GT_ASSERT( r.continuation_identity == UINT64_MAX && r.random.Shared() == rng.Shared(), "continuation changed selection or shared RNG" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Scout combat rejects unsupported resolved domains before mutation", GT() {
		const model::Participant valid( ATTACKER, 1, 10, 1, 1, 0 ), defender( DEFENDER, 1, 10, 1, 1, 0 );
		const SmacGameplayRandomState random( UINT32_MAX, UINT32_MAX );
		const auto attempt = [ & ]( const model::Participant& a, const model::Participant& d, const int allowance, const model::References& refs ) { return Rejects( [ & ]() { model::ResolveNeutralFlatAttack( a, d, allowance, random, refs ); } ); };
		for ( const int value : { std::numeric_limits< int32_t >::min(), -1, 0, 5, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.reactor = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "unsupported reactor accepted" );
		}
		for ( const int value : { -1, 0, 11, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.health = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "invalid HP accepted" );
		}
		for ( const int value : { -1, 7, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.stored_morale = a.effective_morale = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "unsupported morale accepted" );
		}
		auto changed = valid; changed.effective_morale = 2; GT_ASSERT( attempt( changed, defender, 3, { ATTACKER, DEFENDER } ), "modified-morale context accepted" );
		for ( const int value : { -1, 253, 255, 256, std::numeric_limits< int32_t >::max() } ) {
			auto a = valid; a.spent = value; GT_ASSERT( attempt( a, defender, 3, { ATTACKER, DEFENDER } ), "unverified spending/wrap accepted" );
		}
		for ( const int value : { -1, 0, 1, 4, 7, std::numeric_limits< int32_t >::max() } ) {
			GT_ASSERT( attempt( valid, defender, value, { ATTACKER, DEFENDER } ), "unsupported allowance accepted" );
		}
		GT_ASSERT( attempt( valid, valid, 3, { ATTACKER, ATTACKER } ) && attempt( valid, defender, 3, { 42, DEFENDER } ), "invalid identity/reference accepted" );
		GT_ASSERT( random.Shared() == UINT32_MAX && random.Crt() == UINT32_MAX && valid.spent == 0 && defender.health == 10, "rejection changed inputs" );
		GT_OK();
	} );
	task->AddTest( "SMAC Scout transcript bounds and explicit empty-flat limits hold", GT() {
		const SmacGameplayRandomState random( 0, UINT32_MAX );
		const auto result = model::ResolveNeutralFlatAttack( { ATTACKER, 4, 40, 6, 6, 252 }, { DEFENDER, 4, 40, 6, 6, 255 }, 6, random, { ATTACKER, DEFENDER } );
		GT_ASSERT( result.round_widths[ 0 ] == 0 && result.rounds.size() == 40 && result.draws.size() == 41 && result.victory == model::Victory::Defender, "exhausted diagnostic draw suppression/elite admission differs" );
		for ( const auto& d : result.draws ) { GT_ASSERT( d.phase != model::DrawPhase::RoundAttacker && d.phase != model::DrawPhase::PromotionParity, "suppressed draw consumed CRT" ); }
		const auto full = model::ResolveNeutralFlatAttack( { ATTACKER, 4, 40, 6, 6, 0 }, { DEFENDER, 4, 40, 6, 6, 255 }, 6, random, { std::nullopt, std::nullopt } );
		GT_ASSERT( full.rounds.size() <= 79 && full.draws.size() <= 160 && full.random.Shared() == random.Shared(), "bounded admitted transcript exceeded domain" );
		model::Survivor source{ ATTACKER, 1, 10, 1, 255, model::Position::Source, { std::nullopt, std::nullopt } };
		const auto exhausted = model::ContinueEmptyFlatWithoutLottery( source, 3, random, { std::nullopt, ATTACKER } );
		GT_ASSERT( exhausted.outcome == model::EmptyMove::Exhausted && exhausted.survivor.spent == 255 && exhausted.random == random, "exhausted continuation changed state" );
		for ( const int spent : { 1, 2 } ) {
			source.spent = spent; GT_ASSERT( Rejects( [ & ]() { model::ContinueEmptyFlatWithoutLottery( source, 3, random, { ATTACKER, std::nullopt } ); } ), "partial lottery path silently approximated" );
		}
		source.spent = 0; source.stack_links[ 0 ] = DEFENDER;
		GT_ASSERT( Rejects( [ & ]() { model::ContinueEmptyFlatWithoutLottery( source, 3, random, { ATTACKER, std::nullopt } ); } ), "multi-unit continuation admitted" );
		source.stack_links[ 0 ].reset(); source.position = model::Position::Destination;
		GT_ASSERT( Rejects( [ & ]() { model::ContinueEmptyFlatWithoutLottery( source, 3, random, { ATTACKER, std::nullopt } ); } ), "wrong-source continuation admitted" );
		GT_OK();
	} );
}

}
