#ifndef PACKET_PARSER_YAML_HPP
#define PACKET_PARSER_YAML_HPP

#include "packet_parser.hpp"
#include <string>

// Throws std::runtime_error on parse error
// Requires YAML to be in the format described in the examples
PacketDb packetdb_from_yaml(const std::string& yaml_text);

#endif // PACKET_PARSER_YAML_HPP
