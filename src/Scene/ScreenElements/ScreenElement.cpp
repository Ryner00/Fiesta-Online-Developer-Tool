#include "ScreenElement.h"

#include <Scene/ScreenElements/LuaElement/LuaElement.h>
#include <Scene/ScreenElements/LoadObject/AddSingleObject.h>
#include <Scene/ScreenElements/LoadObject/AddMultipleObject.h>
#include <Scene/ScreenElements/LoadObject/ReplaceObjects.h>
#include <Scene/ScreenElements/LoadObject/LoadDiffuseFile.h>
#include <Scene/ScreenElements/ObjectStash/ObjectStash.h>

NiImplementRootRTTI(ScreenElement);

NiImplementRTTI(LuaElement, ScreenElement);
NiImplementRTTI(AddMultipleObject, ScreenElement);
NiImplementRTTI(AddSingleObject, ScreenElement);
NiImplementRTTI(ReplaceObjects, ScreenElement);
NiImplementRTTI(LoadDiffuseFile, ScreenElement);
NiImplementRTTI(ObjectStash, ScreenElement);