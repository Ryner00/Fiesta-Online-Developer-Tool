#include "SHBDMode.h"
#include <NiPick.h>
#include <FiestaOnlineTool.h>
#include <thread>
#include <future>
#include <chrono>
#include <Data/IngameWorld/WorldChanges/FogChange.h>
#include "Data/NiCustom/NiSHMDPickable.h"

NiImplementRTTI(SHBDMode, TerrainMode);

void SHBDMode::Draw()
{
	TerrainMode::Draw();

	NiVisibleArray kVisible;
	NiCullingProcess kCuller(&kVisible);
	NiDrawScene(Camera, _BaseNode, kCuller);
	
	if (_terrainGenerationActive)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 20, viewport->WorkPos.y + 20));
		ImGui::SetNextWindowSize(ImVec2(300, 80));
		ImGui::Begin("Terrain SHBD Generation", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
		
		float progress = 0.0f;
		if (_totalTerrainCells > 0)
		{
			progress = (float)_processedTerrainCells / (float)_totalTerrainCells;
		}
		
		ImGui::Text("Generating terrain SHBD...");
		ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), "");
		ImGui::Text("Progress: %d / %d cells", _processedTerrainCells, _totalTerrainCells);
		
		ImGui::End();
	}
}
void SHBDMode::Update(float fTime)
{
	TerrainMode::Update(fTime);
	if (_Update)
	{
		UpdateSHBD();
	}
	
	_captureHeight = GetCurrentSHBDHeight();
	
	if (_terrainGenerationActive)
	{
		_terrainGenerationTimer += fTime;
		
		float targetFrameTime = 1.0f / 60.0f;
		float maxProcessTime = targetFrameTime * 0.8f;
		
		auto startTime = std::chrono::high_resolution_clock::now();
		
		auto SHBD = kWorld->GetSHBD();
		auto ini = kWorld->GetShineIni();
		
		if (!SHBD || !ini)
		{
			_terrainGenerationActive = false;
			return;
		}
		
		float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
		unsigned int SHBDSize = SHBD->GetSHBDSize();
		
		int cellsProcessed = 0;
		int skipFactor = 8 - _generationAccuracy;
		if (skipFactor < 1) skipFactor = 1;
		if (skipFactor > 16) skipFactor = 16;
		
		_terrainCellsPerFrame = 5000 / skipFactor;
		
		
		while (_currentTerrainY < (int)SHBDSize && cellsProcessed < _terrainCellsPerFrame)
		{
			if (_currentTerrainX >= (int)SHBDSize)
			{
				_currentTerrainX = 0;
				_currentTerrainY += skipFactor;
				continue;
			}
			
			if ((_currentTerrainX % skipFactor == 0) && (_currentTerrainY % skipFactor == 0))
			{
				float worldX = _currentTerrainX * PixelSize;
				float worldY = _currentTerrainY * PixelSize;
				
				bool walkable = true;
				
				NiPoint3 testPoint(worldX, worldY, _captureHeight);
				
				if (kWorld->UpdateZCoord(testPoint))
				{
					float heightDiff = fabsf(testPoint.z - _captureHeight);
					if (heightDiff > _captureTolerance)
					{
						walkable = false;
					}
				}
				else
				{
					walkable = false;
				}
				
				std::vector<std::pair<int, int>> updates;
				updates.reserve(skipFactor * skipFactor);
				
				for (int dy = 0; dy < skipFactor && (_currentTerrainY + dy) < (int)SHBDSize; dy++)
				{
					for (int dx = 0; dx < skipFactor && (_currentTerrainX + dx) < (int)SHBDSize; dx++)
					{
						updates.push_back({_currentTerrainX + dx, _currentTerrainY + dy});
					}
				}
				
				for (const auto& coord : updates)
				{
					SHBD->UpdateSHBDData(coord.first, coord.second, walkable);
				}
			}
			
			_currentTerrainX += skipFactor;
			cellsProcessed++;
			_processedTerrainCells += skipFactor * skipFactor;
			
			auto currentTime = std::chrono::high_resolution_clock::now();
			auto elapsedTime = std::chrono::duration<float>(currentTime - startTime).count();
			
			if (elapsedTime > maxProcessTime)
			{
				break;
			}
		}
		
		if (_currentTerrainY >= (int)SHBDSize)
		{
			_terrainGenerationActive = false;
			CreateSHBDNode();
		}
	}
	
	auto SHBD = kWorld->GetSHBD();
	if (SHBD->HadDirectUpdate() || TextureConnector.empty())
		CreateSHBDNode();

	_BaseNode->Update(fTime);
	_BaseNode->UpdateProperties();
}
void SHBDMode::ProcessInput()
{
	TerrainMode::ProcessInput();

	if (ImGui::IsKeyDown((ImGuiKey)VK_CONTROL) && ImGui::IsKeyPressed((ImGuiKey)0x53))
	{
		kWorld->SaveSHBD();
	}
	ImGuiIO& io = ImGui::GetIO();
	float DeltaTime = FiestaOnlineTool::GetDeltaTime();
	if (io.MouseWheel != 0.0f)
	{
		if (!ImGui::IsKeyDown((ImGuiKey)VK_MENU))
		{
			NiPoint3 NodePositon = _SHBDNode->GetTranslate();
			NiPoint3 MoveDirect(0.0f, 0.0f, 0.0f);

			float SpeedUp = io.MouseWheel;
			if (io.KeyShift)
				SpeedUp *= 5.0f;
			NodePositon.z = NodePositon.z + 115.f * DeltaTime * SpeedUp;

			_SHBDNode->SetTranslate(NodePositon);
		}
	}
	if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) 
	{
		auto SHBD = kWorld->GetSHBD();
		_Data = SHBD->GetSHBDData();
		_Update = true;
	}
	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
	{
		auto SHBD = kWorld->GetSHBD();
		auto NewData = SHBD->GetSHBDData();
		SHBDChangePtr Change = NiNew SHBDChange(kWorld, _Data, NewData);
		kWorld->AttachStack(NiSmartPointerCast(WorldChange, Change));
		_Update = false;
	}
}
void SHBDMode::UpdateSHBD() 
{
	auto ini = kWorld->GetShineIni();
	auto SHBD = kWorld->GetSHBD();

	float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();

	int middlew = MouseIntersect.x / PixelSize;
	int middleh = MouseIntersect.y / PixelSize;
	for (int w = middlew - _BrushSize; w <= middlew + _BrushSize && w < (int)SHBD->GetSHBDSize(); w++)
	{
		if (w < 0)
			continue;
		for (int h = middleh - _BrushSize; h <= middleh + _BrushSize && h < (int)SHBD->GetSHBDSize(); h++)
		{
			if (h < 0)
				continue;
			if (!((w - middlew) * (w - middlew) + (h - middleh) * (h - middleh) <= _BrushSize * _BrushSize))
				continue;

			SHBD->UpdateSHBDData(w, h, _Walkable);

			int TextureH = h / TextureSize;
			int TextureW = w / TextureSize;
			NiPixelDataPtr pixleData = TextureConnector[TextureH][TextureW];

			pixleData->MarkAsChanged();
			int TextureInternH = h % TextureSize;
			int TextureInternW = w % TextureSize;
			unsigned int* NewPtr = ((unsigned int*)pixleData->GetPixels()) + (TextureInternH * pixleData->GetWidth()) + TextureInternW;
			if (_Walkable)
				*NewPtr = Walkable;
			else
				*NewPtr = Blocked;
		}
	}
}
void SHBDMode::CreateSHBDNode() 
{
	auto SHBD = kWorld->GetSHBD();
	auto ini = kWorld->GetShineIni();

	TextureConnector.clear();
	_SHBDNode->RemoveAllChildren();
	float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();

	std::vector<NiPoint3> NormalList = { NiPoint3::UNIT_Z ,NiPoint3::UNIT_Z ,NiPoint3::UNIT_Z ,NiPoint3::UNIT_Z };
	std::vector<NiColorA> ColorList = { NiColorA(0.f,1.0f,0.f,1.f), NiColorA(0.f,1.0f,0.f,1.f),NiColorA(0.f,1.0f,0.f,1.f), NiColorA(0.f,1.0f,0.f,1.f) };
	std::vector<NiPoint2> TextureList1 = { NiPoint2(0.f,0.f),NiPoint2(1.f,0.f),NiPoint2(0.f,1.f),NiPoint2(1.f,1.f) };
	std::vector<NiPoint2> TextureList2 = { NiPoint2(0.f,0.f),NiPoint2(0.5f,0.f),NiPoint2(0.f,0.5f),NiPoint2(0.5f,0.5f) }; //2048 
	for (int i = 0; i < TextureList2.size(); i++)
		TextureList1.push_back(TextureList2.at(i));
	struct Triangle
	{
		unsigned short one;
		unsigned short two;
		unsigned short three;
	};
	std::vector<Triangle> TriangleList;

	TriangleList.push_back(Triangle(0, 1, 2));
	TriangleList.push_back(Triangle(2, 1, 3));

	NiAlphaPropertyPtr alphaprop = NiNew NiAlphaProperty();
	alphaprop->SetAlphaBlending(true);
	alphaprop->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);

	int TextureCt = SHBD->GetSHBDSize() / TextureSize;
	if (SHBD->GetSHBDSize() % TextureSize != 0)
		TextureCt++;
	const NiPixelFormat* SHBDTexturePixelFormat = &NiPixelFormat::RGBA32;
	Blocked = (0xCD << SHBDTexturePixelFormat->GetShift(NiPixelFormat::COMP_RED)) & SHBDTexturePixelFormat->GetMask(NiPixelFormat::COMP_RED);
	Blocked |= (0x0 << SHBDTexturePixelFormat->GetShift(NiPixelFormat::COMP_BLUE)) & SHBDTexturePixelFormat->GetMask(NiPixelFormat::COMP_BLUE);
	Blocked |= (0xA0 << SHBDTexturePixelFormat->GetShift(NiPixelFormat::COMP_ALPHA)) & SHBDTexturePixelFormat->GetMask(NiPixelFormat::COMP_ALPHA);

	Walkable = (0x0 << SHBDTexturePixelFormat->GetShift(NiPixelFormat::COMP_RED)) & SHBDTexturePixelFormat->GetMask(NiPixelFormat::COMP_RED);
	Walkable |= (0xCD << SHBDTexturePixelFormat->GetShift(NiPixelFormat::COMP_GREEN)) & SHBDTexturePixelFormat->GetMask(NiPixelFormat::COMP_GREEN);
	Walkable |= (0x0 << SHBDTexturePixelFormat->GetShift(NiPixelFormat::COMP_BLUE)) & SHBDTexturePixelFormat->GetMask(NiPixelFormat::COMP_BLUE);
	Walkable |= (0xA0 << SHBDTexturePixelFormat->GetShift(NiPixelFormat::COMP_ALPHA)) & SHBDTexturePixelFormat->GetMask(NiPixelFormat::COMP_ALPHA);


	std::vector<std::shared_future<std::pair<std::vector<NiPixelDataPtr>, std::vector<NiTriShapePtr>>>> _ThreadList;
	for (int TextureH = 0; TextureH < TextureCt; TextureH++)
	{
		auto future = std::async(std::launch::async,
			[this, TextureCt, TextureH, NormalList, ColorList, TextureList1, TriangleList, alphaprop,PixelSize, SHBD]
			{
				NiTexture::FormatPrefs kPrefs;
				kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
				kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
				kPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;

		
				std::vector<NiPixelDataPtr> TextureList;
				std::vector<NiTriShapePtr> Shapes;
				for (int TextureW = 0; TextureW < TextureCt; TextureW++)
				{
					NiPoint3 BL(TextureW * TextureSize * PixelSize, TextureH * TextureSize * PixelSize, 1.f);
					NiPoint3 BR((TextureW + 1) * TextureSize * PixelSize, TextureH * TextureSize * PixelSize, 1.f);
					NiPoint3 TL(TextureW * TextureSize * PixelSize, (TextureH + 1) * TextureSize * PixelSize, 1.f);
					NiPoint3 TR((TextureW + 1) * TextureSize * PixelSize, (TextureH + 1) * TextureSize * PixelSize, 1.f);

					std::vector<NiPoint3> VerticesList = { BL,BR,TL,TR };

					NiPoint3* pkVertix = NiNew NiPoint3[4];
					memcpy(pkVertix, &VerticesList[0], (int)VerticesList.size() * sizeof(NiPoint3));

					NiPoint3* pkNormal = NiNew NiPoint3[(int)NormalList.size()];
					NiColorA* pkColor = NiNew NiColorA[(int)ColorList.size()];
					NiPoint2* pkTexture = NiNew NiPoint2[(int)TextureList1.size()];

					unsigned short* pusTriList = (unsigned short*)NiAlloc(char, 12 * (TriangleList.size() / 2));// NiNew NiPoint3[TriangleList.size() / 2];

					memcpy(pkNormal, &NormalList[0], (int)NormalList.size() * sizeof(NiPoint3));
					memcpy(pkColor, &ColorList[0], (int)ColorList.size() * sizeof(NiColorA));
					memcpy(pkTexture, &TextureList1[0], (int)TextureList1.size() * sizeof(NiPoint2));
					memcpy(pusTriList, &TriangleList[0], (int)TriangleList.size() * 3 * sizeof(unsigned short));


					NiTriShapeDataPtr data = NiNew NiTriShapeData((unsigned short)VerticesList.size(), pkVertix, pkNormal, pkColor, pkTexture, 2, NiGeometryData::DataFlags::NBT_METHOD_NONE, (unsigned short)TriangleList.size(), pusTriList);
					NiTriShapePtr BlockShape = NiNew NiTriShape(data);
					BlockShape->AttachProperty(alphaprop);
					BlockShape->CalculateNormals();

					NiPixelData* pixel = NiNew NiPixelData(TextureSize, TextureSize, NiPixelFormat::RGBA32);

					auto pixeloffset = (unsigned int*)pixel->GetPixels();
					TextureList.push_back(pixel);
					for (int h = 0; h < TextureSize; h++)
					{
						for (int w = 0; w < TextureSize; w++)
						{
							unsigned int* NewPtr = (pixeloffset + (h * pixel->GetWidth()) + w);
							int hges = h + TextureH * TextureSize;
							int wges = w + TextureW * TextureSize;

							int index = hges * SHBD->GetSHBDSize() + wges;

							size_t charIndex = index / 8;
							if (charIndex >= SHBD->GetMapSize() * SHBD->GetSHBDSize())
								continue;
							if (SHBD->IsWalkable(wges,hges))
							{
								*pixeloffset = Walkable;
							}else
							{
								*pixeloffset = Blocked;
							}

							pixeloffset++;
						}
					}

					NiSourceTexturePtr texture = NiSourceTexture::Create(pixel, kPrefs);
					texture->SetStatic(false);
					NiTexturingPropertyPtr Texture = NiNew NiTexturingProperty;
					Texture->SetBaseTexture(texture);
					Texture->SetBaseFilterMode(NiTexturingProperty::FILTER_NEAREST);
					Texture->SetApplyMode(NiTexturingProperty::APPLY_REPLACE);


					BlockShape->AttachProperty(Texture);
					Shapes.push_back(BlockShape);
				}
				return std::pair<std::vector<NiPixelDataPtr>, std::vector<NiTriShapePtr>>(TextureList, Shapes);
			});
		_ThreadList.push_back(future.share());
	}
	{
		using namespace std::chrono_literals;
		auto it = _ThreadList.begin();
		while (it != _ThreadList.end())
		{
			auto status = it->wait_for(10ms);
			if (status == std::future_status::ready)
			{
				TextureConnector.push_back(it->get().first);
				for (auto shape : it->get().second)
					_SHBDNode->AttachChild(shape);
				it++;
				continue;
			}
			Sleep(1);
		}
	}
	_SHBDNode->UpdateEffects();
	_SHBDNode->UpdateProperties();
	_SHBDNode->Update(0.0f);
}

