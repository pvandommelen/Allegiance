#pragma once
class UiEngine
{
protected:

    static std::string m_stringArtPath;

public:

    static void SetGlobalArtPath(std::string path);
    static UiEngine* UiEngine::Create(Engine* pEngine);

    //virtual Image* LoadImage(std::string path) = 0;
    virtual TRef<Image> LoadImageFromLua(std::string path) = 0;
};

