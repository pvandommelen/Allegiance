
#include "pch.h"

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

TRef<Image> ImageTransform::Translate(Image* pImage, PointValue* pPoint) {
    return new TransformImage(
        pImage,
        new TranslateTransform2(pPoint)
    );
};

TRef<Image> ImageTransform::Scale(Image* pImage, PointValue* pPoint) {
    return new TransformImage(
        pImage,
        new ScaleTransform2(pPoint)
    );
};

TRef<PointValue> ImageTransform::Size(Image* pImage) {
    return new ImageSize(pImage);
};

TRef<Image> ImageTransform::Justify(Image* pimage, PointValue* pSizeContainer, Justification justification) {
    TRef<PointValue> sizeImage = ImageTransform::Size(pimage);
    TRef<Number> pImageX = PointTransform::X(sizeImage);
    TRef<Number> pImageY = PointTransform::Y(sizeImage);

    TRef<Number> pContainerX = PointTransform::X(pSizeContainer);
    TRef<Number> pContainerY = PointTransform::Y(pSizeContainer);

    TRef<Number> pSpaceX = NumberTransform::Subtract(pContainerX, pImageX);
    TRef<Number> pSpaceY = NumberTransform::Subtract(pContainerY, pImageY);

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

    return (TRef<Image>)new TransformImage(
        pimage,
        new TranslateTransform2(pPointTranslate)
    );
}

TRef<Image> ImageTransform::ScaleFit(Image* pimage, PointValue* pSizeContainer, Justification justification) {
    //this should maybe be redone to return a custom class instead of using so many wrappers, it's a cool example though

    TRef<PointValue> sizeImage = ImageTransform::Size(pimage);
    TRef<Number> pImageX = PointTransform::X(sizeImage);
    TRef<Number> pImageY = PointTransform::Y(sizeImage);

    TRef<Number> pContainerX = PointTransform::X(pSizeContainer);
    TRef<Number> pContainerY = PointTransform::Y(pSizeContainer);

    // What scaling factor would we need for each axis? Pick the smallest one
    TRef<Number> pScale = NumberTransform::Min(
        NumberTransform::Divide(pContainerX, pImageX),
        NumberTransform::Divide(pContainerY, pImageY)
    );

    TRef<PointValue> pPointScale = PointTransform::Create(pScale, pScale);

    return ImageTransform::Justify(
        new TransformImage(
            pimage,
            new ScaleTransform2(pPointScale)
        ),
        pSizeContainer,
        justification
    );
}

TRef<Image> ImageTransform::ScaleFill(Image* pimage, PointValue* pSizeContainer, Justification justification) {
    //this should maybe be redone to return a custom class instead of using so many wrappers, it's a cool example though

    TRef<PointValue> sizeImage = ImageTransform::Size(pimage);
    TRef<Number> pImageX = PointTransform::X(sizeImage);
    TRef<Number> pImageY = PointTransform::Y(sizeImage);

    TRef<Number> pContainerX = PointTransform::X(pSizeContainer);
    TRef<Number> pContainerY = PointTransform::Y(pSizeContainer);

    // What scaling factor would we need for each axis? Pick the largest one
    TRef<Number> pScale = NumberTransform::Max(
        NumberTransform::Divide(pContainerX, pImageX),
        NumberTransform::Divide(pContainerY, pImageY)
    );

    TRef<PointValue> pPointScale = PointTransform::Create(pScale, pScale);

    return ImageTransform::Justify(
        new TransformImage(
            pimage,
            new ScaleTransform2(pPointScale)
        ),
        pSizeContainer,
        justification
    );
}