# -*- coding: UTF-8 -*- 
import os
import re
import CppHeaderParser

def get_all_source_file(dir, Filelist):
    newDir = dir
    if os.path.isfile(dir):
        file_name, suffix_name = os.path.splitext(dir)
        if suffix_name == '.h' or suffix_name == '.hpp' or suffix_name == '.c' or suffix_name == '.cpp':
            Filelist.append(dir)
            # 若只是要返回文件文，使用这个
            # Filelist.append(os.path.basename(dir))
    elif os.path.isdir(dir):
        for s in os.listdir(dir):
            if s == '.vscode' or s == 'build' or s == '.git' or s == 'thirdparty' or s == 'common' or s == 'soc':
                continue
            newDir = os.path.join(dir,s)
            get_all_source_file(newDir, Filelist)
    return Filelist

def get_all_need_generate_file(list):
    result_list = []
    for file in list:
        with open(file, 'r', encoding='utf-8') as f:
            content = f.read()
            match = re.search(r'@reflection@', content)
            if match:
                result_list.append(file)
    return result_list


type_hash = {'private' : '- ','protected' : '# ','public' : '+ '}

def get_mem_var(parse_conent,cur_class,target_type):
    variables = {}
    for class_private_mem_var in parse_conent.classes[cur_class]['properties'][target_type]:
        # 组装private属性
        #tmp_str = type_hash[target_type] + class_private_mem_var['name'] + ' : ' + class_private_mem_var['type']
        tmp_str = class_private_mem_var['type'] + ' : ' +  class_private_mem_var['name']
        print(tmp_str)
        variables[class_private_mem_var['name']] = class_private_mem_var['type']
    return variables

def get_mem_func(parse_conent,cur_class,target_type):
    # 遍历方法 - public
    for class_mem_func in parse_conent.classes[cur_class]['methods'][target_type]:
        tmp_str = ''
        tmp_str = type_hash[target_type] + class_mem_func['name'] + '('
        # 遍历函数参数
        if len(class_mem_func['parameters']):  # 有参数
            p_cnt = len(class_mem_func['parameters'])
            tmp_cnt = 0
            for one_param in class_mem_func['parameters']:  # 一个函数的多个参数，分多行
                tmp_cnt = tmp_cnt + 1
                tmp_str = tmp_str + one_param['name'] + " : " + one_param['type']
                if tmp_cnt != p_cnt:
                    tmp_str = tmp_str + ' , '
        tmp_str = tmp_str + ')' + " : "

        # 组装返回值
        tmp_str = tmp_str + class_mem_func['rtnType']
        print(tmp_str)

def is_basic_type(type):
    if type == 'int8_t' or type == 'uint8_t' \
        or type == 'int' or type == 'int32_t' or type == 'uint32_t' \
        or type == 'int64' or type == 'int64_t' or type == 'uint64_t' \
        or type == 'bool' or type == 'std::string' or type == 'string' \
        or type == 'float' or type == 'double' \
        or type == 'long' or type == 'long long':
        return True
    else:
        return False

def parse_basic_type(name, type, optional):
    text = ''
    if type == 'int8_t' or type == 'uint8_t' \
        or type == 'int16' or type == 'uint16_t' \
        or type == 'int' or type == 'int32_t' or type == 'uint32_t' \
        or type == 'int64' or type == 'int64_t' or type == 'uint64_t':
        text += '   if (!root.isMember("' + name + '") || !root["' + name + '"].isInt() || !root["' + name + '"].isInt64()) {\n'
        if optional == False:
            text += '       error_message = "no paramter ' + name + ' or ' + name + ' type error";\n'
            text += '       return false;\n'
            text += '   }\n'
            text += '   obj.' + name + ' = root["' + name + '"].asInt();\n'
        else:
            text += '   } else {\n'
            text += '       obj.' + name + ' = root["' + name + '"].asInt();\n'
            text += '   }\n'
        text += '\n'

    elif type == 'std::string' or type == 'string':
        text += '   if (!root.isMember("' + name + '") || !root["' + name + '"].isString()) {\n'
        if optional == False:
            text += '       error_message = "no paramter ' + name + ' or ' + name + ' type error";\n'
            text += '       return false;\n'
            text += '   }\n'
            text += '   obj.' + name + ' = root["' + name + '"].asString();\n'
        else:
            text += '   } else {\n'
            text += '       obj.' + name + ' = root["' + name + '"].asString();\n'
            text += '   }\n'
        text += '\n'

    elif type == 'bool':
        text += '   if (!root.isMember("' + name + '") || !root["' + name + '"].isBool()) {\n'
        if optional == False:
            text += '       error_message = "no paramter ' + name + ' or ' + name + ' type error";\n'
            text += '       return false;\n'
            text += '   }\n'
            text += '   obj.' + name + ' = root["' + name + '"].asBool();\n'
        else:
            text += '   } else {\n'
            text += '       obj.' + name + ' = root["' + name + '"].asBool();\n'
            text += '   }\n'
        text += '\n'
    elif type == 'float' or type == 'double':
        text += '   if (!root.isMember("' + name + '") || !root["' + name + '"].isDouble()) {\n'
        if optional == False:
            text += '       error_message = "no paramter ' + name + ' or ' + name + ' type error";\n'
            text += '       return false;\n'
            text += '   }\n'
            text += '   obj.' + name + ' = root["' + name + '"].asDouble();\n'
        else:
            text += '   } else {\n'
            text += '       obj.' + name + ' = root["' + name + '"].asDouble();\n'
            text += '   }\n'
        text += '\n'
    return text

