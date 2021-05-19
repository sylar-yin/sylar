#ifndef __SYLAR_HTTP2_FRAME_H__
#define __SYLAR_HTTP2_FRAME_H__

#include "sylar/bytearray.h"
#include "sylar/stream.h"
#include "hpack.h"

namespace sylar {
namespace http2 {

#pragma pack(push)
#pragma pack(1)

enum class FrameType {
    DATA            = 0x0,
    HEADERS         = 0x1,
    PRIORITY        = 0x2,
    RST_STREAM      = 0x3,
    SETTINGS        = 0x4,
    PUSH_PROMISE    = 0x5,
    PING            = 0x6,
    GOAWAY          = 0x7,
    WINDOW_UPDATE   = 0x8,
    CONTINUATION    = 0x9,
};

enum class FrameFlagData {
    END_STREAM      = 0x1,
    PADDED          = 0x8
};

enum class FrameFlagHeaders {
    END_STREAM      = 0x1,
    END_HEADERS     = 0x4,
    PADDED          = 0x8,
    PRIORITY        = 0x20
};

enum class FrameFlagSettings {
    ACK             = 0x1
};

enum class FrameFlagPing {
    ACK             = 0x1
};

enum class FrameFlagContinuation {
    END_HEADERS     = 0x4
};

enum class FrameFlagPromise {
    END_HEADERS     = 0x4,
    PADDED          = 0x8
};

enum class FrameR {
    UNSET           = 0x0,
    SET             = 0x1,
};

/*
HTTP2 frame 格式
+-----------------------------------------------+
|                 Length (24)                   |
+---------------+---------------+---------------+
|   Type (8)    |   Flags (8)   |
+-+-------------+---------------+-------------------------------+
|R|                 Stream Identifier (31)                      |
+=+=============================================================+
|                   Frame Payload (0...)                      ...
+---------------------------------------------------------------+
*/

struct FrameHeader {
    static const uint32_t SIZE = 9;
    typedef std::shared_ptr<FrameHeader> ptr;
    union {
        struct {
            uint8_t type;
            uint32_t length : 24;
        };
        uint32_t len_type = 0;
    };
    uint8_t flags = 0;
    union {
        struct {
            uint32_t identifier : 31;
            uint32_t r : 1;
        };
        uint32_t r_id = 0;
    };

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba);
    bool readFrom(ByteArray::ptr ba);
};

class IFrame {
public:
    typedef std::shared_ptr<IFrame> ptr;

    virtual ~IFrame() {}
    virtual std::string toString() const = 0;
    virtual bool writeTo(ByteArray::ptr ba, const FrameHeader& header) = 0;
    virtual bool readFrom(ByteArray::ptr ba, const FrameHeader& header) = 0;
};

struct Frame {
    typedef std::shared_ptr<Frame> ptr;
    FrameHeader header;
    IFrame::ptr data;

    std::string toString() const;
};

/*
 +---------------+
 |Pad Length? (8)|
 +---------------+-----------------------------------------------+
 |                            Data (*)                         ...
 +---------------------------------------------------------------+
 |                           Padding (*)                       ...
 +---------------------------------------------------------------+ 
*/

struct DataFrame : public IFrame {
    typedef std::shared_ptr<DataFrame> ptr;
    uint8_t pad = 0;        //Flag & FrameFlagData::PADDED
    std::string data;
    std::string padding;

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

/*
 +-+-------------------------------------------------------------+
 |E|                  Stream Dependency (31)                     |
 +-+-------------+-----------------------------------------------+
 |   Weight (8)  |
 +-+-------------+
*/

struct PriorityFrame : public IFrame {
    typedef std::shared_ptr<PriorityFrame> ptr;
    static const uint32_t SIZE = 5;
    union {
        struct {
            uint32_t stream_dep : 31;
            uint32_t exclusive : 1;
        };
        uint32_t e_stream_dep = 0;
    };
    uint8_t weight = 0;

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

/*
 +---------------+
 |Pad Length? (8)|
 +-+-------------+-----------------------------------------------+
 |E|                 Stream Dependency? (31)                     |
 +-+-------------+-----------------------------------------------+
 |  Weight? (8)  |
 +-+-------------+-----------------------------------------------+
 |                   Header Block Fragment (*)                 ...
 +---------------------------------------------------------------+
 |                           Padding (*)                       ...
 +---------------------------------------------------------------+
 */

struct HeadersFrame : public IFrame {
    typedef std::shared_ptr<HeadersFrame> ptr;
    uint8_t pad = 0;        //flag & FrameFlagHeaders::PADDED
    PriorityFrame priority; //flag & FrameFlagHeaders::PRIORITY
    std::string data;
    std::string padding;

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

/*
 +---------------------------------------------------------------+
 |                        Error Code (32)                        |
 +---------------------------------------------------------------+
*/

struct RstStreamFrame : public IFrame {
    typedef std::shared_ptr<RstStreamFrame> ptr;
    static const uint32_t SIZE = 4;
    uint32_t error_code = 0;

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

/*
 +-------------------------------+
 |       Identifier (16)         |
 +-------------------------------+-------------------------------+
 |                        Value (32)                             |
 +---------------------------------------------------------------+
*/

struct SettingsItem {
    uint16_t identifier = 0;
    uint32_t value = 0;

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba);
    bool readFrom(ByteArray::ptr ba);
};

struct SettingsFrame : public IFrame {
    typedef std::shared_ptr<SettingsFrame> ptr;
    enum class Settings {
        HEADER_TABLE_SIZE           = 0x1,
        ENABLE_PUSH                 = 0x2,
        MAX_CONCURRENT_STREAMS      = 0x3,
        INITIAL_WINDOW_SIZE         = 0x4,
        MAX_FRAME_SIZE              = 0x5,
        MAX_HEADER_LIST_SIZE        = 0x6
    };

