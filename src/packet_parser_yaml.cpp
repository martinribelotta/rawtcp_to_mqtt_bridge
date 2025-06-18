#include "packet_parser_yaml.hpp"
#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <cctype>

namespace {

FieldType parse_field_type(const std::string& str) {
    if (str == "uint8") return FieldType::UINT8;
    if (str == "uint16") return FieldType::UINT16;
    if (str == "uint32") return FieldType::UINT32;
    if (str == "uint64") return FieldType::UINT64;
    if (str == "int8") return FieldType::INT8;
    if (str == "int16") return FieldType::INT16;
    if (str == "int32") return FieldType::INT32;
    if (str == "int64") return FieldType::INT64;
    if (str == "float32") return FieldType::FLOAT32;
    if (str == "float64") return FieldType::FLOAT64;
    if (str == "bytearray") return FieldType::BYTEARRAY;
    throw std::runtime_error("Unknown field type: " + str);
}

uint64_t parse_integer(const YAML::Node& node) {
    if (node.IsScalar()) {
        std::string s = node.as<std::string>();
        size_t idx = 0;
        int base = 10;
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            base = 16;
            idx = 2;
        }
        return std::stoull(s.substr(idx), nullptr, base);
    }
    return node.as<uint64_t>();
}

FieldValue parse_value(const YAML::Node& node, FieldType type) {
    switch (type) {
    case FieldType::UINT8:   return FieldValue(static_cast<uint8_t>(parse_integer(node)));
    case FieldType::UINT16:  return FieldValue(static_cast<uint16_t>(parse_integer(node)));
    case FieldType::UINT32:  return FieldValue(static_cast<uint32_t>(parse_integer(node)));
    case FieldType::UINT64:  return FieldValue(static_cast<uint64_t>(parse_integer(node)));
    case FieldType::INT8:    return FieldValue(static_cast<int8_t>(parse_integer(node)));
    case FieldType::INT16:   return FieldValue(static_cast<int16_t>(parse_integer(node)));
    case FieldType::INT32:   return FieldValue(static_cast<int32_t>(parse_integer(node)));
    case FieldType::INT64:   return FieldValue(static_cast<int64_t>(parse_integer(node)));
    case FieldType::FLOAT32: return FieldValue(node.as<float>());
    case FieldType::FLOAT64: return FieldValue(node.as<double>());
    case FieldType::BYTEARRAY: {
        std::vector<uint8_t> v;
        if (node.IsSequence()) {
            for (const auto& elem : node) v.push_back(static_cast<uint8_t>(parse_integer(elem)));
        } else if (node.IsScalar()) {
            std::string s = node.as<std::string>();
            if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s = s.substr(2);
            if (s.size() % 2 != 0) throw std::runtime_error("BYTEARRAY must have even length");
            for (size_t i = 0; i < s.size(); i += 2) {
                std::string byteStr = s.substr(i, 2);
                v.push_back(static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16)));
            }
        }
        return FieldValue(std::move(v));
    }
    }
    throw std::runtime_error("Invalid type in parse_value");
}

}

PacketDb packetdb_from_yaml(const std::string& yaml_text) {
    YAML::Node root = YAML::Load(yaml_text);
    if (!root.IsMap()) throw std::runtime_error("Packet YAML must be a mapping of packets");

    PacketDb db;
    for (auto it = root.begin(); it != root.end(); ++it) {
        PacketDesc pkt;
        pkt.name = it->first.as<std::string>();
        const YAML::Node& packet_node = it->second;
        
        if (const YAML::Node& mqtt = packet_node["mqtt"]) {
            if (mqtt["topic"]) pkt.mqtt.topic = mqtt["topic"].as<std::string>();
            if (mqtt["payload"]) pkt.mqtt.payload = mqtt["payload"].as<std::string>();
            if (mqtt["qos"]) pkt.mqtt.qos = mqtt["qos"].as<uint8_t>();
            if (mqtt["retain"]) pkt.mqtt.retain = mqtt["retain"].as<bool>();
        }

        const YAML::Node& fields = packet_node["fields"];
        if (!fields.IsSequence()) throw std::runtime_error("Packet " + pkt.name + " must have a sequence of fields");
        size_t field_idx = 0;
        bool found_id = false;
        for (const YAML::Node& field : fields) {
            FieldDesc fdesc;
            fdesc.name = field["name"].as<std::string>();
            fdesc.type = parse_field_type(field["type"].as<std::string>());
            fdesc.offset = field["offset"].as<size_t>();
            if (field["bitfield"]) {
                const auto& bf = field["bitfield"];
                BitfieldInfo binfo;
                binfo.bit_offset = bf["bit_offset"].as<uint8_t>();
                binfo.bit_count = bf["bit_count"].as<uint8_t>();
                fdesc.bitfield = binfo;
            }
            if (fdesc.type == FieldType::BYTEARRAY && field["length"])
                fdesc.length = field["length"].as<size_t>();
            else if (fdesc.type == FieldType::BYTEARRAY && !field["length"])
                throw std::runtime_error("BYTEARRAY must have 'length' field");

            if (field["value"]) {
                fdesc.value = parse_value(field["value"], fdesc.type);
            }

            if (!found_id && fdesc.value.has_value()) {
                pkt.id_field_index = field_idx;
                pkt.id_value = *fdesc.value;
                found_id = true;
            }
            pkt.fields.push_back(std::move(fdesc));
            ++field_idx;
        }
        if (!found_id)
            throw std::runtime_error("Packet " + pkt.name + " does not have an identifier field (with 'value')");
        db.push_back(std::move(pkt));
    }
    return db;
}
