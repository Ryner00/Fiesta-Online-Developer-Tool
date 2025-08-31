#include "MinimapRenderer.h"
#include <ImGui/imgui_internal.h>
#include <Data/ShineBuildingInfo/ShineBuildingInfo.h>
#include <Logger.h>
#include <algorithm>

MinimapRenderer::MinimapRenderer()
    : _visible(false), _size(500), _zoom(1.0f), _offsetX(0), _offsetY(0),
      _selectedDoor(-1), _draggingDoor(-1), _resizeHandle(-1),
      _isDragging(false), _isResizing(false), _paintMode(true), _showPixels(true),
      _brushSize(1), _resizeStartPos(0,0), _originalDoorStart(0,0), _originalDoorEnd(0,0)
{
}

MinimapRenderer::~MinimapRenderer()
{
}

void MinimapRenderer::DrawMinimap(IngameWorldPtr world, NiCameraPtr camera, const std::string& editModeName)
{
    if (!_visible || !world)
        return;
        
    ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(_size + 100, _size + 150), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Minimap", &_visible, ImGuiWindowFlags_None))
    {
        ImGui::Text("Global Minimap (Press M to toggle)");
        
        ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size = ImVec2((float)_size, (float)_size);
        
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(40, 40, 40, 255));
        draw_list->AddRect(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(200, 200, 200, 255));
        
        auto SHBD = world->GetSHBD();
        if (SHBD)
        {
            float shbdSize = (float)SHBD->GetSHBDSize();
            float scale = (canvas_size.x * _zoom) / shbdSize;
            
            // Auto-follow player when zoomed in
            if (_zoom > 1.2f && camera)
            {
                auto ini = world->GetShineIni();
                if (ini)
                {
                    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
                    NiPoint3 cameraPos = camera->GetTranslate();
                    float playerPixelX = cameraPos.x / PixelSize;
                    float playerPixelY = cameraPos.y / PixelSize;
                    
                    float playerScreenX = playerPixelX * scale;
                    float playerScreenY = playerPixelY * scale;
                    
                    _offsetX = (canvas_size.x / 2) - playerScreenX;
                    _offsetY = (canvas_size.y / 2) - playerScreenY;
                }
            }
            else if (_zoom <= 1.2f)
            {
                _offsetX = 0;
                _offsetY = 0;
            }
            
            DrawSHBDBackground(world, draw_list, canvas_pos, canvas_size, scale, shbdSize);
            
            if (editModeName == "SBI")
            {
                DrawSBIDoors(world, draw_list, canvas_pos, canvas_size, scale);
                if (_showPixels)
                {
                    DrawSBIPixels(world, draw_list, canvas_pos, canvas_size, scale);
                }
                if (_selectedDoor >= 0)
                {
                    DrawResizeHandles(world, draw_list, canvas_pos, scale, _selectedDoor);
                }
            }
            
            DrawPlayerPosition(world, camera, draw_list, canvas_pos, canvas_size, scale);
        }
        
        ImGui::SetCursorScreenPos(canvas_pos);
        ImGui::InvisibleButton("minimap_canvas", canvas_size);
        
        if (SHBD)
        {
            float shbdSize = (float)SHBD->GetSHBDSize();
            float scale = (canvas_size.x * _zoom) / shbdSize;
            HandleMinimapInteraction(world, camera, canvas_pos, canvas_size, scale, editModeName);
        }
        
        ImGui::Text("Left click: Teleport | Shift+Click: Select door");
    }
    ImGui::End();
}

