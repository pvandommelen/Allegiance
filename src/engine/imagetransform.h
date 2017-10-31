
#pragma once

class ImageTransform {
public:
    static TRef<Image> Translate(Image* pImage, PointValue* pPoint);
    static TRef<Image> Scale(Image* pImage, PointValue* pPoint);

    static TRef<PointValue> Size(Image* pImage);

    static TRef<Image> Justify(Image* pImage, PointValue* pContainer, Justification justification);
    static TRef<Image> ScaleFit(Image* pImage, PointValue* pContainer, Justification justification);
    static TRef<Image> ScaleFill(Image* pImage, PointValue* pContainer, Justification justification);
};