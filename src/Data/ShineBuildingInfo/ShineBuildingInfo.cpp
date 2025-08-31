#include "ShineBuildingInfo.h"
#include <Logger.h>
#include <fstream>
#include <PgUtil.h>
#include <filesystem>

bool ShineBuildingInfo::Load(MapInfo* Info) 
{
    std::string SBIFilePath = PgUtil::PathFromClientFolder(PgUtil::GetMapFolderPath(Info->KingdomMap, Info->MapFolderName) + Info->MapFolderName + ".sbi");
    std::ifstream SBIFile;
    SBIFile.open(SBIFilePath, std::ios::binary);
    if (!SBIFile.is_open() || SBIFile.bad() || SBIFile.fail())
    {
        LogInfo("No SBI file found for " + Info->MapFolderName + ", creating empty");
        CreateEmpty();
        return true;
    }
    
    uint32_t totalHeadCount;
    SBIFile.read(reinterpret_cast<char*>(&totalHeadCount), sizeof(totalHeadCount));
    
    if (totalHeadCount > 32)
    {
        LogError("Invalid SBI file: too many doors (" + std::to_string(totalHeadCount) + ")");
        SBIFile.close();
        return false;
    }
    
    m_doors.clear();
    m_doors.reserve(totalHeadCount);
    
    for (uint32_t i = 0; i < totalHeadCount; i++)
    {
        SBIDoorBlock door;
        
        char nameBuffer[32] = {0};
        SBIFile.read(nameBuffer, 32);
        door.blockName = std::string(nameBuffer);
        
        uint32_t startX, startY, endX, endY, dataSize, address;
        SBIFile.read(reinterpret_cast<char*>(&startX), sizeof(startX));
        SBIFile.read(reinterpret_cast<char*>(&startY), sizeof(startY));
        SBIFile.read(reinterpret_cast<char*>(&endX), sizeof(endX));
        SBIFile.read(reinterpret_cast<char*>(&endY), sizeof(endY));
        SBIFile.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
        SBIFile.read(reinterpret_cast<char*>(&address), sizeof(address));
        
        door.startPos = NiPoint2(static_cast<float>(startX), static_cast<float>(startY));
        door.endPos = NiPoint2(static_cast<float>(endX), static_cast<float>(endY));
        
        if (door.GetWidth() <= 0 || door.GetHeight() <= 0)
        {
            LogError("Invalid door dimensions in SBI file: width=" + std::to_string(door.GetWidth()) + " height=" + std::to_string(door.GetHeight()));
            continue;
        }
        
        if (door.GetWidth() % 8 != 0)
        {
            LogError("Invalid door width in SBI file - must be divisible by 8: width=" + std::to_string(door.GetWidth()));
            continue;
        }
        
        if (door.GetDataSize() != static_cast<int>(dataSize))
        {
            LogError("Invalid door data size in SBI file: expected=" + std::to_string(door.GetDataSize()) + " actual=" + std::to_string(dataSize));
            continue;
        }
        
        door.data.resize(dataSize);
        door.height = 0.1f + (i * 0.1f);
        
        door.color = NiColorA(0.0f, 0.0f, 1.0f, 0.7f);
        
        m_doors.push_back(door);
    }
    
    m_rawData.resize(SBIFile.tellg());
    SBIFile.seekg(0);
    SBIFile.read(m_rawData.data(), m_rawData.size());
    
    for (size_t i = 0; i < m_doors.size(); i++)
    {
        uint32_t address;
        SBIFile.seekg(sizeof(uint32_t) + i * (32 + 6 * sizeof(uint32_t)) + 32 + 4 * sizeof(uint32_t) + sizeof(uint32_t));
        SBIFile.read(reinterpret_cast<char*>(&address), sizeof(address));
        
        SBIFile.seekg(address);
        SBIFile.read(reinterpret_cast<char*>(m_doors[i].data.data()), m_doors[i].data.size());
    }
    
    SBIFile.close();
    m_mapName = Info->MapFolderName;
    return true;
}

