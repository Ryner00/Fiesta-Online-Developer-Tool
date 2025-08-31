#pragma once
#include <NiMain.h>
#include <ImGui/imgui.h>
#include <Data/IngameWorld/IngameWorld.h>

class MinimapRenderer
{
public:
    MinimapRenderer();
    ~MinimapRenderer();
    
    void DrawMinimap(IngameWorldPtr world, NiCameraPtr camera, const std::string& editModeName);
    void DrawSBISettings(IngameWorldPtr world, const std::string& editModeName);
    void SetVisible(bool visible) { _visible = visible; }
    bool IsVisible() { return _visible; }
    void ToggleVisible() { _visible = !_visible; }
    
    void SetSize(int size) { _size = size; }
    int GetSize() { return _size; }
    
    void SetZoom(float zoom) { _zoom = zoom; }
    float GetZoom() { return _zoom; }
    void ResetZoom() { _zoom = 1.0f; _offsetX = 0; _offsetY = 0; }
    
private:
    void DrawSHBDBackground(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale, float shbdSize);
    void DrawSBIDoors(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale);
    void DrawSBIPixels(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale);
    void HandleMinimapInteraction(IngameWorldPtr world, NiCameraPtr camera, ImVec2 canvasPos, ImVec2 canvasSize, float scale, const std::string& editModeName);
    void DrawResizeHandles(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, float scale, int doorIndex);
    int FindResizeHandle(IngameWorldPtr world, ImVec2 mousePos, ImVec2 canvasPos, float scale, int doorIndex);
    void DrawPlayerPosition(IngameWorldPtr world, NiCameraPtr camera, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale);
    
    bool _visible;
    int _size;
    float _zoom;
    float _offsetX, _offsetY;
    int _selectedDoor;
    int _draggingDoor;
    int _resizeHandle;
    bool _isDragging;
    bool _isResizing;
    bool _paintMode;
    bool _showPixels;
    int _brushSize;
    ImVec2 _dragStartPos;
    ImVec2 _resizeStartPos;
    ImVec2 _originalDoorStart;
    ImVec2 _originalDoorEnd;
};