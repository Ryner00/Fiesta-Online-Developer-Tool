#include "SHMDMode.h"
#include <Scene/ScreenElements/LuaElement/LuaElement.h>
#include <Scene/ScreenElements/LoadObject/AddSingleObject.h>
#include <Scene/ScreenElements/LoadObject/AddMultipleObject.h>
#include <Scene/ScreenElements/LoadObject/ReplaceObjects.h>
#include <Scene/ScreenElements/PasteSettings/PasteSettings.h>
#include <Scene/ScreenElements/ScatterMode/ScatterMode.h>
#include <cmath>

NiImplementRTTI(SHMDMode, EditMode);

void SHMDMode::Draw()
{
	EditMode::Draw();
	DrawGizmo(); 
	_minimap.DrawMinimap(kWorld, kWorld->GetCamera(), GetEditModeName());
}

void SHMDMode::Update(float fTime)
{
    if (_scatterMode)
        _scatterMode->Update(fTime);
}

void SHMDMode::UpdateScale(float Scale) 
{
	kWorld->UpdateScale(GetSelectedNodes(), Scale);
}

void SHMDMode::UpdateMultipleObjectsScale(float scaleMultiplier)
{
	if (SelectedObjects.empty())
		return;
		
	std::vector<float> originalScales;
	for (auto obj : SelectedObjects)
	{
		originalScales.push_back(obj->GetScale());
	}
	
	std::vector<float> newScales;
	for (float originalScale : originalScales)
	{
		newScales.push_back(originalScale * scaleMultiplier);
	}
	
	for (size_t i = 0; i < SelectedObjects.size(); ++i)
	{
		SelectedObjects[i]->SetScale(newScales[i]);
	}
}

void SHMDMode::SetAverageScale(float targetAverageScale)
{
	if (SelectedObjects.empty())
		return;
		
	float currentAverageScale = GetAverageScale();
	if (currentAverageScale == 0.0f)
		return;
		
	float scaleMultiplier = targetAverageScale / currentAverageScale;
	UpdateMultipleObjectsScale(scaleMultiplier);
}

float SHMDMode::GetAverageScale() const
{
	if (SelectedObjects.empty())
		return 1.0f;
		
	float totalScale = 0.0f;
	for (auto obj : SelectedObjects)
	{
		totalScale += obj->GetScale();
	}
	
	return totalScale / SelectedObjects.size();
}

void SHMDMode::ApplyRandomRotation(NiPickablePtr object, const PasteSettings::Settings& settings)
{
	auto matrix = object->GetRotate();
	float x, y, z;
	matrix.ToEulerAnglesXYZ(x, y, z);
	z = 360.f * (rand() / float(RAND_MAX));
	matrix.FromEulerAnglesXYZ(x, y, z);
	object->SetRotate(matrix);
}

void SHMDMode::ApplyTerrainSnap(NiPickablePtr object, const NiPoint3& pos, const NiPoint3& offset, const PasteSettings::Settings& settings)
{
	NiPoint3 terrainPos = NiPoint3(pos.x, pos.y, 0.0f);
	if (!kWorld->UpdateZCoord(terrainPos))
		return;
	
	NiPoint3 rightPos = NiPoint3(terrainPos.x + 5.0f, terrainPos.y, terrainPos.z);
	NiPoint3 forwardPos = NiPoint3(terrainPos.x, terrainPos.y + 5.0f, terrainPos.z);
	NiPoint3 leftPos = NiPoint3(terrainPos.x - 5.0f, terrainPos.y, terrainPos.z);
	NiPoint3 backPos = NiPoint3(terrainPos.x, terrainPos.y - 5.0f, terrainPos.z);
	
	if (!kWorld->UpdateZCoord(rightPos) || !kWorld->UpdateZCoord(forwardPos) || 
		!kWorld->UpdateZCoord(leftPos) || !kWorld->UpdateZCoord(backPos))
		return;
	
	NiPoint3 rightVec = rightPos - leftPos;
	NiPoint3 forwardVec = forwardPos - backPos;
	
	NiPoint3 normal = rightVec.Cross(forwardVec);
	normal.Unitize();
	
	if (normal.z < 0.0f) {
		normal = -normal;
		rightVec = -rightVec;
	}
	
	NiMatrix3 currentRotation = object->GetRotate();
	NiPoint3 objectUp = NiPoint3(currentRotation.GetEntry(0, 2), currentRotation.GetEntry(1, 2), currentRotation.GetEntry(2, 2));
	
	NiPoint3 rotationAxis = objectUp.Cross(normal);
	float rotationAngle = acosf(objectUp.Dot(normal));
	
	if (rotationAxis.SqrLength() <= 0.001f)
		return;
	
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
	object->SetRotate(newRotation);
	
	NiPoint3 finalPosWithHeight = NiPoint3(terrainPos.x, terrainPos.y, terrainPos.z + offset.z);
	object->SetTranslate(finalPosWithHeight);
}

