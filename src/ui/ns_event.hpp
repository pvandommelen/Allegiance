
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

class EventToBoolean : public ModifiableBoolean, IEventSink {
    TRef<IEventSource> m_pEnableSource;
    TRef<IEventSource> m_pDisableSource;

public:
    EventToBoolean(IEventSource* pEnableSource, IEventSource* pDisableSource) :
        m_pEnableSource(pEnableSource),
        m_pDisableSource(pDisableSource),
        ModifiableBoolean(false)
    {
        pEnableSource->AddSink(this);
        pDisableSource->AddSink(this);
    }

    ~EventToBoolean() {
        m_pEnableSource->RemoveSink(this);
        m_pDisableSource->RemoveSink(this);
    }

    bool OnEvent(IEventSource* source) {
        if (source == m_pEnableSource) {
            SetValue(true);
        }
        else if (source == m_pDisableSource) {
            SetValue(false);
        }
        else {
            ZAssert(false);
        }
        return true;
    }
};

template<typename TypeResult>
class EventToMappedValue : public TWrapValue<TypeResult>, IEventSink {
    typedef TRef<TStaticValue<TypeResult>> TypeWrappedResult;

    std::map<TRef<IEventSource>, TypeWrappedResult> m_mapOptions;

public:
    EventToMappedValue(TypeWrappedResult tDefault, std::map<TRef<IEventSource>, TypeWrappedResult> mapOptions) :
        m_mapOptions(mapOptions),
        TWrapValue(tDefault)
    {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->AddSink(this);
        }
    }

    ~EventToMappedValue() {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->RemoveSink(this);
        }
    }

    bool OnEvent(IEventSource* source) {
        auto find = m_mapOptions.find(source);
        if (find == m_mapOptions.end()) {
            ZAssert(false);
        }
        else {
            SetWrappedValue(find->second);
        }
        return true;
    }
};

class EventToMappedImage : public WrapImage, IEventSink {
    typedef TRef<Image> TypeWrappedResult;

    std::map<TRef<IEventSource>, TypeWrappedResult> m_mapOptions;

public:
    EventToMappedImage(Image* pImage, std::map<TRef<IEventSource>, TypeWrappedResult> mapOptions) :
        m_mapOptions(mapOptions),
        WrapImage(pImage)
    {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->AddSink(this);
        }
    }

    ~EventToMappedImage() {
        for (auto kv = m_mapOptions.begin(); kv != m_mapOptions.end(); ++kv)
        {
            kv->first->RemoveSink(this);
        }
    }

    bool OnEvent(IEventSource* source) {
        auto find = m_mapOptions.find(source);
        if (find == m_mapOptions.end()) {
            ZAssert(false);
        }
        else {
            SetImage(find->second);
        }
        return true;
    }
};

class EventNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table table = m_pLua->create_table();

        table["OnEvent"] = [](IEventSink* pEventSink, IEventSource* pEventSource) {
            pEventSource->AddSink(pEventSink);
        };

        table["Get"] = [](Image* image, std::string string) {
            MouseEventImage* pMouseEventImage = (MouseEventImage*)(image);
            return pMouseEventImage->GetEventSource(string);
        };

        table["ToBoolean"] = [](sol::object valueDefault, sol::table table) {
            std::map<TRef<IEventSource>, TRef<Boolean>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<IEventSource*>();
                mapOptions[mapKey] = wrapValue<bool>(value);
            });
            
            return (TRef<Boolean>)new EventToMappedValue<bool>(wrapValue<bool>(valueDefault), mapOptions);
        };

        table["ToNumber"] = [](sol::object valueDefault, sol::table table) {
            std::map<TRef<IEventSource>, TRef<Number>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<IEventSource*>();
                mapOptions[mapKey] = wrapValue<float>(value);
            });

            return (TRef<Number>)new EventToMappedValue<float>(wrapValue<float>(valueDefault), mapOptions);
        };

        table["ToString"] = [](sol::object valueDefault, sol::table table) {
            std::map<TRef<IEventSource>, TRef<StringValue>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<IEventSource*>();
                mapOptions[mapKey] = wrapValue<ZString>(value);
            });

            return (TRef<StringValue>)new EventToMappedValue<ZString>(wrapValue<ZString>(valueDefault), mapOptions);
        };

        table["ToImage"] = [](Image* valueDefault, sol::table table) {
            std::map<TRef<IEventSource>, TRef<Image>> mapOptions;

            table.for_each([&mapOptions](sol::object key, sol::object value) {
                TRef<IEventSource> mapKey = key.as<IEventSource*>();
                mapOptions[mapKey] = value.as<Image*>();
            });

            return (TRef<Image>)new EventToMappedImage(valueDefault, mapOptions);
        };

        m_pLua->set("Event", table);
    }
};