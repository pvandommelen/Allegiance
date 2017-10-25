#define SOL_CHECK_ARGUMENTS 1

#include "pch.h"
#include "ui.h"

#include <stdexcept>


std::string UiEngine::m_stringArtPath;
void UiEngine::SetGlobalArtPath(std::string path)
{
    m_stringArtPath = path;
}

class LoaderImpl {

private:
    std::vector<std::string> m_paths;
    sol::state* m_pLua;

public:

    LoaderImpl(sol::state& lua, Engine* pEngine, std::vector<std::string> paths)
    {
        m_pLua = &lua;
        m_paths = paths;

        sol::table tableImage = m_pLua->create_table();
        tableImage["GetEmpty"] = []() {
            return Image::GetEmpty();
        };
        tableImage["CreateExtent"] = [](RectValue* rect) {
            return CreateExtentImage(rect, new ColorValue(Color(1, 0, 0)));
        };
        tableImage["LoadFile"] = [pEngine](PathString path) {

            TRef<ZFile> zf = new ZFile(path, OF_READ | OF_SHARE_DENY_WRITE);
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
                        path);

                return (Value*)new ConstantImage(psurface, path);
            }
            else
            {
                _ASSERT(false && "Failed to load image.");
            }
        };
        m_pLua->set("Image", tableImage);

        sol::table tableRect = m_pLua->create_table();
        tableRect["Create"] = [](int left, int bottom, int width, int height) {
            return new RectValue(Rect(left, bottom, width, height));
        };
        m_pLua->set("Rect", tableRect);
    }

    ~LoaderImpl() {
    }

    sol::function LoadScript(std::string subpath) {

        std::string path = FindPath(subpath);
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

private:
    // from: https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
    inline bool file_exists(const std::string& name) {
        struct stat buffer;
        return (stat(name.c_str(), &buffer) == 0);
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

    TRef<Engine> m_pEngine;


public:
    UiEngineImpl(Engine* pEngine) : 
        m_pEngine(pEngine)
    {
    }

    ~UiEngineImpl() {}

    //Image* LoadImage(std::string path) {
    //
    //}

    TRef<Image> LoadImageFromLua(std::string path) {
        sol::state lua;
        LoaderImpl loader = LoaderImpl(lua, m_pEngine, {
            m_stringArtPath + "/PBUI",
            m_stringArtPath
        });

        Executor executor = Executor(loader);

        try {
            sol::function script = loader.LoadScript(path);

            TRef<Image> image = executor.Execute<TRef<Image>>(script);
            return image;
        }
        catch (const std::runtime_error& e) {
            return Image::GetEmpty();
        }
    }
};

UiEngine* UiEngine::Create(Engine* pEngine)
{
    return new UiEngineImpl(pEngine);
}