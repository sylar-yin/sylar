#include "sylar_generator.h"
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include "sylar/sylar.h"

namespace sylar {

std::string ReplaceSuffix(const std::string& s, const std::string& search, const std::string& replace) {
    if (s.substr(s.length() - search.length(), search.length()) == search) {
        return s.substr(0, s.length() - search.length()) + replace;
    }
    return s;
}

SylarGenerator::SylarGenerator() {
}

void print_include(std::vector<std::string> headers,
                   google::protobuf::io::Printer& printer) {
    std::sort(headers.begin(), headers.end());
    for(auto& i : headers) {
        printer.Print("#include $item$\n", "item", i);
    }
    printer.Print("\n");
}

std::string ToGrpcPath(std::string name) {
    auto pos = name.rfind(".");
    name[pos] = '/';
    return "/" + name;
}

void print_include_macro(std::string filename,
                   google::protobuf::io::Printer& printer) {
    filename = filename.substr(filename.rfind("/") + 1);
    filename = sylar::replace(filename, ".", "_");
    filename = sylar::ToUpper(filename);

    printer.Print("#ifndef __$item$__\n", "item", filename);
    printer.Print("#define __$item$__\n\n", "item", filename);
}

bool generate_method_header(const google::protobuf::FileDescriptor* file,
                      const std::string& parameter,
                      google::protobuf::compiler::GeneratorContext* context,
                      std::string* error,
                      const google::protobuf::ServiceDescriptor* svc,
                      const google::protobuf::MethodDescriptor* mtd) {
    auto output_path = file->name().substr(0, file->name().rfind("."));
    auto output_filename = output_path + "." + svc->name() + "." + mtd->name() + ".grpc.h";
    std::cout << "\tgenerator file: " << output_filename << std::endl;
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(output_filename));
    google::protobuf::io::Printer printer(output.get(), '$');

    print_include_macro(output_filename, printer);

    auto pb_filename = output_path.substr(output_path.rfind("/") + 1) + ".pb.h";
    std::vector<std::string> headers = {
        "\"sylar/grpc/grpc_servlet.h\"",
        "\"" + pb_filename + "\""
    };

    print_include(headers, printer);

    auto ns = sylar::split(svc->full_name(), ".");
    for(auto it = ns.begin(); it != ns.end(); ++it) {
        printer.Print("namespace $item$ {\n", "item", *it);
    }
    printer.Print("\n");

    std::string servlet_name = mtd->name() + "Servlet";
    std::string input_class_name = sylar::replace(mtd->input_type()->full_name(), ".", "::");
    std::string output_class_name = sylar::replace(mtd->output_type()->full_name(), ".", "::");
    if(mtd->client_streaming() && mtd->server_streaming()) {
        printer.Print("class $item$ : public sylar::grpc::GrpcStreamBidirectionServlet<$input$, $output$> {\n",
                      "item", servlet_name,
                      "input", input_class_name,
                      "output", output_class_name
                      );
        printer.Print("public:\n");
        printer.Print("\tint32_t handle(ServerStream::ptr stream) override;\n");
    } else if(mtd->client_streaming()) {
        printer.Print("class $item$ : public sylar::grpc::GrpcStreamClientServlet<$input$, $output$> {\n",
                      "item", servlet_name,
                      "input", input_class_name,
                      "output", output_class_name
                      );
        printer.Print("public:\n");
        printer.Print("\tint32_t handle(ServerStream::ptr stream, RspPtr rsp) override;\n");
    } else if(mtd->server_streaming()) {
        printer.Print("class $item$ : public sylar::grpc::GrpcStreamServerServlet<$input$, $output$> {\n",
                      "item", servlet_name,
                      "input", input_class_name,
                      "output", output_class_name
                      );
        printer.Print("public:\n");
        printer.Print("\tint32_t handle(ServerStream::ptr stream, ReqPtr req) override;\n");
    } else {
        printer.Print("class $item$ : public sylar::grpc::GrpcUnaryServlet<$input$, $output$> {\n",
                      "item", servlet_name,
                      "input", input_class_name,
                      "output", output_class_name
                      );
        printer.Print("public:\n");
        printer.Print("\tint32_t handle(ReqPtr req, RspPtr rsp) override;\n");
    }
    printer.Print("\tGRPC_SERVLET_INIT_NAME($item$, \"$name$\");\n\n",
                  "item", servlet_name,
                  "name", ToGrpcPath(mtd->full_name())
                  );
    printer.Print("}; //class $item$\n\n", "item", servlet_name);