void SHMDMode::ApplyRandomScaling(NiPickablePtr object, const PasteSettings::Settings& settings)
{
	float range = settings.maxScale - settings.minScale;
	float randomScale = settings.minScale + (range * (rand() / float(RAND_MAX)));
	object->SetScale(object->GetScale() * randomScale);
}

void SHMDMode::ProcessInput() 
{
	if (ImGui::IsKeyDown((ImGuiKey)VK_CONTROL) && ImGui::IsKeyPressed((ImGuiKey)0x53))
	{
		kWorld->SaveSHMD();
	}
	if (ImGuizmo::IsOver())
		return;
	if (_scatterMode->IsScatterModeActive())
	{
		ImGuiIO& io = ImGui::GetIO();
		_scatterMode->ProcessMouseInput(ImVec2(io.MousePos.x, io.MousePos.y));
	}
	else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		SelectObject(kWorld->PickObject(), ImGui::IsKeyDown((ImGuiKey)VK_CONTROL));
		if (HasSelectedObject())
			_scatterMode->SetSelectedObjects(SelectedObjects);
	}
	if (ImGui::IsKeyDown((ImGuiKey)VK_CONTROL)) 
	{
		if (ImGui::IsKeyPressed((ImGuiKey)0x45)) //e
			OperationMode = ImGuizmo::OPERATION::ROTATE;
		if (ImGui::IsKeyPressed((ImGuiKey)0x57)) //w
			OperationMode = ImGuizmo::OPERATION::TRANSLATE;
		if (ImGui::IsKeyPressed((ImGuiKey)0x44)) //d
			SnapMovement = !SnapMovement;
		if (HasSelectedObject() && ImGui::IsKeyPressed((ImGuiKey)0x43))
		{
			CopyObjects.clear();
			CopyGroupCenter = NiPoint3::ZERO;
			
			for (auto obj : SelectedObjects) {
				CopyGroupCenter += obj->GetTranslate();
			}
			CopyGroupCenter /= (float)SelectedObjects.size();
			
			NiPoint3 groupTerrainPos = NiPoint3(CopyGroupCenter.x, CopyGroupCenter.y, 0.0f);
			kWorld->UpdateZCoord(groupTerrainPos);
			
			for (auto obj : SelectedObjects) {
				CopyGroup group;
				group.object = obj;
				NiPoint3 objPos = obj->GetTranslate();
				NiPoint3 objTerrainPos = NiPoint3(objPos.x, objPos.y, 0.0f);
				kWorld->UpdateZCoord(objTerrainPos);
				
				group.offset = NiPoint3(
					objPos.x - CopyGroupCenter.x,
					objPos.y - CopyGroupCenter.y,
					objPos.z - objTerrainPos.z
				);
				CopyObjects.push_back(group);
			}
		}
		if (!CopyObjects.empty() && ImGui::IsKeyPressed((ImGuiKey)0x56))
		{
			auto groupPos = kWorld->GetWorldPoint(IntersectType);
			std::vector<NiPickablePtr> newObjects;
			
			for (const auto& copyGroup : CopyObjects) {
				copyGroup.object->HideBoundingBox();
				NiPickablePtr NewObject = (NiPickable*)copyGroup.object->Clone();
				NiPoint3 pos = groupPos + copyGroup.offset;
				NewObject->SetTranslate(pos);
				
				auto& settings = _pasteSettings->pasteSettings;
				
				if (settings.enableRandomRotation)
					ApplyRandomRotation(NewObject, settings);
				
				if (settings.enableTerrainSnap)
					ApplyTerrainSnap(NewObject, pos, copyGroup.offset, settings);
				
				if (settings.enableRandomScaling)
					ApplyRandomScaling(NewObject, settings);
				
				newObjects.push_back(NewObject);
			}
			
			kWorld->AddObject(newObjects);
			ClearSelectedObjects();
			for (auto obj : newObjects) {
				SelectObject(obj, true);
			}
			
			auto& settings = _pasteSettings->pasteSettings;
			if (settings.autoGenerateSHBD)
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
					EditorScenePtr editorScene = NiSmartPointerCast(EditorScene, _Scene);
					if (editorScene)
					{
						SHBDModePtr tempSHBDMode = NiNew SHBDMode(kWorld, editorScene);
						tempSHBDMode->GenerateSHBDForObjects(shmdObjects);
					}
				}
			}
		}
		if (HasSelectedObject() && ImGui::IsKeyPressed((ImGuiKey)0x42)) //b
		{
			StashSelectedObjects();
		}
		if (ImGui::IsKeyPressed((ImGuiKey)0x4E)) //n
		{
			ToggleStash();
		}
	}
	if (ImGui::IsKeyPressed((ImGuiKey)VK_ESCAPE))
		ClearSelectedObjects();
	if (ImGui::IsKeyDown((ImGuiKey)VK_CONTROL) && ImGui::IsKeyPressed((ImGuiKey)0x31))
		TogglePasteSettings();
	if (ImGui::IsKeyDown((ImGuiKey)VK_CONTROL) && ImGui::IsKeyPressed((ImGuiKey)0x52))
		ToggleScatterMode();
	if (HasSelectedObject() && ImGui::IsKeyPressed((ImGuiKey)VK_DELETE))
	{
		kWorld->RemoveObject(SelectedObjects);
		ClearSelectedObjects();
	}
	if (HasSelectedObject() && ImGui::IsKeyDown((ImGuiKey)VK_CONTROL) && ImGui::IsKeyPressed((ImGuiKey)0x41)) 
	{

		auto obj = SelectedObjects.back();
		auto list = kWorld->GetGroundObjects();
		for (auto listentry : list)
		{
			if (NiIsKindOf(NiPickable, listentry))
			{
				NiPickablePtr ptr = NiSmartPointerCast(NiPickable, listentry);
				if (ptr->GetSHMDPath() == obj->GetSHMDPath() || ImGui::IsKeyDown((ImGuiKey)VK_MENU)) //Select ALL Objects if alt is pressed
					SelectObject(ptr, true);
			}
		}
	}

	if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
	{
		if(NiIsKindOf(EditorScene,_Scene))
		{
			bool HasElement = false;
			for (auto element : ScreenElements)
			{
				if (NiIsKindOf(LuaElement, element)) {
					LuaElementPtr ptr = NiSmartPointerCast(LuaElement, element);
					if (ptr->GetFileName() == "EditorELements/SHMDMiddleMouse.lua")
						HasElement = true;
				}
			}
			if(!HasElement)
				ScreenElements.push_back(NiNew LuaElement(NiSmartPointerCast(EditorScene, _Scene), "EditorELements/SHMDMiddleMouse.lua", ImGui::GetIO().MousePos));
		}
	}
} 

