#include "PasteSettings.h"

NiImplementRTTI(PasteSettings, ScreenElement);

PasteSettings::PasteSettings()
    : _isVisible(false)
    , _world(nullptr)
{
}

PasteSettings::~PasteSettings()
{
}

bool PasteSettings::Draw()
{
    if (!_isVisible)
        return true;
        
    DrawSettingsUI();
    return true;
}

void PasteSettings::DrawSettingsUI()
{
    ImGui::SetNextWindowSize(ImVec2(350, 250), ImGuiCond_FirstUseEver);
    ImGui::Begin("Paste Settings", &_isVisible, ImGuiWindowFlags_None);
    
    ImGui::Text("Object Pasting Configuration");
    ImGui::Separator();
    
    ImGui::Checkbox("Random Rotation", &pasteSettings.enableRandomRotation);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Randomly rotate objects 0-360 degrees on Z axis");
    
    ImGui::Checkbox("Snap to Terrain", &pasteSettings.enableTerrainSnap);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Align objects to ground rotation");
    
    ImGui::Checkbox("Random Scaling", &pasteSettings.enableRandomScaling);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Apply random scaling between min and max values");
    
    if (pasteSettings.enableRandomScaling)
    {
        ImGui::Indent();
        ImGui::SliderFloat("Min Scale", &pasteSettings.minScale, 0.1f, 5.0f, "%.2f");
        ImGui::SliderFloat("Max Scale", &pasteSettings.maxScale, 0.1f, 5.0f, "%.2f");
        
        if (pasteSettings.minScale > pasteSettings.maxScale)
            pasteSettings.maxScale = pasteSettings.minScale;
        
        ImGui::Unindent();
    }
    
    ImGui::Checkbox("Auto Generate SHBD", &pasteSettings.autoGenerateSHBD);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Automatically generate collision areas for newly pasted objects");
    
    ImGui::Separator();
    ImGui::Text("Keybinds:");
    ImGui::Text("Ctrl+V: Paste with current settings");
    ImGui::Text("Ctrl+1: Open this window");
    ImGui::Text("Middle Mouse Click: Context menu (includes this window)");
    
    ImGui::End();
}