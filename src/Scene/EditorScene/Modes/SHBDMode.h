#pragma once
#include "TerrainMode.h"
#include <Scene/ScreenElements/LuaElement/LuaElement.h>
#include "ImGui/ImGuizmo.h"
#include "Data/NiCustom/NiSHMDPickable.h"

NiSmartPointer(SHBDMode);
class SHBDMode : public TerrainMode
{
	NiDeclareRTTI;
	SHBDMode(IngameWorldPtr World, EditorScenePtr Scene) : TerrainMode(World, (FiestaScenePtr)&* Scene)
	{
		ScreenElements.push_back(NiNew LuaElement(Scene, "EditorELements/SHBD.lua"));

		_BaseNode->DetachChild(MouseOrb);//We need to remove it from the Terrain ebcause this mode does use it on its own

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

		
		_SHBDNode = NiNew NiNode;
		_BaseNode->AttachChild(_SHBDNode);
		_BaseNode->AttachChild(MouseOrb);
		SetBrushSize(1);
		
		_terrainGenerationActive = false;
		_terrainGenerationTimer = 0.0f;
		_currentTerrainX = 0;
		_currentTerrainY = 0;
		_terrainCellsPerFrame = 2000;
		_totalTerrainCells = 0;
		_processedTerrainCells = 0;
		_captureHeight = 0.0f;
		_captureTolerance = 15.0f;
		_keepExistingSHBD = false;
		_generationAccuracy = 3;
	}
	~SHBDMode()
	{
		_SHBDNode = NULL;
	}
	virtual void Draw();
	virtual void Update(float fTime);
	virtual void ProcessInput();
	virtual std::string GetEditModeName() { return "SHBD"; }
	
	void SetWalkable(bool Walkable) { _Walkable = Walkable; }
	bool GetWalkable() { return _Walkable; }
	void SetBrushSize(int Size) { _BrushSize = Size; MouseOrb->SetScale((6.25f / 160.f) * _BrushSize); }
	void AutoGenerateSHBD();
	void AutoGenerateTerrainSHBD();
	void GenerateSHBDForObjects(const std::vector<NiSHMDPickablePtr>& objects);
	void ResetSHBD();
	
	void SetCaptureHeight(float height) { _captureHeight = height; }
	float GetCaptureHeight() { return _captureHeight; }
	void SetCaptureTolerance(float tolerance) { _captureTolerance = tolerance; }
	float GetCaptureTolerance() { return _captureTolerance; }
	float GetCurrentSHBDHeight() { return _SHBDNode->GetTranslate().z; }
	void SetKeepExistingSHBD(bool keep) { _keepExistingSHBD = keep; }
	bool GetKeepExistingSHBD() { return _keepExistingSHBD; }
	void SetGenerationAccuracy(int accuracy) { _generationAccuracy = accuracy; }
	int GetGenerationAccuracy() { return _generationAccuracy; }
private:
	void UpdateSHBD();

	NiNodePtr _SHBDNode;

	void CreateSHBDNode();
	void UpdateMouseIntersect();
	void UpdateSHBDArea(int centerX, int centerY, unsigned int SHBDSize, bool value);

	std::vector<std::vector<NiPixelDataPtr>> TextureConnector;
	int TextureSize = 128;

	std::vector<char> _Data;
	unsigned int Walkable, Blocked;
	
	bool _terrainGenerationActive;
	float _terrainGenerationTimer;
	int _currentTerrainX;
	int _currentTerrainY;
	int _terrainCellsPerFrame;
	int _totalTerrainCells;
	int _processedTerrainCells;
	
	float _captureHeight;
	float _captureTolerance;
	bool _keepExistingSHBD;
	int _generationAccuracy;

};