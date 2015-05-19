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
#include <algorithm>


#include "ngx_rpc_defines.h"
#include "ngx_rpc_server_template.h"
#include "ngx_rpc_module_template.h"


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

    size_t pos = filename.rfind("/");
    if( pos != std::string::npos)
    {
        filename = filename.substr(pos + 1);
    }
    if (!StripSuffix(&filename, ".protodevel")) {
        StripSuffix(&filename, ".proto");
    }
    return filename;
}

std::string Lower (const std::string & input){
    std::string tmp = input;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
    return tmp;
}


std::string Upper (const std::string & input){
    std::string tmp = input;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
    return tmp;
}


std::string Replace(const std::string & input, const std::string& search,
                    const std::string& replace) {

    std::string subject = input;
    size_t pos = 0;
    while((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }

    return subject;
}


void Format(google::protobuf::io::Printer& p,
            std::map<std::string, std::string> &vars,
            const char** start, int startnum, int lt = 0)
{

    for(int i = 0; i < startnum; ++i )
    {
        p.Print(vars, start[i]);
        p.Print(vars, "\n");
    }

    for(int i = 0; i < lt; ++i)
    {
        p.Print(vars, "\n");
    }
}

#define P_MARGIN 3



std::string GetServerImplHeader(const google::protobuf::FileDescriptor *file) {
    std::string output;
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer p(&output_stream, '$');
    std::map<std::string, std::string> vars;

    //build vars

    vars[PROTO_NAME] = Lower(StripProto(file->name()));
    vars[PROTO_NAME_UPPER] = Upper(vars[PROTO_NAME]);
    vars[PROTO_PACKAGE] = file->package();

    vars[PROTO_NAMESPACE] = Replace(file->package(), ".", " {\nnamespace ");
    vars[PROTO_NAMEPATH]  = Replace(file->package(), ".", "/");

    //define
    Format(p, vars, NGX_RPC_SERVER_HEADER_START,
           sizeof(NGX_RPC_SERVER_HEADER_START)/sizeof(NGX_RPC_SERVER_HEADER_START[0]), P_MARGIN);

    // includes
    Format(p, vars, NGX_RPC_SERVER_INCLUDE,
           sizeof(NGX_RPC_SERVER_INCLUDE)/sizeof(NGX_RPC_SERVER_INCLUDE[0]), P_MARGIN);

    //name space
    Format(p, vars, NGX_RPC_SERVER_NAMESPACE_START,
           sizeof(NGX_RPC_SERVER_NAMESPACE_START)/sizeof(NGX_RPC_SERVER_NAMESPACE_START[0]), P_MARGIN);


    for(int i = 0; i < file->service_count(); ++ i )
    {
        const ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);

        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());

        //class
        Format(p, vars, NGX_RPC_SERVER_CLASS_START,
               sizeof(NGX_RPC_SERVER_CLASS_START)/sizeof(NGX_RPC_SERVER_CLASS_START[0]), P_MARGIN);

        for(int i = 0 ; i < srvdesc->method_count(); ++i )
        {
            const ::google::protobuf::MethodDescriptor * method = srvdesc->method(i);

            vars[PROTO_SERVER_METHOD_NAME] = method->name();

            vars[PROTO_SERVER_METHOD_REQUEST_NAME] = method->input_type()->name();
            vars[PROTO_SERVER_METHOD_RESPONSE_NAME] = method->output_type()->name();

            // method
            Format(p , vars, NGX_RPC_SERVER_METHOD,
                   sizeof(NGX_RPC_SERVER_METHOD)/sizeof(NGX_RPC_SERVER_METHOD[0]), P_MARGIN);

        }

        //end
        Format(p, vars, NGX_RPC_SERVER_CLASS_END,
               sizeof(NGX_RPC_SERVER_CLASS_END)/sizeof(NGX_RPC_SERVER_CLASS_END[0]), P_MARGIN);

    }

   vars[PROTO_NAMESPACE] = Replace(file->package(), ".", " \n} //for ");
    Format(p, vars, NGX_RPC_SERVER_NAMESPACE_END,
           sizeof(NGX_RPC_SERVER_NAMESPACE_END)/sizeof(NGX_RPC_SERVER_NAMESPACE_END[0]), P_MARGIN);


    Format(p, vars, NGX_RPC_SERVER_HEADER_END,
           sizeof(NGX_RPC_SERVER_HEADER_END)/sizeof(NGX_RPC_SERVER_HEADER_END[0]), P_MARGIN);

    return output;
}



