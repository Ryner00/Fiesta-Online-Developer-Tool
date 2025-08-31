#include "SBIMode.h"
#include <NiPick.h>
#include <FiestaOnlineTool.h>
#include <thread>
#include <future>
#include <Data/IngameWorld/WorldChanges/WorldChanges.h>
#include <ImGui/imgui.h>
#include <ImGui/imgui_internal.h>

NiImplementRTTI(SBIMode, TerrainMode);

void SBIMode::Draw()
{
    TerrainMode::Draw();
    
    _minimap.DrawSBISettings(kWorld, GetEditModeName());

    NiVisibleArray kVisible;
    NiCullingProcess kCuller(&kVisible);
    NiDrawScene(Camera, _BaseNode, kCuller);
}

void SBIMode::Update(float fTime)
{
    TerrainMode::Update(fTime);
    if (_Update)
    {
        UpdateSBI();
    }
    
    auto SBI = kWorld->GetSBI();
    if (SBI && (SBI->HadDirectUpdate() || TextureConnector.empty()))
        CreateSBINode();
        

    _BaseNode->Update(fTime);
    _BaseNode->UpdateProperties();
}

void SBIMode::ProcessInput()
{
    TerrainMode::ProcessInput();
    UpdateMouseIntersect();
    

    if (ImGui::IsKeyDown((ImGuiKey)VK_CONTROL) && ImGui::IsKeyPressed((ImGuiKey)0x53))
    {
        kWorld->SaveSBI();
    }
    
    ImGuiIO& io = ImGui::GetIO();
    float DeltaTime = FiestaOnlineTool::GetDeltaTime();
    if (io.MouseWheel != 0.0f)
    {
        if (!ImGui::IsKeyDown((ImGuiKey)VK_MENU))
        {
            NiPoint3 NodePositon = _SBINode->GetTranslate();
            NiPoint3 MoveDirect(0.0f, 0.0f, 0.0f);

            float SpeedUp = io.MouseWheel;
            if (io.KeyShift)
                SpeedUp *= 5.0f;
            NodePositon.z = NodePositon.z + 115.f * DeltaTime * SpeedUp;

            _SBINode->SetTranslate(NodePositon);
        }
    }
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) 
    {
        auto SBI = kWorld->GetSBI();
        if (SBI)
        {
            if (_dragMode && _selectedDoor >= 0)
            {
                _isDragging = true;
                _dragStartPos = MouseIntersect;
                auto door = SBI->GetDoor(_selectedDoor);
                if (door)
                {
                    _doorStartPos = door->startPos;
                    _doorEndPos = door->endPos;
                }
            }
            else
            {
                int selectedDoor = FindDoorAtMousePosition();
                if (selectedDoor >= 0)
                {
                    _selectedDoor = selectedDoor;
                    LogInfo("Selected door: " + std::to_string(_selectedDoor));
                    if (!_dragMode)
                        _Update = true;
                }
            }
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) 
    {
        if (_isDragging)
        {
            _isDragging = false;
        }
        else
        {
            _Update = false;
        }
    }
    
    if (_isDragging && _selectedDoor >= 0)
    {
        UpdateDoorDrag();
    }
}

void SBIMode::UpdateSBI() 
{
    if (_selectedDoor < 0)
        return;
        
    auto ini = kWorld->GetShineIni();
    auto SBI = kWorld->GetSBI();
    auto SHBD = kWorld->GetSHBD();
    if (!SBI || !SHBD)
        return;
        
    auto door = SBI->GetDoor(_selectedDoor);
    if (!door)
        return;

    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
    
    float worldMouseX = MouseIntersect.x;
    float worldMouseY = MouseIntersect.y;
    
    int pixelMouseX = worldMouseX / PixelSize;
    int pixelMouseY = worldMouseY / PixelSize;
    
    int doorPixelX = pixelMouseX - door->startPos.x;
    int doorPixelY = pixelMouseY - door->startPos.y;
    
    
    for (int w = doorPixelX - _BrushSize; w <= doorPixelX + _BrushSize && w < door->GetWidth(); w++)
    {
        if (w < 0)
            continue;
        for (int h = doorPixelY - _BrushSize; h <= doorPixelY + _BrushSize && h < door->GetHeight(); h++)
        {
            if (h < 0)
                continue;
            if (!((w - doorPixelX) * (w - doorPixelX) + (h - doorPixelY) * (h - doorPixelY) <= _BrushSize * _BrushSize))
                continue;

            SBI->UpdateDoorData(_selectedDoor, w, h, _paintMode);
        }
    }
    
    CreateSBINode();
}

