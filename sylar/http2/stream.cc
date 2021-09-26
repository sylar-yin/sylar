#include "stream.h"
#include "http2_stream.h"
#include "sylar/log.h"
#include "sylar/macro.h"

namespace sylar {
namespace http2 {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

static const std::vector<std::string> s_state_strings = {
        "IDLE",
        "OPEN",
        "CLOSED",
        "RESERVED_LOCAL",
        "RESERVED_REMOTE",
        "HALF_CLOSE_LOCAL",
        "HALF_CLOSE_REMOTE",
};

std::string Stream::StateToString(State state) {
    uint8_t v = (uint8_t)state;
    if(v < 7) {
        return s_state_strings[v];
    }
    return "UNKNOW(" + std::to_string((uint32_t)v) + ")";
}

Stream::Stream(std::shared_ptr<Http2Stream> stm, uint32_t id)
    :m_stream(stm)
    ,m_state(State::IDLE)
    ,m_handleCount(0)
    ,m_id(id){

    send_window = stm->getPeerSettings().initial_window_size;
    recv_window = stm->getOwnerSettings().initial_window_size;
}

void Stream::addHandleCount() {
    ++m_handleCount;
}

std::string Stream::getHeader(const std::string& name) const {
    if(!m_recvHPack) {
        return "";
    }
    auto& m = m_recvHPack->getHeaders();
    for(auto& i : m) {
        if(i.name == name) {
            return i.value;
        }
    }
    return "";
}

Stream::~Stream() {
    close();
    SYLAR_LOG_INFO(g_logger) << "Stream::~Stream id=" << m_id
            << " - " << StateToString(m_state);
}

void Stream::close() {
    if(m_handler) {
        m_handler(nullptr);
    }
}

std::shared_ptr<Http2Stream> Stream::getStream() const {
    return m_stream.lock();
}

int32_t Stream::handleRstStreamFrame(Frame::ptr frame, bool is_client) {
    m_state = State::CLOSED;
    return 0;
}

int32_t Stream::handleFrame(Frame::ptr frame, bool is_client) {
    int rt = 0;
    if(frame->header.type == (uint8_t)FrameType::HEADERS) {
        rt = handleHeadersFrame(frame, is_client);
        SYLAR_ASSERT(rt != -1);
    } else if(frame->header.type == (uint8_t)FrameType::DATA) {
        rt = handleDataFrame(frame, is_client);
    } else if(frame->header.type == (uint8_t)FrameType::RST_STREAM) {
        rt = handleRstStreamFrame(frame, is_client);
    }

    if(m_handler) {
        m_handler(frame);
    }

    if(frame->header.flags & (uint8_t)FrameFlagHeaders::END_STREAM) {
        m_state = State::CLOSED;
        if(is_client) {
            if(!m_response) {
                m_response = std::make_shared<http::HttpResponse>(0x20);
            }
            m_response->setBody(m_body);
            if(m_recvHPack) {
                auto& m = m_recvHPack->getHeaders();
                for(auto& i : m) {
                    m_response->setHeader(i.name, i.value);
                }
            }
            Http2InitResponseForRead(m_response);
        } else {
            initRequest();
        }
        SYLAR_LOG_DEBUG(g_logger) << "id=" << m_id << " is_client=" << is_client
            << " req=" << m_request << " rsp=" << m_response;
    }
    return rt;
}

void Stream::initRequest() {
    if(!m_request) {
        m_request = std::make_shared<http::HttpRequest>(0x20);
    }
    m_request->setBody(m_body);
    if(m_recvHPack) {
        auto& m = m_recvHPack->getHeaders();
        for(auto& i : m) {
            m_request->setHeader(i.name, i.value);
        }
    }
    Http2InitRequestForRead(m_request);
}

int32_t Stream::handleHeadersFrame(Frame::ptr frame, bool is_client) {
    auto headers = std::dynamic_pointer_cast<HeadersFrame>(frame->data);
    if(!headers) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " handleHeadersFrame data not HeadersFrame "
            << frame->toString();
        return -1;
    }
    auto stream = getStream();
    if(!stream) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " handleHeadersFrame stream is closed " 
            << frame->toString();
        return -1;
    }

    if(!m_recvHPack) {
        m_recvHPack = std::make_shared<HPack>(stream->getRecvTable());
    }
    return m_recvHPack->parse(headers->data);
}

