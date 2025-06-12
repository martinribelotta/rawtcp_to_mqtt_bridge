#include "packet_parser.hpp"
#include <cassert>
#include <cstring>


#include <fmt/format.h>

std::string FieldValue::to_string() const {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            std::string result = "bytes[";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i > 0) result += " ";
                result += fmt::format("{:02X}", v[i]);
            }
            result += "]";
            return result;
        } else if constexpr (std::is_floating_point_v<T>) {
            return fmt::format("{:.6g}", v);
        } else if constexpr (std::is_integral_v<T>) {
            if constexpr (std::is_unsigned_v<T>) {
                return fmt::format("0x{:X} ({})", v, v);
            } else {
                return std::to_string(v);
            }
        }
        return "[invalid]";
    }, value_);
}

namespace {

FieldValue extract_value(FieldType t, std::span<const uint8_t> data, const FieldDesc& desc) {
    const uint8_t* ptr = data.data() + desc.offset;
    switch (t) {
    case FieldType::UINT8:
        return FieldValue(ptr[0]);
    case FieldType::UINT16:
        return FieldValue(uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8));
    case FieldType::UINT32:
        return FieldValue(uint32_t(ptr[0]) | (uint32_t(ptr[1]) << 8) | (uint32_t(ptr[2]) << 16) | (uint32_t(ptr[3]) << 24));
    case FieldType::UINT64: {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v |= uint64_t(ptr[i]) << (i * 8);
        return FieldValue(v);
    }
    case FieldType::INT8:
        return FieldValue(int8_t(ptr[0]));
    case FieldType::INT16:
        return FieldValue(int16_t(uint16_t(ptr[0]) | (uint16_t(ptr[1]) << 8)));
    case FieldType::INT32:
        return FieldValue(int32_t(uint32_t(ptr[0]) | (uint32_t(ptr[1]) << 8) | (uint32_t(ptr[2]) << 16) | (uint32_t(ptr[3]) << 24)));
    case FieldType::INT64: {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) v |= uint64_t(ptr[i]) << (i * 8);
        return FieldValue(static_cast<int64_t>(v));
    }
    case FieldType::FLOAT32: {
        float f;
        std::memcpy(&f, ptr, 4);
        return FieldValue(f);
    }
    case FieldType::FLOAT64: {
        double d;
        std::memcpy(&d, ptr, 8);
        return FieldValue(d);
    }
    case FieldType::BYTEARRAY: {
        size_t len = desc.length.value_or(0);
        return FieldValue(std::vector<uint8_t>(ptr, ptr + len));
    }
    default:
        throw std::runtime_error("Invalid field type in extract_value");
    }
}

size_t type_size(FieldType t, const FieldDesc& desc) {
    switch (t) {
    case FieldType::UINT8: case FieldType::INT8: return 1;
    case FieldType::UINT16: case FieldType::INT16: return 2;
    case FieldType::UINT32: case FieldType::INT32: case FieldType::FLOAT32: return 4;
    case FieldType::UINT64: case FieldType::INT64: case FieldType::FLOAT64: return 8;
    case FieldType::BYTEARRAY: return desc.length.value_or(0);
    }
    return 0;
}

size_t packet_total_size(const PacketDesc& pkt) {
    size_t max_end = 0;
    for (const auto& f : pkt.fields) {
        size_t end = f.offset + type_size(f.type, f);
        if (end > max_end) max_end = end;
    }
    return max_end;
}

}

std::string FieldDesc::to_string() const {
    std::string result = "FieldDesc{name: " + name;
    result += ", type: ";
    switch (type) {
        case FieldType::UINT8:    result += "UINT8"; break;
        case FieldType::UINT16:   result += "UINT16"; break;
        case FieldType::UINT32:   result += "UINT32"; break;
        case FieldType::UINT64:   result += "UINT64"; break;
        case FieldType::INT8:     result += "INT8"; break;
        case FieldType::INT16:    result += "INT16"; break;
        case FieldType::INT32:    result += "INT32"; break;
        case FieldType::INT64:    result += "INT64"; break;
        case FieldType::FLOAT32:  result += "FLOAT32"; break;
        case FieldType::FLOAT64:  result += "FLOAT64"; break;
        case FieldType::BYTEARRAY:result += "BYTEARRAY"; break;
        default:                  result += "UNKNOWN"; break;
    }
    result += ", offset: " + std::to_string(offset);
    if (bitfield) {
        result += ", bitfield: {offset: " + std::to_string(bitfield->bit_offset)
               + ", count: " + std::to_string(bitfield->bit_count) + "}";
    }
    if (length) {
        result += ", length: " + std::to_string(*length);
    }
    if (value) {
        result += ", value: " + fmt::format("[{}]", value->to_string());
    }
    result += "}";
    return result;
}

std::pair<size_t, size_t> scan_packets(const PacketDb& db, std::span<const uint8_t> data, const FieldVisitor& visitor) {
    size_t packets_found = 0;
    size_t offset = 0;
    while (offset < data.size()) {
        bool found = false;
        for (const auto& packet : db) {
            const auto& id_field = packet.fields[packet.id_field_index];
            size_t id_size = type_size(id_field.type, id_field);
            size_t required_size = packet_total_size(packet);
            if (data.size() - offset < required_size) continue;
            std::span<const uint8_t> view = data.subspan(offset, required_size);

            FieldValue id_val = extract_value(id_field.type, view, id_field);
            if (!(id_val == packet.id_value)) continue;

            for (const auto& field : packet.fields) {
                size_t len = type_size(field.type, field);
                if (view.size() < field.offset + len) continue;
                auto field_value = extract_value(field.type, view, field);
                visitor(FieldView{ 
                    std::span<const uint8_t>(view.data() + field.offset, len), 
                    field,
                    field_value
                }, packet);
            }

            offset += required_size;
            ++packets_found;
            found = true;
            break;
        }
        if (!found) {
            ++offset;
        }
    }
    return {packets_found, offset};
}