std::string GetNgxModuleHeader(const google::protobuf::FileDescriptor *file) {
    std::string output;
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer p(&output_stream, '$');
    std::map<std::string, std::string> vars;

    //build vars

    vars[PROTO_NAME] = Lower(StripProto(file->name()));
    vars[PROTO_NAME_UPPER] = Upper(vars[PROTO_NAME]);
    vars[PROTO_PACKAGE] = file->package();

    vars[PROTO_NAMESPACE] = Replace(file->package(), ".", "::");

    vars[PROTO_NAMEPATH]  = Replace(file->package(), ".", "/");
    vars[PROTO_NAMEPATH_DOT]  = "/" + file->package();


    // includes
    Format(p, vars, NGX_RPC_MODULE_INLUCDE,
           sizeof(NGX_RPC_MODULE_INLUCDE)/sizeof(NGX_RPC_MODULE_INLUCDE[0]), P_MARGIN);


    //conf space
    Format(p, vars, NGX_PRC_MODULE_CONF_START,
           sizeof(NGX_PRC_MODULE_CONF_START)/sizeof(NGX_PRC_MODULE_CONF_START[0]));

    for(int i = 0; i < file->service_count(); ++ i )
    {
        const ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);

        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());

        //class
        Format(p, vars, NGX_RPC_MODULE_CONF_SERVERS,
               sizeof(NGX_RPC_MODULE_CONF_SERVERS)/sizeof(NGX_RPC_MODULE_CONF_SERVERS[0]), 1);
    }
    Format(p, vars, NGX_PRC_MODULE_CONF_END,
           sizeof(NGX_PRC_MODULE_CONF_END)/sizeof(NGX_PRC_MODULE_CONF_END[0]), P_MARGIN);


    //set
    for(int i = 0; i < file->service_count(); ++ i )
    {
        const  ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);
        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());

        Format(p, vars, NGX_RPC_SERVER_SET_CONF,
               sizeof(NGX_RPC_SERVER_SET_CONF)/sizeof(NGX_RPC_SERVER_SET_CONF[0]), 1);
    }


    // commands
    Format(p, vars, NGX_RPC_COMMANDS_START,
           sizeof(NGX_RPC_COMMANDS_START)/sizeof(NGX_RPC_COMMANDS_START[0]));
    for(int i = 0; i < file->service_count(); ++ i )
    {
        const ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);
        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());
        Format(p, vars, NGX_RPC_COMMANDS_ITEMS,
               sizeof(NGX_RPC_COMMANDS_ITEMS)/sizeof(NGX_RPC_COMMANDS_ITEMS[0]));
    }
    Format(p, vars, NGX_RPC_COMMANDS_END,
           sizeof(NGX_RPC_COMMANDS_END)/sizeof(NGX_RPC_COMMANDS_END[0]), P_MARGIN);


    // http_module
    Format(p, vars, NGX_RPC_HTTP_MODULE_DECALRES,
           sizeof(NGX_RPC_HTTP_MODULE_DECALRES)/sizeof(NGX_RPC_HTTP_MODULE_DECALRES[0]), 1);

    Format(p, vars, NGX_RPC_HTTP_MODULE_DEFINE,
           sizeof(NGX_RPC_HTTP_MODULE_DEFINE)/sizeof(NGX_RPC_HTTP_MODULE_DEFINE[0]), P_MARGIN);


    // ngx_module

    Format(p, vars, NGX_RPC_MOUDLE_DECARLES,
           sizeof(NGX_RPC_MOUDLE_DECARLES)/sizeof(NGX_RPC_MOUDLE_DECARLES[0]), 1);

    Format(p, vars, NGX_RPC_MOUDLE_DEFINE,
           sizeof(NGX_RPC_MOUDLE_DEFINE)/sizeof(NGX_RPC_MOUDLE_DEFINE[0]), P_MARGIN);


    //creat loc conf
    Format(p, vars, NGX_RPC_CREATE_LOC_START,
           sizeof(NGX_RPC_CREATE_LOC_START)/sizeof(NGX_RPC_CREATE_LOC_START[0]));
    for(int i = 0; i < file->service_count(); ++ i )
    {
        const  ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);
        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());

        Format(p, vars, NGX_RPC_CREATE_LOC_ITEMS,
               sizeof(NGX_RPC_CREATE_LOC_ITEMS)/sizeof(NGX_RPC_CREATE_LOC_ITEMS[0]), 2);
    }
    Format(p, vars, NGX_RPC_CREATE_LOC_END,
           sizeof(NGX_RPC_CREATE_LOC_END)/sizeof(NGX_RPC_CREATE_LOC_END[0]), P_MARGIN);


    //method
    for(int i = 0; i < file->service_count(); ++ i )
    {
        const ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);
        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());

        for(int i = 0 ; i < srvdesc->method_count(); ++i )
        {
            const ::google::protobuf::MethodDescriptor * method = srvdesc->method(i);

            vars[PROTO_SERVER_METHOD_NAME] = method->name();

            vars[PROTO_SERVER_METHOD_REQUEST_NAME] = method->input_type()->name();
            vars[PROTO_SERVER_METHOD_RESPONSE_NAME] = method->output_type()->name();

            // method
            Format(p , vars, NGX_RPC_METHOD_PROCESS,
                   sizeof(NGX_RPC_METHOD_PROCESS)/sizeof(NGX_RPC_METHOD_PROCESS[0]), P_MARGIN);
        }
    }

    //set command
    for(int i = 0; i < file->service_count(); ++ i )
    {
        const ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);
        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());

        Format(p, vars, NGX_RPC_SET_SERVER_METHOD_START,
               sizeof(NGX_RPC_SET_SERVER_METHOD_START)/sizeof(NGX_RPC_SET_SERVER_METHOD_START[0]));

        for(int i = 0 ; i < srvdesc->method_count(); ++i )
        {
            const ::google::protobuf::MethodDescriptor * method = srvdesc->method(i);

            vars[PROTO_SERVER_METHOD_NAME] = method->name();

            vars[PROTO_SERVER_METHOD_REQUEST_NAME] = method->input_type()->name();
            vars[PROTO_SERVER_METHOD_RESPONSE_NAME] = method->output_type()->name();

            // method
            Format(p , vars, NGX_RPC_SET_SERVER_METHOD_ITEM,
                   sizeof(NGX_RPC_SET_SERVER_METHOD_ITEM)/sizeof(NGX_RPC_SET_SERVER_METHOD_ITEM[0]));
        }


        Format(p, vars, NGX_RPC_SET_SERVER_METHOD_END,
               sizeof(NGX_RPC_SET_SERVER_METHOD_END)/sizeof(NGX_RPC_SET_SERVER_METHOD_END[0]), P_MARGIN);
    }

    // ext
    Format(p, vars, NGX_RPC_EXIT_START,
           sizeof(NGX_RPC_EXIT_START)/sizeof(NGX_RPC_EXIT_START[0]));
    for(int i = 0; i < file->service_count(); ++ i )
    {
        const ::google::protobuf::ServiceDescriptor * srvdesc = file->service(i);
        vars[PROTO_SERVER_NAME] = srvdesc->name();
        vars[PROTO_SERVER_NAME_LOWER] = Lower(srvdesc->name());

        Format(p, vars, NGX_RPC_EXIT_ITEM,
               sizeof(NGX_RPC_EXIT_ITEM)/sizeof(NGX_RPC_EXIT_ITEM[0]));
    }

    Format(p, vars, NGX_RPC_EXIT_END,
           sizeof(NGX_RPC_EXIT_END)/sizeof(NGX_RPC_EXIT_END[0]));

    return output;
}


