// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not 
// be copied or disclosed except in accordance with the terms of that 
// agreement.
//
//      Copyright (c) 1996-2007 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Chapel Hill, North Carolina 27517
// http://www.emergent.net
//---------------------------------------------------------------------------
// NSBD3D10ConstantMap inline functions
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
inline NSBD3D10ConstantMap::NSBD3D10CM_Entry::NSBD3D10CM_Entry() :
    NiShaderConstantMapEntry()
{
}
//---------------------------------------------------------------------------
inline unsigned int NSBD3D10ConstantMap::GetProgramType()
{
    return m_uiProgramType;
}
//---------------------------------------------------------------------------
inline void NSBD3D10ConstantMap::SetProgramType(unsigned int uiProgramType)
{
    m_uiProgramType = uiProgramType;
}
//---------------------------------------------------------------------------
