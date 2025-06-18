#ifndef SPLIP_HPP
#define SPLIP_HPP
#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstring>
#include <functional>

namespace slip {
    // SLIP special characters
    constexpr uint8_t END = 0xC0;     // End of packet
    constexpr uint8_t ESC = 0xDB;     // Escape character
    constexpr uint8_t ESC_END = 0xDC; // Escaped END character
    constexpr uint8_t ESC_ESC = 0xDD; // Escaped ESC character
    constexpr uint8_t ACK = 0x06;     // Define ACK as ASCII 0x06 (Acknowledge)
    constexpr uint8_t NAK = 0x15;     // Define NAK as ASCII 0x15 (Negative Acknowledge)

    class SlipError : public std::runtime_error {
    public:
        explicit SlipError(const std::string& message) : std::runtime_error(message) {}
    };

    std::vector<uint8_t> encode(const std::span<const uint8_t>& data);

    using PacketHandler = std::function<void(std::span<const uint8_t>)>;

    class Decoder {
    public:
        Decoder() : state_(State::Normal) {}

        void setPacketHandler(PacketHandler handler) {
            onPacket_ = std::move(handler);
        }

        void reset() {
            buffer_.clear();
            state_ = State::Normal;
        }

        std::vector<uint8_t> decode(const std::span<const uint8_t>& data);
        bool isComplete() const {
            return state_ == State::Normal && !buffer_.empty();
        }
        std::vector<uint8_t> getBuffer() const {
            return buffer_;
        }
        void clearBuffer() {
            buffer_.clear();
        }
        static std::vector<uint8_t> makeResponse(uint8_t type) {
            return encode(std::span<const uint8_t>(&type, 1));
        }
    private:
        enum class State {
            Normal,
            Escaped
        };

        State state_;
        std::vector<uint8_t> buffer_;
        PacketHandler onPacket_;

        void processByte(uint8_t byte);
    };
    
    static inline std::vector<uint8_t> decode(const std::span<const uint8_t>& data) {
        Decoder decoder;
        return decoder.decode(data);
    }
}

#endif // SPLIP_HPP