void SHBDMode::AutoGenerateSHBD()
{
	auto SHBD = kWorld->GetSHBD();
	auto ini = kWorld->GetShineIni();
	if (!SHBD || !ini) return;
	
	_captureHeight = GetCurrentSHBDHeight();
	
	if (!_keepExistingSHBD)
	{
		unsigned int SHBDSize = SHBD->GetSHBDSize();
		for (unsigned int h = 0; h < SHBDSize; h++)
		{
			for (unsigned int w = 0; w < SHBDSize; w++)
			{
				SHBD->UpdateSHBDData(w, h, true);
			}
		}
		CreateSHBDNode();
	}
	
	std::vector<NiSHMDPickablePtr> objects;
	for (auto obj : kWorld->GetGroundObjects())
	{
		if (obj && NiIsKindOf(NiSHMDPickable, obj))
		{
			NiSHMDPickablePtr ptr = NiSmartPointerCast(NiSHMDPickable, obj);
			if (ptr)
			{
				NiPoint3 objPos = ptr->GetWorldTranslate();
				if (fabsf(objPos.z - _captureHeight) <= _captureTolerance)
				{
					objects.push_back(ptr);
				}
			}
		}
	}
	
	GenerateSHBDForObjects(objects);
}

void SHBDMode::GenerateSHBDForObjects(const std::vector<NiSHMDPickablePtr>& objects)
{
	auto SHBD = kWorld->GetSHBD();
	auto ini = kWorld->GetShineIni();
	if (!SHBD || !ini) return;
	
	float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
	unsigned int SHBDSize = SHBD->GetSHBDSize();
	
	for (auto& obj : objects)
	{
		if (!obj) continue;
		
		NiPoint3 objPos = obj->GetWorldTranslate();
		float objScale = obj->GetWorldScale();
		
		NiBound objBound = obj->GetWorldBound();
		float boundRadius = objBound.GetRadius();
		
		if (boundRadius < 1.5f) continue;
		
		float sampleRadius = boundRadius * 2.0f;
		int sampleGrid = 8 + ((_generationAccuracy - 1) * 8);
		if (sampleGrid < 8) sampleGrid = 8;
		if (sampleGrid > 64) sampleGrid = 64;
		
		std::vector<std::vector<bool>> occupancyGrid(sampleGrid, std::vector<bool>(sampleGrid, false));
		
		for (int y = 0; y < sampleGrid; y++)
		{
			for (int x = 0; x < sampleGrid; x++)
			{
				float normX = (float)x / (float)(sampleGrid - 1);
				float normY = (float)y / (float)(sampleGrid - 1);
				
				float testX = objPos.x + (normX - 0.5f) * sampleRadius * 2.0f;
				float testY = objPos.y + (normY - 0.5f) * sampleRadius * 2.0f;
				float testZ = objPos.z;
				
				NiPoint3 rayStart(testX, testY, objPos.z + 50.0f);
				NiPoint3 rayEnd(testX, testY, objPos.z - 10.0f);
				NiPoint3 rayDir = rayEnd - rayStart;
				rayDir.Unitize();
				
				NiPick objPick;
				objPick.SetPickType(NiPick::FIND_FIRST);
				objPick.SetSortType(NiPick::SORT);
				objPick.SetIntersectType(NiPick::TRIANGLE_INTERSECT);
				objPick.SetFrontOnly(false);
				objPick.SetReturnNormal(false);
				objPick.SetObserveAppCullFlag(true);
				objPick.SetTarget(obj);
				
				if (objPick.PickObjects(rayStart, rayDir, true))
				{
					occupancyGrid[y][x] = true;
				}
			}
		}
		
		for (int y = 0; y < sampleGrid; y++)
		{
			for (int x = 0; x < sampleGrid; x++)
			{
				if (!occupancyGrid[y][x]) continue;
				
				float normX = (float)x / (float)(sampleGrid - 1);
				float normY = (float)y / (float)(sampleGrid - 1);
				
				float worldX = objPos.x + (normX - 0.5f) * sampleRadius * 2.0f;
				float worldY = objPos.y + (normY - 0.5f) * sampleRadius * 2.0f;
				
				int sHBDX = (int)(worldX / PixelSize);
				int sHBDY = (int)(worldY / PixelSize);
				
				if (sHBDX < 0 || sHBDX >= (int)SHBDSize || sHBDY < 0 || sHBDY >= (int)SHBDSize) continue;
				
				UpdateSHBDArea(sHBDX, sHBDY, SHBDSize, false);
			}
		}
	}
	
	CreateSHBDNode();
}