void SBIMode::CreateSBINode() 
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return;
        
    auto ini = kWorld->GetShineIni();

    TextureConnector.clear();
    _SBINode->RemoveAllChildren();


    for (size_t doorIndex = 0; doorIndex < SBI->GetDoorCount(); doorIndex++)
    {
        auto door = SBI->GetDoor(doorIndex);
        if (door)
        {
            auto doorMesh = CreateDoorMesh(*door, doorIndex);
            if (doorMesh)
                _SBINode->AttachChild(doorMesh);
        }
    }

    _SBINode->UpdateEffects();
    _SBINode->UpdateProperties();
    _SBINode->Update(0.0f);
}

NiNodePtr SBIMode::CreateDoorMesh(const SBIDoorBlock& door, int doorIndex)
{
    auto ini = kWorld->GetShineIni();
    auto SHBD = kWorld->GetSHBD();
    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
    
    float worldStartX = door.startPos.x * PixelSize;
    float worldStartY = door.startPos.y * PixelSize;
    float worldEndX = (door.endPos.x + 1) * PixelSize;
    float worldEndY = (door.endPos.y + 1) * PixelSize;
    
    NiPoint3 BL(worldStartX, worldStartY, door.height);
    NiPoint3 BR(worldEndX, worldStartY, door.height);
    NiPoint3 TL(worldStartX, worldEndY, door.height);
    NiPoint3 TR(worldEndX, worldEndY, door.height);

    std::vector<NiPoint3> VerticesList = { BL, BR, TL, TR };
    std::vector<NiPoint3> NormalList = { NiPoint3::UNIT_Z, NiPoint3::UNIT_Z, NiPoint3::UNIT_Z, NiPoint3::UNIT_Z };
    NiColorA blueColor(0.0f, 0.0f, 1.0f, 0.7f);
    std::vector<NiColorA> ColorList = { blueColor, blueColor, blueColor, blueColor };
    std::vector<NiPoint2> TextureList = { NiPoint2(0.f, 0.f), NiPoint2(1.f, 0.f), NiPoint2(0.f, 1.f), NiPoint2(1.f, 1.f) };
    
    struct Triangle { unsigned short one, two, three; };
    std::vector<Triangle> TriangleList = { {0, 1, 2}, {2, 1, 3} };

    NiAlphaPropertyPtr alphaprop = NiNew NiAlphaProperty();
    alphaprop->SetAlphaBlending(true);
    alphaprop->SetSrcBlendMode(NiAlphaProperty::ALPHA_SRCALPHA);

    NiPoint3* pkVertix = NiNew NiPoint3[4];
    NiPoint3* pkNormal = NiNew NiPoint3[4];
    NiColorA* pkColor = NiNew NiColorA[4];
    NiPoint2* pkTexture = NiNew NiPoint2[4];
    unsigned short* pusTriList = (unsigned short*)NiAlloc(char, 12);

    memcpy(pkVertix, &VerticesList[0], 4 * sizeof(NiPoint3));
    memcpy(pkNormal, &NormalList[0], 4 * sizeof(NiPoint3));
    memcpy(pkColor, &ColorList[0], 4 * sizeof(NiColorA));
    memcpy(pkTexture, &TextureList[0], 4 * sizeof(NiPoint2));
    memcpy(pusTriList, &TriangleList[0], 2 * 3 * sizeof(unsigned short));

    NiTriShapeDataPtr data = NiNew NiTriShapeData(4, pkVertix, pkNormal, pkColor, pkTexture, 1, NiGeometryData::DataFlags::NBT_METHOD_NONE, 2, pusTriList);
    NiTriShapePtr BlockShape = NiNew NiTriShape(data);
    BlockShape->AttachProperty(alphaprop);
    BlockShape->AttachProperty(NiNew NiVertexColorProperty);
    
    NiPixelData* pixel = NiNew NiPixelData(door.GetWidth(), door.GetHeight(), NiPixelFormat::RGBA32);
    auto pixeloffset = (unsigned int*)pixel->GetPixels();
    
    for (int h = 0; h < door.GetHeight(); h++)
    {
        for (int w = 0; w < door.GetWidth(); w++)
        {
            unsigned int* NewPtr = (pixeloffset + (h * pixel->GetWidth()) + w);
            
            auto SBI = kWorld->GetSBI();
            bool isSet = SBI->IsDoorPixelSet(doorIndex, w, h);
            bool isBorder = (w == 0 || w == door.GetWidth() - 1 || h == 0 || h == door.GetHeight() - 1);
            bool isBlackOutline = (w <= 1 || w >= door.GetWidth() - 2 || h <= 1 || h >= door.GetHeight() - 2);
            
            if (isBlackOutline)
            {
                *NewPtr = 0xFF000000;
            }
            else if (isSet)
            {
                *NewPtr = 0xFF0000FF;
            }
            else if (isBorder)
            {
                *NewPtr = 0x800000FF;
            }
            else
            {
                *NewPtr = 0x00000000;
            }
        }
    }
    
    NiTexture::FormatPrefs kPrefs;
    kPrefs.m_eAlphaFmt = NiTexture::FormatPrefs::SMOOTH;
    kPrefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    kPrefs.m_eMipMapped = NiTexture::FormatPrefs::NO;

    NiSourceTexturePtr texture = NiSourceTexture::Create(pixel, kPrefs);
    texture->SetStatic(false);
    NiTexturingPropertyPtr Texture = NiNew NiTexturingProperty;
    Texture->SetBaseTexture(texture);
    Texture->SetBaseFilterMode(NiTexturingProperty::FILTER_NEAREST);
    Texture->SetApplyMode(NiTexturingProperty::APPLY_REPLACE);

    BlockShape->AttachProperty(Texture);
    

    NiNodePtr doorNode = NiNew NiNode;
    doorNode->AttachChild(BlockShape);
    return doorNode;
}

