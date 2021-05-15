#include "frame.h"
#include <sstream>
#include "sylar/log.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

std::string FrameHeader::toString() const {
    std::stringstream ss;
    ss << "[FrameHeader length=" << length
       << " type=" << FrameTypeToString((FrameType)type)
       << " flags=" << FrameFlagToString(type, flags)
       << " r=" << (int)r
       << " identifier=" << identifier
       << "]";
    return ss.str();
}

bool FrameHeader::writeTo(ByteArray::ptr ba) {
    try {
        ba->writeFuint32(len_type);
        ba->writeFuint8(flags);
        ba->writeFuint32(r_id);
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write FrameHeader fail, " << toString();
    }
    return false;
}

bool FrameHeader::readFrom(ByteArray::ptr ba) {
    try {
        len_type = ba->readFuint32();
        flags = ba->readFuint8();
        r_id = ba->readFuint32();
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read FrameHeader fail, " << toString();
    }
    return false;
}


std::string DataFrame::toString() const {
    std::stringstream ss;
    ss << "[DataFrame";
    if(pad) {
        ss << " pad=" << (uint32_t)pad;
    }
    ss << " data.size=" << data.size();
    ss << "]";
    return ss.str();
}

bool DataFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        if(header.flags & (uint8_t)FrameFlagData::PADDED) {
            ba->writeFuint8(pad);
            ba->write(data.c_str(), data.size());
            ba->write(padding.c_str(), padding.size());
        } else {
            ba->write(data.c_str(), data.size());
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write DataFrame fail, " << toString();
    }
    return false;
}

bool DataFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        if(header.flags & (uint8_t)FrameFlagData::PADDED) {
            pad = ba->readFuint8();
            data.resize(header.length - pad - 1);
            ba->read(&data[0], data.size());
            padding.resize(pad);
            ba->read(&padding[0], pad);
        } else {
            data.resize(header.length);
            ba->read(&data[0], data.size());
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read DataFrame fail, " << toString();
    }
    return false;
}

std::string PriorityFrame::toString() const {
    std::stringstream ss;
    ss << "[PriorityFrame exclusive=" << exclusive
       << " stream_dep=" << stream_dep
       << " weight=" << weight << "]";
    return ss.str();
}

bool PriorityFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        ba->writeFuint32(e_stream_dep);
        ba->writeFuint8(weight);
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write PriorityFrame fail, " << toString();
    }
    return false;
}

bool PriorityFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        e_stream_dep = ba->readFuint32();
        weight = ba->readFuint8();
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read PriorityFrame fail, " << toString();
    }
    return false;
}

std::string HeadersFrame::toString() const {
    std::stringstream ss;
    ss << "[HeadersFrame";
    if(pad) {
        ss << " pad=" << (uint32_t)pad;
    }
    ss << " data.size=" << data.size();
    ss << "]";
    return ss.str();
}

bool HeadersFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        if(header.flags & (uint8_t)FrameFlagHeaders::PADDED) {
            ba->writeFuint8(pad);
        }
        if(header.flags & (uint8_t)FrameFlagHeaders::PRIORITY) {
            priority.writeTo(ba, header);
        }
        ba->write(data.c_str(), data.size());
        if(header.flags & (uint8_t)FrameFlagHeaders::PADDED) {
            ba->write(padding.c_str(), padding.size());
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write HeadersFrame fail, " << toString();
    }
    return false;
}

bool HeadersFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        int len = header.length;
        if(header.flags & (uint8_t)FrameFlagHeaders::PADDED) {
            pad = ba->readFuint8();
            len -= 1 + pad;
        }
        if(header.flags & (uint8_t)FrameFlagHeaders::PRIORITY) {
            priority.readFrom(ba, header);
            len -= 5;
        }
        //check len
        data.resize(len);
        ba->read(&data[0], data.size());
        if(header.flags & (uint8_t)FrameFlagHeaders::PADDED) {
            padding.resize(pad);
            ba->read(&padding[0], padding.size());
        }

        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read HeadersFrame fail, " << toString();
    }
    return false;
}

std::string RstFrame::toString() const {
    std::stringstream ss;
    ss << "[RstFrame error_code=" << error_code << "]";
    return ss.str();
}

bool RstFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        ba->writeFuint32(error_code);
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write RstFrame fail, " << toString();
    }
    return false;
}

