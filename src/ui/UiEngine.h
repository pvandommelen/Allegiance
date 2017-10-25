#pragma once
class UiEngine : public IObject
{
protected:

    static std::string m_stringArtPath;

public:

    static std::string m_stringLogPath;

    static void SetGlobalArtPath(std::string path);
    static UiEngine* UiEngine::Create(Engine* pEngine);

    //virtual Image* LoadImage(std::string path) = 0;
    virtual TRef<Image> LoadImageFromLua(std::string path) = 0;

    virtual void TriggerReload() = 0;
};

