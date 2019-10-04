// Credit here for design of memory pool: https://thinkingeek.com/2017/11/19/simple-memory-pool/

#pragma once

#include <array>

#define ARENA_SIZE 32

namespace hvk {

	template <typename T>
	union PoolItem
	{
		using Storage = char[sizeof(T)];

	private:
		PoolItem<T>* next;
		//alignas(alignof(T)) Storage data;
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
		std::unique_ptr<PoolItem<T>[]> mStorage;
		std::unique_ptr<Arena<T>> mPreviousArena;

	public:
		Arena() : 
			//mStorage(new std::array<PoolItem<T>, ARENA_SIZE>())
			mStorage(new PoolItem<T>[ARENA_SIZE])
		{
			for (size_t i = 1; i < ARENA_SIZE; ++i) {
				mStorage[i - 1].setNext(&mStorage[i]);
			}
			//PoolItem<T> lastItem = mStorage[ARENA_SIZE - 1];
			//lastItem.setNext(nullptr);
			mStorage[ARENA_SIZE - 1].setNext(nullptr);
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
			return mStorage.get();
		}
	};


	template <typename T>
	class Pool
	{

	private:
		static PoolItem<T>* sFreeItem;
		static std::unique_ptr<Arena<T>> sArena;

	public:
		//Pool(uint32_t size);
		//~Pool();

		template <typename... Args> static T* alloc(Args&& ... args)
		{
			if (sFreeItem == nullptr) {
				std::unique_ptr<Arena<T>> newArena(new Arena<T>());
				newArena->setPrevious(std::move(sArena));
				sArena.reset(newArena.release());
				sFreeItem = sArena->getStorage();
			}

			PoolItem<T>* item = sFreeItem;
			sFreeItem = item->getNext();
			//T* result = item->getData();
			T* result = reinterpret_cast<T*>(item);
			new (result) T(std::forward<Args>(args)...);
			return result;
		}

		static void free(T* t)
		{
			t->T::~T();
			//PoolItem<T>* item = 
			//PoolItem<T>* item = std::reinterpret_cast<PoolItem*>(t);
			PoolItem<T>* item = reinterpret_cast<PoolItem<T>*>(t);
			item->setNext(sFreeItem);
			sFreeItem = item;
		}
	};

	template <typename T>
	PoolItem<T>* Pool<T>::sFreeItem = nullptr;

	template <typename T>
	std::unique_ptr<Arena<T>> Pool<T>::sArena = nullptr;

	class ResourceManager
	{
	public:
		ResourceManager();
		~ResourceManager();
	};
}