bool RstFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        error_code = ba->readFuint32();
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read RstFrame fail, " << toString();
    }
    return false;
}

static std::vector<std::string> s_settings_string = {
        "",
        "HEADER_TABLE_SIZE",
        "ENABLE_PUSH",
        "MAX_CONCURRENT_STREAMS",
        "INITIAL_WINDOW_SIZE",
        "MAX_FRAME_SIZE",
        "MAX_HEADER_LIST_SIZE"
};

std::string SettingsFrame::SettingsToString(Settings s) {
    uint32_t idx = (uint32_t)s;
    if(idx <= 0 || idx > 6) {
        return "UNKONW(" + std::to_string(idx) + ")";
    }
    return s_settings_string[idx];
}

std::string SettingsItem::toString() const {
    std::stringstream ss;
    ss << "[SettingsFrame identifier="
       << SettingsFrame::SettingsToString((SettingsFrame::Settings)identifier)
       << " value=" << value << "]";
    return ss.str();
}

bool SettingsItem::writeTo(ByteArray::ptr ba) {
    ba->writeFuint16(identifier);
    ba->writeFuint32(value);
    return true;

}

bool SettingsItem::readFrom(ByteArray::ptr ba) {
    identifier = ba->readFuint16();
    value = ba->readFuint32();
    return true;
}

bool SettingsFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        for(auto& i : items) {
            i.writeTo(ba);
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write SettingsFrame fail, " << toString();
    }
    return false;
}

bool SettingsFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        uint32_t size = header.length / sizeof(SettingsItem);
        items.resize(size);
        for(uint32_t i = 0; i < size; ++i) {
            items[i].readFrom(ba);
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read SettingsFrame fail, " << toString();
    }
    return false;
}

std::string SettingsFrame::toString() const {
    std::stringstream ss;
    ss << "[SettingsFrame size=" << items.size() << " items=[";
    for(auto& i : items) {
        ss << i.toString();
    }
    ss << "]]";
    return ss.str();
}


std::string PushPromisedFrame::toString() const {
    std::stringstream ss;
    ss << "[PushPromisedFrame";
    if(pad) {
        ss << " pad=" << (uint32_t)pad;
    }
    ss << " data.size=" << data.size();
    ss << "]";
    return ss.str();
}

bool PushPromisedFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        if(header.flags & (uint8_t)FrameFlagPromise::PADDED) {
            ba->writeFuint8(pad);
            ba->writeFuint32(r_stream_id);
            ba->write(data.c_str(), data.size());
            ba->write(padding.c_str(), padding.size());
        } else {
            ba->writeFuint32(r_stream_id);
            ba->write(data.c_str(), data.size());
        }
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write PushPromisedFrame fail, " << toString();
    }
    return false;
}

bool PushPromisedFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        if(header.flags & (uint8_t)FrameFlagPromise::PADDED) {
            pad = ba->readFuint8();
            r_stream_id = ba->readFuint32();
            data.resize(header.length - 5 - pad);
            ba->read(&data[0], data.size());
            padding.resize(pad);
            ba->read(&padding[0], padding.size());
        } else {
            r_stream_id = ba->readFuint32();
            data.resize(header.length - 4);
            ba->read(&data[0], data.size());
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read PushPromisedFrame fail, " << toString();
    }
    return false;
}


std::string PingFrame::toString() const {
    std::stringstream ss;
    ss << "[PingFrame uint64=" << uint64 << "]";
    return ss.str();
}

bool PingFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        ba->write(data, 8);
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write PingFrame fail, " << toString();
    }
    return false;
}

bool PingFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        ba->read(data, 8);
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read PingFrame fail, " << toString();
    }
    return false;
}


std::string GoAwayFrame::toString() const {
    std::stringstream ss;
    ss << "[GoAwayFrame r=" << r
       << " last_stream_id=" << last_stream_id
       << " error_code=" << error_code
       << " debug.size=" << data.size()
       << "]";
    return ss.str();
}

bool GoAwayFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        ba->writeFuint32(r_last_stream_id);
        ba->writeFuint32(error_code);
        if(!data.empty()) {
            ba->write(data.c_str(), data.size());
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write GoAwayFrame fail, " << toString();
    }
    return false;
}