void SHBDMode::UpdateSHBDArea(int centerX, int centerY, unsigned int SHBDSize, bool value)
{
	auto SHBD = kWorld->GetSHBD();
	if (!SHBD) return;
	
	for (int dY = -1; dY <= 1; dY++)
	{
		for (int dX = -1; dX <= 1; dX++)
		{
			int fillX = centerX + dX;
			int fillY = centerY + dY;
			
			if (fillX < 0 || fillX >= (int)SHBDSize || fillY < 0 || fillY >= (int)SHBDSize) continue;
			
			SHBD->UpdateSHBDData(fillX, fillY, value);
		}
	}
}

void SHBDMode::AutoGenerateTerrainSHBD()
{
	auto SHBD = kWorld->GetSHBD();
	auto ini = kWorld->GetShineIni();
	if (!SHBD || !ini) return;
	
	_captureHeight = GetCurrentSHBDHeight();
	
	_terrainGenerationActive = true;
	_terrainGenerationTimer = 0.0f;
	_currentTerrainX = 0;
	_currentTerrainY = 0;
	_processedTerrainCells = 0;
	_totalTerrainCells = SHBD->GetSHBDSize() * SHBD->GetSHBDSize();
	
	unsigned int SHBDSize = SHBD->GetSHBDSize();
	
	if (!_keepExistingSHBD)
	{
		for (unsigned int h = 0; h < SHBDSize; h++)
		{
			for (unsigned int w = 0; w < SHBDSize; w++)
			{
				SHBD->UpdateSHBDData(w, h, true);
			}
		}
	}
	
	float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
	
}

