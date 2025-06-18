#ifndef PACKET_PARSER_YAML_HPP
#define PACKET_PARSER_YAML_HPP

#include "packet_parser.hpp"
#include <string>

PacketDb packetdb_from_yaml(const std::string& yaml_text);

#endif // PACKET_PARSER_YAML_HPP
