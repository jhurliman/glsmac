#pragma once

#include "gse/Wrappable.h"
#include <atomic>
#include <memory>

namespace util {
class ConfigManager;
}

namespace config {
namespace wrapped {

class Config : public gse::Wrappable {
public:

	Config( util::ConfigManager* const manager );
    ~Config() override;

	WRAPDEFS_PTR( Config )

private:
	util::ConfigManager* const m_manager;
    const std::shared_ptr<std::atomic<bool>> m_alive=std::make_shared<std::atomic<bool>>(true);

};

}
}
