#pragma once

#include <string>
#include <vector>
#include <map>

namespace hvk {
	struct Context {
		size_t id;
		std::string name;
	};

	class ContextManager
	{
	private:
		size_t mActiveContextId;
		std::vector<Context> mContexts;
		std::map<const char*, size_t> mContextLookup;
	public:
		ContextManager();
		~ContextManager();

		size_t registerContext(const char* contextName);
		bool activateContextById(size_t contextId);
		bool activateContextByName(const char* contextName);
		const Context& getActiveContext() const;
	};
}