bool GoAwayFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        r_last_stream_id = ba->readFuint32();
        error_code = ba->readFuint32();
        if(header.length > 8) {
            data.resize(header.length - 8);
            ba->read(&data[0], data.size());
        }
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read GoAwayFrame fail, " << toString();
    }
    return false;
}

std::string WindowUpdateFrame::toString() const {
    std::stringstream ss;
    ss << "[WindowUpdateFrame r=" << r
       << " increment=" << increment
       << "]";
    return ss.str();
}

bool WindowUpdateFrame::writeTo(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        ba->writeFuint32(r_increment);
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "write WindowUpdateFrame fail, " << toString();
    }
    return false;
}

bool WindowUpdateFrame::readFrom(ByteArray::ptr ba, const FrameHeader& header) {
    try {
        r_increment = ba->readFuint32();
        return true;
    } catch(...) {
        SYLAR_LOG_WARN(g_logger) << "read WindowUpdateFrame fail, " << toString();
    }
    return false;
}

static const std::vector<std::string> s_frame_types = {
    "DATA",
    "HEADERS",
    "PRIORITY",
    "RST_STREAM",
    "SETTINGS",
    "PUSH_PROMISE",
    "PING",
    "GOAWAY",
    "WINDOW_UPDATE",
    "CONTINUATION",
};

std::string FrameTypeToString(FrameType type) {
    uint8_t v = (uint8_t)type;
    if(v > 9) {
        return "UNKONW(" + std::to_string(v) + ")";
    }
    return s_frame_types[v];
}

#define XX(ff, str) \
    std::string rt; \
    if((uint8_t)flag & (uint8_t)ff) { \
        rt = str; \
    }

#define XX_IF(ff, str) \
    if((uint8_t)flag & (uint8_t)ff) { \
        if(!rt.empty()) { \
            rt += "|"; \
        } \
        rt += str; \
    }

#define XX_END() \
    if(rt.empty()) { \
        if((uint8_t)flag) { \
            rt = "UNKONW(" + std::to_string((uint32_t)flag) + ")"; \
        } else { \
            rt = "0"; \
        } \
    } \
    return rt;


std::string FrameFlagDataToString(FrameFlagData flag) {
    XX(FrameFlagData::END_STREAM, "END_STREAM");
    XX_IF(FrameFlagData::PADDED, "PADDED");
    XX_END();
}

std::string FrameFlagHeadersToString(FrameFlagHeaders flag) {
    XX(FrameFlagHeaders::END_STREAM, "END_STREAM");
    XX_IF(FrameFlagHeaders::END_HEADERS, "END_HEADERS");
    XX_IF(FrameFlagHeaders::PADDED, "PADDED");
    XX_IF(FrameFlagHeaders::PRIORITY, "PRIORITY");
    XX_END();
}

std::string FrameFlagSettingsToString(FrameFlagSettings flag) {
    XX(FrameFlagSettings::ACK, "ACK");
    XX_END();
}
std::string FrameFlagPingToString(FrameFlagPing flag) {
    XX(FrameFlagPing::ACK, "ACK");
    XX_END();
}
std::string FrameFlagContinuationToString(FrameFlagContinuation flag) {
    XX(FrameFlagContinuation::END_HEADERS, "END_HEADERS");
    XX_END();
}
std::string FrameFlagPromiseToString(FrameFlagPromise flag) {
    XX(FrameFlagPromise::END_HEADERS, "END_HEADERS");
    XX_IF(FrameFlagPromise::PADDED, "PADDED");
    XX_END();
}

std::string FrameFlagToString(uint8_t type, uint8_t flag) {
    switch((FrameType)type) {
        case FrameType::DATA:
            return FrameFlagDataToString((FrameFlagData)flag);
        case FrameType::HEADERS:
            return FrameFlagHeadersToString((FrameFlagHeaders)flag);
        case FrameType::SETTINGS:
            return FrameFlagSettingsToString((FrameFlagSettings)flag);
        case FrameType::PING:
            return FrameFlagPingToString((FrameFlagPing)flag);
        case FrameType::CONTINUATION:
            return FrameFlagContinuationToString((FrameFlagContinuation)flag);
        default:
            return "UNKONW(" + std::to_string((uint32_t)flag) + ")";
    }
}

std::string FrameRToString(FrameR r) {
    if(r == FrameR::SET) {
        return "SET";
    } else {
        return "UNSET";
    }
}

}
}
