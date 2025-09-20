// Minimal JSON reader for the demo purposes.
// Supports parsing a subset of JSON: objects, arrays, strings, numbers, booleans, null.
// Not a full implementation. Sufficient for reading `unit_spawns.json` defined below.

#ifndef THIRD_PARTY_MINI_JSON_HPP
#define THIRD_PARTY_MINI_JSON_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace mini_json {

struct Value;
using Object = std::map<std::string, std::shared_ptr<Value>>;
using Array = std::vector<std::shared_ptr<Value>>;

struct Value {
    enum class Type {Null, Bool, Number, String, Object, Array} type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    Object objectValue;
    Array arrayValue;

    Value() = default;
    static std::shared_ptr<Value> makeNull() { return std::make_shared<Value>(); }
    static std::shared_ptr<Value> makeBool(bool b) { auto v=std::make_shared<Value>(); v->type=Type::Bool; v->boolValue=b; return v; }
    static std::shared_ptr<Value> makeNumber(double n) { auto v=std::make_shared<Value>(); v->type=Type::Number; v->numberValue=n; return v; }
    static std::shared_ptr<Value> makeString(const std::string& s) { auto v=std::make_shared<Value>(); v->type=Type::String; v->stringValue=s; return v; }
    static std::shared_ptr<Value> makeObject(const Object& o) { auto v=std::make_shared<Value>(); v->type=Type::Object; v->objectValue=o; return v; }
    static std::shared_ptr<Value> makeArray(const Array& a) { auto v=std::make_shared<Value>(); v->type=Type::Array; v->arrayValue=a; return v; }

    bool isObject() const { return type==Type::Object; }
    bool isArray() const { return type==Type::Array; }
    bool isString() const { return type==Type::String; }
    bool isNumber() const { return type==Type::Number; }
    bool isBool() const { return type==Type::Bool; }
};

class Parser {
public:
    explicit Parser(const std::string& s):src(s),i(0){}
    std::shared_ptr<Value> parse() { skip(); auto v=parseValue(); skip(); if (i!=src.size()) throw std::runtime_error("Extra data"); return v; }
private:
    std::string src; size_t i;
    void skip(){ while(i<src.size() && std::isspace((unsigned char)src[i])) ++i; }
    char peek(){ return i<src.size()?src[i]:'\0'; }
    char get(){ return i<src.size()?src[i++]: '\0'; }

    std::shared_ptr<Value> parseValue(){ skip(); char c=peek();
        if (c=='{') return parseObject();
        if (c=='[') return parseArray();
        if (c=='\"') return parseString();
        if (c=='t' || c=='f') return parseBool();
        if (c=='n') return parseNull();
        return parseNumber();
    }

    std::shared_ptr<Value> parseObject(){
        get(); // '{'
        Object obj;
        skip();
        if (peek()=='}') { get(); return Value::makeObject(obj); }
        while(true){
            skip(); auto keyV = parseString();
            if (!keyV->isString()) throw std::runtime_error("Object key not string");
            skip(); if (get()!=':') throw std::runtime_error(": expected");
            skip(); auto val = parseValue();
            obj[keyV->stringValue]=val;
            skip(); char c=get();
            if (c=='}') break;
            if (c!=',') throw std::runtime_error(", expected");
        }
        return Value::makeObject(obj);
    }

    std::shared_ptr<Value> parseArray(){
        get(); // '['
        Array arr;
        skip();
        if (peek()==']') { get(); return Value::makeArray(arr); }
        while(true){
            skip(); arr.push_back(parseValue());
            skip(); char c=get();
            if (c==']') break;
            if (c!=',') throw std::runtime_error(", expected");
        }
        return Value::makeArray(arr);
    }

    std::shared_ptr<Value> parseString(){
        if (get()!='\"') throw std::runtime_error("string start");
        std::ostringstream ss;
        while(true){ char c=get(); if (c=='\0') throw std::runtime_error("unterminated string"); if (c=='\\') { char e=get(); ss<<e; } else if (c=='\"') break; else ss<<c; }
        return Value::makeString(ss.str());
    }

    std::shared_ptr<Value> parseBool(){
        if (src.compare(i,4,"true")==0) { i+=4; return Value::makeBool(true); }
        if (src.compare(i,5,"false")==0) { i+=5; return Value::makeBool(false); }
        throw std::runtime_error("invalid bool");
    }

    std::shared_ptr<Value> parseNull(){ if (src.compare(i,4,"null")==0){ i+=4; return Value::makeNull(); } throw std::runtime_error("invalid null"); }

    std::shared_ptr<Value> parseNumber(){
        size_t start=i; if (peek()=='-' ) ++i;
        while(std::isdigit((unsigned char)peek())) ++i;
        if (peek()=='.') { ++i; while(std::isdigit((unsigned char)peek())) ++i; }
        std::string token = src.substr(start, i-start);
        double v = 0.0; try { v = std::stod(token); } catch(...) { throw std::runtime_error("invalid number: " + token); }
        return Value::makeNumber(v);
    }
};

inline std::shared_ptr<Value> parseString(const std::string& s) {
    Parser p(s);
    return p.parse();
}

} // namespace mini_json

#endif // THIRD_PARTY_MINI_JSON_HPP
