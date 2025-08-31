#pragma once
#include "EditMode.h"
#include <Scene/ScreenElements/LuaElement/LuaElement.h>
#include "ImGui/ImGuizmo.h"
#include "Data/NiCustom/NiSHMDPickable.h"
#include "MinimapRenderer.h"
#include <Scene/ScreenElements/ObjectStash/ObjectStash.h>
#include <Scene/ScreenElements/PasteSettings/PasteSettings.h>
#include <Scene/ScreenElements/ScatterMode/ScatterMode.h>
#include "SHBDMode.h"

NiSmartPointer(SHMDMode);
class SHMDMode : public EditMode
{
	NiDeclareRTTI;
	SHMDMode(IngameWorldPtr World, EditorScenePtr Scene) : EditMode(World, (FiestaScenePtr)&*Scene)
	{
		ScreenElements.push_back(NiNew LuaElement(Scene, "EditorELements/SHMD.lua"));
		ScreenElements.push_back(NiNew LuaElement(Scene, "EditorELements/SHMDObject.lua"));
		_objectStash = NiNew ObjectStash();
		_objectStash->SetIngameWorld(World);
		_objectStash->SetSelectionCallbacks(
			[this]() { return this->GetSelectedNodes(); },
			[this]() { this->ClearSelectedObjects(); }
		);
		ScreenElements.push_back((ScreenElementPtr)_objectStash);
		
		_pasteSettings = NiNew PasteSettings();
		_pasteSettings->SetWorld(World);
		ScreenElements.push_back((ScreenElementPtr)_pasteSettings);
		
		_scatterMode = NiNew ScatterMode();
		_scatterMode->SetWorld(World);
		_scatterMode->SetPasteSettings(_pasteSettings);
		_scatterMode->SetClearSelectionCallback([this]() { this->ClearSelectedObjects(); });
		ScreenElements.push_back((ScreenElementPtr)_scatterMode);
	}
	~SHMDMode() 
	{
		for (auto ptr : SelectedObjects) {
			ptr->HideBoundingBox();
		}
		SelectedObjects.clear();
	}
	virtual void Draw();
	virtual void Update(float fTime);
	virtual void ProcessInput();
	virtual std::string GetEditModeName() { return "SHMD"; }
	virtual void ToggleMinimap() { _minimap.ToggleVisible(); }
	void SelectObject(NiPickablePtr Obj, bool append = false) 
	{
		if (!Obj || !NiIsKindOf(NiSHMDPickable, Obj))
			return;
		auto it = std::find(SelectedObjects.begin(), SelectedObjects.end(), Obj);
		if (it != SelectedObjects.end())
		{
			return;
		}
		if (!append && !SelectedObjects.empty())
		{
			for (auto ptr : SelectedObjects) {
				ptr->HideBoundingBox();
			}
			SelectedObjects.clear();
		}
		Obj->ShowBoundingBox();
		SelectedObjects.push_back(Obj);
	}
	void ClearSelectedObjects() 
	{
		if (!SelectedObjects.empty())
		{
			for (auto ptr : SelectedObjects) {
				ptr->HideBoundingBox();
			}
			SelectedObjects.clear();
		}
	}
	bool HasSelectedObject() { return !SelectedObjects.empty(); }
	int GetSelectedObjectCount() const { return static_cast<int>(SelectedObjects.size()); }
	ImGuizmo::OPERATION GetOperationMode() { return OperationMode; }
	void SetOperationMode(ImGuizmo::OPERATION Mode) { OperationMode = Mode; }
	std::vector<NiPickablePtr> GetSelectedNodes() { return SelectedObjects; }
	NiPoint3 GetSnapSize() { return SnapSize; }
	void SetSnapSize(float Size){ SnapSize = NiPoint3(Size, Size, Size); }
	void SetSnapeMovement(bool Snap) { SnapMovement = Snap; }
	bool GetSnapMovement() { return SnapMovement; }
	void CreateAddElement(std::string type);
	void UpdateScale(float Scale);
	void UpdateMultipleObjectsScale(float scaleMultiplier);
	void SetAverageScale(float targetAverageScale);
	float GetAverageScale() const;
	void SaveSelectedPickableNif();
	void UpdatePoint(IngameWorld::WorldIntersectType newPoint) { IntersectType = newPoint; }
	IngameWorld::WorldIntersectType GetWorldIntersectType() { return IntersectType; }
	void ToggleStash() { _objectStash->SetVisible(!_objectStash->IsVisible()); }
	void StashSelectedObjects() { 
		_objectStash->StashAndRemoveObjects(SelectedObjects); 
		ClearSelectedObjects();
	}
	ObjectStashPtr GetObjectStash() { return _objectStash; }
	void TogglePasteSettings() { _pasteSettings->Toggle(); }
	PasteSettingsPtr GetPasteSettings() { return _pasteSettings; }
	void ToggleScatterMode() { _scatterMode->Toggle(); }
	ScatterModePtr GetScatterMode() { return _scatterMode; }
private:
	ImGuizmo::OPERATION OperationMode = ImGuizmo::OPERATION::TRANSLATE;
	void DrawGizmo();
	void ApplyRandomRotation(NiPickablePtr object, const PasteSettings::Settings& settings);
	void ApplyTerrainSnap(NiPickablePtr object, const NiPoint3& pos, const NiPoint3& offset, const PasteSettings::Settings& settings);
	void ApplyRandomScaling(NiPickablePtr object, const PasteSettings::Settings& settings);
	bool SnapMovement = false;
	NiPoint3 SnapSize = NiPoint3(2.5f, 2.5f, 2.5f);
	std::vector<NiPickablePtr> SelectedObjects;
	struct CopyGroup {
		NiPickablePtr object;
		NiPoint3 offset;
	};
	std::vector<CopyGroup> CopyObjects;
	NiPoint3 CopyGroupCenter;
	IngameWorld::WorldIntersectType IntersectType = IngameWorld::WorldIntersectType::GroundScene;
	MinimapRenderer _minimap;
	ObjectStashPtr _objectStash;
	PasteSettingsPtr _pasteSettings;
	ScatterModePtr _scatterMode;
};