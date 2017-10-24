#include "pch.h"

#include "ui.h"
#include "introscreen2.h"


IntroScreen2::IntroScreen2()
{
}


IntroScreen2::~IntroScreen2()
{
}

TRef<Image> IntroScreen2::CreateImage()
{
    sol::state lua;
    int value = lua.script("return 54");
    
    //TRef<Image> img = );

    return CreateExtentImage(new RectValue(Rect(0, 0, 1000, 1000)), new ColorValue(Color(1, 0, 0)));
}