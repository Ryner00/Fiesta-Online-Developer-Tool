#pragma once
#include <NiMain.h>
#include <SHN/SHNStruct.h>
#include <vector>
#include <string>
#include <iostream>

NiSmartPointer(ShineBuildingInfo);

struct SBIDoorBlock
{
    std::string blockName;
    NiPoint2 startPos;
    NiPoint2 endPos;
    std::vector<uint8_t> data;
    NiColorA color;
    bool visible;
    float height;
    
    SBIDoorBlock()
    {
        blockName = "";
        startPos = NiPoint2(0, 0);
        endPos = NiPoint2(0, 0);
        color = NiColorA(1.0f, 0.0f, 0.0f, 0.7f);
        visible = true;
        height = 0.1f;
    }
    
    int GetWidth() const { return static_cast<int>(endPos.x - startPos.x + 1); }
    int GetHeight() const { return static_cast<int>(endPos.y - startPos.y + 1); }
    int GetDataSize() const { return (GetWidth() / 8) * GetHeight(); }
    int GetPaddedWidth() const { 
        int width = GetWidth();
        if (width % 8 != 0) {
            width = ((width / 8) + 1) * 8;
        }
        return width;
    }
    int GetActualDataSize() const { return (GetPaddedWidth() / 8) * GetHeight(); }
};

class ShineBuildingInfo : public NiRefObject
{
public:
    ShineBuildingInfo() = default;
    
    bool Load(MapInfo* Info);
    bool Save(std::string Path);
    void AddDoor(const SBIDoorBlock& door);
    void RemoveDoor(int index);
    SBIDoorBlock* GetDoor(int index);
    size_t GetDoorCount() const { return m_doors.size(); }
    void CreateEmpty();
    bool UpdateDoorData(int doorIndex, int x, int y, bool value);
    bool IsDoorPixelSet(int doorIndex, int x, int y);
    
    std::vector<char> GetSBIData() { return m_rawData; }
    void UpdateSBIData(std::vector<char> NewData) 
    {
        m_rawData = NewData;
        _HadDirectDataUpdate = true;
    }
    bool HadDirectUpdate()
    {
        bool b = _HadDirectDataUpdate;
        _HadDirectDataUpdate = false;
        return b;
    }
    
private:
    std::vector<SBIDoorBlock> m_doors;
    std::string m_mapName;
    NiPoint2 m_mapSize;
    std::vector<char> m_rawData;
    bool _HadDirectDataUpdate = false;
    
    void ParseRawData();
    void GenerateRawData();
};