
#ifndef _PROTO_SERIALIZER_H
#define _PROTO_SERIALIZER_H

#include <vector>
#include <memory>

#include "protocol_types.h"
#include "vector_stream.h"

#define _SERIALIZE_BEGIN \
    std::vector<uint8_t> __result;

#define _SERIALIZE_BEGIN_OPC \
    std::vector<uint8_t> __result; \
    __result.push_back(getOpcode());

#define _SERIALIZE_FIELD(fieldRef) \
    Serializer::serialize(fieldRef, __result);

#define _DESERIALIZE_FIELD(fieldRef) \
    Serializer::deserialize(fieldRef, from);

#define _SERIALIZE_RETURN \
    return __result;

template<typename T>
using enable_if_serializer = typename std::enable_if<std::is_same<decltype(T().serialize()), std::vector<uint8_t>>::value>;
template<typename T>
using enable_if_deserializer = typename std::enable_if<std::is_same<decltype(std::declval<T>().deserialize(std::declval<VectorStream&>())), void>::value>;

struct Serializer {
    static void serialize(const bool b, std::vector<uint8_t>& to);
    static void deserialize(bool& b, VectorStream& from);

    static void serialize(const int8_t num, std::vector<uint8_t>& to);
    static void deserialize(int8_t& num, VectorStream& from);

    static void serialize(const uint8_t num, std::vector<uint8_t>& to);
    static void deserialize(uint8_t& num, VectorStream& from);

    static void serialize(uint64_t num, std::vector<uint8_t>& to);
    static void deserialize(uint64_t& num, VectorStream& from);

    static void serialize(uint16_t num, std::vector<uint8_t>& to);
    static void deserialize(uint16_t& num, VectorStream& from);

    static void serialize(const std::string& str, std::vector<uint8_t>& to);
    static void deserialize(std::string& str, VectorStream& from);

    static void serialize(const float num, std::vector<uint8_t>& to);
    static void deserialize(float& num, VectorStream& from);

    static void serialize(const vec2 vec, std::vector<uint8_t>& to);
    static void deserialize(vec2& vec, VectorStream& from);

    static void serialize(const vec3 vec, std::vector<uint8_t>& to);
    static void deserialize(vec3& vec, VectorStream& from);

    template<typename T, typename Chk = typename enable_if_serializer<T>::type>
    static void serialize(const T& obj, std::vector<uint8_t>& to) {
        serialize(&obj, to);
    }
    template<typename T, typename Chk = typename enable_if_deserializer<T>::type>
    static void deserialize(T& obj, VectorStream& from) {
        deserialize(&obj, from);
    }

    private:
    static void serialize(const Serializable* obj, std::vector<uint8_t>& to);
    static void deserialize(Serializable* obj, VectorStream& from);

    public:
    template<typename T, typename Chk = typename enable_if_serializer<T>::type>
    static void serialize(const std::unique_ptr<T>& obj, std::vector<uint8_t>& to) {
        serialize(dynamic_cast<const Serializable*>(obj.get()), to);
    }
    template<typename T, typename Chk = typename enable_if_deserializer<T>::type>
    static void deserialize(std::unique_ptr<T>& obj, VectorStream& from) {
        Serializable* ptr = new T;
        deserialize(ptr, from);
        obj.reset(dynamic_cast<T*>(ptr));
    }

    template<typename T>
    static void serialize(const std::vector<T>& list,std::vector<uint8_t>& to) {
        serialize(static_cast<uint64_t>(list.size()), to);

        for (const auto& element : list)
            serialize(element, to);        
    }

    template<typename T>
    static void deserialize(std::vector<T>& list, VectorStream& from) {
        uint64_t len;
        deserialize(len, from);

        list.resize(len);
        for (size_t i = 0; i < len; ++i)
            deserialize(list[i], from);
    }
};

#endif
