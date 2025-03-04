//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/common/serializer/format_serializer.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/common/field_writer.hpp"
#include "duckdb/common/serializer.hpp"
#include "duckdb/common/serializer/enum_serializer.hpp"
#include "duckdb/common/serializer/serialization_traits.hpp"
#include "duckdb/common/types/interval.hpp"
#include "duckdb/common/types/string_type.hpp"
#include "duckdb/common/unordered_map.hpp"
#include "duckdb/common/unordered_set.hpp"

namespace duckdb {

class FormatDeserializer {
protected:
	bool deserialize_enum_from_string = false;

public:
	// We fake return-type overloading using templates and enable_if
	/*
	template<typename T>
	T ReadProperty(const char* tag) {
	    SetTag(tag);
	    return std::move<T>(Read<T>());
	}
	 */

	// Read into an existing value
	template <typename T>
	inline void ReadProperty(const char *tag, T &ret) {
		SetTag(tag);
		ret = Read<T>();
	}

	// Read and return a value
	template <typename T>
	inline T ReadProperty(const char *tag) {
		SetTag(tag);
		return Read<T>();
	}

	// Read optional property and return a value, or forward a default value
	template <typename T>
	inline T ReadOptionalPropertyOrDefault(const char *tag, T &&default_value) {
		SetTag(tag);
		auto present = OnOptionalBegin();
		if (present) {
			return Read<T>();
		} else {
			return std::forward<T>(default_value);
		}
	}

	// Read optional property into an existing value, or use a default value
	template <typename T>
	inline void ReadOptionalPropertyOrDefault(const char *tag, T &ret, T &&default_value) {
		SetTag(tag);
		auto present = OnOptionalBegin();
		if (present) {
			ret = Read<T>();
		} else {
			ret = std::forward<T>(default_value);
		}
	}

	// Read optional property and return a value, or default construct it
	template <typename T>
	inline typename std::enable_if<std::is_default_constructible<T>::value, T>::type
	ReadOptionalProperty(const char *tag) {
		SetTag(tag);
		auto present = OnOptionalBegin();
		if (present) {
			return Read<T>();
		} else {
			return T();
		}
	}

	// Read optional property into an existing value, or default construct it
	template <typename T>
	inline typename std::enable_if<std::is_default_constructible<T>::value, void>::type
	ReadOptionalProperty(const char *tag, T &ret) {
		SetTag(tag);
		auto present = OnOptionalBegin();
		if (present) {
			ret = Read<T>();
		} else {
			ret = T();
		}
	}

private:
	// Deserialize anything implementing a FormatDeserialize method
	template <typename T = void>
	inline typename std::enable_if<has_deserialize<T>::value, T>::type Read() {
		return T::FormatDeserialize(*this);
	}

	// Structural Types
	// Deserialize a unique_ptr
	template <class T = void>
	inline typename std::enable_if<is_unique_ptr<T>::value, T>::type Read() {
		using ELEMENT_TYPE = typename is_unique_ptr<T>::ELEMENT_TYPE;
		return std::move(ELEMENT_TYPE::FormatDeserialize(*this));
	}

	// Deserialize shared_ptr
	template <typename T = void>
	inline typename std::enable_if<is_shared_ptr<T>::value, T>::type Read() {
		using ELEMENT_TYPE = typename is_shared_ptr<T>::ELEMENT_TYPE;
		return std::move(ELEMENT_TYPE::FormatDeserialize(*this));
	}

	// Deserialize a vector
	template <typename T = void>
	inline typename std::enable_if<is_vector<T>::value, T>::type Read() {
		using ELEMENT_TYPE = typename is_vector<T>::ELEMENT_TYPE;
		T vec;
		auto size = ReadUnsignedInt32();
		for (idx_t i = 0; i < size; i++) {
			vec.push_back(Read<ELEMENT_TYPE>());
		}

		return vec;
	}

	// Deserialize a map
	template <typename T = void>
	inline typename std::enable_if<is_unordered_map<T>::value, T>::type Read() {
		using KEY_TYPE = typename is_unordered_map<T>::KEY_TYPE;
		using VALUE_TYPE = typename is_unordered_map<T>::VALUE_TYPE;
		auto size = ReadUnsignedInt32();
		T map;
		for (idx_t i = 0; i < size; i++) {
			map[Read<KEY_TYPE>()] = Read<VALUE_TYPE>();
		}

		return map;
	}

	// Deserialize an unordered set
	template <typename T = void>
	inline typename std::enable_if<is_unordered_set<T>::value, T>::type Read() {
		using ELEMENT_TYPE = typename is_unordered_set<T>::ELEMENT_TYPE;
		auto size = ReadUnsignedInt32();
		T set;
		for (idx_t i = 0; i < size; i++) {
			set.insert(Read<ELEMENT_TYPE>());
		}

		return set;
	}

