
#pragma once

#include "pch.h"
#include "ui.h"
#include "items.hpp"

class ImageNamespace {
public:
    static void AddNamespace(sol::state* m_pLua, Engine* pEngine, PathFinder* pPathFinder) {
        sol::table tableImage = m_pLua->create_table();
        tableImage["GetEmpty"] = []() {
            return Image::GetEmpty();
        };
        tableImage["CreateExtent"] = [](RectValue* rect, ColorValue* color) {
            return CreateExtentImage(rect, color);
        };
        tableImage["CreateMouseEvent"] = [](TRef<Image> image) {
            return (TRef<Image>)new MouseEventImage(image);
        };
        tableImage["LoadFile"] = [pEngine, pPathFinder](std::string path) {
            //std::string subpath = (std::string)path;
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
    }
};