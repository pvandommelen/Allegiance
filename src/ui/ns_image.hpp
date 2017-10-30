
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

TRef<Image> LoadImageFile(LuaScriptContext& context, std::string path) {
    std::string fullpath = context.FindPath(path);
    Engine* pEngine = context.GetEngine();

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
    virtual TRef<Image> GetEvaluatedImage(A bValue) = 0;

public:

    EvaluateImage(TStaticValue<A>* pvalue1) :
        WrapImage(Image::GetEmpty(), pvalue1)
    {

    }

    void Evaluate()
    {
        A value = ((TStaticValue<A>*)GetChild(1))->GetValue();

        TRef<Image> evaluated = GetEvaluatedImage(value);

        ZAssert(evaluated != NULL);

        SetImage(evaluated);
    }

};

template<class A>
class CallbackEvaluateImage : public EvaluateImage<A> {

    std::function<TRef<Image>(A)> m_callback;

protected:
    TRef<Image> GetEvaluatedImage(A value) {
        return m_callback(value);
    }

public:
    CallbackEvaluateImage(TStaticValue<A>* value, std::function<TRef<Image>(A)> callback) :
        EvaluateImage(value),
        m_callback(callback)
    {}
};

class BoolSwitchImage : public EvaluateImage<bool> {

    std::map<bool, TRef<Image>> m_options;

protected:
    TRef<Image> GetEvaluatedImage(bool bValue) {
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

class ImageSize : public PointValue {
private:
    Image* GetImage() { return Image::Cast(GetChild(0)); }

public:
    ImageSize(Image* pimage) :
        PointValue(pimage)
    {
    }

    void Evaluate()
    {
        GetValueInternal() = Point::Cast(GetImage()->GetBounds().GetRect().Size());
    }
};

class ImageNamespace {
public:
    static void AddNamespace(LuaScriptContext& context) {

        sol::table table = context.GetLua().create_table();
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
            return (TRef<Image>)new MouseEventImage(image);
        };
        table["LoadFile"] = [&context](std::string path) {
            return LoadImageFile(context, path);
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

                return (TRef<Image>)new BoolSwitchImage(value.as<ModifiableBoolean*>(), mapOptions);
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
            return (TRef<Image>)new CallbackEvaluateImage<Point>(pPoint, [imageRefKeeping](Point point) {
                return (TRef<Image>)new TranslateImage(
                    imageRefKeeping,
                    point
                );
            });
        };

        sol::table tableJustification = context.GetLua().create_table();
        tableJustification["Left"] = (Justification)JustifyLeft();
        tableJustification["Right"] = (Justification)JustifyRight();
        tableJustification["XCenter"] = (Justification)JustifyXCenter();
        tableJustification["Top"] = (Justification)JustifyTop();
        tableJustification["Bottom"] = (Justification)JustifyBottom();
        tableJustification["YCenter"] = (Justification)JustifyYCenter();
        tableJustification["Center"] = (Justification)JustifyCenter();
        table["Justification"] = tableJustification;

        table["ScaleFit"] = [](Image* pimage, PointValue* pPoint, Justification justification) {
            //this should maybe be redone to return a custom class instead of using so many wrappers, it's a cool example though

            TRef<ImageSize> sizeImage = new ImageSize(pimage);
            TRef<Number> pImageX = PointTransform::X(sizeImage);
            TRef<Number> pImageY = PointTransform::Y(sizeImage);

            TRef<Number> pContainerX = PointTransform::X(pPoint);
            TRef<Number> pContainerY = PointTransform::Y(pPoint);

            TRef<Number> pScale = NumberTransform::Min(
                NumberTransform::Divide(pContainerX, pImageX),
                NumberTransform::Divide(pContainerY, pImageY)
            );

            TRef<Number> pSpaceX = NumberTransform::Subtract(pContainerX, NumberTransform::Multiply(pScale, pImageX));
            TRef<Number> pSpaceY = NumberTransform::Subtract(pContainerY, NumberTransform::Multiply(pScale, pImageY));

            TRef<Number> pNumberZero = new Number(0.0f);
            TRef<Number> pNumberHalf = new Number(0.5f);

            TRef<Number> pOffsetX;
            TRef<Number> pOffsetY;

            if (justification.Test(JustifyLeft())) {
                pOffsetX = pNumberZero;
            }
            else if (justification.Test(JustifyRight())) {
                pOffsetX = pSpaceX;
            }
            else {
                pOffsetX = NumberTransform::Multiply(pNumberHalf, pSpaceX);
            }

            if (justification.Test(JustifyTop())) {
                pOffsetY = pSpaceY;
            }
            else if (justification.Test(JustifyBottom())) {
                pOffsetY = pNumberZero;
            }
            else {
                pOffsetY = NumberTransform::Multiply(pNumberHalf, pSpaceY);
            }

            TRef<PointValue> pPointTranslate = PointTransform::Create(pOffsetX, pOffsetY);
            TRef<PointValue> pPointScale = PointTransform::Create(pScale, pScale);

            return (TRef<Image>)new TransformImage(
                new TransformImage(
                    pimage,
                    new ScaleTransform2(pPointScale)
                ),
                new TranslateTransform2(pPointTranslate)
            );
        };

        context.GetLua().set("Image", table);
    }
};