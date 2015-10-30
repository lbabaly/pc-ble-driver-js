#ifndef COMMON_H
#define COMMON_H

#include <nan.h>
#include <map>
#include <mutex>
#include <string>
#include "ble.h"
#include "ble_hci.h"

#define NAME_MAP_ENTRY(EXP) { EXP, ""#EXP"" }
#define ERROR_STRING_SIZE 1024
#define BATON_CONSTRUCTOR(BatonType) BatonType(v8::Local<v8::Function> callback) : Baton(callback) {}
#define BATON_DESTRUCTOR(BatonType) ~BatonType()

#define METHOD_DEFINITIONS(MainName) \
    NAN_METHOD(MainName); \
    void MainName(uv_work_t *req); \
    void After##MainName(uv_work_t *req);

// Typedef of name to string with enum name, covers most cases
typedef std::map<uint16_t, char*> name_map_t;
typedef std::map<uint16_t, char*>::iterator name_map_it_t;

// Mutex used to assure that only one call is done to the BLE Driver API at a time.
// The BLE Driver API is not thread safe.
static std::mutex ble_driver_call_mutex;

class ConversionUtility;


template<typename NativeType>
class BleToJs
{
protected:
    v8::Local<v8::Object> jsobj;
    NativeType *native;
public:
    BleToJs(v8::Local<v8::Object> js) : jsobj(js) {}
    BleToJs(NativeType *native) : native(native) {}

    virtual v8::Local<v8::Object> ToJs()
    {
        /*TODO: ASSERT*/
        Nan::EscapableHandleScope scope;
        return scope.Escape(Nan::New<v8::Object>());
    }
    virtual NativeType *ToNative() { /*TODO: ASSERT*/ return new NativeType(); }

    operator NativeType*() { return ToNative(); }
    operator NativeType() { return *(ToNative()); }
    operator v8::Handle<v8::Value>()
    {
        Nan::EscapableHandleScope scope;
        return scope.Escape(ToJs());
    }
};

class Utility
{
public:
	static v8::Local<v8::Value> Get(v8::Local<v8::Object> jsobj, char *name);
	static void SetMethod(v8::Handle<v8::Object> target, char *exportName, Nan::FunctionCallback function);

	static bool Set(v8::Handle<v8::Object> target, char *name, int32_t value);
	static bool Set(v8::Handle<v8::Object> target, char *name, uint32_t value);
	static bool Set(v8::Handle<v8::Object> target, char *name, int16_t value);
	static bool Set(v8::Handle<v8::Object> target, char *name, uint16_t value);
	static bool Set(v8::Handle<v8::Object> target, char *name, int8_t value);
	static bool Set(v8::Handle<v8::Object> target, char *name, uint8_t value);
	static bool Set(v8::Handle<v8::Object> target, char *name, bool value);
	static bool Set(v8::Handle<v8::Object> target, char *name, double value);
	static bool Set(v8::Handle<v8::Object> target, char *name, char *value);
	static bool Set(v8::Handle<v8::Object> target, char *name, std::string value);
	static bool Set(v8::Handle<v8::Object> target, char *name, v8::Local<v8::Value> value);

	static void SetReturnValue(Nan::NAN_METHOD_ARGS_TYPE info, v8::Local<v8::Object> value);

    static bool IsObject(v8::Local<v8::Object> jsobj, char *name);
};

template<typename EventType>
class BleDriverEvent : public BleToJs<EventType>
{
private:
    BleDriverEvent() {}

protected:
    uint16_t evt_id;
    std::string timestamp;
    uint16_t conn_handle;
    EventType *evt;

public:
    BleDriverEvent(uint16_t evt_id, std::string timestamp, uint16_t conn_handle, EventType *evt)
        : BleToJs<EventType>(0),
        evt_id(evt_id),
        timestamp(timestamp),
        conn_handle(conn_handle),
        evt(evt)
    {
    }

    virtual void ToJs(v8::Local<v8::Object> obj)
    {
        Utility::Set(obj, "id", evt_id);
        Utility::Set(obj, "name", getEventName());
        Utility::Set(obj, "time", timestamp);
        Utility::Set(obj, "conn_handle", conn_handle);
    }

    virtual v8::Local<v8::Object> ToJs() = 0;
    virtual EventType *ToNative() = 0;
    virtual char *getEventName() = 0;
};

struct Baton {
public:
    Baton(v8::Local<v8::Function> cb) {
        req = new uv_work_t();
        callback = new Nan::Callback(cb);
        req->data = (void*)this;
    }

    ~Baton()
    {
        delete callback;
    }

    uv_work_t *req;
    Nan::Callback *callback;

    int result;
    char errorString[ERROR_STRING_SIZE];
};

const std::string getCurrentTimeInMilliseconds();

uint16_t uint16_decode(const uint8_t *p_encoded_data);
uint32_t uint32_decode(const uint8_t *p_encoded_data);

uint16_t fromNameToValue(name_map_t names, char *name);

template<typename NativeType>
class ConvUtil
{
public:
    static NativeType getNativeUnsigned(v8::Local<v8::Value> js)
    {
        if (!js->IsNumber())
        {
            throw "number";
        }

        return (NativeType)js->ToUint32()->Uint32Value();
    }

    static NativeType getNativeSigned(v8::Local<v8::Value> js)
    {
        if (!js->IsNumber())
        {
            throw "number";
        }

        return (NativeType)js->ToInt32()->Int32Value();
    }

    static NativeType getNativeFloat(v8::Local<v8::Value> js)
    {
        if (!js->IsNumber())
        {
            throw "number";
        }

        return (NativeType)js->ToNumber()->NumberValue();
    }

