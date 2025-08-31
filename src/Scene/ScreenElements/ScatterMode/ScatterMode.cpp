#include "ScatterMode.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <NiPick.h>
#include "../../../Scene/EditorScene/Modes/SHBDMode.h"

NiImplementRTTI(ScatterMode, ScreenElement);

ScatterMode::ScatterMode()
    : _isVisible(false)
    , _scatterActive(false)
    , _hasData(false)
    , _world(nullptr)
    , _selectedObject(nullptr)
    , _pasteSettings(nullptr)
    , _isDrawing(false)
    , _clearSelectionCallback(nullptr)
    , _highlightTimer(0.0f)
    , _predictionDirty(true)
    , _predictionUpdateTimer(0.0f)
{
}

ScatterMode::~ScatterMode()
{
}

bool ScatterMode::Draw()
{
    if (!_isVisible)
        return true;
        
    DrawScatterUI();
    
    if (_scatterActive || (scatterSettings.keepSelection && _hasData))
        DrawBrushOverlay();
    
    return true;
}

void ScatterMode::Update(float deltaTime)
{
    _predictionUpdateTimer += deltaTime;
    
    if (_predictionDirty && _predictionUpdateTimer >= 0.1f)
    {
        UpdatePrediction();
        _predictionDirty = false;
        _predictionUpdateTimer = 0.0f;
    }
}