void SHMDMode::DrawGizmo() 
{
	if (SelectedObjects.empty())
		return;

	/*
	Huge Credits to Maki for helping me with this Code
	by giving me his :)
	*/

	float_t matrix[16]{};
	float matrixScale[3] = { 1.f, 1.f, 1.f };
	glm::vec3 tmpRotation{};
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::BeginFrame();
	NiPickablePtr SelectedNode = SelectedObjects.back();

	NiCameraPtr Camera = kWorld->GetCamera();

	NiPoint3 target = Camera->GetTranslate() + Camera->GetWorldDirection() * 10.f;
	const auto eye = Camera->GetTranslate();
	const auto up = glm::vec3{ 0, 0, 1 };

	const auto view = glm::lookAt(glm::vec3(eye.x, eye.y, eye.z), glm::vec3(target.x, target.y, target.z), up);

	glm::mat4 projectionMatrix = glm::perspective(glm::radians(kWorld->GetFOV()), (((float_t)io.DisplaySize.x / (float_t)io.DisplaySize.y)), Camera->GetViewFrustum().m_fNear, Camera->GetViewFrustum().m_fFar);

	NiPoint3 PreviousPos = SelectedNode->GetTranslate();
	NiPoint3 ChangeingPosition = SelectedNode->GetTranslate();
	
	ImGuizmo::RecomposeMatrixFromComponents(&ChangeingPosition.x, &tmpRotation[0], matrixScale, (float*)matrix);

	ImGuizmo::SetRect((float_t)0, (float_t)0, (float_t)io.DisplaySize.x, (float_t)io.DisplaySize.y);
	if (SnapMovement)
		ImGuizmo::Manipulate(&view[0][0], &projectionMatrix[0][0], OperationMode, ImGuizmo::MODE::WORLD, (float*)matrix, nullptr, &SnapSize.x);
	else
		ImGuizmo::Manipulate(&view[0][0], &projectionMatrix[0][0], OperationMode, ImGuizmo::MODE::WORLD, (float*)matrix, nullptr, nullptr);

	ImGuizmo::DecomposeMatrixToComponents((float*)matrix, (float*)&ChangeingPosition.x, &tmpRotation[0], matrixScale);

	NiPoint3 Move = ChangeingPosition - PreviousPos;
	if (Move != NiPoint3::ZERO)
		kWorld->UpdatePos(SelectedObjects, ChangeingPosition - PreviousPos);
	if (tmpRotation != glm::vec3{ 0,0,0 })
		kWorld->UpdateRotation(SelectedObjects, tmpRotation);

}

