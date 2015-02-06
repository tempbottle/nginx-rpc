#include <memory>
#include "google/protobuf/service.h"

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
        readonly(new ReadonlyClass())
    {
    }

    // TODO use thread local
    virtual int InitTLSData(){
        std::unique_ptr<TlsClass> tls(new TlsClass());
        return 0;
    }

public:
    const ReadonlyClass& GetReadonly(){
        return *readonly;
    }

    TlsClass& GetTls(){
        std::unique_ptr<TlsClass> tls(new TlsClass());
        return *tls;
    }

public:
    std::unique_ptr<ReadonlyClass> readonly;

};








