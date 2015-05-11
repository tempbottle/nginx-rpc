#ifndef _NGX_RPC_GENERATOR_H_
#define _NGX_RPC_GENERATOR_H_
#include <memory>
#include <string>
#include <google/protobuf/compiler/code_generator.h>

//namespace google::protobuf::compiler;


#include <vector>



class NginxRpcGenerator : public ::google::protobuf::compiler::CodeGenerator {
public:

    NginxRpcGenerator() {}
    virtual ~NginxRpcGenerator() {}


    virtual bool Generate(const google::protobuf::FileDescriptor *file,
                          const std::string &parameter,
                          google::protobuf::compiler::GeneratorContext *context,
                          std::string *error) const ;
private:

    void Insert(google::protobuf::compiler::GeneratorContext *context,
                const std::string &filename,
                const std::string &code) const;
};

#endif