int32_t Stream::handleDataFrame(Frame::ptr frame, bool is_client) {
    sleep(1);
    if(m_handleCount > 0) {
        return 0;
    }
    auto data = std::dynamic_pointer_cast<DataFrame>(frame->data);
    if(!data) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " handleDataFrame data not DataFrame "
            << frame->toString();
        return -1;
    }
    auto stream = getStream();
    if(!stream) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " handleDataFrame stream is closed " 
            << frame->toString();
        return -1;
    }
    m_body += data->data;
    //SYLAR_LOG_DEBUG(g_logger) << "stream_id=" << m_id << " cur_body_size=" << m_body.size();
    //if(is_client) {
    //    m_response = std::make_shared<http::HttpResponse>(0x20);
    //    m_response->setBody(data->data);
    //} else {
    //    m_request = std::make_shared<http::HttpRequest>(0x20);
    //    m_request->setBody(data->data);
    //}
    return 0;
}

int32_t Stream::sendRequest(sylar::http::HttpRequest::ptr req, bool end_stream, bool async) {
    auto stream = getStream();
    if(!stream) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " sendResponse stream is closed";
        return -1;
    }

    Http2InitRequestForWrite(req, stream->isSsl());

    Frame::ptr headers = std::make_shared<Frame>();
    headers->header.type = (uint8_t)FrameType::HEADERS;
    headers->header.flags = (uint8_t)FrameFlagHeaders::END_HEADERS;
    headers->header.identifier = m_id;
    HeadersFrame::ptr data;
    data = std::make_shared<HeadersFrame>();
    auto m = req->getHeaders();
    if(end_stream && req->getBody().empty()) {
        headers->header.flags |= (uint8_t)FrameFlagHeaders::END_STREAM;
    }

    data->hpack = std::make_shared<HPack>(stream->m_sendTable);
    for(auto& i : m) {
        data->kvs.emplace_back(sylar::ToLower(i.first), i.second);
    }
    // debug stream_id
    data->kvs.push_back(std::make_pair("stream_id", std::to_string(m_id)));
    headers->data = data;
    int32_t ok = stream->sendFrame(headers, async);
    if(ok < 0) {
        SYLAR_LOG_INFO(g_logger) << "sendHeaders fail";
        return ok;
    }
    if(!req->getBody().empty()) {
        ok = stream->sendData(shared_from_this(), req->getBody(), async);
        if(ok <= 0) {
            SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id 
                << " sendData fail, rt=" << ok << " size=" << req->getBody().size();
            return ok;
        }
    }
    return ok;
}

int32_t Stream::sendHeaders(const std::map<std::string, std::string>& m, bool end_stream, bool async) {
    auto stream = getStream();
    if(!stream) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " sendHeaders stream is closed";
        return -1;
    }

    Frame::ptr headers = std::make_shared<Frame>();
    headers->header.type = (uint8_t)FrameType::HEADERS;
    headers->header.flags = (uint8_t)FrameFlagHeaders::END_HEADERS;
    if(end_stream) {
        headers->header.flags |= (uint8_t)FrameFlagHeaders::END_STREAM;
    }
    headers->header.identifier = m_id;
    HeadersFrame::ptr data;
    data = std::make_shared<HeadersFrame>();

    data->hpack = std::make_shared<HPack>(stream->getSendTable());
    for(auto& i : m) {
        data->kvs.emplace_back(sylar::ToLower(i.first), i.second);
    }
    headers->data = data;
    int ok = stream->sendFrame(headers, async);
    if(ok < 0) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " sendHeaders fail " << ok;
        return ok;
    }
    return ok;
}

int32_t Stream::sendResponse(http::HttpResponse::ptr rsp, bool end_stream, bool async) {
    auto stream = getStream();
    if(!stream) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " sendResponse stream is closed";
        return -1;
    }

    Http2InitResponseForWrite(rsp);

    Frame::ptr headers = std::make_shared<Frame>();
    headers->header.type = (uint8_t)FrameType::HEADERS;
    headers->header.flags = (uint8_t)FrameFlagHeaders::END_HEADERS;
    headers->header.identifier = m_id;
    HeadersFrame::ptr data;
    auto m = rsp->getHeaders();
    data = std::make_shared<HeadersFrame>();

    auto trailer = rsp->getHeader("trailer");
    std::set<std::string> trailers;
    if(!trailer.empty()) {
        auto vec = sylar::split(trailer, ',');
        for(auto& i : vec) {
            trailers.insert(sylar::StringUtil::Trim(i));
        }
    }

    if(end_stream && rsp->getBody().empty() && trailers.empty()) {
        headers->header.flags |= (uint8_t)FrameFlagHeaders::END_STREAM;
    }

    data->hpack = std::make_shared<HPack>(stream->getSendTable());
    for(auto& i : m) {
        if(trailers.count(i.first)) {
            continue;
        }

        data->kvs.emplace_back(sylar::ToLower(i.first), i.second);
    }
    headers->data = data;
    int ok = stream->sendFrame(headers, async);
    if(ok < 0) {
        SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
            << " sendResponse send Headers fail";
        return ok;
    }
    if(!rsp->getBody().empty()) {
        ok = stream->sendData(shared_from_this(), rsp->getBody(), async, trailers.empty());
        if(ok < 0) {
            SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_id
                << " sendData fail, rt=" << ok << " size=" << rsp->getBody().size();
        }
    }
    if(end_stream && !trailers.empty()) {
        Frame::ptr headers = std::make_shared<Frame>();
        headers->header.type = (uint8_t)FrameType::HEADERS;
        headers->header.flags = (uint8_t)FrameFlagHeaders::END_HEADERS | (uint8_t)FrameFlagHeaders::END_STREAM;
        headers->header.identifier = m_id;

        HeadersFrame::ptr data = std::make_shared<HeadersFrame>();
        data->hpack = std::make_shared<HPack>(stream->getSendTable());
        for(auto& i : trailers) {
            auto v = rsp->getHeader(i);
            data->kvs.emplace_back(sylar::ToLower(i), v);
        }
        headers->data = data;
        bool ok = stream->sendFrame(headers, async) > 0;
        if(!ok) {
            SYLAR_LOG_INFO(g_logger) << "sendHeaders trailer fail";
            return ok;
        }
    }

    return ok;
}

