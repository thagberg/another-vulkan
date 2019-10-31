#include "pch.h"
#include "ResourceManager.h"

#include <memory>

namespace hvk {

	void* ResourceManager::sStorage = nullptr;
	//void* ResourceManager::sNeedle = nullptr;
	size_t ResourceManager::sSize = 0;
	size_t ResourceManager::sRemaining = 0;
	bool ResourceManager::sInitialized = false;
	std::list<ResourceManager::MemoryNode> ResourceManager::sFreeNodes(200);

	ResourceManager::ResourceManager()
	{
	}


	ResourceManager::~ResourceManager()
	{
	}

	void ResourceManager::initialize(size_t startingSize)
	{
		if (!sInitialized) {
			sInitialized = true;
			sSize = startingSize;
			sRemaining = startingSize;
			sStorage = static_cast<char*>(malloc(startingSize));
			sFreeNodes.insert(sFreeNodes.begin(), MemoryNode{ sStorage, sSize });
		}
	}

	void* ResourceManager::alloc(size_t size, size_t alignment)
	{
		void* start = nullptr;
		if (sInitialized) {
			start = claimSpace(size, alignment);
		}
		return start;
	}

	void* ResourceManager::claimSpace(size_t size, size_t alignment)
	{
		for (auto it = sFreeNodes.begin(); it != sFreeNodes.end(); ++it)
		{
			if (it->size >= size) {
				void* m = std::align(alignment, size, it->data, it->size);
				if (m != nullptr) {
					size_t alignmentOffset = static_cast<char*>(m) - it->data;
					if (m > it->data) {
						sFreeNodes.insert(std::prev(it), { it->data, alignmentOffset });
					}
					size_t leftOver = (it->size - alignmentOffset) - size;
					if (leftOver) {
						sFreeNodes.insert(std::next(it), MemoryNode{ (static_cast<char*>(m) + size), leftOver });
					}
					sFreeNodes.erase(it);
					return m;
				}
			}
		}
        return nullptr;
	}
}