bool ShineBuildingInfo::Save(std::string Path)
{
    if (std::filesystem::exists(Path))
    {
        if (std::filesystem::exists(Path + ".bak"))
            std::filesystem::remove(Path + ".bak");
        std::filesystem::copy(Path, Path + ".bak");
    }
    
    std::ofstream SBIFile;
    SBIFile.open(Path, std::ios::binary);
    if (!SBIFile.is_open())
    {
        LogError("Failed to open and save SBI for:\n" + Path);
        return false;
    }
    
    uint32_t totalHeadCount = static_cast<uint32_t>(m_doors.size());
    SBIFile.write(reinterpret_cast<const char*>(&totalHeadCount), sizeof(totalHeadCount));
    
    uint32_t currentAddress = sizeof(uint32_t) + totalHeadCount * (32 + 6 * sizeof(uint32_t));
    
    for (const auto& door : m_doors)
    {
        char nameBuffer[32] = {0};
        strncpy(nameBuffer, door.blockName.c_str(), 31);
        SBIFile.write(nameBuffer, 32);
        
        uint32_t startX = static_cast<uint32_t>(door.startPos.x);
        uint32_t startY = static_cast<uint32_t>(door.startPos.y);
        uint32_t endX = static_cast<uint32_t>(door.endPos.x);
        uint32_t endY = static_cast<uint32_t>(door.endPos.y);
        uint32_t dataSize = static_cast<uint32_t>(door.data.size());
        
        SBIFile.write(reinterpret_cast<const char*>(&startX), sizeof(startX));
        SBIFile.write(reinterpret_cast<const char*>(&startY), sizeof(startY));
        SBIFile.write(reinterpret_cast<const char*>(&endX), sizeof(endX));
        SBIFile.write(reinterpret_cast<const char*>(&endY), sizeof(endY));
        SBIFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
        SBIFile.write(reinterpret_cast<const char*>(&currentAddress), sizeof(currentAddress));
        
        currentAddress += dataSize;
    }
    
    for (const auto& door : m_doors)
    {
        SBIFile.write(reinterpret_cast<const char*>(door.data.data()), door.data.size());
    }
    
    SBIFile.close();
    return true;
}

void ShineBuildingInfo::AddDoor(const SBIDoorBlock& door)
{
    if (m_doors.size() >= 32)
        return;
    m_doors.push_back(door);
}

void ShineBuildingInfo::RemoveDoor(int index)
{
    if (index >= 0 && index < static_cast<int>(m_doors.size()))
    {
        m_doors.erase(m_doors.begin() + index);
    }
}

SBIDoorBlock* ShineBuildingInfo::GetDoor(int index)
{
    if (index >= 0 && index < static_cast<int>(m_doors.size()))
    {
        return &m_doors[index];
    }
    return nullptr;
}

void ShineBuildingInfo::CreateEmpty()
{
    m_doors.clear();
    m_mapName = "";
    m_rawData.clear();
}

bool ShineBuildingInfo::UpdateDoorData(int doorIndex, int x, int y, bool value)
{
    if (doorIndex < 0 || doorIndex >= static_cast<int>(m_doors.size()))
        return false;
        
    SBIDoorBlock& door = m_doors[doorIndex];
    if (x < 0 || x >= door.GetWidth() || y < 0 || y >= door.GetHeight())
        return false;
        
    int byteIndex = (x / 8) + y * (door.GetWidth() / 8);
    int bitIndex = x % 8;
    
    if (byteIndex >= static_cast<int>(door.data.size()))
        return false;
        
    if (value)
        door.data[byteIndex] |= (1 << bitIndex);
    else
        door.data[byteIndex] &= ~(1 << bitIndex);
        
    return true;
}

bool ShineBuildingInfo::IsDoorPixelSet(int doorIndex, int x, int y)
{
    if (doorIndex < 0 || doorIndex >= static_cast<int>(m_doors.size()))
        return false;
        
    const SBIDoorBlock& door = m_doors[doorIndex];
    if (x < 0 || x >= door.GetWidth() || y < 0 || y >= door.GetHeight())
        return false;
        
    int byteIndex = (x / 8) + y * (door.GetWidth() / 8);
    int bitIndex = x % 8;
    
    if (byteIndex >= static_cast<int>(door.data.size()))
        return false;
        
    return (door.data[byteIndex] >> bitIndex) & 1;
}