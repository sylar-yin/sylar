#include "sylar/protocol.h"
#include "sylar/util.h"

namespace sylar {

ByteArray::ptr Message::toByteArray() {
    ByteArray::ptr ba = std::make_shared<ByteArray>();
    if(serializeToByteArray(ba)) {
        return ba;
    }
    return nullptr;
}

Request::Request()
    :m_sn(0)
    ,m_cmd(0) {
}

bool Request::serializeToByteArray(ByteArray::ptr bytearray) {
    bytearray->writeFuint8(getType());
    bytearray->writeFuint32(m_sn);
    bytearray->writeFuint32(m_cmd);
    return true;
}

bool Request::parseFromByteArray(ByteArray::ptr bytearray) {
    m_sn = bytearray->readFuint32();
    m_cmd = bytearray->readFuint32();
    return true;
}

Response::Response()
    :m_sn(0)
    ,m_cmd(0)
    ,m_result(404)
    ,m_resultStr("unhandle") {
}

bool Response::serializeToByteArray(ByteArray::ptr bytearray) {
    bytearray->writeFuint8(getType());
    bytearray->writeFuint32(m_sn);
    bytearray->writeFuint32(m_cmd);
    bytearray->writeFuint32(m_result);
    bytearray->writeStringF32(m_resultStr);
    return true;
}

bool Response::parseFromByteArray(ByteArray::ptr bytearray) {
    m_sn = bytearray->readFuint32();
    m_cmd = bytearray->readFuint32();
    m_result = bytearray->readFuint32();
    m_resultStr = bytearray->readStringF32();
    return true;
}

Notify::Notify()
    :m_notify(0) {
}

bool Notify::serializeToByteArray(ByteArray::ptr bytearray) {
    bytearray->writeFuint8(getType());
    bytearray->writeFuint32(m_notify);
    return true;
}

bool Notify::parseFromByteArray(ByteArray::ptr bytearray) {
    m_notify = bytearray->readFuint32();
    return true;
}

}