void SHMDMode::CreateAddElement(std::string name) 
{
	if (name == "Moveable Object") 
	{
		auto AddFunc = &IngameWorld::AddObject;
		for (auto element : ScreenElements) 
		{
			if (NiIsKindOf(AddMultipleObject, element))
				return;
		}
		ScreenElements.push_back(NiNew AddMultipleObject(kWorld, AddFunc, kWorld->GetWorldPoint()));
	}
	else if (name == "Replace Object") 
	{
		auto nodes = GetSelectedNodes();
		for (auto element : ScreenElements)
		{
			if (NiIsKindOf(ReplaceObjects, element))
				return;
		}
		ScreenElements.push_back(NiNew ReplaceObjects(kWorld, nodes));
	}
	else
	{
		void (IngameWorld:: * AddFunc)(NiNodePtr, bool);
		if (name == "Sky") 
		{
			AddFunc = &IngameWorld::AddSky;
		}
		else if (name == "Water")
		{
			AddFunc = &IngameWorld::AddWater;
		}
		else if (name == "Ground") 
		{
			AddFunc = &IngameWorld::AddGroundObject;
		}else
		{
			return;
		}
		for (auto element : ScreenElements)
		{
			if (NiIsKindOf(AddSingleObject, element))
				return;
		}
		ScreenElements.push_back(NiNew AddSingleObject(kWorld, AddFunc,NiPoint3::ZERO));
	}
}

void SHMDMode::SaveSelectedPickableNif() 
{
	NiPickablePtr CurObj = SelectedObjects.back();
	
	auto node = kWorld->GetTerrainNode(); CurObj->ToNiNode();

	NiStream s;
	s.InsertObject(kWorld->GetTerrainNode());
	s.InsertObject(kWorld->GetCollide());
	s.Save(PgUtil::PathFromClientFolder("resmap/field/Rou/test.nif").c_str());
}