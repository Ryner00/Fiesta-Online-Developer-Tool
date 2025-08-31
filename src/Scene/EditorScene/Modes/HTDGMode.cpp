#include "HTDGMode.h"
#include <FiestaOnlineTool.h>
#include <filesystem>
#include "Logger.h"
#include "Brush/LuaBrush.h"
#include <Data/IngameWorld/WorldChanges/FogChange.h>
NiImplementRTTI(HTDGMode, TerrainBrushMode);

void HTDGMode::ProcessInput() 
{
	if (!_brushMode)
	{
		TerrainBrushMode::ProcessInput();
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && kWorld->HasHTD())
		{
			_Data = *kWorld->GetHTD();
			_Update = true;
		}
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && kWorld->HasHTD())
		{
			auto NewData = *kWorld->GetHTD();

			HTDGChangePtr Change = NiNew HTDGChange(kWorld, _Data, NewData);
			kWorld->AttachStack(NiSmartPointerCast(WorldChange, Change));
			_Update = false;
		}
	}
	else
	{
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			_isDrawing = true;
			NiPoint3 worldPos = kWorld->GetWorldPoint();
			PaintedArea area;
			area.worldPos = worldPos;
			area.brushSize = GetBrushSize();
			_paintedPoints.push_back(area);
		}
		
		if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && _isDrawing)
		{
			NiPoint3 worldPos = kWorld->GetWorldPoint();
			if (_paintedPoints.empty() || (worldPos - _paintedPoints.back().worldPos).Length() > GetBrushSize() * 0.3f)
			{
				PaintedArea area;
				area.worldPos = worldPos;
				area.brushSize = GetBrushSize();
				_paintedPoints.push_back(area);
			}
		}
		
		if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			_isDrawing = false;
		}
	}

	if (ImGui::IsKeyDown((ImGuiKey)VK_CONTROL) && ImGui::IsKeyPressed((ImGuiKey)0x53))
	{
		kWorld->SaveHTD();
	}
}

bool HTDGMode::IsPointInPaintedArea(const NiPoint3& point) const
{
	for (const auto& area : _paintedPoints)
	{
		float distance = (point - area.worldPos).Length();
		if (distance <= area.brushSize)
			return true;
	}
	return false;
}

void HTDGMode::ConfirmBrushGeneration()
{
	if (!kWorld->HasHTD() || _paintedPoints.empty())
		return;
		
	_Data = *kWorld->GetHTD();
	
	for (const auto& area : _paintedPoints)
	{
		int centerX = (int)(area.worldPos.x);
		int centerY = (int)(area.worldPos.z);
		int radius = (int)area.brushSize;
		
		for (int x = centerX - radius; x <= centerX + radius; x++)
		{
			for (int y = centerY - radius; y <= centerY + radius; y++)
			{
				NiPoint3 testPoint(x, 0, y);
				if (IsPointInPaintedArea(testPoint))
				{
					if (x >= 0 && y >= 0 && x < _Data.GetWidth() && y < _Data.GetHeight())
					{
						if (_CurrentBrush && NiIsKindOf(LuaBrush, _CurrentBrush))
						{
							LuaBrushPtr luabrush = NiSmartPointerCast(LuaBrush, _CurrentBrush);
							float currentHeight = kWorld->GetHTD(x, y);
							luabrush->RunAlgorithm(x, y, currentHeight, _Data.GetWidth(), _Data.GetHeight(), GetBrushSize() * 0.5f, kWorld, currentHeight);
						}
					}
				}
			}
		}
	}
	
	auto NewData = *kWorld->GetHTD();
	HTDGChangePtr Change = NiNew HTDGChange(kWorld, _Data, NewData);
	kWorld->AttachStack(NiSmartPointerCast(WorldChange, Change));
	
	_paintedPoints.clear();
}