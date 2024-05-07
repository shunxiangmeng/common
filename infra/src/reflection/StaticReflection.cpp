#include <cstdio>
#include <iostream>
#include <sstream>
#include "StaticReflection.h"

template<typename T>
void serializeObj(std::ostream& out, const T& obj,
        const char* fieldName = "", int depth = 0) {
    auto indent = [&] {
        for (int i = 0; i < depth; ++i)
        { out << "    "; }
    };

    if constexpr(IsReflected_v<T>) {
        indent();
        out << fieldName << (*fieldName ? ": {" : "{") << std::endl;
        forEach(obj,
                [&](auto&& fieldName, auto&& value)
                { serializeObj(out, value, fieldName, depth + 1); });
        indent();
        out << "}" << std::endl;
    } else {
        indent();
        out << fieldName << ": " << obj << std::endl;
    }
}

template<typename T>
void deserializeObj(std::istream& in, T& obj,
        const char* fieldName = "") {
    if constexpr(IsReflected_v<T>) {
        std::string token;
        in >> token; // eat '{'
        if (*fieldName) {
            in >> token; // WARNING: needs check fieldName valid
        }

        forEach(obj,
                [&](auto&& fieldName, auto&& value)
                { deserializeObj(in, value, fieldName); });

        in >> token; // eat '}'
    } else {
        if (*fieldName) {
            std::string token;
            in >> token; // WARNING: needs check fieldName valid
        }
        in >> obj; // dump value
    }
}
