// serializeJSON.hpp - JSON serialization/deserialization helper functions.
//
// Copyright (c) 2016 Timothy Zhu.
// Licensed under the MIT License. See LICENSE file for details.
//

#ifndef _SERIALIZE_JSON_HPP
#define _SERIALIZE_JSON_HPP

#include <string>
#include <iostream>
#include "../json/json.h"

using namespace std;

class Serializable
{
public:
    virtual ~Serializable() {}

    // Serialize object into a JSON object.
    virtual void serialize(Json::Value& json) const = 0;
    // Deserialize JSON object into object.
    virtual void deserialize(const Json::Value& json) = 0;
};

inline void assignData2JSON(Json::Value& json, const char* val)
{
    json = Json::Value(val);
}

inline void assignData2JSON(Json::Value& json, double val)
{
    json = Json::Value(val);
}

inline void assignData2JSON(Json::Value& json, int val)
{
    json = Json::Value(val);
}

inline void assignData2JSON(Json::Value& json, unsigned int val)
{
    json = Json::Value(val);
}

inline void assignData2JSON(Json::Value& json, bool val)
{
    json = Json::Value(val);
}

inline void assignData2JSON(Json::Value& json, string val)
{
    json = Json::Value(val);
}

inline void assignData2JSON(Json::Value& json, const Serializable& obj)
{
    obj.serialize(json);
}

inline void assignData2JSON(Json::Value& json, const Serializable* obj)
{
    obj->serialize(json);
}

template <typename T>
inline void assignData2JSON(Json::Value& json, const vector<T>& vals)
{
    json = Json::Value(Json::arrayValue);
    for (unsigned int i = 0; i < vals.size(); i++) {
        assignData2JSON(json[i], vals[i]);
    }
}

template <typename T>
inline void serializeJSON(Json::Value& json, string name, T val)
{
    assignData2JSON(json[name], val);
}

inline void assignJSON2Data(const Json::Value& json, double& val)
{
    val = json.asDouble();
}

inline void assignJSON2Data(const Json::Value& json, int& val)
{
    val = json.asInt();
}

inline void assignJSON2Data(const Json::Value& json, unsigned int& val)
{
    val = json.asUInt();
}

inline void assignJSON2Data(const Json::Value& json, bool& val)
{
    val = json.asBool();
}

inline void assignJSON2Data(const Json::Value& json, string& val)
{
    val = json.asString();
}

inline void assignJSON2Data(const Json::Value& json, Serializable& obj)
{
    obj.deserialize(json);
}

template <class T>
inline void assignJSON2Data(const Json::Value& json, T*& obj)
{
    obj = T::create(json);
}

template <typename T>
inline void assignJSON2Data(const Json::Value& json, vector<T>& vals)
{
    vals.resize(json.size());
    for (unsigned int i = 0; i < json.size(); i++) {
        assignJSON2Data(json[i], vals[i]);
    }
}

template <typename T>
inline void deserializeJSON(const Json::Value& json, string name, T& val)
{
    if (json.isMember(name)) {
        assignJSON2Data(json[name], val);
    } else {
        cerr << "Invalid serialization data: Missing " << name << " data" << endl;
    }
}

#endif // _SERIALIZE_JSON_HPP
