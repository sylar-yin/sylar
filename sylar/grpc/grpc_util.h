#ifndef __SYALR_GRPC_GRPC_UTIL_H__
#define __SYALR_GRPC_GRPC_UTIL_H__

#include <string>
#include "grpc_protocol.h"

namespace sylar {
namespace grpc {

bool DecodeGrpcBody(const std::string& body, std::string& data, const std::string& encoding = "gzip");
bool DecodeGrpcBody(const std::string& body, GrpcMessage& data, const std::string& encoding = "gzip");
bool EncodeGrpcBody(const std::string& data, std::string& body, bool compress = false, const std::string& encoding = "gzip");
}
}

#endif
