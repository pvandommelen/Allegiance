#define SOL_CHECK_ARGUMENTS 1

#include "pch.h"
#include "ui.h"

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

class LoaderImpl {

private:
    PathFinder m_pathfinder;
    sol::state* m_pLua;

public:

    LoaderImpl(sol::state& lua, Engine* pEngine, std::vector<std::string> paths)
        : m_pathfinder(paths)
    {
        m_pLua = &lua;
        PathFinder* pathfinder = &m_pathfinder;

        sol::table tableImage = m_pLua->create_table();
        tableImage["GetEmpty"] = []() {
            return Image::GetEmpty();
        };
        tableImage["CreateExtent"] = [](RectValue* rect, ColorValue* color) {
            return CreateExtentImage(rect, color);
        };
        tableImage["LoadFile"] = [pEngine, pathfinder](std::string path) {
            //std::string subpath = (std::string)path;
            std::string fullpath = pathfinder->FindPath(path);

            if (fullpath == "") {
                throw std::runtime_error("Path not found: " + path);
            }

            const char* charString = fullpath.c_str();

            PathString pathString = PathString(ZString(charString));

            TRef<ZFile> zf = new ZFile(pathString, OF_READ | OF_SHARE_DENY_WRITE);
            ZFile * pFile = (ZFile*)zf;

            D3DXIMAGE_INFO fileInfo;
            if (D3DXGetImageInfoFromFileInMemory(pFile->GetPointer(),
                pFile->GetLength(),
                &fileInfo) == D3D_OK)
            {
                _ASSERT(fileInfo.ResourceType == D3DRTYPE_TEXTURE);

                // We can resize non-UI textures.
                WinPoint targetSize(fileInfo.Width, fileInfo.Height);

                DWORD dwMaxTextureSize = CD3DDevice9::Get()->GetMaxTextureSize();
                _ASSERT(dwMaxTextureSize >= 256);
                while ((targetSize.x > (LONG)dwMaxTextureSize) ||
                    (targetSize.y > (LONG)dwMaxTextureSize))
                {
                    targetSize.x = targetSize.x >> 1;
                    targetSize.y = targetSize.y >> 1;
                }
                // For D3D9, we only allow black colour keys.
                TRef<Surface> psurface =
                    pEngine->CreateSurfaceD3DX(
                        &fileInfo,
                        &targetSize,
                        zf,
                        false,
                        Color(0, 0, 0),
                        pathString);

                return (TRef<Image>)new ConstantImage(psurface, pathString);
            }
            else
            {
                _ASSERT(false && "Failed to load image.");
            }
        };
        tableImage["Group"] = [](sol::table list) {
            TRef<GroupImage> pgroup = new GroupImage();

            int count = list.size();

            TRef<Image> child;

            for (int i = 1; i <= count; ++i) {
                child = list.get<TRef<Image>>(i);
                pgroup->AddImageToTop(child);
            }

            return (TRef<Image>)pgroup;
        };
        m_pLua->set("Image", tableImage);

        sol::table tableRect = m_pLua->create_table();
        tableRect["Create"] = [](int left, int bottom, int width, int height) {
            return new RectValue(Rect(left, bottom, width, height));
        };
        m_pLua->set("Rect", tableRect);

        sol::table tableColor = m_pLua->create_table();
        tableColor["Create"] = [](float r, float g, float b, sol::optional<float> alpha) {
            return new ColorValue(Color(r, g, b, alpha.value_or(1.0f)));
        };
        m_pLua->set("Color", tableColor);
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