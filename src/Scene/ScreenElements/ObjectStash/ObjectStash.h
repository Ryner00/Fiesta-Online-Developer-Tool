#pragma once
#include <Scene/ScreenElements/ScreenElement.h>
#include <ImGui/imgui.h>
#include <Data/NiCustom/NiPickable.h>
#include <Data/IngameWorld/IngameWorld.h>
#include <vector>
#include <string>
#include <functional>

NiSmartPointer(ObjectStash);

class ObjectStash : public ScreenElement
{
    NiDeclareRTTI;

public:
    struct StashedObject
    {
        NiPickablePtr Object;
        std::string Name;
        std::string FilePath;
        NiPoint3 Position;
        NiMatrix3 Rotation;
        float Scale;
        
        StashedObject(NiPickablePtr obj, const std::string& name)
        {
            Object = obj;
            Name = name;
            FilePath = obj->GetSHMDPath();
            Position = obj->GetTranslate();
            Rotation = obj->GetRotate();
            Scale = obj->GetScale();
        }
    };

    ObjectStash();
    virtual ~ObjectStash();
    
    virtual bool Draw() override;
    
    void StashObject(NiPickablePtr object, const std::string& customName = "");
    void StashSelectedObjects(const std::vector<NiPickablePtr>& selectedObjects);
    void StashAndRemoveObjects(const std::vector<NiPickablePtr>& selectedObjects);
    NiPickablePtr RetrieveObject(int index);
    void RemoveStashedObject(int index);
    void ClearStash();
    
    typedef std::function<std::vector<NiPickablePtr>()> GetSelectedNodesFunc;
    typedef std::function<void()> ClearSelectedFunc;
    
    void SetSelectionCallbacks(GetSelectedNodesFunc getSelected, ClearSelectedFunc clearSelected) {
        _getSelectedNodes = getSelected;
        _clearSelectedObjects = clearSelected;
    }
    
    size_t GetStashCount() const { return _stashedObjects.size(); }
    const StashedObject& GetStashedObject(int index) const { return _stashedObjects[index]; }
    
    void SetVisible(bool visible) { _isVisible = visible; }
    bool IsVisible() const { return _isVisible; }
    
    void SetIngameWorld(IngameWorldPtr world) { _world = world; }

private:
    std::vector<StashedObject> _stashedObjects;
    bool _isVisible;
    int _selectedIndex;
    IngameWorldPtr _world;
    GetSelectedNodesFunc _getSelectedNodes;
    ClearSelectedFunc _clearSelectedObjects;
    
    char _stashSearchBuffer[256];
    char _mapSearchBuffer[256];
    std::vector<int> _filteredStashIndices;
    std::vector<NiPickablePtr> _mapObjects;
    std::vector<int> _filteredMapIndices;
    
    void DrawStashUI();
    void DrawObjectEntry(int index);
    void DrawMapObjectsSection();
    void DrawMapObjectEntry(int index);
    std::string GenerateObjectName(NiPickablePtr object);
    
    void UpdateStashFilter();
    void UpdateMapFilter();
    void RefreshMapObjects();
    void TeleportToObject(NiPickablePtr object);
    bool MatchesSearch(const std::string& text, const std::string& search);
};