void SBIMode::UpdateMouseIntersect()
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
        _Pick.SetTarget(_SBINode);
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

void SBIMode::CreateDoor(const std::string& name, float startX, float startY, float endX, float endY)
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return;
    
    SBIDoorBlock door;
    door.blockName = name;
    door.startPos = NiPoint2(startX, startY);
    
    int width = static_cast<int>(endX - startX + 1);
    if (width % 8 != 0)
        width = ((width / 8) + 1) * 8;
    
    door.endPos = NiPoint2(startX + width - 1, endY);
    door.height = 50.0f + (SBI->GetDoorCount() * 10.0f);
    
    LogInfo("Creating door: " + name + " at " + std::to_string(startX) + "," + std::to_string(startY) + " to " + std::to_string(door.endPos.x) + "," + std::to_string(door.endPos.y) + " height=" + std::to_string(door.height));
    
    door.data.resize(door.GetDataSize(), 0);
    
    door.color = NiColorA(0.0f, 0.0f, 1.0f, 0.7f);
    
    SBI->AddDoor(door);
}

void SBIMode::DeleteDoor(int index)
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return;
    SBI->RemoveDoor(index);
    if (_selectedDoor == index)
        _selectedDoor = -1;
    else if (_selectedDoor > index)
        _selectedDoor--;
}

size_t SBIMode::GetDoorCount()
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return 0;
    return SBI->GetDoorCount();
}

SBIDoorBlock* SBIMode::GetDoor(int index)
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return nullptr;
    return SBI->GetDoor(index);
}

void SBIMode::TeleportToDoor(int index)
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return;
        
    auto door = SBI->GetDoor(index);
    if (!door)
        return;
        
    auto ini = kWorld->GetShineIni();
    auto SHBD = kWorld->GetSHBD();
    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
    
    float pixelCenterX = (door->startPos.x + door->endPos.x) / 2.0f;
    float pixelCenterY = (door->startPos.y + door->endPos.y) / 2.0f;
    
    float worldCenterX = pixelCenterX * PixelSize;
    float worldCenterY = pixelCenterY * PixelSize;
    float height = door->height + 20.0f;
    
    NiPoint3 teleportPos(worldCenterX, worldCenterY, height);
    Camera->SetTranslate(teleportPos);
    
    LogInfo("Teleported to door " + door->blockName + " at world coords " + std::to_string(worldCenterX) + "," + std::to_string(worldCenterY) + "," + std::to_string(height));
}