void ScatterMode::DrawScatterUI()
{
    ImGui::SetNextWindowSize(ImVec2(350, 280), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(400, 50), ImGuiCond_FirstUseEver);
    ImGui::Begin("Object Scatter Mode", &_isVisible, ImGuiWindowFlags_None);
    
    ImGui::Text("Scatter Mode Settings");
    ImGui::Separator();
    
    if (!_selectedObject)
    {
        ImGui::Text("No object selected!");
        ImGui::Text("Select an object first to use scatter mode.");
        ImGui::End();
        return;
    }
    
    std::string objName = _selectedObject->GetSHMDPath();
    size_t lastSlash = objName.find_last_of("/\\");
    if (lastSlash != std::string::npos)
        objName = objName.substr(lastSlash + 1);
    
    ImGui::Text("Selected Object: %s", objName.c_str());
    ImGui::Separator();
    
    if (ImGui::SliderFloat("Brush Size", &scatterSettings.brushSize, 5.0f, 100.0f, "%.1f"))
        _predictionDirty = true;
    if (ImGui::SliderFloat("Density", &scatterSettings.density, 0.1f, 1.0f, "%.2f"))
        _predictionDirty = true;
    if (ImGui::SliderInt("Max Objects", &scatterSettings.maxObjects, 10, 1000))
        _predictionDirty = true;
    
    ImGui::Separator();
    ImGui::Text("Scatter Mode:");
    if (ImGui::RadioButton("Area Scatter", scatterSettings.useAreaScatter == true))
    {
        scatterSettings.useAreaScatter = true;
        _predictionDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Path Follow", scatterSettings.useAreaScatter == false))
    {
        scatterSettings.useAreaScatter = false;
        _predictionDirty = true;
    }
    
    ImGui::Separator();
    if (ImGui::Checkbox("Draw on Ground & Objects", &scatterSettings.drawOnObjects))
        _predictionDirty = true;
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("When enabled, scatter will work on both terrain and objects.\nWhen disabled, scatter only works on terrain.");
    
    bool prevKeepSelection = scatterSettings.keepSelection;
    ImGui::Checkbox("Keep Selection", &scatterSettings.keepSelection);
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("When enabled, painted area remains after scattering.\nWhen disabled, painted area is cleared after scattering.");
    
    if (ImGui::Checkbox("No Overlap Objects", &scatterSettings.noOverlap))
        _predictionDirty = true;
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("When enabled, objects will not overlap with each other.");
    
    if (scatterSettings.noOverlap)
    {
        if (ImGui::SliderFloat("Object Radius", &scatterSettings.objectRadius, 1.0f, 20.0f, "%.1f"))
            _predictionDirty = true;
    }
    
    if (prevKeepSelection && !scatterSettings.keepSelection)
    {
        _paintedPoints.clear();
        _hasData = false;
        _predictionDirty = true;
    }
    
    ImGui::Separator();
    
    if (_hasData && _selectedObject)
    {
        ImGui::Text("Estimated Objects: %d / %d", _lastPrediction.finalCount, scatterSettings.maxObjects);
        ImGui::Separator();
    }
    
    ImGui::Text("Uses Paste Settings for:");
    ImGui::BulletText("Random Rotation");
    ImGui::BulletText("Terrain Snapping");
    ImGui::BulletText("Random Scaling");
    
    ImGui::Separator();
    
    if (!_scatterActive)
    {
        if (ImGui::Button("Start Scatter Mode", ImVec2(-1, 30)))
            StartScatterMode();
    }
    else
    {
        ImGui::Text("SCATTER MODE ACTIVE");
        ImGui::Text("Paint on terrain to mark scatter areas");
        ImGui::Separator();
        
        if (_hasData)
        {
            if (ImGui::Button("Confirm Scatter", ImVec2(-1, 25)))
                ConfirmScatter();
            
            if (ImGui::Button("Cancel", ImVec2(-1, 25)))
                CancelScatter();
        }
        else
        {
            if (ImGui::Button("Stop Scatter Mode", ImVec2(-1, 25)))
                StopScatterMode();
        }
    }
    
    ImGui::Separator();
    ImGui::Text("Keybinds: Ctrl+R");
    
    ImGui::End();
}

void ScatterMode::DrawBrushOverlay()
{
    if (!_world)
        return;
    
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    
    // Draw painted areas
    NiCameraPtr camera = _world->GetCamera();
    if (camera)
    {
        for (const auto& paintedArea : _paintedPoints)
        {
            float screenX, screenY;
            if (camera->WorldPtToScreenPt(paintedArea.worldPos, screenX, screenY))
            {
                const NiRect<float>& viewport = camera->GetViewPort();
                float pixelX = (screenX - viewport.m_left) / (viewport.m_right - viewport.m_left) * io.DisplaySize.x;
                float pixelY = (1.0f - (screenY - viewport.m_bottom) / (viewport.m_top - viewport.m_bottom)) * io.DisplaySize.y;
                
                ImVec2 center(pixelX, pixelY);
                
                float screenRadius = paintedArea.brushSize * 2.0f;
                
                drawList->AddCircleFilled(center, screenRadius, IM_COL32(0, 255, 0, 80));
            }
        }
    }
    
    NiPoint3 worldMousePos = ScreenToWorld(io.MousePos);
    if (worldMousePos != NiPoint3::ZERO && camera)
    {
        float screenX, screenY;
        if (camera->WorldPtToScreenPt(worldMousePos, screenX, screenY))
        {
            const NiRect<float>& viewport = camera->GetViewPort();
            float pixelX = (screenX - viewport.m_left) / (viewport.m_right - viewport.m_left) * io.DisplaySize.x;
            float pixelY = (1.0f - (screenY - viewport.m_bottom) / (viewport.m_top - viewport.m_bottom)) * io.DisplaySize.y;
            
            ImVec2 center(pixelX, pixelY);
            float brushRadius = scatterSettings.brushSize * 2.0f;
            drawList->AddCircle(center, brushRadius, IM_COL32(255, 255, 255, 200), 32, 2.0f);
        }
    }
}

void ScatterMode::StartScatterMode()
{
    if (!_selectedObject || !_world)
        return;
    
    _scatterActive = true;
    if (!scatterSettings.keepSelection)
    {
        _hasData = false;
        _paintedPoints.clear();
    }
    _predictionDirty = true;
    _isDrawing = false;
    
    if (_pasteSettings)
        _pasteSettings->Show();
}

void ScatterMode::StopScatterMode()
{
    _scatterActive = false;
    _hasData = false;
    _paintedPoints.clear();
    _isDrawing = false;
    _predictionDirty = true;
}

void ScatterMode::ProcessMouseInput(const ImVec2& mousePos)
{
    if (!_scatterActive || !_world)
        return;
    
    ImGuiIO& io = ImGui::GetIO();
    
    if (io.MouseDown[0])
    {
        if (!_isDrawing)
        {
            _isDrawing = true;
            _lastMousePos = mousePos;
        }
        
        float distance = sqrtf(powf(mousePos.x - _lastMousePos.x, 2) + powf(mousePos.y - _lastMousePos.y, 2));
        if (distance > 5.0f)
        {
            NiPoint3 currentWorldPos = ScreenToWorld(mousePos);
            NiPoint3 lastWorldPos = ScreenToWorld(_lastMousePos);
            
            if ((currentWorldPos.x != 0.0f || currentWorldPos.y != 0.0f || currentWorldPos.z != 0.0f) &&
                (lastWorldPos.x != 0.0f || lastWorldPos.y != 0.0f || lastWorldPos.z != 0.0f))
            {
                float worldDistance = sqrtf(powf(currentWorldPos.x - lastWorldPos.x, 2) + powf(currentWorldPos.y - lastWorldPos.y, 2));
                float stepSize = scatterSettings.brushSize * 0.5f;
                int steps = (int)(worldDistance / stepSize) + 1;
                
                for (int i = 0; i <= steps; i++)
                {
                    float t = (steps > 0) ? (float)i / (float)steps : 0.0f;
                    NiPoint3 interpPos;
                    interpPos.x = lastWorldPos.x + t * (currentWorldPos.x - lastWorldPos.x);
                    interpPos.y = lastWorldPos.y + t * (currentWorldPos.y - lastWorldPos.y);
                    interpPos.z = lastWorldPos.z + t * (currentWorldPos.z - lastWorldPos.z);
                    
                    AddPaintPoint(interpPos);
                }
                
                _lastMousePos = mousePos;
                _hasData = true;
                _predictionDirty = true;
            }
        }
    }
    else
    {
        _isDrawing = false;
    }
}

void ScatterMode::ConfirmScatter()
{
    if (!_hasData || !_selectedObject || !_world)
        return;
    
    ScatterObjects();
    
    if (_clearSelectionCallback)
        _clearSelectionCallback();
    
    if (!scatterSettings.keepSelection)
    {
        StopScatterMode();
    }
    else
    {
        _scatterActive = false;
        _isDrawing = false;
    }
}

void ScatterMode::CancelScatter()
{
    StopScatterMode();
}

void ScatterMode::AddPaintPoint(const NiPoint3& worldPos)
{
    PaintedArea area;
    area.worldPos = worldPos;
    area.brushSize = scatterSettings.brushSize;
    _paintedPoints.push_back(area);
    _predictionDirty = true;
}

NiPoint3 ScatterMode::ScreenToWorld(const ImVec2& screenPos)
{
    if (!_world)
        return NiPoint3::ZERO;
    
    NiPoint3 worldPos;
    
    if (scatterSettings.drawOnObjects)
    {
        NiCameraPtr camera = _world->GetCamera();
        NiPoint3 kOrigin, kDir;
        if (camera && camera->WindowPointToRay(screenPos.x, screenPos.y, kOrigin, kDir))
        {
            NiPick _Pick;
            _Pick.SetPickType(NiPick::FIND_FIRST);
            _Pick.SetSortType(NiPick::SORT);
            _Pick.SetIntersectType(NiPick::TRIANGLE_INTERSECT);
            _Pick.SetFrontOnly(true);
            _Pick.SetReturnNormal(true);
            _Pick.SetObserveAppCullFlag(true);
            _Pick.SetTarget(_world->GetWorldNode());
            
            if (_Pick.PickObjects(kOrigin, kDir, true))
            {
                NiPick::Results& results = _Pick.GetResults();
                if (results.GetSize() > 0)
                {
                    worldPos = results.GetAt(0)->GetIntersection();
                }
            }
        }
        
        if (worldPos == NiPoint3::ZERO)
        {
            worldPos = _world->GetWorldPointFromScreen(screenPos.x, screenPos.y, IngameWorld::WorldIntersectType::GroundScene);
        }
    }
    else
    {
        worldPos = _world->GetWorldPointFromScreen(screenPos.x, screenPos.y, IngameWorld::WorldIntersectType::GroundScene);
    }
    
    if (_world->IsValidWorldPosition(worldPos))
    {
        return worldPos;
    }
    
    return NiPoint3::ZERO;
}

bool ScatterMode::IsPointInPaintedArea(const NiPoint3& point)
{
    for (const auto& paintedArea : _paintedPoints)
    {
        float distance = sqrtf(powf(point.x - paintedArea.worldPos.x, 2) + powf(point.y - paintedArea.worldPos.y, 2));
        if (distance <= paintedArea.brushSize)
            return true;
    }
    return false;
}

bool ScatterMode::IsPointInUnifiedPaintedArea(const NiPoint3& point)
{
    return IsPointInPaintedArea(point);
}

bool ScatterMode::CheckObjectOverlap(const NiPoint3& newPos, const std::vector<NiPoint3>& placedPositions)
{
    if (!scatterSettings.noOverlap)
        return false;
    
    for (const auto& existingPos : placedPositions)
    {
        float distance = sqrtf(powf(newPos.x - existingPos.x, 2) + powf(newPos.y - existingPos.y, 2));
        if (distance < scatterSettings.objectRadius * 2.0f)
            return true;
    }
    return false;
}

ScatterMode::PredictionResult ScatterMode::CalculatePredictedObjectCount()
{
    PredictionResult result = {0, 0, 0, 0, 0};
    
    if (!_selectedObject || !_world || _paintedPoints.empty())
        return result;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    NiPoint3 minBounds = _paintedPoints[0].worldPos;
    NiPoint3 maxBounds = _paintedPoints[0].worldPos;
    
    for (const auto& paintedArea : _paintedPoints)
    {
        minBounds.x = std::min(minBounds.x, paintedArea.worldPos.x - paintedArea.brushSize);
        minBounds.y = std::min(minBounds.y, paintedArea.worldPos.y - paintedArea.brushSize);
        maxBounds.x = std::max(maxBounds.x, paintedArea.worldPos.x + paintedArea.brushSize);
        maxBounds.y = std::max(maxBounds.y, paintedArea.worldPos.y + paintedArea.brushSize);
    }
    
    std::vector<NiPoint3> placedPositions;
    int placedObjects = 0;
    
    int sampleSize = 1000;
    int attempts = 0;
    int maxAttempts = std::min(sampleSize, scatterSettings.maxObjects * 5);
    
    while (placedObjects < scatterSettings.maxObjects && attempts < maxAttempts)
    {
        attempts++;
        result.totalAttempts++;
        
        float x = minBounds.x + dis(gen) * (maxBounds.x - minBounds.x);
        float y = minBounds.y + dis(gen) * (maxBounds.y - minBounds.y);
        NiPoint3 testPoint(x, y, 0.0f);
        
        bool inPaintedArea = false;
        if (scatterSettings.useAreaScatter)
        {
            inPaintedArea = IsPointInUnifiedPaintedArea(testPoint);
        }
        else
        {
            inPaintedArea = IsPointInPaintedArea(testPoint);
        }
        
        if (inPaintedArea)
        {
            result.validPositions++;
            
            if (dis(gen) <= scatterSettings.density)
            {
                result.afterDensity++;
                
                if (!CheckObjectOverlap(testPoint, placedPositions))
                {
                    result.afterOverlap++;
                    placedPositions.push_back(testPoint);
                    placedObjects++;
                }
            }
        }
    }
    
    result.finalCount = placedObjects;
    return result;
}

void ScatterMode::UpdatePrediction()
{
    _lastPrediction = CalculatePredictedObjectCount();
}

void ScatterMode::ScatterObjects()
{
    if (!_selectedObject || !_world || _paintedPoints.empty())
        return;
    
    bool wasShowingBoundingBox = false;
    if (_selectedObject)
    {
        wasShowingBoundingBox = true;
        _selectedObject->HideBoundingBox();
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);
    
    NiPoint3 minBounds = _paintedPoints[0].worldPos;
    NiPoint3 maxBounds = _paintedPoints[0].worldPos;
    
    for (const auto& paintedArea : _paintedPoints)
    {
        minBounds.x = std::min(minBounds.x, paintedArea.worldPos.x - paintedArea.brushSize);
        minBounds.y = std::min(minBounds.y, paintedArea.worldPos.y - paintedArea.brushSize);
        maxBounds.x = std::max(maxBounds.x, paintedArea.worldPos.x + paintedArea.brushSize);
        maxBounds.y = std::max(maxBounds.y, paintedArea.worldPos.y + paintedArea.brushSize);
    }
    
    
    std::vector<NiPickablePtr> newObjects;
    std::vector<NiPoint3> placedPositions;
    int placedObjects = 0;
    
    if (scatterSettings.useAreaScatter)
    {
        int attempts = 0;
        int maxAttempts = scatterSettings.maxObjects * 10;
        
        while (placedObjects < scatterSettings.maxObjects && attempts < maxAttempts)
        {
            attempts++;
            
            float x = minBounds.x + dis(gen) * (maxBounds.x - minBounds.x);
            float y = minBounds.y + dis(gen) * (maxBounds.y - minBounds.y);
            NiPoint3 testPoint(x, y, 0.0f);
            
            if (IsPointInUnifiedPaintedArea(testPoint) && dis(gen) <= scatterSettings.density)
            {
                bool validPosition = false;
                if (scatterSettings.drawOnObjects)
                {
                    NiCameraPtr camera = _world->GetCamera();
                    if (camera)
                    {
                        NiPoint3 rayStart(testPoint.x, testPoint.y, 10000.0f);
                        NiPoint3 rayDir(0.0f, 0.0f, -1.0f);
                        
                        NiPick _Pick;
                        _Pick.SetPickType(NiPick::FIND_FIRST);
                        _Pick.SetSortType(NiPick::SORT);
                        _Pick.SetIntersectType(NiPick::TRIANGLE_INTERSECT);
                        _Pick.SetFrontOnly(true);
                        _Pick.SetReturnNormal(true);
                        _Pick.SetObserveAppCullFlag(true);
                        _Pick.SetTarget(_world->GetWorldNode());
                        
                        if (_Pick.PickObjects(rayStart, rayDir, true))
                        {
                            NiPick::Results& results = _Pick.GetResults();
                            if (results.GetSize() > 0)
                            {
                                testPoint = results.GetAt(0)->GetIntersection();
                                validPosition = true;
                            }
                        }
                    }
                }
                else
                {
                    validPosition = _world->IsValidWorldPosition(testPoint) && _world->UpdateZCoord(testPoint);
                }
                
                if (validPosition && !CheckObjectOverlap(testPoint, placedPositions))
                {
                    placedPositions.push_back(testPoint);
                    
                    NiPoint3 groupCenter = testPoint;
                    NiPoint3 overallCenter = NiPoint3::ZERO;
                    for (auto obj : _selectedObjects) {
                        overallCenter += obj->GetTranslate();
                    }
                    overallCenter /= (float)_selectedObjects.size();
                    
                    for (auto selectedObj : _selectedObjects) {
                        NiPickablePtr newObject = (NiPickable*)selectedObj->Clone();
                        if (newObject)
                        {
                            NiPoint3 objPos = selectedObj->GetTranslate();
                            NiPoint3 objTerrainPos = NiPoint3(objPos.x, objPos.y, 0.0f);
                            _world->UpdateZCoord(objTerrainPos);
                            
                            NiPoint3 offset = NiPoint3(
                                objPos.x - overallCenter.x,
                                objPos.y - overallCenter.y,
                                objPos.z - objTerrainPos.z
                            );
                            NiPoint3 finalPos = groupCenter + offset;
                            newObject->SetTranslate(finalPos);
                        
                        if (_pasteSettings)
                        {
                            if (_pasteSettings->pasteSettings.enableRandomRotation)
                            {
                                auto matrix = newObject->GetRotate();
                                float x, y, z;
                                matrix.ToEulerAnglesXYZ(x, y, z);
                                z = 360.f * (rand() / float(RAND_MAX));
                                matrix.FromEulerAnglesXYZ(x, y, z);
                                newObject->SetRotate(matrix);
                            }
                            
                            if (_pasteSettings->pasteSettings.enableTerrainSnap)
                            {
                                NiPoint3 terrainPos = NiPoint3(finalPos.x, finalPos.y, 0.0f);
                                if (_world->UpdateZCoord(terrainPos))
                                {
                                    NiPoint3 rightPos = NiPoint3(terrainPos.x + 5.0f, terrainPos.y, terrainPos.z);
                                    NiPoint3 forwardPos = NiPoint3(terrainPos.x, terrainPos.y + 5.0f, terrainPos.z);
                                    NiPoint3 leftPos = NiPoint3(terrainPos.x - 5.0f, terrainPos.y, terrainPos.z);
                                    NiPoint3 backPos = NiPoint3(terrainPos.x, terrainPos.y - 5.0f, terrainPos.z);
                                    //Dear god flatten this out later
                                    
                                    if (_world->UpdateZCoord(rightPos) && _world->UpdateZCoord(forwardPos) && 
                                        _world->UpdateZCoord(leftPos) && _world->UpdateZCoord(backPos))
                                    {
                                        NiPoint3 rightVec = rightPos - leftPos;
                                        NiPoint3 forwardVec = forwardPos - backPos;
                                        
                                        NiPoint3 normal = rightVec.Cross(forwardVec);
                                        normal.Unitize();
                                        
                                        if (normal.z < 0.0f) {
                                            normal = -normal;
                                            rightVec = -rightVec;
                                        }
                                        
                                        NiPoint3 up = NiPoint3(0.0f, 0.0f, 1.0f);
                                        NiPoint3 right = normal.Cross(up);
                                        right.Unitize();
                                        NiPoint3 forward = right.Cross(normal);
                                        forward.Unitize();
                                        
                                        NiMatrix3 terrainMatrix;
                                        terrainMatrix.SetCol(0, right);
                                        terrainMatrix.SetCol(1, forward);
                                        terrainMatrix.SetCol(2, normal);
                                        
                                        NiMatrix3 currentRotation = newObject->GetRotate();
                                        
                                        NiPoint3 objectUp = NiPoint3(currentRotation.GetEntry(0, 2), currentRotation.GetEntry(1, 2), currentRotation.GetEntry(2, 2));
                                        
                                        NiPoint3 rotationAxis = objectUp.Cross(normal);
                                        float rotationAngle = acosf(objectUp.Dot(normal));
                                        
                                        if (rotationAxis.SqrLength() > 0.001f) {
                                            rotationAxis.Unitize();
                                            
                                            float cosAngle = cosf(rotationAngle);
                                            float sinAngle = sinf(rotationAngle);
                                            float oneMinusCos = 1.0f - cosAngle;
                                            
                                            NiMatrix3 rotationMatrix;
                                            rotationMatrix.SetEntry(0, 0, cosAngle + rotationAxis.x * rotationAxis.x * oneMinusCos);
                                            rotationMatrix.SetEntry(0, 1, rotationAxis.x * rotationAxis.y * oneMinusCos - rotationAxis.z * sinAngle);
                                            rotationMatrix.SetEntry(0, 2, rotationAxis.x * rotationAxis.z * oneMinusCos + rotationAxis.y * sinAngle);
                                            
                                            rotationMatrix.SetEntry(1, 0, rotationAxis.y * rotationAxis.x * oneMinusCos + rotationAxis.z * sinAngle);
                                            rotationMatrix.SetEntry(1, 1, cosAngle + rotationAxis.y * rotationAxis.y * oneMinusCos);
                                            rotationMatrix.SetEntry(1, 2, rotationAxis.y * rotationAxis.z * oneMinusCos - rotationAxis.x * sinAngle);
                                            
                                            rotationMatrix.SetEntry(2, 0, rotationAxis.z * rotationAxis.x * oneMinusCos - rotationAxis.y * sinAngle);
                                            rotationMatrix.SetEntry(2, 1, rotationAxis.z * rotationAxis.y * oneMinusCos + rotationAxis.x * sinAngle);
                                            rotationMatrix.SetEntry(2, 2, cosAngle + rotationAxis.z * rotationAxis.z * oneMinusCos);
                                            
                                            NiMatrix3 newRotation = rotationMatrix * currentRotation;
                                            newObject->SetRotate(newRotation);
                                        }
                                        
                                        NiPoint3 finalPosWithHeight = NiPoint3(terrainPos.x, terrainPos.y, terrainPos.z + offset.z);
                                        newObject->SetTranslate(finalPosWithHeight);
                                    }
                                }
                            }
                            
                            if (_pasteSettings->pasteSettings.enableRandomScaling)
                            {
                                float range = _pasteSettings->pasteSettings.maxScale - _pasteSettings->pasteSettings.minScale;
                                float randomScale = _pasteSettings->pasteSettings.minScale + (range * (rand() / float(RAND_MAX)));
                                newObject->SetScale(newObject->GetScale() * randomScale);
                            }
                        }
                        
                        newObjects.push_back(newObject);
                        }
                    }
                    placedObjects++;
                }
            }
        }
    }
    else
    {
        int attempts = 0;
        int maxAttempts = scatterSettings.maxObjects * 10;
        
        while (placedObjects < scatterSettings.maxObjects && attempts < maxAttempts)
        {
            attempts++;
            
            float x = minBounds.x + dis(gen) * (maxBounds.x - minBounds.x);
            float y = minBounds.y + dis(gen) * (maxBounds.y - minBounds.y);
            NiPoint3 testPoint(x, y, 0.0f);
            
            if (IsPointInPaintedArea(testPoint))
            {
                if (dis(gen) <= scatterSettings.density)
                {
                    bool validPosition = false;
                    if (scatterSettings.drawOnObjects)
                    {
                        NiCameraPtr camera = _world->GetCamera();
                        if (camera)
                        {
                            NiPoint3 rayStart(testPoint.x, testPoint.y, 10000.0f);
                            NiPoint3 rayDir(0.0f, 0.0f, -1.0f);
                            
                            NiPick _Pick;
                            _Pick.SetPickType(NiPick::FIND_FIRST);
                            _Pick.SetSortType(NiPick::SORT);
                            _Pick.SetIntersectType(NiPick::TRIANGLE_INTERSECT);
                            _Pick.SetFrontOnly(true);
                            _Pick.SetReturnNormal(true);
                            _Pick.SetObserveAppCullFlag(true);
                            _Pick.SetTarget(_world->GetWorldNode());
                            
                            if (_Pick.PickObjects(rayStart, rayDir, true))
                            {
                                NiPick::Results& results = _Pick.GetResults();
                                if (results.GetSize() > 0)
                                {
                                    testPoint = results.GetAt(0)->GetIntersection();
                                    validPosition = true;
                                }
                            }
                        }
                    }
                    else
                    {
                        validPosition = _world->IsValidWorldPosition(testPoint) && _world->UpdateZCoord(testPoint);
                    }
                    
                    if (validPosition && !CheckObjectOverlap(testPoint, placedPositions))
                    {
                        placedPositions.push_back(testPoint);
                        
                        NiPoint3 groupCenter = testPoint;
                        NiPoint3 overallCenter = NiPoint3::ZERO;
                        for (auto obj : _selectedObjects) {
                            overallCenter += obj->GetTranslate();
                        }
                        overallCenter /= (float)_selectedObjects.size();
                        
                        for (auto selectedObj : _selectedObjects) {
                            NiPickablePtr newObject = (NiPickable*)selectedObj->Clone();
                            if (newObject)
                            {
                                NiPoint3 objPos = selectedObj->GetTranslate();
                                NiPoint3 objTerrainPos = NiPoint3(objPos.x, objPos.y, 0.0f);
                                _world->UpdateZCoord(objTerrainPos);
                                
                                NiPoint3 offset = NiPoint3(
                                    objPos.x - overallCenter.x,
                                    objPos.y - overallCenter.y,
                                    objPos.z - objTerrainPos.z
                                );
                                NiPoint3 finalPos = groupCenter + offset;
                                newObject->SetTranslate(finalPos);
                                newObject->HideBoundingBox();
                            
                            if (_pasteSettings)
                            {
                                if (_pasteSettings->pasteSettings.enableRandomRotation)
                                {
                                    auto matrix = newObject->GetRotate();
                                    float x, y, z;
                                    matrix.ToEulerAnglesXYZ(x, y, z);
                                    z = 360.f * (rand() / float(RAND_MAX));
                                    matrix.FromEulerAnglesXYZ(x, y, z);
                                    newObject->SetRotate(matrix);
                                }
                                
                                if (_pasteSettings->pasteSettings.enableTerrainSnap)
                                {
                                    NiPoint3 terrainPos = NiPoint3(finalPos.x, finalPos.y, 0.0f);
                                    if (_world->UpdateZCoord(terrainPos))
                                    {
                                        NiPoint3 rightPos = NiPoint3(terrainPos.x + 5.0f, terrainPos.y, terrainPos.z);
                                        NiPoint3 forwardPos = NiPoint3(terrainPos.x, terrainPos.y + 5.0f, terrainPos.z);
                                        NiPoint3 leftPos = NiPoint3(terrainPos.x - 5.0f, terrainPos.y, terrainPos.z);
                                        NiPoint3 backPos = NiPoint3(terrainPos.x, terrainPos.y - 5.0f, terrainPos.z);
                                        
                                        if (_world->UpdateZCoord(rightPos) && _world->UpdateZCoord(forwardPos) && 
                                            _world->UpdateZCoord(leftPos) && _world->UpdateZCoord(backPos))
                                        {
                                            NiPoint3 rightVec = rightPos - leftPos;
                                            NiPoint3 forwardVec = forwardPos - backPos;
                                            
                                            NiPoint3 normal = rightVec.Cross(forwardVec);
                                            normal.Unitize();
                                            
                                            // Flatten out later
                                            if (normal.z < 0.0f) {
                                                normal = -normal;
                                                rightVec = -rightVec;
                                            }
                                            
                                            NiMatrix3 currentRotation = newObject->GetRotate();
                                            
                                            NiPoint3 objectUp = NiPoint3(currentRotation.GetEntry(0, 2), currentRotation.GetEntry(1, 2), currentRotation.GetEntry(2, 2));
                                            
                                            NiPoint3 rotationAxis = objectUp.Cross(normal);
                                            float rotationAngle = acosf(objectUp.Dot(normal));
                                            
                                            if (rotationAxis.SqrLength() > 0.001f) {
                                                rotationAxis.Unitize();
                                                
                                                float cosAngle = cosf(rotationAngle);
                                                float sinAngle = sinf(rotationAngle);
                                                float oneMinusCos = 1.0f - cosAngle;
                                                
                                                NiMatrix3 rotationMatrix;
                                                rotationMatrix.SetEntry(0, 0, cosAngle + rotationAxis.x * rotationAxis.x * oneMinusCos);
                                                rotationMatrix.SetEntry(0, 1, rotationAxis.x * rotationAxis.y * oneMinusCos - rotationAxis.z * sinAngle);
                                                rotationMatrix.SetEntry(0, 2, rotationAxis.x * rotationAxis.z * oneMinusCos + rotationAxis.y * sinAngle);
                                                
                                                rotationMatrix.SetEntry(1, 0, rotationAxis.y * rotationAxis.x * oneMinusCos + rotationAxis.z * sinAngle);
                                                rotationMatrix.SetEntry(1, 1, cosAngle + rotationAxis.y * rotationAxis.y * oneMinusCos);
                                                rotationMatrix.SetEntry(1, 2, rotationAxis.y * rotationAxis.z * oneMinusCos - rotationAxis.x * sinAngle);
                                                
                                                rotationMatrix.SetEntry(2, 0, rotationAxis.z * rotationAxis.x * oneMinusCos - rotationAxis.y * sinAngle);
                                                rotationMatrix.SetEntry(2, 1, rotationAxis.z * rotationAxis.y * oneMinusCos + rotationAxis.x * sinAngle);
                                                rotationMatrix.SetEntry(2, 2, cosAngle + rotationAxis.z * rotationAxis.z * oneMinusCos);
                                                
                                                NiMatrix3 newRotation = rotationMatrix * currentRotation;
                                                newObject->SetRotate(newRotation);
                                            }
                                            
                                            NiPoint3 finalPosWithHeight = NiPoint3(terrainPos.x, terrainPos.y, terrainPos.z + offset.z);
                                            newObject->SetTranslate(finalPosWithHeight);
                                        }
                                    }
                                }
                                
                                if (_pasteSettings->pasteSettings.enableRandomScaling)
                                {
                                    float range = _pasteSettings->pasteSettings.maxScale - _pasteSettings->pasteSettings.minScale;
                                    float randomScale = _pasteSettings->pasteSettings.minScale + (range * (rand() / float(RAND_MAX)));
                                    newObject->SetScale(newObject->GetScale() * randomScale);
                                }
                            }
                            
                            newObjects.push_back(newObject);
                            }
                        }
                        placedObjects++;
                    }
                }
            }
        }
    }
    
    if (!newObjects.empty())
    {
        _world->AddObject(newObjects);
        
        if (_pasteSettings && _pasteSettings->pasteSettings.autoGenerateSHBD)
        {
            std::vector<NiSHMDPickablePtr> shmdObjects;
            for (auto obj : newObjects)
            {
                if (obj && NiIsKindOf(NiSHMDPickable, obj))
                {
                    NiSHMDPickablePtr shmdObj = NiSmartPointerCast(NiSHMDPickable, obj);
                    if (shmdObj)
                    {
                        shmdObjects.push_back(shmdObj);
                    }
                }
            }
            
            if (!shmdObjects.empty())
            {
                EditorScenePtr editorScene = nullptr;
                SHBDModePtr tempSHBDMode = NiNew SHBDMode(_world, editorScene);
                tempSHBDMode->GenerateSHBDForObjects(shmdObjects);
            }
        }
    }
}