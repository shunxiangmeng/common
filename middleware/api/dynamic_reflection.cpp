// clang++ dynamic_iostream.cc -std=c++14 -Wall -o d_iostream && ./d_iostream

#include <iostream>
#include <string>

#include "dynamic_reflection.h"

struct SimpleStruct {
  bool bool_;
  int int_;
  double double_;
  std::string string_;
};

template<typename T>
auto t_converter = [](T* field, const std::string& name) {
    std::cout << std::boolalpha << name << ": " << *field << std::endl;
};

#define STRUCT_DEBUG(X) StructValueConverter<X> X##_;
#define DAIED(x,type, y) x##_.RegisterField(&x::y, #y, ValueConverter<type>(t_converter<type>));

int test_dynamic_reflection() {
  auto bool_converter = [](bool* field, const std::string& name) {
    std::cout << std::boolalpha << name << ": " << *field << std::endl;
  };
  auto int_converter = [](int* field, const std::string& name) {
    std::cout << name << ": " << *field << std::endl;
  };
  auto double_converter = [](double* field, const std::string& name) {
    std::cout << std::fixed << name << ": " << *field << std::endl;
  };
  auto string_converter = [](std::string* field, const std::string& name) {
    std::cout << name << ": " << *field << std::endl;
  };

  StructValueConverter<SimpleStruct> converter;
  converter.RegisterField(&SimpleStruct::bool_, "bool", ValueConverter<bool>(t_converter<bool>));
  converter.RegisterField(&SimpleStruct::int_, "int", ValueConverter<int>(int_converter));
  converter.RegisterField(&SimpleStruct::double_, "double", ValueConverter<double>(double_converter));
  converter.RegisterField(&SimpleStruct::string_, "string", ValueConverter<std::string>(string_converter));

  STRUCT_DEBUG(SimpleStruct);
  DAIED(SimpleStruct, bool, bool_);

  SimpleStruct simple{true, 2, 2.0, "hello dynamic reflection"};
  converter(&simple);
  return 0;
}