int SBIMode::FindDoorAtMousePosition()
{
    auto SBI = kWorld->GetSBI();
    auto ini = kWorld->GetShineIni();
    auto SHBD = kWorld->GetSHBD();
    if (!SBI || !SHBD)
        return -1;
        
    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
    
    float worldMouseX = MouseIntersect.x;
    float worldMouseY = MouseIntersect.y;
    
    int pixelMouseX = worldMouseX / PixelSize;
    int pixelMouseY = worldMouseY / PixelSize;
    
    for (size_t i = 0; i < SBI->GetDoorCount(); i++)
    {
        auto door = SBI->GetDoor(i);
        if (door)
        {
            if (pixelMouseX >= door->startPos.x && pixelMouseX <= door->endPos.x &&
                pixelMouseY >= door->startPos.y && pixelMouseY <= door->endPos.y)
            {
                return static_cast<int>(i);
            }
        }
    }
    
    return -1;
}

void SBIMode::CreateDoorNearCamera(const std::string& name)
{
    auto SBI = kWorld->GetSBI();
    auto ini = kWorld->GetShineIni();
    auto SHBD = kWorld->GetSHBD();
    if (!SBI || !SHBD)
        return;
    
    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
    
    NiPoint3 cameraPos = Camera->GetTranslate();
    float pixelX = cameraPos.x / PixelSize;
    float pixelY = cameraPos.y / PixelSize;
    
    float startX = pixelX - 50;
    float startY = pixelY - 50;
    float endX = pixelX + 50;
    float endY = pixelY + 50;
    
    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (endX >= SHBD->GetSHBDSize()) endX = SHBD->GetSHBDSize() - 1;
    if (endY >= SHBD->GetSHBDSize()) endY = SHBD->GetSHBDSize() - 1;
    
    CreateDoor(name, startX, startY, endX, endY);
}

void SBIMode::ResizeDoor(int index, float startX, float startY, float endX, float endY)
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return;
        
    auto door = SBI->GetDoor(index);
    if (!door)
        return;
    
    int width = static_cast<int>(endX - startX + 1);
    if (width % 8 != 0)
        width = ((width / 8) + 1) * 8;
    
    door->startPos = NiPoint2(startX, startY);
    door->endPos = NiPoint2(startX + width - 1, endY);
    door->data.resize(door->GetDataSize(), 0);
    
    CreateSBINode();
}

SBIDoorBlock* SBIMode::GetDoorData(int index)
{
    auto SBI = kWorld->GetSBI();
    if (!SBI)
        return nullptr;
    return SBI->GetDoor(index);
}


void SBIMode::UpdateDoorDrag()
{
    auto SBI = kWorld->GetSBI();
    auto ini = kWorld->GetShineIni();
    auto SHBD = kWorld->GetSHBD();
    if (!SBI || !SHBD)
        return;
        
    auto door = SBI->GetDoor(_selectedDoor);
    if (!door)
        return;
    
    float PixelSize = SHBD->GetMapSize() * ini->GetOneBlockWidht() / SHBD->GetSHBDSize();
    
    NiPoint3 deltaWorld = MouseIntersect - _dragStartPos;
    float deltaPixelX = deltaWorld.x / PixelSize;
    float deltaPixelY = deltaWorld.y / PixelSize;
    
    float newStartX = _doorStartPos.x + deltaPixelX;
    float newStartY = _doorStartPos.y + deltaPixelY;
    float newEndX = _doorEndPos.x + deltaPixelX;
    float newEndY = _doorEndPos.y + deltaPixelY;
    
    if (newStartX >= 0 && newStartY >= 0 && newEndX < SHBD->GetSHBDSize() && newEndY < SHBD->GetSHBDSize())
    {
        door->startPos.x = newStartX;
        door->startPos.y = newStartY;
        door->endPos.x = newEndX;
        door->endPos.y = newEndY;
        
        CreateSBINode();
    }
}