void MinimapRenderer::DrawSBISettings(IngameWorldPtr world, const std::string& editModeName)
{
    if (!_visible || editModeName != "SBI")
        return;
        
    ImGui::SetNextWindowPos(ImVec2(650, 100), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
    
    bool settingsOpen = true;
    if (ImGui::Begin("SBI Settings", &settingsOpen, ImGuiWindowFlags_None))
    {
        ImGui::Text("Edit Mode: %s", editModeName.c_str());
        ImGui::Separator();
        
        ImGui::Text("Minimap Controls:");
        if (ImGui::DragInt("Size", &_size, 10, 300, 1000))
        {
            _size = std::max(300, std::min(1000, _size));
        }
        
        if (ImGui::DragFloat("Zoom", &_zoom, 0.1f, 0.5f, 5.0f))
        {
            _zoom = std::max(0.5f, std::min(5.0f, _zoom));
        }
        
        if (ImGui::Button("Reset View"))
        {
            ResetZoom();
        }
        
        ImGui::Separator();
        ImGui::Text("Display Options:");
        
        ImGui::Checkbox("Show Pixels", &_showPixels);
        
        ImGui::Separator();
        ImGui::Text("Instructions:");
        ImGui::TextWrapped("- Left Click: Teleport camera");
        ImGui::TextWrapped("- Shift+Click: Select/drag door");
        ImGui::TextWrapped("- Map follows player when zoomed in");
        ImGui::TextWrapped("- Use 3D view for pixel editing");
        
        if (_selectedDoor >= 0)
        {
            auto SBI = world->GetSBI();
            if (SBI)
            {
                auto door = SBI->GetDoor(_selectedDoor);
                if (door)
                {
                    ImGui::Separator();
                    ImGui::Text("Selected Door: %d", _selectedDoor);
                    ImGui::Text("Name: %s", door->blockName.c_str());
                    ImGui::Text("Position: (%.0f,%.0f) to (%.0f,%.0f)", 
                               door->startPos.x, door->startPos.y,
                               door->endPos.x, door->endPos.y);
                    ImGui::Text("Size: %dx%d", door->GetWidth(), door->GetHeight());
                }
            }
        }
    }
    ImGui::End();
}

void MinimapRenderer::DrawSHBDBackground(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale, float shbdSize)
{
    auto SHBD = world->GetSHBD();
    if (!SHBD)
        return;
        
    int step = std::max(1, (int)(shbdSize / (canvasSize.x * _zoom / 2)));
    
    for (int y = 0; y < shbdSize; y += step)
    {
        for (int x = 0; x < shbdSize; x += step)
        {
            float screenX = canvasPos.x + (x * scale) + _offsetX;
            float screenY = canvasPos.y + (y * scale) + _offsetY;
            float stepScale = step * scale;
            
            if (screenX + stepScale >= canvasPos.x && screenY + stepScale >= canvasPos.y &&
                screenX <= canvasPos.x + canvasSize.x && screenY <= canvasPos.y + canvasSize.y)
            {
                ImU32 color;
                if (SHBD->IsWalkable(x, y))
                {
                    color = IM_COL32(0, 150, 0, 180);
                }
                else
                {
                    color = IM_COL32(60, 60, 60, 180);
                }
                
                drawList->AddRectFilled(
                    ImVec2(std::max(screenX, canvasPos.x), std::max(screenY, canvasPos.y)),
                    ImVec2(std::min(screenX + stepScale, canvasPos.x + canvasSize.x), 
                           std::min(screenY + stepScale, canvasPos.y + canvasSize.y)),
                    color
                );
            }
        }
    }
}

void MinimapRenderer::DrawSBIDoors(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale)
{
    auto SBI = world->GetSBI();
    if (!SBI)
        return;
        
    for (size_t i = 0; i < SBI->GetDoorCount(); i++)
    {
        auto door = SBI->GetDoor(i);
        if (!door)
            continue;
            
        float x1 = canvasPos.x + (door->startPos.x * scale) + _offsetX;
        float y1 = canvasPos.y + (door->startPos.y * scale) + _offsetY;
        float x2 = canvasPos.x + ((door->endPos.x + 1) * scale) + _offsetX;
        float y2 = canvasPos.y + ((door->endPos.y + 1) * scale) + _offsetY;
        
        if (x2 >= canvasPos.x && y2 >= canvasPos.y &&
            x1 <= canvasPos.x + canvasSize.x && y1 <= canvasPos.y + canvasSize.y)
        {
            ImU32 color = (i == _selectedDoor) ? IM_COL32(255, 255, 0, 200) : IM_COL32(0, 128, 255, 200);
            
            drawList->AddRectFilled(
                ImVec2(std::max(x1, canvasPos.x), std::max(y1, canvasPos.y)),
                ImVec2(std::min(x2, canvasPos.x + canvasSize.x), std::min(y2, canvasPos.y + canvasSize.y)),
                color & 0x60FFFFFF
            );
            drawList->AddRect(
                ImVec2(std::max(x1, canvasPos.x), std::max(y1, canvasPos.y)),
                ImVec2(std::min(x2, canvasPos.x + canvasSize.x), std::min(y2, canvasPos.y + canvasSize.y)),
                IM_COL32(0, 0, 0, 255), 0.0f, 0, 4.0f
            );
        }
    }
}

void MinimapRenderer::DrawSBIPixels(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale)
{
    auto SBI = world->GetSBI();
    if (!SBI)
        return;
        
    for (size_t i = 0; i < SBI->GetDoorCount(); i++)
    {
        auto door = SBI->GetDoor(i);
        if (!door)
            continue;
            
        float doorStartX = canvasPos.x + (door->startPos.x * scale) + _offsetX;
        float doorStartY = canvasPos.y + (door->startPos.y * scale) + _offsetY;
        
        float pixelScale = std::max(scale, 2.0f);
        
        for (int y = 0; y < door->GetHeight(); y++)
        {
            for (int x = 0; x < door->GetWidth(); x++)
            {
                bool isSet = SBI->IsDoorPixelSet(i, x, y);
                if (isSet)
                {
                    float pixelX = doorStartX + (x * scale);
                    float pixelY = doorStartY + (y * scale);
                    
                    if (pixelX + pixelScale >= canvasPos.x && pixelY + pixelScale >= canvasPos.y &&
                        pixelX <= canvasPos.x + canvasSize.x && pixelY <= canvasPos.y + canvasSize.y)
                    {
                        ImU32 pixelColor = IM_COL32(0, 0, 255, 200);
                        drawList->AddRectFilled(
                            ImVec2(std::max(pixelX, canvasPos.x), std::max(pixelY, canvasPos.y)),
                            ImVec2(std::min(pixelX + pixelScale, canvasPos.x + canvasSize.x), 
                                   std::min(pixelY + pixelScale, canvasPos.y + canvasSize.y)),
                            pixelColor
                        );
                        drawList->AddRect(
                            ImVec2(std::max(pixelX, canvasPos.x), std::max(pixelY, canvasPos.y)),
                            ImVec2(std::min(pixelX + pixelScale, canvasPos.x + canvasSize.x), 
                                   std::min(pixelY + pixelScale, canvasPos.y + canvasSize.y)),
                            IM_COL32(0, 0, 0, 255), 0.0f, 0, 2.0f
                        );
                    }
                }
            }
        }
    }
}

void MinimapRenderer::HandleMinimapInteraction(IngameWorldPtr world, NiCameraPtr camera, ImVec2 canvasPos, ImVec2 canvasSize, float scale, const std::string& editModeName)
{
    if (!ImGui::IsItemHovered())
    {
        if (ImGui::IsMouseReleased(0))
        {
            _isDragging = false;
            _isResizing = false;
            _draggingDoor = -1;
            _resizeHandle = -1;
        }
        return;
    }
    
    ImVec2 mouse_pos = ImGui::GetMousePos();
    auto SHBD = world->GetSHBD();
    auto SBI = world->GetSBI();
    
    if (!SHBD)
        return;
        
    float shbdSize = (float)SHBD->GetSHBDSize();
    float mapX = ((mouse_pos.x - canvasPos.x - _offsetX) / scale);
    float mapY = ((mouse_pos.y - canvasPos.y - _offsetY) / scale);
    
    ImGui::SetTooltip("Map: %.0f, %.0f", mapX, mapY);
    
    if (editModeName == "SBI" && SBI && ImGui::GetIO().KeyShift)
    {
        if (ImGui::IsMouseClicked(0))
        {
            int resizeHandle = FindResizeHandle(world, mouse_pos, canvasPos, scale, _selectedDoor);
            if (resizeHandle >= 0)
            {
                _isResizing = true;
                _resizeHandle = resizeHandle;
                _resizeStartPos = ImVec2(mapX, mapY);
                auto door = SBI->GetDoor(_selectedDoor);
                if (door)
                {
                    _originalDoorStart = ImVec2(door->startPos.x, door->startPos.y);
                    _originalDoorEnd = ImVec2(door->endPos.x, door->endPos.y);
                }
            }
            else
            {
                for (int i = (int)SBI->GetDoorCount() - 1; i >= 0; i--)
                {
                    auto door = SBI->GetDoor(i);
                    if (door && mapX >= door->startPos.x && mapX <= door->endPos.x &&
                        mapY >= door->startPos.y && mapY <= door->endPos.y)
                    {
                        _selectedDoor = i;
                        _isDragging = true;
                        _draggingDoor = i;
                        _dragStartPos = ImVec2(mapX, mapY);
                        break;
                    }
                }
            }
        }
        
        if (_isResizing && _resizeHandle >= 0 && ImGui::IsMouseDragging(0))
        {
            auto door = SBI->GetDoor(_selectedDoor);
            if (door)
            {
                float deltaX = mapX - _resizeStartPos.x;
                float deltaY = mapY - _resizeStartPos.y;
                
                float newStartX = _originalDoorStart.x;
                float newStartY = _originalDoorStart.y;
                float newEndX = _originalDoorEnd.x;
                float newEndY = _originalDoorEnd.y;
                
                switch (_resizeHandle)
                {
                    case 0: // Top-left
                        newStartX = _originalDoorStart.x + deltaX;
                        newStartY = _originalDoorStart.y + deltaY;
                        break;
                    case 1: // Top-right
                        newEndX = _originalDoorEnd.x + deltaX;
                        newStartY = _originalDoorStart.y + deltaY;
                        break;
                    case 2: // Bottom-left
                        newStartX = _originalDoorStart.x + deltaX;
                        newEndY = _originalDoorEnd.y + deltaY;
                        break;
                    case 3: // Bottom-right
                        newEndX = _originalDoorEnd.x + deltaX;
                        newEndY = _originalDoorEnd.y + deltaY;
                        break;
                }
                
                if (newStartX > newEndX) { float temp = newStartX; newStartX = newEndX; newEndX = temp; }
                if (newStartY > newEndY) { float temp = newStartY; newStartY = newEndY; newEndY = temp; }
                
                int width = static_cast<int>(newEndX - newStartX + 1);
                if (width % 8 != 0)
                    width = ((width / 8) + 1) * 8;
                newEndX = newStartX + width - 1;
                
                if (newStartX >= 0 && newStartY >= 0 && newEndX < shbdSize && newEndY < shbdSize && width >= 8)
                {
                    door->startPos.x = newStartX;
                    door->startPos.y = newStartY;
                    door->endPos.x = newEndX;
                    door->endPos.y = newEndY;
                    door->data.resize(door->GetDataSize(), 0);
                }
            }
        }
        else if (_isDragging && _draggingDoor >= 0 && ImGui::IsMouseDragging(0))
        {
            auto door = SBI->GetDoor(_draggingDoor);
            if (door)
            {
                float deltaX = mapX - _dragStartPos.x;
                float deltaY = mapY - _dragStartPos.y;
                
                float width = door->endPos.x - door->startPos.x;
                float height = door->endPos.y - door->startPos.y;
                
                float newStartX = door->startPos.x + deltaX;
                float newStartY = door->startPos.y + deltaY;
                
                if (newStartX >= 0 && newStartY >= 0 && 
                    newStartX + width < shbdSize && newStartY + height < shbdSize)
                {
                    door->startPos.x = newStartX;
                    door->startPos.y = newStartY;
                    door->endPos.x = newStartX + width;
                    door->endPos.y = newStartY + height;
                    _dragStartPos = ImVec2(mapX, mapY);
                }
            }
        }
        
    }
    
    if (ImGui::IsMouseClicked(0) && !ImGui::GetIO().KeyShift && !_isDragging)
    {
        if (mapX >= 0 && mapY >= 0 && mapX < shbdSize && mapY < shbdSize && camera)
        {
            auto ini = world->GetShineIni();
            if (ini)
            {
                float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
                float worldX = mapX * PixelSize;
                float worldY = mapY * PixelSize;
                camera->SetTranslate(NiPoint3(worldX, worldY, 100.0f));
                LogInfo("Camera teleported to " + std::to_string(worldX) + ", " + std::to_string(worldY));
            }
        }
    }
    
    if (ImGui::IsMouseReleased(0))
    {
        _isDragging = false;
        _draggingDoor = -1;
    }
}

void MinimapRenderer::DrawPlayerPosition(IngameWorldPtr world, NiCameraPtr camera, ImDrawList* drawList, ImVec2 canvasPos, ImVec2 canvasSize, float scale)
{
    if (!world || !camera)
        return;
        
    auto SHBD = world->GetSHBD();
    auto ini = world->GetShineIni();
    if (!SHBD || !ini)
        return;
        
    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
    
    NiPoint3 cameraPos = camera->GetTranslate();
    float pixelX = cameraPos.x / PixelSize;
    float pixelY = cameraPos.y / PixelSize;
    
    float screenX = canvasPos.x + (pixelX * scale) + _offsetX;
    float screenY = canvasPos.y + (pixelY * scale) + _offsetY;
    
    if (screenX >= canvasPos.x && screenY >= canvasPos.y &&
        screenX <= canvasPos.x + canvasSize.x && screenY <= canvasPos.y + canvasSize.y)
    {
        float dotRadius = 6.0f;
        drawList->AddCircleFilled(
            ImVec2(screenX, screenY),
            dotRadius,
            IM_COL32(255, 255, 0, 255)
        );
        drawList->AddCircle(
            ImVec2(screenX, screenY),
            dotRadius,
            IM_COL32(0, 0, 0, 255),
            0, 4.0f
        );
    }
}void MinimapRenderer::DrawResizeHandles(IngameWorldPtr world, ImDrawList* drawList, ImVec2 canvasPos, float scale, int doorIndex)
{
    auto SBI = world->GetSBI();
    if (!SBI || doorIndex < 0)
        return;
        
    auto door = SBI->GetDoor(doorIndex);
    if (!door)
        return;
        
    float x1 = canvasPos.x + (door->startPos.x * scale) + _offsetX;
    float y1 = canvasPos.y + (door->startPos.y * scale) + _offsetY;
    float x2 = canvasPos.x + ((door->endPos.x + 1) * scale) + _offsetX;
    float y2 = canvasPos.y + ((door->endPos.y + 1) * scale) + _offsetY;
    
    float handleSize = 8.0f;
    ImU32 handleColor = IM_COL32(255, 255, 0, 255);
    
    ImVec2 corners[4] = {
        ImVec2(x1, y1),
        ImVec2(x2, y1),
        ImVec2(x1, y2),
        ImVec2(x2, y2)
    };
    
    for (int i = 0; i < 4; i++)
    {
        drawList->AddRectFilled(
            ImVec2(corners[i].x - handleSize/2, corners[i].y - handleSize/2),
            ImVec2(corners[i].x + handleSize/2, corners[i].y + handleSize/2),
            handleColor
        );
        drawList->AddRect(
            ImVec2(corners[i].x - handleSize/2, corners[i].y - handleSize/2),
            ImVec2(corners[i].x + handleSize/2, corners[i].y + handleSize/2),
            IM_COL32(0, 0, 0, 255), 0.0f, 0, 4.0f
        );
    }
}

int MinimapRenderer::FindResizeHandle(IngameWorldPtr world, ImVec2 mousePos, ImVec2 canvasPos, float scale, int doorIndex)
{
    auto SBI = world->GetSBI();
    if (!SBI || doorIndex < 0)
        return -1;
        
    auto door = SBI->GetDoor(doorIndex);
    if (!door)
        return -1;
        
    float x1 = canvasPos.x + (door->startPos.x * scale) + _offsetX;
    float y1 = canvasPos.y + (door->startPos.y * scale) + _offsetY;
    float x2 = canvasPos.x + ((door->endPos.x + 1) * scale) + _offsetX;
    float y2 = canvasPos.y + ((door->endPos.y + 1) * scale) + _offsetY;
    
    float handleSize = 8.0f;
    
    ImVec2 corners[4] = {
        ImVec2(x1, y1),
        ImVec2(x2, y1),
        ImVec2(x1, y2),
        ImVec2(x2, y2)
    };
    
    for (int i = 0; i < 4; i++)
    {
        if (mousePos.x >= corners[i].x - handleSize/2 && mousePos.x <= corners[i].x + handleSize/2 &&
            mousePos.y >= corners[i].y - handleSize/2 && mousePos.y <= corners[i].y + handleSize/2)
        {
            return i;
        }
    }
    
    return -1;
}