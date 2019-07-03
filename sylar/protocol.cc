#include "sylar/protocol.h"
#include "sylar/util.h"

namespace sylar {

ByteArray::ptr Message::toByteArray() {
    ByteArray::ptr ba(new ByteArray);
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
    bytearray->writeUint32(m_sn);
    bytearray->writeUint32(m_cmd);
    return true;
}

bool Request::parseFromByteArray(ByteArray::ptr bytearray) {
    m_sn = bytearray->readUint32();
    m_cmd = bytearray->readUint32();
    return true;
}

Response::Response()
    :m_sn(0)
    ,m_cmd(0)
    ,m_result(0)
    ,m_resultStr() {
}

bool Response::serializeToByteArray(ByteArray::ptr bytearray) {
    bytearray->writeFuint8(getType());
    bytearray->writeUint32(m_sn);
    bytearray->writeUint32(m_cmd);
    bytearray->writeUint32(m_result);
    bytearray->writeStringVint(m_resultStr);
    return true;
}

bool Response::parseFromByteArray(ByteArray::ptr bytearray) {
    m_sn = bytearray->readUint32();
    m_cmd = bytearray->readUint32();
    m_result = bytearray->readUint32();
    m_resultStr = bytearray->readStringVint();
    return true;
}

Notify::Notify()
    :m_notify(0) {
}

bool Notify::serializeToByteArray(ByteArray::ptr bytearray) {
    bytearray->writeFuint8(getType());
    bytearray->writeUint32(m_notify);
    return true;
}

bool Notify::parseFromByteArray(ByteArray::ptr bytearray) {
    m_notify = bytearray->readUint32();
    return true;
}

}
