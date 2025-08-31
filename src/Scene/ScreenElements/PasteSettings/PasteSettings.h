#pragma once
#include <Scene/ScreenElements/ScreenElement.h>
#include <ImGui/imgui.h>
#include <Data/IngameWorld/IngameWorld.h>

NiSmartPointer(PasteSettings);
class PasteSettings : public ScreenElement
{
    NiDeclareRTTI;

public:
    PasteSettings();
    virtual ~PasteSettings();
    virtual bool Draw() override;

    void SetWorld(IngameWorldPtr world) { _world = world; }
    void Show() { _isVisible = true; }
    void Hide() { _isVisible = false; }
    void Toggle() { _isVisible = !_isVisible; }
    bool IsVisible() const { return _isVisible; }

    struct Settings {
        bool enableRandomRotation = false;
        bool enableTerrainSnap = false;
        bool enableRandomScaling = false;
        float minScale = 0.5f;
        float maxScale = 2.0f;
        bool autoGenerateSHBD = false;
    } pasteSettings;

private:
    void DrawSettingsUI();
    bool _isVisible;
    IngameWorldPtr _world;
};