#pragma once

#include <string>
#include <vector>
#include <map>

namespace hvk {
	struct Context {
		uint16_t id;
		std::string name;
	};

	class ContextManager
	{
	private:
		uint16_t mActiveContextId;
		std::vector<Context> mContexts;
		std::map<const char*, uint16_t> mContextLookup;
	public:
		ContextManager();
		~ContextManager();

		uint16_t registerContext(const char* contextName);
		bool activateContextById(uint16_t contextId);
		bool activateContextByName(const char* contextName);
		const Context& getActiveContext() const;
	};
}
