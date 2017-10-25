

#include "pch.h"
#include "ui.h"

class MouseEventImage : public WrapImage {
    std::map<std::string, TRef<EventSourceImpl>> m_mapEventSources;

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
        Trigger("mouse.leave");
    }
};