    static NativeType getNativeBool(v8::Local<v8::Value> js)
    {
        if (!js->IsBoolean())
        {
            throw "bool";
        }
        return (NativeType)js->ToBoolean()->BooleanValue() ? 1 : 0;
    }

    static NativeType getNativeUnsigned(v8::Local<v8::Object> js, char *name)
    {
        return getNativeUnsigned(js->Get(Nan::New(name).ToLocalChecked()));
    }

    static NativeType getNativeSigned(v8::Local<v8::Object> js, char *name)
    {
        return getNativeSigned(js->Get(Nan::New(name).ToLocalChecked()));
    }

    static NativeType getNativeFloat(v8::Local<v8::Object> js, char *name)
    {
        return getNativeFloat(js->Get(Nan::New(name).ToLocalChecked()));
    }

    static NativeType getNativeBool(v8::Local<v8::Object> js, char *name)
    {
        return getNativeBool(js->Get(Nan::New(name).ToLocalChecked()));
    }
};

class ConversionUtility
{
public:
    enum ConversionUnits {
        ConversionUnit625ms = 625,
        ConversionUnit1250ms = 1250,
        ConversionUnit10000ms = 10000,
        ConversionUnit10s = ConversionUnit10000ms
    };

    static uint32_t     getNativeUint32(v8::Local<v8::Object>js, char *name);
    static uint32_t     getNativeUint32(v8::Local<v8::Value> js);
    static uint16_t     getNativeUint16(v8::Local<v8::Object>js, char *name);
    static uint16_t     getNativeUint16(v8::Local<v8::Value> js);
    static uint8_t      getNativeUint8(v8::Local<v8::Object>js, char *name);
    static uint8_t      getNativeUint8(v8::Local<v8::Value> js);
    static int32_t      getNativeInt32(v8::Local<v8::Object>js, char *name);
    static int32_t      getNativeInt32(v8::Local<v8::Value> js);
    static int16_t      getNativeInt16(v8::Local<v8::Object>js, char *name);
    static int16_t      getNativeInt16(v8::Local<v8::Value> js);
    static int8_t       getNativeInt8(v8::Local<v8::Object>js, char *name);
    static int8_t       getNativeInt8(v8::Local<v8::Value> js);
    static double       getNativeDouble(v8::Local<v8::Object>js, char *name);
    static double       getNativeDouble(v8::Local<v8::Value> js);
    static uint8_t      getNativeBool(v8::Local<v8::Object>js, char *name);
    static uint8_t      getNativeBool(v8::Local<v8::Value>js);
    static uint8_t *    getNativePointerToUint8(v8::Local<v8::Object>js, char *name);
    static uint8_t *    getNativePointerToUint8(v8::Local<v8::Value>js);
    static uint16_t *   getNativePointerToUint16(v8::Local<v8::Object>js, char *name);
    static uint16_t *   getNativePointerToUint16(v8::Local<v8::Value>js);
    static v8::Local<v8::Object> getJsObject(v8::Local<v8::Object>js, char *name);
    static v8::Local<v8::Object> getJsObject(v8::Local<v8::Value>js);
    static uint16_t     stringToValue(name_map_t name_map, v8::Local<v8::Object> string, uint16_t defaultValue = -1);

    static uint16_t msecsToUnitsUint16(v8::Local<v8::Object>js, char *name, enum ConversionUnits unit);
    static uint16_t msecsToUnitsUint16(double msecs, enum ConversionUnits unit);
    static uint8_t  msecsToUnitsUint8(v8::Local<v8::Object>js, char *name, enum ConversionUnits unit);
    static uint8_t  msecsToUnitsUint8(double msecs, enum ConversionUnits unit);
    static v8::Handle<v8::Value> unitsToMsecs(uint16_t units, enum ConversionUnits unit);

    static v8::Handle<v8::Value> toJsNumber(int32_t nativeValue);
    static v8::Handle<v8::Value> toJsNumber(uint32_t nativeValue);
    static v8::Handle<v8::Value> toJsNumber(uint16_t nativeValue);
    static v8::Handle<v8::Value> toJsNumber(uint8_t nativeValue);
    static v8::Handle<v8::Value> toJsNumber(double nativeValue);
    static v8::Handle<v8::Value> toJsBool(uint8_t nativeValue);
    static v8::Handle<v8::Value> toJsValueArray(uint8_t *nativeValue, uint16_t length);
    static v8::Handle<v8::Value> toJsString(char *cString);
    static v8::Handle<v8::Value> toJsString(char *cString, uint16_t length);
    static v8::Handle<v8::Value> toJsString(uint8_t *cString, uint16_t length);
    static v8::Handle<v8::Value> toJsString(std::string string);
    static char *                valueToString(uint16_t value, name_map_t name_map, char *defaultValue = "Unknown value");
    static v8::Handle<v8::Value> valueToJsString(uint16_t, name_map_t name_map, v8::Handle<v8::Value> defaultValue = Nan::New<v8::String>("Unknown value").ToLocalChecked());

    static v8::Local<v8::Function> getCallbackFunction(v8::Local<v8::Object> js, char *name);
    static v8::Local<v8::Function> getCallbackFunction(v8::Local<v8::Value> js);

    static uint8_t extractHexHelper(char text);
    static uint8_t *extractHex(v8::Local<v8::Value> js);
    static v8::Handle<v8::Value> encodeHex(char *text, int length);
};

class ErrorMessage
{
public:
    static v8::Local<v8::Value> getErrorMessage(int errorCode, char const*customMessage);
    static v8::Local<v8::String> getTypeErrorMessage(int argumentNumber, char const *message);
    static v8::Local<v8::String> getStructErrorMessage(char const *name, char const *message);
};

class HciStatus
{
public:
    static v8::Local<v8::Value> getHciStatus(int statusCode);
};

#endif
