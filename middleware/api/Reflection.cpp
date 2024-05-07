// clang++ static_iostream.cc -std=c++14 -Wall -o s_iostream && ./s_iostream

#include <iostream>
#include <string>
#include "Reflection.h"
#include "jsoncpp/include/value.h"
#include "infra/include/Optional.h"

struct GenericFunctor {
    // ... context data

    template <typename Field, typename Name>
    void operator()(Field&& field, Name&& name) {
    std::cout << std::boolalpha << std::fixed << name << ": " << field
                << std::endl;
    }
};

namespace {

template <class... Fs>
struct overload_set;

template <class F1, class... Fs>
struct overload_set<F1, Fs...> : F1, overload_set<Fs...>::type {
    typedef overload_set type;

    overload_set(F1 head, Fs... tail)
        : F1(head), overload_set<Fs...>::type(tail...) {}

    using F1::operator();
    using overload_set<Fs...>::type::operator();
};

template <class F>
struct overload_set<F> : F {
    typedef F type;
    using F::operator();
};

template <class... Fs>
typename overload_set<Fs...>::type overload(Fs... x) {
    return overload_set<Fs...>(x...);
}

}  // namespace

typedef struct {
    double x;
    double y;
}Point;
DEFINE_STRUCT_SCHEMA(Point,
    DEFINE_STRUCT_FIELD(x),
    DEFINE_STRUCT_FIELD(y));


struct Response {
    int code;
    std::string message;
    std::string concept;
    double x;
    uint32_t y;
    infra::optional<int> z;
    Point point;
    infra::optional<Point> m;
    std::vector<int> vec;
};
DEFINE_STRUCT_SCHEMA(Response,
    DEFINE_STRUCT_FIELD(code),
    DEFINE_STRUCT_FIELD(message),
    DEFINE_STRUCT_FIELD(concept),
    DEFINE_STRUCT_FIELD(x),
    DEFINE_STRUCT_FIELD(y),
    DEFINE_STRUCT_FIELD(z),
    //DEFINE_STRUCT_FIELD(point),
    DEFINE_STRUCT_FIELD(m),
    DEFINE_STRUCT_FIELD(vec));

std::string objectToString() {
    Response response{};

    response.z = 2;

    Json::Value data;

    typedef struct {
        std::string type;
        std::string name;
        std::string value;
    }Item;

    std::vector<Item> items;
    
    //template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
    //bool is_even(T t) {
    //    return !is_odd(t);
    //}

    auto cb = [&data](auto&& field, auto&& name) {
        Item item{};

        const std::type_info& info = typeid(field);
        printf("type:%s, name:%s\n", info.name(), name);
        std::string type_name = info.name();
        //data[name] = field;
        if (type_name.find("optional") != std::string::npos) {
        }
    };


    ForEachField(response, cb);
    std::string result = data.toStyledString();
    return result;
}


int test_reflection() {
    auto s = objectToString();
    printf("%s\n", s.data());
    #if 0
    ForEachField(SimpleStruct{ true, 1, 1.0, "0hello static reflection" },
        [](auto&& field, auto&& name) {
            //auto id = typeid(field);
            const std::type_info& info = typeid(field);
    std::cout << "type: " << info.name() << std::endl;
                    std::cout << std::boolalpha << std::fixed << name << ": "
                            << field << std::endl;
                });

    
    ForEachField(SimpleStruct{true, 1, 1.0, "1hello static reflection"},
                GenericFunctor{/* ... context data */});

    ForEachField(SimpleStruct{true, 1, 1.0, "2hello static reflection"},
                overload(
                    [](bool field, const char* name) {
                        std::cout << "b " << std::boolalpha << name << ": "
                                << field << std::endl;
                    },
                    [](int field, const char* name) {
                        std::cout << "i " << name << ": " << field << std::endl;
                    },
                    [](double field, const char* name) {
                        std::cout << "d " << std::fixed << name << ": " << field
                                << std::endl;
                    },
                    [](const std::string& field, const char* name) {
                        std::cout << "s " << name << ": " << field.c_str()
                                << std::endl;
                    }));
    #endif



    return 0;
}