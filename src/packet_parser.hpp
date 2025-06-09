#ifndef PACKET_PARSER_HPP
#define PACKET_PARSER_HPP

#include <string>
#include <vector>
#include <variant>
#include <span>
#include <cstdint>
#include <optional>
#include <functional>
#include <utility>

enum class FieldType {
    UINT8, UINT16, UINT32, UINT64,
    INT8, INT16, INT32, INT64,
    FLOAT32, FLOAT64,
    BYTEARRAY
};

class FieldValue {
public:
    using ValueVariant = std::variant<
        uint8_t, uint16_t, uint32_t, uint64_t,
        int8_t, int16_t, int32_t, int64_t,
        float, double,
        std::vector<uint8_t>
    >;
    FieldValue() = default;
    template <typename T>
    FieldValue(T&& v) : value_(std::forward<T>(v)) {}

    const ValueVariant& value() const { return value_; }

    template <typename T>
    const T* get_if() const { return std::get_if<T>(&value_); }

    bool operator==(const FieldValue& other) const { return value_ == other.value_; }
    bool operator!=(const FieldValue& other) const { return value_ != other.value_; }
private:
    ValueVariant value_;
};

struct BitfieldInfo {
    uint8_t bit_offset;
    uint8_t bit_count;
};

struct FieldDesc {
    std::string name;
    FieldType type;
    size_t offset;
    std::optional<BitfieldInfo> bitfield;
    std::optional<size_t> length; // Para bytearray
    std::optional<FieldValue> value; // Para valores esperados (como el identificador)
};

struct PacketDesc {
    std::string name;
    std::vector<FieldDesc> fields;
    size_t id_field_index;    // Index en fields
    FieldValue id_value;      // Identification value
};

using PacketDb = std::vector<PacketDesc>;

struct FieldView {
    std::span<const uint8_t> raw;
    const FieldDesc& desc;
};

// Signature fija: void(const FieldView&)
using FieldVisitor = std::function<void(const FieldView&)>;

// Devuelve un par (cantidad de paquetes parseados exitosamente, offset final alcanzado)
std::pair<size_t, size_t> scan_packets(const PacketDb& db, std::span<const uint8_t> data, const FieldVisitor& visitor);

#endif // PACKET_PARSER_HPP
