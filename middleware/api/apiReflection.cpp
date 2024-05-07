//This CPP file is automatically created, please do not manually modify it
#include "jsoncpp/include/value.h"
#include "infra/include/Logger.h"
#include "api.h"


//this function is automatically created, please do not manually modify it
std::string Response0ToString(Response0& obj) {
   std::string result;
   Json::Value root;
   root["code"] = obj.code;
   root["tag"] = obj.tag;
   root["message"] = obj.message;
   root["concept"] = obj.concept;
   root["x"] = obj.x;
   root["xx"] = obj.xx;
   root["xx2"] = obj.xx2;
   root["y"] = obj.y;
   if (obj.optional_z.has_value()) {
       root["optional_z"] = *obj.optional_z;
   }
   //root["point"] = obj.point;
   root["enable"] = obj.enable;
   root["ldkjsnfjn"] = obj.ldkjsnfjn;
   result = root.toStyledString();
   return result;
}


//this function is automatically created, please do not manually modify it
bool jsonToResponse0(Response0& obj, Json::Value &root, std::string &error_message) {
   std::string result;
   if (!root.isMember("code") || !root["code"].isInt() || !root["code"].isInt64()) {
       error_message = "no paramter code or code type error";
       return false;
   }
   obj.code = root["code"].asInt();

   if (!root.isMember("tag") || !root["tag"].isInt() || !root["tag"].isInt64()) {
       error_message = "no paramter tag or tag type error";
       return false;
   }
   //obj.tag = root["tag"].asInt();

   if (!root.isMember("message") || !root["message"].isString()) {
       error_message = "no paramter message or message type error";
       return false;
   }
   obj.message = root["message"].asString();

   if (!root.isMember("concept") || !root["concept"].isString()) {
       error_message = "no paramter concept or concept type error";
       return false;
   }
   obj.concept = root["concept"].asString();

   if (!root.isMember("x") || !root["x"].isDouble()) {
       error_message = "no paramter x or x type error";
       return false;
   }
   obj.x = root["x"].asDouble();

   if (!root.isMember("xx") || !root["xx"].isDouble()) {
       error_message = "no paramter xx or xx type error";
       return false;
   }
   obj.xx = root["xx"].asDouble();

   if (!root.isMember("xx2") || !root["xx2"].isInt() || !root["xx2"].isInt64()) {
       error_message = "no paramter xx2 or xx2 type error";
       return false;
   }
   obj.xx2 = root["xx2"].asInt();

   if (!root.isMember("y") || !root["y"].isInt() || !root["y"].isInt64()) {
       error_message = "no paramter y or y type error";
       return false;
   }
   obj.y = root["y"].asInt();

   if (!root.isMember("optional_z") || !root["optional_z"].isInt() || !root["optional_z"].isInt64()) {
   } else {
       obj.optional_z = root["optional_z"].asInt();
   }

   if (!root.isMember("enable") || !root["enable"].isBool()) {
       error_message = "no paramter enable or enable type error";
       return false;
   }
   obj.enable = root["enable"].asBool();

   if (!root.isMember("ldkjsnfjn") || !root["ldkjsnfjn"].isDouble()) {
       error_message = "no paramter ldkjsnfjn or ldkjsnfjn type error";
       return false;
   }
   obj.ldkjsnfjn = root["ldkjsnfjn"].asDouble();

   return true;
}
