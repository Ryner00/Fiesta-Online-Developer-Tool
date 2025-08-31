#pragma once
#include "TerrainBrushMode.h"
#include "EditorScene/EditorScene.h"
#include <Scene/ScreenElements/LuaElement/LuaElement.h>

NiSmartPointer(HTDGMode);
class HTDGMode : public TerrainBrushMode
{
	NiDeclareRTTI;
	HTDGMode(IngameWorldPtr World, EditorScenePtr Scene) : TerrainBrushMode(World, (FiestaScenePtr)&*Scene)
	{
		kWorld->ShowTerrain(true);
		ScreenElements.push_back(NiNew LuaElement(Scene, "EditorELements/HTDG.lua"));
		LoadBrushes("HTDGBrushes");
	} 
	~HTDGMode()
	{
		kWorld->ShowTerrain(true);
	} 

	virtual void ProcessInput();
	void SetBrushMode(bool enabled) { _brushMode = enabled; }
	bool GetBrushMode() const { return _brushMode; }
	void ConfirmBrushGeneration();
	void ClearPaintedAreas() { _paintedPoints.clear(); }
	bool IsPointInPaintedArea(const NiPoint3& point) const;
	
	virtual std::string GetEditModeName() { return "HTDG"; }

private:
	HeightTerrainData _Data;
	bool _brushMode = false;
	bool _isDrawing = false;
	struct PaintedArea {
		NiPoint3 worldPos;
		float brushSize;
	};
	std::vector<PaintedArea> _paintedPoints;
	
};