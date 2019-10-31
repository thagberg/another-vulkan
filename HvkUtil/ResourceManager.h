// Credit here for design of memory pool: https://thinkingeek.com/2017/11/19/simple-memory-pool/

#pragma once

#include <memory>
#include <array>
#include <list>
#include <functional>

#define ARENA_SIZE 32

namespace hvk {

	template <typename T>
	class Pool;
	
	class ResourceManager
	{
		struct MemoryNode
		{
			void* data;
			size_t size;
		};
	private:
		static void* sStorage;
		static size_t sSize;
		static size_t sRemaining;
		static bool sInitialized;
		static std::list<MemoryNode> sFreeNodes;

		static void* claimSpace(size_t size, size_t alignment);
	public:
		ResourceManager();
		~ResourceManager();

		static void initialize(size_t startingSize);
		static void* alloc(size_t size, size_t alignment);

		template <typename T>
		static void free(T* p, size_t size)
		{
			for (auto it = sFreeNodes.begin(); it->data <= p; ++it)
			{
				if (it->data > p) {
					bool insertNew = true;
					bool deleteNext = false;

					MemoryNode m{ p, size };

					// if contiguous blocks, merge
					if (reinterpret_cast<char*>(p) + size == it->data) {
						m.size += it->size;
						deleteNext = true;
					}

					if (it != sFreeNodes.begin()) {
						auto prev = std::prev(it);
						if (reinterpret_cast<char*>(prev->data) + prev->size == m.data) {
							prev->size += m.size;
							insertNew = false;
						}
					}

					if (insertNew) {
						sFreeNodes.insert(it, m);
					}
					if (deleteNext) {
						sFreeNodes.erase(it);
					}
				}
			}
		}
	};


	template <typename T>
	struct Hallocator
	{
		// ACR == Allocator Completeness Requirement
		// https://en.cppreference.com/w/cpp/named_req/Allocator#Allocator_completeness_requirements

		typedef T value_type; // ACR

		Hallocator() noexcept {} // ACR
		template <class U> Hallocator(Hallocator<U> const&) noexcept {} // ACR

		T* allocate(size_t n) // ACR
		{
			return static_cast<T*>(ResourceManager::alloc(n*sizeof(T), alignof(T)));
		}

		void deallocate(T* p, size_t n) // ACR
		{
			ResourceManager::free<T>(p, n*sizeof(T));
		}
	};

	template <typename T, typename U>
	bool operator==(const Hallocator<T>&, const Hallocator<U>&) { return true; } // ACR
	template <typename T, typename U>
	bool operator!=(const Hallocator<T>&, const Hallocator<U>&) { return false; } // ACR


	template <typename T>
	union PoolItem
	{
		using Storage = char[sizeof(T)];

	private:
		PoolItem<T>* next;
		T* data;

	public:
		PoolItem<T>* getNext() const { return next; }
		void setNext(PoolItem<T>* n) { next = n; }
		T* getData() { return data; }
	};


	template <typename T>
	class Arena
	{
	private:
		//std::unique_ptr<std::array<PoolItem<T>, ARENA_SIZE>> mStorage;
		//std::unique_ptr<PoolItem<T>[]> mStorage;
		PoolItem<T>* mStorage;
		std::unique_ptr<Arena<T>> mPreviousArena;

	public:
		Arena() : 
			//mStorage(new std::array<PoolItem<T>, ARENA_SIZE>())
			mStorage(nullptr)
		{
			mStorage = reinterpret_cast<PoolItem<T>*>(ResourceManager::alloc(sizeof(PoolItem<T>) * ARENA_SIZE, alignof(PoolItem<T>)));
			//mStorage = new (mem) PoolItem<T>[ARENA_SIZE]();
			//mStorage(new PoolItem<T>[ARENA_SIZE])
			//mStorage = new PoolItem<T>[ARENA_SIZE];
			//PoolItem<T>* t = (PoolItem<T>*)mem;
			for (size_t i = 1; i < ARENA_SIZE; ++i) {
				mStorage[i - 1].setNext(&mStorage[i]);
			}
			//PoolItem<T> lastItem = mStorage[ARENA_SIZE - 1];
			//lastItem.setNext(nullptr);
			mStorage[ARENA_SIZE - 1].setNext(nullptr);
		}

		~Arena()
		{
			ResourceManager::free(mStorage, sizeof(PoolItem<T>) * ARENA_SIZE);
		}

		void setPrevious(std::unique_ptr<Arena<T>>&& n)
		{
			assert(!mPreviousArena);
			mPreviousArena.reset(n.release());
		}

		PoolItem<T>* getStorage() const
		{
			//return mStorage.get()->at(0);
			//mStorage
			//return mStorage.get();
			return mStorage;
		}
	};

	template <typename T>
	void extFree(T*)
	{

	}

	template <typename T>
	class Pool
	{

	private:
		static PoolItem<T>* sFreeItem;
		static std::unique_ptr<Arena<T>> sArena;

	public:
		//Pool(uint32_t size);
		//~Pool();

		static void free(T* t)
		{
			t->T::~T();
			//PoolItem<T>* item = 
			//PoolItem<T>* item = std::reinterpret_cast<PoolItem*>(t);
			PoolItem<T>* item = reinterpret_cast<PoolItem<T>*>(t);
			item->setNext(sFreeItem);
			sFreeItem = item;
		}

		template <typename... Args> static std::unique_ptr<T, void(*)(T*) > alloc(Args&& ... args)
		{
			if (sFreeItem == nullptr) {
				void* mem = ResourceManager::alloc(sizeof(Arena<T>) * ARENA_SIZE, alignof(Arena<T>));
				std::unique_ptr<Arena<T>> newArena(new (mem) Arena<T>());
				newArena->setPrevious(std::move(sArena));
				sArena.reset(newArena.release());
				sFreeItem = sArena->getStorage();
			}

			PoolItem<T>* item = sFreeItem;
			sFreeItem = item->getNext();
			//T* result = item->getData();
			T* result = reinterpret_cast<T*>(item);
			new (result) T(std::forward<Args>(args)...);
			return std::unique_ptr<T, void(*)(T*) >(result, Pool<T>::free);
		}
	};

	template <typename T>
	PoolItem<T>* Pool<T>::sFreeItem = nullptr;

	template <typename T>
	std::unique_ptr<Arena<T>> Pool<T>::sArena = nullptr;
}