    static std::string SettingsToString(Settings s);
    std::string toString() const;

    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);

    std::vector<SettingsItem> items;
};

/*
 +---------------+
 |Pad Length? (8)|
 +-+-------------+-----------------------------------------------+
 |R|                  Promised Stream ID (31)                    |
 +-+-----------------------------+-------------------------------+
 |                   Header Block Fragment (*)                 ...
 +---------------------------------------------------------------+
 |                           Padding (*)                       ...
 +---------------------------------------------------------------+
*/

struct PushPromisedFrame : public IFrame {
    typedef std::shared_ptr<PushPromisedFrame> ptr;
    uint8_t pad = 0;
    union {
        struct {
            uint32_t stream_id : 31;
            uint32_t r : 1;
        };
        uint32_t r_stream_id = 0;
    };
    std::string data;       //headers
    std::string padding;

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

/*
 +---------------------------------------------------------------+
 |                                                               |
 |                      Opaque Data (64)                         |
 |                                                               |
 +---------------------------------------------------------------+
 */

struct PingFrame : public IFrame {
    typedef std::shared_ptr<PingFrame> ptr;
    static const uint32_t SIZE = 8;
    union {
        uint8_t data[8];
        uint64_t uint64 = 0;
    };

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

/*
 +-+-------------------------------------------------------------+
 |R|                  Last-Stream-ID (31)                        |
 +-+-------------------------------------------------------------+
 |                      Error Code (32)                          |
 +---------------------------------------------------------------+
 |                  Additional Debug Data (*)                    |
 +---------------------------------------------------------------+
 */

struct GoAwayFrame : public IFrame {
    typedef std::shared_ptr<GoAwayFrame> ptr;
    union {
        struct {
            uint32_t last_stream_id : 31;
            uint32_t r : 1;
        };
        uint32_t r_last_stream_id = 0;
    };
    uint32_t error_code = 0;
    std::string data;

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

/*
 +-+-------------------------------------------------------------+
 |R|              Window Size Increment (31)                     |
 +-+-------------------------------------------------------------+
 */

struct WindowUpdateFrame : public IFrame {
    typedef std::shared_ptr<WindowUpdateFrame> ptr;
    static const uint32_t SIZE = 4;
    union {
        struct {
            uint32_t increment : 31;
            uint32_t r : 1;
        };
        uint32_t r_increment = 0;
    };

    std::string toString() const;
    bool writeTo(ByteArray::ptr ba, const FrameHeader& header);
    bool readFrom(ByteArray::ptr ba, const FrameHeader& header);
};

class FrameCodec {
public:
    typedef std::shared_ptr<FrameCodec> ptr;

    Frame::ptr parseFrom(Stream::ptr stream);
    int32_t serializeTo(Stream::ptr stream, Frame::ptr frame);
};


std::string FrameTypeToString(FrameType type);
std::string FrameFlagDataToString(FrameFlagData flag);
std::string FrameFlagHeadersToString(FrameFlagHeaders flag);
std::string FrameFlagSettingsToString(FrameFlagSettings flag);
std::string FrameFlagPingToString(FrameFlagPing flag);
std::string FrameFlagContinuationToString(FrameFlagContinuation flag);
std::string FrameFlagPromiseToString(FrameFlagContinuation flag);
std::string FrameFlagToString(uint8_t type, uint8_t flag);
std::string FrameRToString(FrameR r);

#pragma pack(pop)

}
}

#endif
