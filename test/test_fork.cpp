#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <memory>
#include <string>
#include <iostream>
#include <vector>

class Base {
public :
    int x;

};



class Derive : Base {
public:

    int z;
};



class A {
public:

    A(const std::string & n):name(n)
    {
        std::cout<<name<<" born in"<<getpid()<<std::endl;
    }

    ~A(){
        std::cout<<name<<" dead in"<<getpid()<<std::endl;
    }

    A(const A& a):
        name(a.name)
    {
         std::cout<<name<<"copy"<<getpid()<<std::endl;
    }

    virtual const char *do_v(Base* base)
    {
        return "Base";
    }

    std::string name;
};


class  B : public A {
public:

     B(const std::string & n):
         A(n),
     a("hehe"){

    }
    virtual const char *  do_v(Base * d) override
    {
        return "Derive";
    }

     A& get(){
         return a;
     }
     A a;
};


int test(){

    std::shared_ptr<A> sp (new A("test"));//
    pid_t pid = fork();
    std::vector<std::string> vec = {" 1"," 2"," 3"," 4"};

    switch (pid) {

    case -1:
        std::cout<<"failed fork"<<std::endl;
        return 0;
    case 0:
        std::cout<<"child:"<<getpid()<<" "<<sp->name<<std::endl;

        for(auto a : vec){
            sp->name += a;
            std::cout<<"child:"<<getpid()<<" "<<sp->name<<std::endl;
            sleep(1);
        }
        break;

    default:
        std::cout<<"parent:"<<getpid()<<",ret:"<<pid<<" "<<sp->name<<std::endl;
        for(auto a : vec){
            sp->name += a;
            std::cout<<"parent:"<<getpid()<<" "<<sp->name<<std::endl;
            sleep(1);
        }

        int status  =0;
        int ret = wait(&status);
        std::cout<<"waitpid:"<<ret<<",status:"<<status<<std::endl;
    }

    return 0;
}

//std::shared_ptr<A> sp(new B("gloabl"));



int main(void)
{

    B b("b");
    A& a = b.get();

    return 0;
    std::cout<<"enter main:"<<getpid()<<std::endl;
    test();
    std::cout<<"exit main:"<<getpid()<<std::endl;
    return 0;
}
