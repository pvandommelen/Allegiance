
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

using namespace std::literals;

class PlaySoundSink : public IEventSink {
private:
    TRef<ISoundEngine> m_pSoundEngine;
    TRef<ISoundTemplate> m_pTemplate;

public:

    PlaySoundSink(ISoundEngine* pSoundEngine, ISoundTemplate* pTemplate) :
        m_pSoundEngine(pSoundEngine),
        m_pTemplate(pTemplate)
    {}

    bool OnEvent(IEventSource* pevent) {

        TRef<ISoundInstance> pSoundInstance;

        m_pTemplate->CreateSound(pSoundInstance, m_pSoundEngine->GetBufferSource(), NULL);

        return true;
    }
};

class ScreenNamespace {
public:
    static void AddNamespace(LuaScriptContext& context) {
        sol::table table = context.GetLua().create_table();

        table["PlayOnEvent"] = [&context](std::string path, IEventSource* pEventSource) {
            TRef<ISoundTemplate> pTemplate;

            std::string full_path = context.FindPath(path);

            ZSucceeded(CreateWaveFileSoundTemplate(pTemplate, full_path.c_str()));
            pEventSource->AddSink(new PlaySoundSink(context.GetSoundEngine(), pTemplate));
        };

        table["DelegateEventToExternalSink"] = [&context](std::string path, IEventSource* pEventSource) {
            IEventSink& sink = context.GetExternalEventSink(path);
            pEventSource->AddSink(&sink);
        };
        
        table["GetResolution"] = [&context]() {
            return (TRef<PointValue>)new TransformedValue<Point, WinPoint>([](WinPoint winpoint) {
                return Point(winpoint.X(), winpoint.Y());
            }, context.GetEngine()->GetResolutionSizeModifiable());
        };

        context.GetLua().set("Screen", table);
    }
};
