#include "slip.hpp"

namespace slip {

std::vector<uint8_t> encode(const std::span<const uint8_t>& data) {
    std::vector<uint8_t> encoded;
    encoded.reserve(data.size() * 2);  // Worst case scenario
    
    encoded.push_back(END);  // Start delimiter
    
    for (uint8_t byte : data) {
        if (byte == END) {
            encoded.push_back(ESC);
            encoded.push_back(ESC_END);
        } else if (byte == ESC) {
            encoded.push_back(ESC);
            encoded.push_back(ESC_ESC);
        } else {
            encoded.push_back(byte);
        }
    }
    
    encoded.push_back(END);  // End delimiter
    return encoded;
}

std::vector<uint8_t> Decoder::decode(const std::span<const uint8_t>& data) {
    for (uint8_t byte : data) {
        processByte(byte);
    }
    return buffer_;
}

void Decoder::processByte(uint8_t byte) {
    switch (state_) {
        case State::Normal:
            if (byte == END) {
                if (!buffer_.empty()) {
                    if (onPacket_) {
                        onPacket_(std::span<const uint8_t>(buffer_));
                    }
                    buffer_.clear();
                }
            } else if (byte == ESC) {
                state_ = State::Escaped;
            } else {
                buffer_.push_back(byte);
            }
            break;

        case State::Escaped:
            if (byte == ESC_END) {
                buffer_.push_back(END);
            } else if (byte == ESC_ESC) {
                buffer_.push_back(ESC);
            } else {
                throw SlipError("Invalid escape sequence");
            }
            state_ = State::Normal;
            break;
    }
}

}
