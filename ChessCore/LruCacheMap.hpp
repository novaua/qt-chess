#pragma once 
#include <map>
#include <queue>

const size_t MaxCacheSize = 5000u;
template<class K, class V>
class LruCacheMap
{
public:
	LruCacheMap(size_t maxLenght = MaxCacheSize);
	~LruCacheMap();

	typename std::map<K, V>::iterator begin()
	{
		return _map.begin();
	}

	typename std::map<K, V>::iterator end()
	{
		return _map.end();
	}

	typename std::map<K, V>::iterator find(const K &key)
	{
		return _map.find(key);
	}

	size_t size() const
	{
		return _map.size();
	}

	V &operator[] (const K &key)
	{
		return _map[key];
	}

	bool Contains(const K & k) const
	{
		return _map.find(k) != _map.end();
	}

	V &At(const K & k)
	{
		return _map[k];
	}

	// return true if a new element added
	bool AddOrUpdate(const K &key, const V &value);
	
private:
	std::map<K, V> _map;
	std::queue<K> _queue;
	size_t _maxLenght;
};

template<class K, class V>
LruCacheMap<K, V>::LruCacheMap(size_t maxLenght)
	:_maxLenght(maxLenght)
{
}

template<class K, class V>
LruCacheMap<K, V>::~LruCacheMap()
{
}

template<class K, class V>
bool LruCacheMap<K, V>::AddOrUpdate(const K &key, const V &value)
{
	auto isNew = !Contains(key);
	if (isNew && (_map.size() >= _maxLenght))
	{
		auto last = _queue.front();
		_queue.pop();
		_map.erase(last);
	}

	_map[key] = value;
	_queue.push(key);
	return isNew;
}
