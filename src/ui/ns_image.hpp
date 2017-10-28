
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

TRef<Image> LoadImageFile(Engine* pEngine, PathFinder* pPathFinder, std::string path) {
    std::string fullpath = pPathFinder->FindPath(path);

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
        return Image::GetEmpty();
    }
}

template<class A>
class EvaluateImage : public WrapImage {

private:


protected:
    virtual Image* GetEvaluatedImage(A bValue) = 0;

public:

    EvaluateImage(TStaticValue<A>* pvalue1) :
        WrapImage(Image::GetEmpty(), pvalue1)
    {

    }

    void Evaluate()
    {
        A value = ((TStaticValue<A>*)GetChild(1))->GetValue();

        Image* evaluated = GetEvaluatedImage(value);

        ZAssert(evaluated != NULL);

        SetImage(evaluated);
    }

};

template<class A>
class CallbackEvaluateImage : public EvaluateImage<A> {

    std::function<Image*(A)> m_callback;

protected:
    Image* GetEvaluatedImage(A value) {
        return m_callback(value);
    }

public:
    CallbackEvaluateImage(TStaticValue<A>* value, std::function<Image*(A)> callback) :
        EvaluateImage(value),
        m_callback(callback)
    {}
};

class BoolSwitchImage : public EvaluateImage<bool> {

    std::map<bool, TRef<Image>> m_options;

protected:
    Image* GetEvaluatedImage(bool bValue) {
        auto find = m_options.find(bValue);
        if (find == m_options.end()) {
            return Image::GetEmpty();
        }
        return find->second;
    }

public:
    BoolSwitchImage(TStaticValue<bool>* value, std::map<bool, TRef<Image>> options) :
        EvaluateImage(value),
        m_options(options)
    {}
};

class ImageNamespace {
public:
    static void AddNamespace(sol::state* m_pLua, Engine* pEngine, PathFinder* pPathFinder) {
        sol::table table = m_pLua->create_table();
        table["GetEmpty"] = []() {
            return Image::GetEmpty();
        };
        table["CreateExtent"] = sol::overload(
            [](RectValue* rect, ColorValue* color) {
                return CreateExtentImage(rect, color);
            },
            [](PointValue* pPoint, ColorValue* color) {
                return CreateExtentImage(
                    new RectValue(Rect(
                        Point(0, 0), 
                        pPoint->GetValue()
                    )), 
                    color
                );
            }
        );
        table["CreateMouseEvent"] = [](Image* image) {
            return (Image*)new MouseEventImage(image);
        };
        table["LoadFile"] = [pEngine, pPathFinder](std::string path) {
            return LoadImageFile(pEngine, pPathFinder, path);
        };
        table["Group"] = [](sol::table list) {
            TRef<GroupImage> pgroup = new GroupImage();

            int count = list.size();

            TRef<Image> child;

            for (int i = 1; i <= count; ++i) {
                child = list.get<Image*>(i);
                pgroup->AddImageToTop(child);
            }

            return (TRef<Image>)pgroup;
        };
        table["Switch"] = [](sol::object value, sol::table table) {
            int count = table.size();
            if (value.is<ModifiableBoolean*>() || value.is<ModifiableNumber*>() || value.is<ModifiableString*>()) {
                std::map<bool, TRef<Image>> mapOptions;

                table.for_each([&mapOptions](sol::object key, sol::object value) {
                    bool bKey = key.as<bool>();
                    mapOptions[bKey] = value.as<Image*>();
                });

                return (Image*)new BoolSwitchImage(value.as<ModifiableBoolean*>(), mapOptions);
            }
            else if (value.is<bool>() || value.is<int>() || value.is<std::string>()) {

            }
            throw std::runtime_error("Expected value argument of Image.Switch to be either a wrapped or unwrapped bool, int, or string");
        };
        table["String"] = [](FontValue* font, ColorValue* color, int width, std::string string) {
            return CreateStringImage(JustifyLeft(), font->GetValue(), color, width, new StringValue(ZString(string.c_str())));
        };
        table["Translate"] = [](Image* pimage, PointValue* pPoint) {
            //bit of a hack. The inner function only gets called in the next event loop. This keeps a reference around. I assume this gets cleaned up correctly.
            TRef<Image> imageRefKeeping = pimage;
            return (Image*)new CallbackEvaluateImage<Point>(pPoint, [imageRefKeeping](Point point) {
                return (Image*)new TranslateImage(
                    imageRefKeeping,
                    point
                );
            });
            
        };

        m_pLua->set("Image", table);
    }
};