#pragma once
struct BinarySerializer
{
	template<class T>
	static void Serialize(const T & plainObject, std::ostream &ostream)
	{
		ostream.write((char*)&plainObject, sizeof(T));
	}

	template<class T>
	static T Deserialize(std::istream &istream)
	{
		T out;
		istream.read((char*)&out, sizeof(T));
		return out;
	}
};