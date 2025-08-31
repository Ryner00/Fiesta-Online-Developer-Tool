#pragma once
#include "TerrainMode.h"
#include <Scene/ScreenElements/LuaElement/LuaElement.h>
#include <Data/ShineBuildingInfo/ShineBuildingInfo.h>
#include "ImGui/ImGuizmo.h"

NiSmartPointer(SBIMode);
class SBIMode : public TerrainMode
{
    NiDeclareRTTI;
    SBIMode(IngameWorldPtr World, EditorScenePtr Scene) : TerrainMode(World, (FiestaScenePtr)&* Scene)
    {
        ScreenElements.push_back(NiNew LuaElement(Scene, "EditorELements/SBI.lua"));

        _BaseNode->DetachChild(MouseOrb);

        _BaseNode = NiNew NiNode;

        Light = NiNew NiAmbientLight();
        Light->SetAmbientColor(NiColor(0.792157f, 0.792157f, 0.792157f));
        Light->SetDiffuseColor(NiColor::WHITE);
        Light->SetSpecularColor(NiColor::BLACK);
        Light->AttachAffectedNode(_BaseNode);
        _BaseNode->AttachProperty(NiNew NiVertexColorProperty);
        _BaseNode->AttachChild(Light);

        _BaseNode->UpdateEffects();
        _BaseNode->UpdateProperties();
        _BaseNode->Update(0.f);

        _SBINode = NiNew NiNode;
        _BaseNode->AttachChild(_SBINode);
        
        
        _BaseNode->AttachChild(MouseOrb);
        SetBrushSize(5);
        
        _selectedDoor = -1;
        _paintMode = true;
        _teleportDoorIndex = 0;
        _dragMode = false;
        _isDragging = false;
    }
    ~SBIMode()
    {
        _SBINode = NULL;
    }
    virtual void Draw();
    virtual void Update(float fTime);
    virtual void ProcessInput();
    virtual std::string GetEditModeName() { return "SBI"; }
    
    void SetPaintMode(bool paintMode) { _paintMode = paintMode; }
    bool GetPaintMode() { return _paintMode; }
    void SetBrushSize(int Size) { _BrushSize = Size; MouseOrb->SetScale((6.25f / 160.f) * _BrushSize); }
    void SetSelectedDoor(int index) { _selectedDoor = index; }
    int GetSelectedDoor() { return _selectedDoor; }
    void SetTeleportDoorIndex(int index) { _teleportDoorIndex = index; }
    int GetTeleportDoorIndex() { return _teleportDoorIndex; }
    void CreateDoor(const std::string& name, float startX, float startY, float endX, float endY);
    void DeleteDoor(int index);
    size_t GetDoorCount();
    SBIDoorBlock* GetDoor(int index);
    void TeleportToDoor(int index);
    void CreateDoorNearCamera(const std::string& name);
    void ResizeDoor(int index, float startX, float startY, float endX, float endY);
    SBIDoorBlock* GetDoorData(int index);
    void SetDragMode(bool enabled) { _dragMode = enabled; }
    bool GetDragMode() { return _dragMode; }
    IngameWorldPtr GetWorld() { return kWorld; }
    NiCameraPtr GetCamera() { return Camera; }
    
private:
    void UpdateSBI();
    void CreateSBINode();
    void UpdateMouseIntersect();
    NiNodePtr CreateDoorMesh(const SBIDoorBlock& door, int doorIndex);
    int FindDoorAtMousePosition();
    void UpdateDoorDrag();
    
    NiNodePtr _SBINode;
    int _selectedDoor;
    bool _paintMode;
    int _teleportDoorIndex;
    bool _dragMode;
    bool _isDragging;
    NiPoint3 _dragStartPos;
    NiPoint2 _doorStartPos;
    NiPoint2 _doorEndPos;
    
    
    std::vector<std::vector<NiPixelDataPtr>> TextureConnector;
    int TextureSize = 128;
};