int32_t Stream::sendFrame(Frame::ptr frame, bool async) {
    auto stream = getStream();
    if(stream) {
        return stream->sendFrame(frame, async);
    }
    return 0;
}

int32_t Stream::updateSendWindowByDiff(int32_t diff) {
    return updateWindowSizeByDiff(&send_window, diff);
}

int32_t Stream::updateRecvWindowByDiff(int32_t diff) {
    return updateWindowSizeByDiff(&recv_window, diff);
}

int32_t Stream::updateWindowSizeByDiff(int32_t* window_size, int32_t diff) {
    int64_t new_value = *window_size + diff;
    if(new_value < 0 || new_value > MAX_INITIAL_WINDOW_SIZE) {
        SYLAR_LOG_DEBUG(g_logger) << (window_size == &recv_window? "recv_window" : "send_window")
            << " update to " << new_value << ", from=" << *window_size << " diff=" << diff << ", invalid"
            << " stream_id=" << m_id << " " << this;
        //return -1;
    }
    sylar::Atomic::addFetch(*window_size, diff);
    //*window_size += diff;
    return 0;
}

StreamClient::ptr StreamClient::Create(Stream::ptr stream) {
    auto rt = std::make_shared<StreamClient>();
    rt->m_stream = stream;
    stream->setFrameHandler(std::bind(&StreamClient::onFrame, rt, std::placeholders::_1));
    return rt;
}

int32_t StreamClient::close() {
    return sendData("", true);
}

int32_t StreamClient::sendData(const std::string& data, bool end_stream) {
    auto stm = m_stream->getStream();
    if(stm) {
        return stm->sendData(m_stream, data, true, end_stream);
    }
    return -1;
}

DataFrame::ptr StreamClient::recvData() {
    auto pd = m_data.pop();
    return pd;
}

int32_t StreamClient::onFrame(Frame::ptr frame) {
    if(!frame) {
        m_data.push(nullptr);
        return 0;
    }
    if(frame->header.type == (uint8_t)FrameType::DATA) {
        auto data = std::dynamic_pointer_cast<DataFrame>(frame->data);
        if(!data) {
            SYLAR_LOG_ERROR(g_logger) << "Stream id=" << m_stream->getId()
                << " onFrame data not DataFrame "
                << frame->toString();
            return -1;
        }
        m_data.push(data);
    }
    if(frame->header.flags & (uint8_t)FrameFlagHeaders::END_STREAM) {
        m_data.push(nullptr);
    }
    return 0;
}

Stream::ptr StreamManager::get(uint32_t id) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_streams.find(id);
    return it == m_streams.end() ? nullptr : it->second;
}

void StreamManager::add(Stream::ptr stream) {
    RWMutexType::WriteLock lock(m_mutex);
    m_streams[stream->getId()] = stream;
}

void StreamManager::del(uint32_t id) {
    RWMutexType::WriteLock lock(m_mutex);
    m_streams.erase(id);
}

void StreamManager::clear() {
    RWMutexType::WriteLock lock(m_mutex);
    auto streams = m_streams;
    lock.unlock();
    for(auto& i : streams) {
        i.second->close();
    }
}

void StreamManager::foreach(std::function<void(Stream::ptr)> cb) {
    RWMutexType::ReadLock lock(m_mutex);
    auto m = m_streams;
    lock.unlock();
    for(auto& i : m) {
        cb(i.second);
    }
}

}
}
