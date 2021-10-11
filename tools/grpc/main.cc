#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include "sylar_generator.h"

int main(int argc, char** argv) {
    google::protobuf::compiler::CommandLineInterface cli;
    google::protobuf::compiler::cpp::CppGenerator cpp_generator;
    cli.RegisterGenerator("--cpp_out", &cpp_generator, "Generate C++ source and header.");
    sylar::SylarGenerator sylar_generator;
    cli.RegisterGenerator("--sylar_out", &sylar_generator, "Generate sylar C++ grpc source and header.");
    return cli.Run(argc, argv);
}
