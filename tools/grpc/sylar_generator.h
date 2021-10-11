#ifndef __SYLAR_SYLAR_GENERATOR_H__
#define __SYLAR_SYLAR_GENERATOR_H__

#include <string>
#include <google/protobuf/compiler/code_generator.h>

namespace sylar {

class SylarGenerator : public google::protobuf::compiler::CodeGenerator {
public:
    SylarGenerator();
    bool Generate(const google::protobuf::FileDescriptor* file,
                  const std::string& parameter,
                  google::protobuf::compiler::GeneratorContext* generator_context,
                  std::string* error) const;
};

}

#endif
