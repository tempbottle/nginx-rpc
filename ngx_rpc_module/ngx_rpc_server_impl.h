#include "google/protobuf/service.h"
#include <memory>
///
/// \brief The ReadonlyDataDefault class
///
class ReadonlyDataDefault
{
public:
};

///
/// \brief The AppStaticData class
///
/// static object which construct before the main
class TLSDataDefault{
public:
};





template<class Baseclass,
         class ReadonlyClass = ReadonlyDataDefault,
         class TlsClass = TLSDataDefault >
class NgxRpcServer : public Baseclass {
public:

    NgxRpcServer():
        readonly(ReadonlyClass()),
        base(new Baseclass()),
        tls(nullptr)
    {
    }

    virtual TlsClass* InitTLSData(){
        tls = new TLSData();
        return 0;
    }

public:
    const ReadonlyClass& GetReadonly(){
        return *readonly;
    }

    TlsClass& GetTls(){
        return *tls;
    }

public:
    std::unique_ptr<ReadonlyClass> readonly;
    std::unique_ptr<TlsClass> tls;
};



// 
class NgxRpcServerController : public ::google::protobuf::RpcController {
public:

    void FinishRequest(
            std::shared_ptr< ::google::protobuf::Message> req,
            std::shared_ptr< ::google::protobuf::Message> res)
    {

        //send header

        //send body
    }
public :

};