def create_reflection_cpp(file):
    file_name, suffix_name = os.path.splitext(file)
    cpp_file = file_name + "Reflection.cpp"
    header_file = file_name + ".h"
    print("ofile:", file, " cpp_file:", cpp_file)

    parse_conent = CppHeaderParser.CppHeader(header_file)
    print(parse_conent)
    # 遍历每个解析到的类

    variables = {}
    for class_name in parse_conent.classes.keys():
        # 当前类
        print("###################################################")
        print(class_name + '\n')
        # 获取属性 - private - protected - public
        v1 = get_mem_var(parse_conent, class_name, 'private')
        v2 = get_mem_var(parse_conent, class_name, 'protected')
        variables = get_mem_var(parse_conent, class_name, 'public')
        variables.update(v1)
        variables.update(v2)
        print(variables)
        # 获取方法 - private - protected - public
        get_mem_func(parse_conent, class_name, 'private')
        get_mem_func(parse_conent, class_name, 'protected')
        get_mem_func(parse_conent, class_name, 'public')

        with open(cpp_file, "w") as fp:
            cpp_content_header = '//This CPP file is automatically created, please do not manually modify it\n'
            cpp_content_header += '#include "jsoncpp/include/value.h"\n'
            cpp_content_header += '#include "infra/include/Logger.h"\n'
            fp.write(cpp_content_header)

            # functon header
            # std::string {class}ToString({class}& obj)
            text = '\n\n'
            text += '//this function is automatically created, please do not manually modify it\n'
            text += 'std::string ' + class_name + 'ToString(' + class_name + '& obj) {\n'
            text += '   std::string result;\n'
            text += '   Json::Value root;\n'

            for name, type in variables.items():
                if re.search(r'optional', type):
                    type_tmp = type
                    type_tmp = type_tmp.replace('<', ' ')
                    type_tmp = type_tmp.replace('>', ' ')
                    result = re.split(' ', type_tmp)
                    internal_type = result[1]
                    if is_basic_type(internal_type):
                        text += '   if (obj.'+ name + '.has_value()) {\n'
                        text += '       root["'+ name +'"] = ' + '*obj.' + name + ';\n'
                        text += '   }\n'
                    text += ''
                elif re.search(r'vector', type):
                    text += ''
                else:
                    text += '   root["'+ name +'"] = ' + 'obj.' + name + ';\n'

            text += '   result = root.toStyledString();\n'
            text += '   return result;\n'
            text += "}\n"
            fp.write(text)

            # 解析函数
            # std::string jsonTo{class}({class}& obj, Json::Value &root)
            text = '\n\n'
            text += '//this function is automatically created, please do not manually modify it\n'
            text += 'bool jsonTo' + class_name + '(' + class_name + '& obj, Json::Value &root, std::string &error_message) {\n'

            for name, type in variables.items():
                if re.search(r'optional', type):
                    type_tmp = type
                    type_tmp = type_tmp.replace('<', ' ')
                    type_tmp = type_tmp.replace('>', ' ')
                    result = re.split(' ', type_tmp)
                    print(result[1])
                    if is_basic_type(result[1]):
                        text += parse_basic_type(name, result[1], True)
                else:
                    text += parse_basic_type(name, type, False)

            text += '   return true;\n'
            text += "}\n"
            fp.write(text)

def reflection(generate_list):
    for file in generate_list:
        create_reflection_cpp(file)

if __name__ == '__main__':
    path = os.getcwd()
    print(path)
    allfile = get_all_source_file(path, [])
    #print(allfile)
    print("file count:", len(allfile))

    generate_list = get_all_need_generate_file(allfile)
    print(generate_list)
    print("file count:", len(generate_list))

    reflection(generate_list)

