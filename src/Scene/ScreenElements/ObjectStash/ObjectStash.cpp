#include "ObjectStash.h"
#include <algorithm>
#include <cctype>

ObjectStash::ObjectStash()
    : _isVisible(false)
    , _selectedIndex(-1)
    , _world(nullptr)
{
    memset(_stashSearchBuffer, 0, sizeof(_stashSearchBuffer));
    memset(_mapSearchBuffer, 0, sizeof(_mapSearchBuffer));
}

ObjectStash::~ObjectStash()
{
    ClearStash();
}

bool ObjectStash::Draw()
{
    if (!_isVisible)
        return true;
        
    DrawStashUI();
    return true;
}

void ObjectStash::DrawStashUI()
{
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin("Object Stash & Map Browser", &_isVisible, ImGuiWindowFlags_None);
    
    if (ImGui::BeginTabBar("StashTabs"))
    {
        if (ImGui::BeginTabItem("My Stash"))
        {
            ImGui::Text("Stashed Objects: %d", (int)_stashedObjects.size());
            ImGui::Separator();
            
            if (ImGui::CollapsingHeader("Stashed Objects", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::InputText("Search Stash", _stashSearchBuffer, sizeof(_stashSearchBuffer)))
                {
                    UpdateStashFilter();
                }
                
                if (_filteredStashIndices.empty() && _stashedObjects.empty())
                {
                    ImGui::Text("No objects stashed");
                }
                else if (_filteredStashIndices.empty() && !_stashedObjects.empty())
                {
                    ImGui::Text("No objects match search");
                }
                else
                {
                    ImGui::BeginChild("StashList", ImVec2(0, 300), true);
                    for (int index : _filteredStashIndices)
                    {
                        DrawObjectEntry(index);
                    }
                    ImGui::EndChild();
                }
            }
            
            if (ImGui::CollapsingHeader("Stash Management"))
            {
                if (ImGui::Button("Clear All") && !_stashedObjects.empty())
                {
                    if (ImGui::GetIO().KeyCtrl)
                    {
                        ClearStash();
                        UpdateStashFilter();
                    }
                    else
                    {
                        ImGui::OpenPopup("Confirm Clear");
                    }
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Hold Ctrl to skip confirmation");
                }
                
                if (ImGui::BeginPopupModal("Confirm Clear", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Are you sure you want to clear all stashed objects?");
                    ImGui::Separator();
                    
                    if (ImGui::Button("Yes"))
                    {
                        ClearStash();
                        UpdateStashFilter();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("Map Objects"))
        {
            DrawMapObjectsSection();
            ImGui::EndTabItem();
        }
        
        ImGui::EndTabBar();
    }
    
    ImGui::End();
}

void ObjectStash::DrawObjectEntry(int index)
{
    const StashedObject& stashed = _stashedObjects[index];
    
    ImGui::PushID(index);
    
    ImGui::Text("%s", stashed.Name.c_str());
    
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("File: %s", stashed.FilePath.c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", stashed.Position.x, stashed.Position.y, stashed.Position.z);
        ImGui::Text("Scale: %.2f", stashed.Scale);
        ImGui::EndTooltip();
    }
    
    ImGui::SameLine();
    if (ImGui::SmallButton("Retrieve"))
    {
        if (!_world)
        {
            return;
        }
        
        NiPickablePtr retrievedObject = RetrieveObject(index);
        if (!retrievedObject)
        {
            return;
        }
        
        try 
        {
            NiCameraPtr camera = _world->GetCamera();
            if (camera)
            {
                NiPoint3 cameraPos = camera->GetTranslate();
                NiPoint3 cameraDir = camera->GetWorldDirection();
                NiPoint3 placePos = cameraPos + (cameraDir * 50.0f);
                retrievedObject->SetTranslate(placePos);
            }
            else
            {
                NiPoint3 worldPoint = _world->GetWorldPoint();
                retrievedObject->SetTranslate(worldPoint);
            }
            _world->AddObject({ retrievedObject });
        }
        catch (...)
        {
        }
    }
    
    ImGui::SameLine();
    if (ImGui::SmallButton("Delete"))
    {
        RemoveStashedObject(index);
    }
    
    ImGui::PopID();
}

void ObjectStash::StashObject(NiPickablePtr object, const std::string& customName)
{
    if (!object)
        return;
        
    std::string name = customName.empty() ? GenerateObjectName(object) : customName;
    
    NiPickablePtr clonedObject = (NiPickable*)object->Clone();
    if (!clonedObject)
        return;
        
    _stashedObjects.emplace_back(clonedObject, name);
    UpdateStashFilter();
}

void ObjectStash::StashSelectedObjects(const std::vector<NiPickablePtr>& selectedObjects)
{
    if (selectedObjects.empty())
        return;
        
    for (size_t i = 0; i < selectedObjects.size(); ++i)
    {
        std::string name = "";
        if (selectedObjects.size() > 1)
        {
            name = "Object_" + std::to_string(i + 1);
        }
        
        StashObject(selectedObjects[i], name);
    }
}

void ObjectStash::StashAndRemoveObjects(const std::vector<NiPickablePtr>& selectedObjects)
{
    if (selectedObjects.empty() || !_world)
        return;
        
    StashSelectedObjects(selectedObjects);
    _world->RemoveObject(selectedObjects);
}

NiPickablePtr ObjectStash::RetrieveObject(int index)
{
    if (index < 0 || index >= (int)_stashedObjects.size())
        return nullptr;
        
    const StashedObject& stashed = _stashedObjects[index];
    
    if (!stashed.Object)
        return nullptr;
    
    NiPickablePtr newObject = (NiPickable*)stashed.Object->Clone();
    if (!newObject)
        return nullptr;
    
    newObject->SetTranslate(stashed.Position);
    newObject->SetRotate(stashed.Rotation);
    newObject->SetScale(stashed.Scale);
    newObject->SetFilePathOrName(stashed.FilePath);
    
    return newObject;
}

void ObjectStash::RemoveStashedObject(int index)
{
    if (index < 0 || index >= (int)_stashedObjects.size())
        return;
        
    _stashedObjects.erase(_stashedObjects.begin() + index);
    UpdateStashFilter();
}

void ObjectStash::ClearStash()
{
    _stashedObjects.clear();
    _selectedIndex = -1;
}

std::string ObjectStash::GenerateObjectName(NiPickablePtr object)
{
    std::string filePath = object->GetSHMDPath();
    
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos)
    {
        filePath = filePath.substr(lastSlash + 1);
    }
    
    size_t lastDot = filePath.find_last_of('.');
    if (lastDot != std::string::npos)
    {
        filePath = filePath.substr(0, lastDot);
    }
    
    return filePath.empty() ? "Unknown_Object" : filePath;
}

void ObjectStash::DrawMapObjectsSection()
{
    if (ImGui::Button("Refresh Map Objects"))
    {
        RefreshMapObjects();
    }
    
    ImGui::Text("Map Objects: %d", (int)_mapObjects.size());
    ImGui::Separator();
    
    if (ImGui::InputText("Search Map Objects", _mapSearchBuffer, sizeof(_mapSearchBuffer)))
    {
        UpdateMapFilter();
    }
    
    if (_filteredMapIndices.empty() && _mapObjects.empty())
    {
        ImGui::Text("No objects in map (click Refresh)");
    }
    else if (_filteredMapIndices.empty() && !_mapObjects.empty())
    {
        ImGui::Text("No objects match search");
    }
    else
    {
        ImGui::BeginChild("MapObjectsList", ImVec2(0, 400), true);
        for (int index : _filteredMapIndices)
        {
            DrawMapObjectEntry(index);
        }
        ImGui::EndChild();
    }
}

void ObjectStash::DrawMapObjectEntry(int index)
{
    if (index < 0 || index >= (int)_mapObjects.size())
        return;
        
    NiPickablePtr object = _mapObjects[index];
    if (!object)
        return;
        
    ImGui::PushID(index);
    
    std::string objectName = GenerateObjectName(object);
    NiPoint3 pos = object->GetTranslate();
    
    ImGui::Text("%s", objectName.c_str());
    
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::Text("File: %s", object->GetSHMDPath().c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
        ImGui::Text("Scale: %.2f", object->GetScale());
        ImGui::EndTooltip();
    }
    
    ImGui::SameLine();
    if (ImGui::SmallButton("Teleport"))
    {
        TeleportToObject(object);
    }
    
    ImGui::SameLine();
    if (ImGui::SmallButton("Stash This"))
    {
        std::string name = GenerateObjectName(object);
        StashObject(object, name);
        if (_world)
        {
            _world->RemoveObject({ object });
            RefreshMapObjects(); // Refresh to remove from list
        }
    }
    
    ImGui::PopID();
}

void ObjectStash::UpdateStashFilter()
{
    _filteredStashIndices.clear();
    
    std::string searchText = _stashSearchBuffer;
    std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
    
    if (searchText.empty())
    {
        for (int i = 0; i < (int)_stashedObjects.size(); ++i)
        {
            _filteredStashIndices.push_back(i);
        }
    }
    else
    {
        for (int i = 0; i < (int)_stashedObjects.size(); ++i)
        {
            const StashedObject& obj = _stashedObjects[i];
            if (MatchesSearch(obj.Name, searchText) || MatchesSearch(obj.FilePath, searchText))
            {
                _filteredStashIndices.push_back(i);
            }
        }
    }
}

void ObjectStash::UpdateMapFilter()
{
    _filteredMapIndices.clear();
    
    std::string searchText = _mapSearchBuffer;
    std::transform(searchText.begin(), searchText.end(), searchText.begin(), ::tolower);
    
    if (searchText.empty())
    {
        for (int i = 0; i < (int)_mapObjects.size(); ++i)
        {
            _filteredMapIndices.push_back(i);
        }
    }
    else
    {
        for (int i = 0; i < (int)_mapObjects.size(); ++i)
        {
            NiPickablePtr obj = _mapObjects[i];
            if (obj)
            {
                std::string objName = GenerateObjectName(obj);
                std::string objPath = obj->GetSHMDPath();
                if (MatchesSearch(objName, searchText) || MatchesSearch(objPath, searchText))
                {
                    _filteredMapIndices.push_back(i);
                }
            }
        }
    }
}

void ObjectStash::RefreshMapObjects()
{
    _mapObjects.clear();
    
    if (!_world)
        return;
        
    auto& groundObjects = _world->GetGroundObjects();
    
    for (NiNodePtr node : groundObjects)
    {
        if (NiIsKindOf(NiPickable, node))
        {
            NiPickablePtr pickable = NiSmartPointerCast(NiPickable, node);
            if (pickable)
            {
                _mapObjects.push_back(pickable);
            }
        }
    }
    
    UpdateMapFilter();
}

void ObjectStash::TeleportToObject(NiPickablePtr object)
{
    if (!object || !_world)
        return;
        
    NiPoint3 objectPos = object->GetTranslate();
    NiCameraPtr camera = _world->GetCamera();
    
    if (camera)
    {
        NiPoint3 cameraPos = objectPos;
        cameraPos.z += 50.0f;
        cameraPos.x -= 30.0f;
        cameraPos.y -= 30.0f;
        
        camera->SetTranslate(cameraPos);
        camera->LookAtWorldPoint(objectPos, NiPoint3(0.0f, 0.0f, 1.0f));
        camera->Update(0.0f);
    }
}

bool ObjectStash::MatchesSearch(const std::string& text, const std::string& search)
{
    if (search.empty())
        return true;
        
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    return lowerText.find(search) != std::string::npos;
}