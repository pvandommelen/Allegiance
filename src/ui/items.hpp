
#pragma once

#include "ui.h"
#include "enginep.h"
#include "font.h"
#include "valuetransform.h"
#include "imagetransform.h"

class PathFinder {

private:
    // from: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
    inline bool file_exists(const std::string& name) {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
    }
    std::vector<std::string> m_paths;

public:

    PathFinder(std::vector<std::string> paths) :
        m_paths(paths)
    {

    }

    std::string FindPath(std::string& subpath) {
        for (auto& path : m_paths) {
            if (file_exists(path + '/' + subpath) == true) {
                return path + '/' + subpath;
            }
        }
        return "";
    }
};

class MouseEventImage : public WrapImage {
    std::map<std::string, TRef<EventSourceImpl>> m_mapEventSources;
    bool m_bDown = false;

public:

    MouseEventImage(Image* wrapped) :
        WrapImage(wrapped)
    {
    }

    IEventSource* GetEventSource(std::string string) {
        auto found = m_mapEventSources.find(string);
        if (found != m_mapEventSources.end()) {
            return found->second;
        }

        TRef<EventSourceImpl> pEventSource = new EventSourceImpl();
        m_mapEventSources[string] = pEventSource;
        return pEventSource;
    }

    void Trigger(std::string string) {
        auto found = m_mapEventSources.find(string);
        if (found != m_mapEventSources.end()) {
            found->second->Trigger();
        }
    }

    void MouseEnter(IInputProvider* pprovider, const Point& point) {
        Trigger("mouse.enter");
    }

    void MouseLeave(IInputProvider* pprovider) {
        m_bDown = false; //reset state
        Trigger("mouse.leave");
    }

    void TriggerMouseButton(std::string button_name, std::string what) {
        Trigger("mouse." + button_name + "." + what);
    }

    MouseResult Button(IInputProvider* pprovider, const Point& point, int button, bool bCaptured, bool bInside, bool bDown)
    {
        // inspired by button.cpp~:~690
        if (button != 0 && button != 1) {
            return MouseResult();
        }
        std::string button_name = button == 0 ? "left" : "right";

        if (bDown) {
            m_bDown = true;
            TriggerMouseButton(button_name, "down");

            if (pprovider->IsDoubleClick()) {
                TriggerMouseButton(button_name, "doubleclick");
            }
        }
        else {
            bool wasDown = m_bDown;
            m_bDown = false;
            TriggerMouseButton(button_name, "up");

            if (wasDown) {
                TriggerMouseButton(button_name, "click");
            }
        }

        return MouseResult();
    }
};

template<typename A>
TRef<TStaticValue<A>> wrapValue(sol::object a) {
    TStaticValue<A>* converted_a;
    if (a.is<A>()) {
        converted_a = new TStaticValue<A>(a.as<A>());
    }
    else if (a.is<TStaticValue<A>*>()) {
        converted_a = a.as<TStaticValue<A>*>();
    }
    else {
        // force a cast, this sometimes still works, exception otherwise
        converted_a = new TStaticValue<A>(a.as<A>());
    }
    TRef<TStaticValue<A>> refcounted = converted_a;
    return refcounted;
};

TRef<StringValue> wrapString(sol::object a) {
    if (a.is<std::string>()) {
        return (TRef<StringValue>)new StringValue(ZString(a.as<std::string>().c_str()));
    }
    return wrapValue<ZString>(a);
};