std::string GetNgxConfig(const google::protobuf::FileDescriptor *file) {
    std::string output;
    google::protobuf::io::StringOutputStream output_stream(&output);
    google::protobuf::io::Printer p(&output_stream, '%');
    std::map<std::string, std::string> vars;

    vars[PROTO_NAME] = Lower(StripProto(file->name()));

    Format(p, vars, NGX_RPC_CONFIG,
           sizeof(NGX_RPC_CONFIG)/sizeof(NGX_RPC_CONFIG[0]), P_MARGIN);

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

    Insert(context, "ngx_rpc_"+file_name+"_module/"+file_name+"_impl.h", GetServerImplHeader(file));

    Insert(context, "ngx_rpc_"+file_name+"_module/ngx_http_"+file_name+ "_module.cpp", GetNgxModuleHeader(file));

    Insert(context, "ngx_rpc_"+file_name+"_module/config", GetNgxConfig(file));

    return true;
}


void  NginxRpcGenerator::Insert(google::protobuf::compiler::GeneratorContext *context,
                                const std::string &filename,
                                const std::string &code) const
{

    //std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
    //            context->OpenForInsert(filename, insertion_point));

    std::unique_ptr<google::protobuf::io::ZeroCopyOutputStream> output(
                context->Open(filename));
    google::protobuf::io::CodedOutputStream coded_out(output.get());

    coded_out.WriteRaw(code.data(), code.size());
}
