#include "CampaignFile.h"
#include "State.h"
#include "types/Buffer.h"
#include "util/FinallyGuard.h"
#include <filesystem>
#include <fstream>
#include <atomic>
#include <stdexcept>
#include <cerrno>
#include <limits>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace game::backend::campaign {
namespace {
constexpr size_t SETTINGS_LIMIT = 65532;
constexpr const char* MAGIC = "GLSMAC CAMPAIGN";
void Validate(const Record& value) {
    if(value.settings.empty() || value.settings.size()>SETTINGS_LIMIT || value.world.empty() || value.world.size()>MAX_BYTES-SETTINGS_LIMIT-65536)
        throw std::runtime_error("GLSMAC campaign payload exceeds its file budget");
    if(value.roster.slots.size()!=7 || value.local_slot!=0) throw std::runtime_error("GLSMAC campaign requires the local seven-slot roster");
    for(size_t i=0;i<value.roster.slots.size();++i) {
        const auto& slot=value.roster.slots[i];
        if(slot.state!=roster::SlotState::Occupied) { if(i==value.local_slot) throw std::runtime_error("Campaign local player is absent"); continue; }
        if(!slot.player || !slot.player->faction_id || slot.player->role!=(i==0?roster::Role::Single:roster::Role::Player))
            throw std::runtime_error("Campaign roster is not a configured local human roster");
    }
}
std::filesystem::path Path(const std::string& text) {
    if(text.empty() || text.find('\0')!=std::string::npos) throw std::runtime_error("Campaign path is empty or invalid");
    auto path=std::filesystem::u8path(text);
    if(path.extension()!=".glsmac") throw std::runtime_error("GLSMAC campaign files use the .glsmac extension");
    return std::filesystem::absolute(path);
}
}
std::string Encode(const Record& record) {
    Validate(record);
    const auto roster=roster::EncodeRoster(record.roster,State::ROSTER_LIMITS);
    types::Buffer buffer;
    buffer.WriteString(MAGIC);buffer.WriteInt(record.cursor?2:1);buffer.WriteInt(record.local_slot);
    buffer.WriteString(record.settings);buffer.WriteString(roster);buffer.WriteString(record.world);
    if(record.cursor) {for(const auto coordinate:*record.cursor) {buffer.WriteInt(coordinate);}}
    if(buffer.lenw>MAX_BYTES) throw std::runtime_error("GLSMAC campaign file is too large");
    return buffer.ToString();
}
Record Decode(const std::string& bytes) {
    if(bytes.size()>MAX_BYTES) throw std::runtime_error("GLSMAC campaign file is too large");
    types::Buffer buffer(bytes);
    if(buffer.ReadString(32)!=MAGIC) throw std::runtime_error("Unsupported GLSMAC campaign format");
    const auto version=buffer.ReadInt();
    if(version!=1 && version!=2) throw std::runtime_error("Unsupported GLSMAC campaign format");
    const auto local=buffer.ReadInt();
    if(local!=0) throw std::runtime_error("Unsupported campaign local player");
    Record value{buffer.ReadString(SETTINGS_LIMIT),{},static_cast<size_t>(local),{}};
    try { value.roster=roster::DecodeRoster(buffer.ReadString(State::ROSTER_LIMITS.max_encoded_bytes),State::ROSTER_LIMITS); }
    catch(const std::invalid_argument& error) { throw std::runtime_error(error.what()); }
    value.world=buffer.ReadString(MAX_BYTES-SETTINGS_LIMIT-65536);
    if(version==2) {
        ui_view::Cursor cursor{};
        for(auto& coordinate:cursor) {
            const auto value=buffer.ReadInt();
            if(value<std::numeric_limits<int32_t>::min() || value>std::numeric_limits<int32_t>::max())
                throw std::runtime_error("Campaign cursor exceeds its signed 32-bit field");
            coordinate=static_cast<int32_t>(value);
        }
        value.cursor=cursor;
    }
    if(buffer.lenr!=buffer.lenw) throw std::runtime_error("Trailing GLSMAC campaign data");
    Validate(value);return value;
}
Record Read(const std::string& text) {
    const auto path=Path(text);
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    if(!input) throw std::runtime_error("Cannot open GLSMAC campaign: "+text);
    const auto size=input.tellg();
    if(size<0 || static_cast<uint64_t>(size)>MAX_BYTES) throw std::runtime_error("GLSMAC campaign file is too large");
    std::string bytes(static_cast<size_t>(size),'\0');input.seekg(0);
    if(!input.read(bytes.data(),static_cast<std::streamsize>(bytes.size()))) throw std::runtime_error("Cannot read complete GLSMAC campaign");
    return Decode(bytes);
}
std::string Write(const std::string& text,const Record& record) {
    const auto bytes=Encode(record);
    const auto path=Path(text);
    static std::atomic<uint64_t> next{0};
    auto temporary=path;temporary += ".tmp-"+std::to_string(++next);
    // Exclusive creation owns this temporary path even across game processes.
    // A colliding attempt cannot truncate or remove another writer's bytes.
#ifdef _WIN32
    auto file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create campaign temporary file exclusively");
    util::FinallyGuard cleanup([&]{if(file!=INVALID_HANDLE_VALUE){CloseHandle(file);}std::error_code ignored;std::filesystem::remove(temporary,ignored);});
    size_t offset=0;
    while(offset<bytes.size()) {
        DWORD written=0;
        if(!WriteFile(file,bytes.data()+offset,static_cast<DWORD>(bytes.size()-offset),&written,nullptr) || !written)
            throw std::runtime_error("Cannot write GLSMAC campaign");
        offset+=written;
    }
    if(!FlushFileBuffers(file)) throw std::runtime_error("Cannot flush GLSMAC campaign");
    const auto closed=CloseHandle(file);file=INVALID_HANDLE_VALUE;
    if(!closed) throw std::runtime_error("Cannot finish GLSMAC campaign");
#else
    auto file=::open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
    if(file<0) throw std::runtime_error("Cannot create campaign temporary file exclusively");
    util::FinallyGuard cleanup([&]{if(file>=0){::close(file);}std::error_code ignored;std::filesystem::remove(temporary,ignored);});
    size_t offset=0;
    while(offset<bytes.size()) {
        const auto written=::write(file,bytes.data()+offset,bytes.size()-offset);
        if(written<0 && errno==EINTR) continue;
        if(written<=0) throw std::runtime_error("Cannot write GLSMAC campaign");
        offset+=static_cast<size_t>(written);
    }
    if(::fsync(file)) throw std::runtime_error("Cannot flush GLSMAC campaign");
    const auto closed=::close(file);file=-1;
    if(closed) throw std::runtime_error("Cannot finish GLSMAC campaign");
#endif
#ifdef _WIN32
    if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) throw std::runtime_error("Cannot replace GLSMAC campaign");
#else
    std::filesystem::rename(temporary,path);
#endif
    return path.u8string();
}
}