    for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
        printer.Print("} //namespace $item$\n", "item", *it);
    }
    printer.Print("\n#endif\n");
    return true;
}

bool generate_method_source(const google::protobuf::FileDescriptor* file,
                      const std::string& parameter,
                      google::protobuf::compiler::GeneratorContext* context,
                      std::string* error,
                      const google::protobuf::ServiceDescriptor* svc,
                      const google::protobuf::MethodDescriptor* mtd) {
    auto output_path = file->name().substr(0, file->name().rfind("."));
    auto output_filename = output_path + "." + svc->name() + "." + mtd->name() + ".grpc.cc";
    std::cout << "\tgenerator file: " << output_filename << std::endl;
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(output_filename));
    google::protobuf::io::Printer printer(output.get(), '$');

    std::vector<std::string> headers = {
        "\"" + output_path + "." + svc->name() + "." + mtd->name() + ".grpc.h" + "\"",
        "\"sylar/log.h\""

    };
    print_include(headers, printer);

    auto ns = sylar::split(svc->full_name(), ".");
    for(auto it = ns.begin(); it != ns.end(); ++it) {
        printer.Print("namespace $item$ {\n", "item", *it);
    }

    printer.Print("\n");
    printer.Print("static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();\n");
    printer.Print("\n");

    std::string servlet_name = mtd->name() + "Servlet";
    std::string input_class_name = sylar::replace(mtd->input_type()->full_name(), ".", ":");
    std::string output_class_name = sylar::replace(mtd->output_type()->full_name(), ".", ":");
    if(mtd->client_streaming() && mtd->server_streaming()) {
        printer.Print("int32_t $item$::handle(ServerStream::ptr stream) {\n", "item", servlet_name);
        printer.Print("\tSYLAR_LOG_WARN(g_logger) << \"Unhandle $item$\";\n",
                "item", mtd->full_name());
        printer.Print("\tm_response->setResult(404);\n");
        printer.Print("\tm_response->setError(\"Unhandle\");\n");
        printer.Print("\treturn 0;\n");
    } else if(mtd->client_streaming()) {
        printer.Print("int32_t $item$::handle(ServerStream::ptr stream, RspPtr rsp) {\n", "item", servlet_name);
        printer.Print("\tSYLAR_LOG_WARN(g_logger) << \"Unhandle $item$\";\n",
                "item", mtd->full_name());
        printer.Print("\tm_response->setResult(404);\n");
        printer.Print("\tm_response->setError(\"Unhandle\");\n");
        printer.Print("\treturn 0;\n");
    } else if(mtd->server_streaming()) {
        printer.Print("int32_t $item$::handle(ServerStream::ptr stream, ReqPtr req) {\n", "item", servlet_name);
        printer.Print("\tSYLAR_LOG_WARN(g_logger) << \"Unhandle $item$ req=\" << sylar::PBToJsonString(*req);\n",
                "item", mtd->full_name());
        printer.Print("\tm_response->setResult(404);\n");
        printer.Print("\tm_response->setError(\"Unhandle\");\n");
        printer.Print("\treturn 0;\n");
    } else {
        printer.Print("int32_t $item$::handle(ReqPtr req, RspPtr rsp) {\n", "item", servlet_name);
        printer.Print("\tSYLAR_LOG_WARN(g_logger) << \"Unhandle $item$ req=\" << sylar::PBToJsonString(*req);\n",
                "item", mtd->full_name());
        printer.Print("\tm_response->setResult(404);\n");
        printer.Print("\tm_response->setError(\"Unhandle\");\n");
        printer.Print("\treturn 0;\n");
    }
    printer.Print("}\n\n");

    for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
        printer.Print("} //namespace $item$\n", "item", *it);
    }
    return true;
}

bool generate_method(const google::protobuf::FileDescriptor* file,
                      const std::string& parameter,
                      google::protobuf::compiler::GeneratorContext* context,
                      std::string* error,
                      const google::protobuf::ServiceDescriptor* svc,
                      const google::protobuf::MethodDescriptor* mtd) {
    return generate_method_header(file, parameter, context, error, svc, mtd)
    && generate_method_source(file, parameter, context, error, svc, mtd);
}

