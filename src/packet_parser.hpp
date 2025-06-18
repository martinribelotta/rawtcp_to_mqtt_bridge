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
    template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, FieldValue>>>
    FieldValue(T&& v) : value_(std::forward<T>(v)) {}

    const ValueVariant& value() const { return value_; }

    template <typename T>
    const T* get_if() const { return std::get_if<T>(&value_); }

    bool operator==(const FieldValue& other) const { return value_ == other.value_; }
    bool operator!=(const FieldValue& other) const { return value_ != other.value_; }
    std::string to_string() const;
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
    std::optional<size_t> length;
    std::optional<FieldValue> value;

    std::string to_string() const;
};

struct MqttTemplate {
    std::string topic;
    std::string payload;
    uint8_t qos = 0;
    bool retain = false;
};

struct PacketDesc {
    std::string name;
    std::vector<FieldDesc> fields;
    size_t id_field_index;
    FieldValue id_value;
    MqttTemplate mqtt;
};

using PacketDb = std::vector<PacketDesc>;

struct FieldView {
    std::span<const uint8_t> raw;
    const FieldDesc& desc;
    FieldValue value;
};

using FieldVisitor = std::function<void(const FieldView&, const PacketDesc&)>;

std::pair<size_t, size_t> scan_packets(const PacketDb& db, std::span<const uint8_t> data, const FieldVisitor& visitor);

#endif // PACKET_PARSER_HPP