void SHBDMode::ResetSHBD()
{
	auto SHBD = kWorld->GetSHBD();
	if (!SHBD) return;
	
	unsigned int SHBDSize = SHBD->GetSHBDSize();
	
	for (unsigned int h = 0; h < SHBDSize; h++)
	{
		for (unsigned int w = 0; w < SHBDSize; w++)
		{
			SHBD->UpdateSHBDData(w, h, true);
		}
	}
	
	CreateSHBDNode();
}

void SHBDMode::UpdateMouseIntersect()
{
	NiPoint3 kOrigin, kDir;
	auto Point = FiestaOnlineTool::CurrentMousePosition();
	if (Camera->WindowPointToRay(Point.x, Point.y, kOrigin, kDir))
	{
		NiPick _Pick;
		_Pick.SetPickType(NiPick::FIND_FIRST);
		_Pick.SetSortType(NiPick::SORT);
		_Pick.SetIntersectType(NiPick::TRIANGLE_INTERSECT);
		_Pick.SetFrontOnly(true);
		_Pick.SetReturnNormal(true);
		_Pick.SetObserveAppCullFlag(true);
		_Pick.SetTarget(_SHBDNode);
		if (_Pick.PickObjects(kOrigin, kDir, true))
		{
			NiPick::Results& results = _Pick.GetResults();
			if (results.GetSize() > 0)
			{
				MouseIntersect = results.GetAt(0)->GetIntersection();
				return;
			}
		}
	}
}