bool generate_service_header(const google::protobuf::FileDescriptor* file,
                      const std::string& parameter,
                      google::protobuf::compiler::GeneratorContext* context,
                      std::string* error,
                      const google::protobuf::ServiceDescriptor* svc) {
    auto output_path = file->name().substr(0, file->name().rfind("."));
    auto output_filename = output_path + "." + svc->name() + ".grpc.h";
    std::cout << "\tgenerator file: " << output_filename << std::endl;
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(output_filename));
    google::protobuf::io::Printer printer(output.get(), '$');

    print_include_macro(output_filename, printer);

    std::vector<std::string> headers = {
        "\"sylar/grpc/grpc_server.h\""
    };
    print_include(headers, printer);

    auto ns = sylar::split(svc->full_name(), ".");
    for(auto it = ns.begin(); it != ns.end(); ++it) {
        printer.Print("namespace $item$ {\n", "item", *it);
    }
    printer.Print("\n");

    printer.Print("void RegisterService(sylar::grpc::GrpcServer::ptr server);\n\n");

    for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
        printer.Print("} //namespace $item$\n", "item", *it);
    }
    printer.Print("\n#endif\n");
    return true;
}

bool generate_service_source(const google::protobuf::FileDescriptor* file,
                      const std::string& parameter,
                      google::protobuf::compiler::GeneratorContext* context,
                      std::string* error,
                      const google::protobuf::ServiceDescriptor* svc) {
    auto output_path = file->name().substr(0, file->name().rfind("."));
    auto output_filename = output_path + "." + svc->name() + ".grpc.cc";
    std::cout << "\tgenerator file: " << output_filename << std::endl;
    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(context->Open(output_filename));
    google::protobuf::io::Printer printer(output.get(), '$');

    print_include_macro(output_filename, printer);

    std::vector<std::string> headers = {
        "\"" + output_path + "." + svc->name() + ".grpc.h" + "\"",
        "\"sylar/log.h\""
    };

    for(int i = 0; i < svc->method_count(); ++i) {
        auto m = svc->method(i);
        headers.push_back(
            "\"" + output_path + "." + svc->name() + "." + m->name() + ".grpc.h" + "\""
        );
    }

    print_include(headers, printer);

    auto ns = sylar::split(svc->full_name(), ".");
    for(auto it = ns.begin(); it != ns.end(); ++it) {
        printer.Print("namespace $item$ {\n", "item", *it);
    }
    printer.Print("\n");
    printer.Print("\n");
    printer.Print("static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();\n");
    printer.Print("\n");

    printer.Print("void RegisterService(sylar::grpc::GrpcServer::ptr server) {\n");
    for(int i = 0; i < svc->method_count(); ++i) {
        auto m = svc->method(i);
        printer.Print("\tserver->addGrpcServlet(\"$path$\", std::make_shared<$clazz$>());\n",
                "path", ToGrpcPath(m->full_name()),
                "clazz", m->name() + "Servlet"
                );
        printer.Print("\tSYLAR_LOG_INFO(g_logger) << \"register grpc $path$\";\n",
                "path", ToGrpcPath(m->full_name()));
    }

    printer.Print("}\n\n");

    for(auto it = ns.rbegin(); it != ns.rend(); ++it) {
        printer.Print("} //namespace $item$\n", "item", *it);
    }
    printer.Print("\n#endif\n");
    return true;
}


bool generate_service(const google::protobuf::FileDescriptor* file,
                      const std::string& parameter,
                      google::protobuf::compiler::GeneratorContext* context,
                      std::string* error,
                      const google::protobuf::ServiceDescriptor* svc) {
    for(int i = 0; i < svc->method_count(); ++i) {
        auto mtd = svc->method(i);
        if(!generate_method(file, parameter, context, error, svc, mtd)) {
            std::cout << "generator method " << mtd->full_name() << " fail" << std::endl;
            return false;
        }
    }
    return generate_service_header(file, parameter, context, error, svc)
        && generate_service_source(file, parameter, context, error, svc);
}

bool SylarGenerator::Generate(const google::protobuf::FileDescriptor* file,
                              const std::string& parameter,
                              google::protobuf::compiler::GeneratorContext* context,
                              std::string* error) const {
    std::cout << "parameter=" << parameter << std::endl;
    std::cout << "process: " << file->name() << std::endl;
    for(auto i = 0; i < file->service_count(); ++i) {
        auto svc = file->service(i);
        if(!generate_service(file, parameter, context, error, svc)) {
            std::cout << "generator service " << svc->full_name() << " fail" << std::endl;
            return false;
        }
    }
    return true;
}

}
