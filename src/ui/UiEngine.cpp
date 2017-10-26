#define SOL_CHECK_ARGUMENTS 1

#include "pch.h"
#include "ui.h"
#include "items.hpp";

#include "ns_math.hpp";
#include "ns_color.hpp";
#include "ns_event.hpp";
#include "ns_image.hpp";

#include <stdexcept>
#include <fstream>

std::string UiEngine::m_stringLogPath = "";

void WriteLog(const std::string &text)
{
    if (UiEngine::m_stringLogPath == "") {
        return;
    }
    std::ofstream log_file(UiEngine::m_stringLogPath, std::ios_base::out | std::ios_base::app);
    log_file << text << std::endl;
}

std::string UiEngine::m_stringArtPath;
void UiEngine::SetGlobalArtPath(std::string path)
{
    m_stringArtPath = path;
}

class LoaderImpl {

private:
    PathFinder m_pathfinder;
    sol::state* m_pLua;

public:

    LoaderImpl(sol::state& lua, Engine* pEngine, std::vector<std::string> paths)
        : m_pathfinder(paths)
    {
        m_pLua = &lua;

        ImageNamespace::AddNamespace(m_pLua, pEngine, &m_pathfinder);
        EventNamespace::AddNamespace(m_pLua);

        MathNamespace::AddNamespace(m_pLua);
        RectNamespace::AddNamespace(m_pLua);

        ColorNamespace::AddNamespace(m_pLua);        

        m_pLua->new_usertype<MouseEventImage>("MouseEventImage",
            sol::base_classes, sol::bases<Image>()
            );
    }

    ~LoaderImpl() {
    }

    sol::function LoadScript(std::string subpath) {

        std::string path = m_pathfinder.FindPath(subpath);
        if (path == "") {
            throw std::runtime_error("File not found: " + subpath);
        }

        sol::load_result script = m_pLua->load_file(path);
        if (script.valid() == false) {
            sol::error error = script;
            throw error;
        }
        sol::function function = script;

        return function;
    }
};

class Executor {

private:
    LoaderImpl* m_pLoader;

public:

    Executor(LoaderImpl& pLoader) {
        m_pLoader = &pLoader;
    }

    template <class T>
    T Execute(sol::function script) {
        try {
            sol::function_result result = script.call();
            if (result.valid() == false) {
                sol::error err = result;
                throw err;
            }
            sol::object object = result;
            if (object.is<T>() == false) {
                throw std::runtime_error("Expected return value to be of a specific type");
            }
            T image = result;
            return image;
        }
        catch (const sol::error& e) {
            throw e;
        }
        catch (const std::runtime_error& e) {
            throw e;
        }
    }
};

class UiEngineImpl : public UiEngine {

private:
    TRef<Engine> m_pEngine;

    TRef<EventSourceImpl> m_pReloadEventSource;


public:
    UiEngineImpl(Engine* pEngine) : 
        m_pEngine(pEngine),
        m_pReloadEventSource(new EventSourceImpl())
    {
    }

    void TriggerReload() {
        m_pReloadEventSource->Trigger();
    }

    ~UiEngineImpl() {}

    //Image* LoadImage(std::string path) {
    //
    //}

    TRef<Image> InnerLoadImageFromLua(std::string path) {
        sol::state lua;
        LoaderImpl loader = LoaderImpl(lua, m_pEngine, {
            m_stringArtPath + "/PBUI",
            m_stringArtPath
        });

        Executor executor = Executor(loader);

        WriteLog(path + ": " + "Loading");
        try {
            sol::function script = loader.LoadScript(path);

            WriteLog(path + ": " + "Parsed");

            TRef<Image> image = executor.Execute<TRef<Image>>(script);

            WriteLog(path + ": " + "Executed");
            return image;
        }
        catch (const std::runtime_error& e) {
            WriteLog(path + ": ERROR " + e.what());
            return Image::GetEmpty();
        }
    }

    class ImageReloadSink : public IEventSink, public WrapImage {

        TRef<UiEngineImpl> m_pUiEngine;
        std::string m_path;

    public:
        ImageReloadSink(UiEngineImpl* pUiEngine, std::string path) :
            m_pUiEngine(pUiEngine),
            m_path(path),
            WrapImage(pUiEngine->InnerLoadImageFromLua(path))
        {
        }

        ~ImageReloadSink() {
            m_pUiEngine->m_pReloadEventSource->RemoveSink(this);
        }

        bool OnEvent(IEventSource* pevent) {
            SetImage(m_pUiEngine->InnerLoadImageFromLua(m_path));
            return true;
        }

    };

    TRef<Image> LoadImageFromLua(std::string path) {
        //TRef<Image> inner = InnerLoadImageFromLua(path);
        //TRef<Image> wrapper = new WrapImage(inner);

        ImageReloadSink* result = new ImageReloadSink(this, path);
        m_pReloadEventSource->AddSink(result);
        return result;
    }
};


TRef<UiEngine> g_pUiEngine;
UiEngine* UiEngine::Create(Engine* pEngine)
{
    if (!g_pUiEngine) {
        g_pUiEngine = new UiEngineImpl(pEngine);
    }
    return g_pUiEngine;
}