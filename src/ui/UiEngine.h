#pragma once
class UiEngine : public IObject
{
protected:

    static std::string m_stringArtPath;

public:

    static std::string m_stringLogPath;

    static void SetGlobalArtPath(std::string path);
    static UiEngine* UiEngine::Create(Engine* pEngine, ISoundEngine* pSoundEngine);

    //virtual Image* LoadImage(std::string path) = 0;
    virtual TRef<Image> LoadImageFromLua(std::string path) = 0;

    virtual void TriggerReload() = 0;
};

class Loader {
public:
    virtual sol::function LoadScript(std::string subpath) = 0;
};

class LuaScriptContext {
public:
    virtual sol::state& GetLua() = 0;

    virtual sol::function LoadScript(std::string path) = 0;

    virtual std::string FindPath(std::string path) = 0;

    virtual Engine* GetEngine() = 0;

    virtual ISoundEngine* GetSoundEngine() = 0;
};