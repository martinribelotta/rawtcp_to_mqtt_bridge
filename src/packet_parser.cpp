#include "packet_parser.hpp"
#include <cassert>
#include <cstring>

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
    }
    return FieldValue();
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

            // Llamar visitor para cada campo
            for (const auto& field : packet.fields) {
                size_t len = type_size(field.type, field);
                if (view.size() < field.offset + len) continue;
                visitor(FieldView{ std::span<const uint8_t>(view.data() + field.offset, len), field });
            }

            offset += required_size;
            ++packets_found;
            found = true;
            break;
        }
        if (!found) {
            // No se reconoció ningún paquete en esta posición, avanzar 1 byte
            ++offset;
        }
    }
    return {packets_found, offset};
}
