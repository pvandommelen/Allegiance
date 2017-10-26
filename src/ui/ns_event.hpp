
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

class EventNamespace {
public:
    static void AddNamespace(sol::state* m_pLua) {
        sol::table tableEvent = m_pLua->create_table();

        tableEvent["Get"] = [](TRef<Image> image, std::string string) {
            MouseEventImage* pMouseEventImage = (MouseEventImage*)((Image*)image);
            return pMouseEventImage->GetEventSource(string);
        };

        

        tableEvent["ToBoolean"] = [](IEventSource* pEnableSource, IEventSource* pDisableSource) {
            return new EventToBoolean(pEnableSource, pDisableSource);
        };

        m_pLua->set("Event", tableEvent);
    }
};