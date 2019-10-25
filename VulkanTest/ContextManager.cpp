#include "ContextManager.h"

#define CONTEXT_PAIR(x, y) std::pair<const char*, uint16_t>(x, y)

namespace hvk {

	ContextManager::ContextManager() :
		mActiveContextId(),
		mContexts(),
		mContextLookup()
	{
	}


	ContextManager::~ContextManager()
	{
	}

	size_t ContextManager::registerContext(const char* contextName) {
		size_t newId = mContexts.size();
		// TODO: add ctor for struct so that we can emplace instead of push
		mContexts.push_back({ newId, contextName });
		mContextLookup.insert(CONTEXT_PAIR(contextName, newId));
		return newId;
	}

	bool ContextManager::activateContextById(size_t contextId) {
		mActiveContextId = contextId;
		return true;
	}

	bool ContextManager::activateContextByName(const char* contextName) {
		const auto& found = mContextLookup.find(contextName);
		bool success = false;
		if (found != mContextLookup.end()) {
			activateContextById(found->second);
			success = true;
		}
		return success;
	}

	const Context& ContextManager::getActiveContext() const {
		return mContexts[mActiveContextId];
	}
}