	// Deserialize a set
	template <typename T = void>
	inline typename std::enable_if<is_set<T>::value, T>::type Read() {
		using ELEMENT_TYPE = typename is_set<T>::ELEMENT_TYPE;
		auto size = ReadUnsignedInt32();
		T set;
		for (idx_t i = 0; i < size; i++) {
			set.insert(Read<ELEMENT_TYPE>());
		}

		return set;
	}

	// Deserialize a pair
	template <typename T = void>
	inline typename std::enable_if<is_pair<T>::value, T>::type Read() {
		using FIRST_TYPE = typename is_pair<T>::FIRST_TYPE;
		using SECOND_TYPE = typename is_pair<T>::SECOND_TYPE;

		OnPairBegin();
		OnPairKeyBegin();
		FIRST_TYPE first = Read<FIRST_TYPE>();
		OnPairKeyEnd();
		OnPairValueBegin();
		SECOND_TYPE second = Read<SECOND_TYPE>();
		OnPairValueEnd();
		OnPairEnd();
		return std::make_pair(first, second);
	}

	// Primitive types
	// Deserialize a bool
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, bool>::value, T>::type Read() {
		return ReadBool();
	}

	// Deserialize a int8_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, int8_t>::value, T>::type Read() {
		return ReadSignedInt8();
	}

	// Deserialize a uint8_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, uint8_t>::value, T>::type Read() {
		return ReadUnsignedInt8();
	}

	// Deserialize a int16_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, int16_t>::value, T>::type Read() {
		return ReadSignedInt16();
	}

	// Deserialize a uint16_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, uint16_t>::value, T>::type Read() {
		return ReadUnsignedInt16();
	}

	// Deserialize a int32_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, int32_t>::value, T>::type Read() {
		return ReadSignedInt32();
	}

	// Deserialize a uint32_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, uint32_t>::value, T>::type Read() {
		return ReadUnsignedInt32();
	}

	// Deserialize a int64_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, int64_t>::value, T>::type Read() {
		return ReadSignedInt64();
	}

	// Deserialize a uint64_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, uint64_t>::value, T>::type Read() {
		return ReadUnsignedInt64();
	}

	// Deserialize a float
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, float>::value, T>::type Read() {
		return ReadFloat();
	}

	// Deserialize a double
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, double>::value, T>::type Read() {
		return ReadDouble();
	}

	// Deserialize a string
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, string>::value, T>::type Read() {
		return ReadString();
	}

	// Deserialize a Enum
	template <typename T = void>
	inline typename std::enable_if<std::is_enum<T>::value, T>::type Read() {
		auto str = ReadString();
		return EnumSerializer::StringToEnum<T>(str.c_str());
	}

	// Deserialize a interval_t
	template <typename T = void>
	inline typename std::enable_if<std::is_same<T, interval_t>::value, T>::type Read() {
		return ReadInterval();
	}

protected:
	virtual void SetTag(const char *tag) {
		(void)tag;
	}

	virtual idx_t OnListBegin() = 0;
	virtual void OnListEnd() {
	}
	virtual idx_t OnMapBegin() = 0;
	virtual void OnMapEnd() {
	}
	virtual void OnMapEntryBegin() {
	}
	virtual void OnMapEntryEnd() {
	}
	virtual void OnMapKeyBegin() {
	}
	virtual void OnMapKeyEnd() {
	}
	virtual void OnMapValueBegin() {
	}
	virtual void OnMapValueEnd() {
	}
	virtual bool OnOptionalBegin() = 0;
	virtual void OnOptionalEnd() {
	}
	virtual void OnObjectBegin() {
	}
	virtual void OnObjectEnd() {
	}
	virtual void OnPairBegin() {
	}
	virtual void OnPairKeyBegin() {
	}
	virtual void OnPairKeyEnd() {
	}
	virtual void OnPairValueBegin() {
	}
	virtual void OnPairValueEnd() {
	}
	virtual void OnPairEnd() {
	}

	virtual bool ReadBool() = 0;
	virtual int8_t ReadSignedInt8() = 0;
	virtual uint8_t ReadUnsignedInt8() = 0;
	virtual int16_t ReadSignedInt16() = 0;
	virtual uint16_t ReadUnsignedInt16() = 0;
	virtual int32_t ReadSignedInt32() = 0;
	virtual uint32_t ReadUnsignedInt32() = 0;
	virtual int64_t ReadSignedInt64() = 0;
	virtual uint64_t ReadUnsignedInt64() = 0;
	virtual float ReadFloat() = 0;
	virtual double ReadDouble() = 0;
	virtual string ReadString() = 0;
	virtual interval_t ReadInterval() = 0;
};

} // namespace duckdb
