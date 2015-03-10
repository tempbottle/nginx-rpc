#include "ngx_rpc_generator.h"
#include <iostream>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/compiler/plugin.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>


inline bool StripSuffix(std::string *filename, const std::string &suffix) {
  if (filename->length() >= suffix.length()) {
    size_t suffix_pos = filename->length() - suffix.length();
    if (filename->compare(suffix_pos, std::string::npos, suffix) == 0) {
      filename->resize(filename->size() - suffix.size());
      return true;
    }
  }

  return false;
}

inline std::string StripProto(std::string filename) {
  if (!StripSuffix(&filename, ".protodevel")) {
    StripSuffix(&filename, ".proto");
  }
  return filename;
}


std::string GetHeaderServices(const google::protobuf::FileDescriptor *file) {
  std::string output;
  google::protobuf::io::StringOutputStream output_stream(&output);
  google::protobuf::io::Printer printer(&output_stream, '$');
  std::map<std::string, std::string> vars;
  printer.Print("hehe");
  printer.Print("\n");
  return output;
}


bool NginxRpcGenerator::Generate(const google::protobuf::FileDescriptor *file,
                                      const std::string &parameter,
                                      google::protobuf::compiler::GeneratorContext *context,
                                      std::string *error) const
{

    if (!file->options().cc_generic_services()) {
        *error =
                "Nginx rpc proto compiler plugin does not work with generic "
                "services. To generate Nginx rpc APIs, please set \""
                "cc_generic_service = true.";
        return false;
    }

    std::string file_name = StripProto(file->name());

    //std::cerr<<"file:"<<file_name<<std::endl;

    //1 create the dir

    //2 create the ngx_rpc_header

    //3 create the ngx_rpc_header
    Insert(context, file_name+".h", "includes", GetHeaderServices(file));
    //Insert(context, "hehe.pb.h", "namespace_scope", GetHeaderServices(file));
    return true;
}


void  NginxRpcGenerator::Insert(google::protobuf::compiler::GeneratorContext *context,
                                const std::string &filename,
                                const std::string &insertion_point,
                                const std::string &code) const
{

    //std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
    //            context->OpenForInsert(filename, insertion_point));

    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
                context->Open(filename));
    google::protobuf::io::CodedOutputStream coded_out(output.get());
    coded_out.WriteRaw(code.data(), code.size());
}
