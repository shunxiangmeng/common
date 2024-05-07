/************************************************************************
 * Copyright(c) 2024 shanghai ulucu technology
 * 
 * File        :  api.cpp
 * Author      :  mengshunxiang 
 * Data        :  2024-05-04 12:52:02
 * Description :  None
 * Note        : 
 ************************************************************************/
#include "api.h"
#include "http/include/IHttpServer.h"
#include "infra/include/Logger.h"
#include "Reflection.h"

bool apiHandler(const queryMap&, const std::string& body, std::string& response) {
    infof("handler \n");

    response = "ok";

    return true;
}



int test() {
    struct Test { int a; int b; int c; int d; };
    const std::type_info& info = typeid(Test);
    return 0;
}
int test_reflection();
int test_dynamic_reflection();

bool apiInit() {

    //test();
    //test_reflection();
    test_dynamic_reflection();

    return IHttpServer::instance()->registerHandler(HttpMethod::GET, "/api/device", std::move(apiHandler));
}
