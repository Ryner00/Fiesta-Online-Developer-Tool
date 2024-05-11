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

#ifndef NSBD3D10IMPLEMENTATION_H
#define NSBD3D10IMPLEMENTATION_H

#include "NiD3D10BinaryShaderLibLibType.h"
#include "NSBD3D10Requirements.h"
#include "NSBD3D10RenderStates.h"
#include "NSBD3D10StateGroup.h"
#include "NSBD3D10ConstantMap.h"
#include "NSBD3D10UserDefinedDataSet.h"

class NSBD3D10Pass;
NiSmartPointer(NiD3D10BinaryShader);

class NID3D10BINARYSHADERLIB_ENTRY NSBD3D10Implementation : public NiMemObject
{
public:
    NSBD3D10Implementation();
    ~NSBD3D10Implementation();

    // Name
    const char* GetName();
    void SetName(const char* pcName);
    const char* GetDesc();
    void SetDesc(const char* pcDesc);

    // User-defined class name
    const char* GetClassName();
    void SetClassName(const char* pcClassName);

    // Requirements
    NSBD3D10Requirements* GetRequirements();

    // PackingDef
    const char* GetPackingDef();
    void SetPackingDef(const char* pcPackingDef);

    // RenderStateGroup
    NSBD3D10StateGroup* GetRenderStateGroup();

    // Constant Maps
    unsigned int GetVertexConstantMapCount();
    unsigned int GetGeometryConstantMapCount();
    unsigned int GetPixelConstantMapCount();
    NSBD3D10ConstantMap* GetVertexConstantMap(unsigned int uiIndex);
    NSBD3D10ConstantMap* GetGeometryConstantMap(unsigned int uiIndex);
    NSBD3D10ConstantMap* GetPixelConstantMap(unsigned int uiIndex);

    // Passes
    unsigned int GetPassCount();
    NSBD3D10Pass* GetPass(unsigned int uiIndex, bool bCreate = true);

    // User defined data set
    NSBD3D10UserDefinedDataSet* GetUserDefinedDataSet();
    void SetUserDefinedDataSet(NSBD3D10UserDefinedDataSet* pkUDDSet);

    // *** begin Emergent internal use only
    // Index
    unsigned int GetIndex();
    void SetIndex(unsigned int uiIndex);

    void SetSoftwareVP(bool bSoftwareVP);
    bool GetSoftwareVP() const;

    bool SetupNiD3D10BinaryShader(NiD3D10BinaryShader& kShader, 
        NiShaderDesc* pkShaderDesc);
    bool ReleaseShader();

    // Serialization
    bool SaveBinary(NiBinaryStream& kStream);
    bool LoadBinary(NiBinaryStream& kStream);

#if defined(_DEBUG)
    void Dump(FILE* pf);
#endif  //#if defined(_DEBUG)
    // *** end Emergent internal use only

protected:
    bool SaveBinaryRequirements(NiBinaryStream& kStream);
    bool SaveBinaryPackingDef(NiBinaryStream& kStream);
    bool SaveBinaryPasses(NiBinaryStream& kStream);

    bool LoadBinaryRequirements(NiBinaryStream& kStream);
    bool LoadBinaryPackingDef(NiBinaryStream& kStream);
    bool LoadBinaryPasses(NiBinaryStream& kStream);

    char* m_pcName;
    char* m_pcDesc;
    char* m_pcClassName;

    unsigned int m_uiIndex;

    NSBD3D10Requirements* m_pkRequirements;

    // User defined data set
    NSBD3D10UserDefinedDataSetPtr m_spUserDefinedDataSet;

    // Packing definition name
    char* m_pcPackingDef;


    // 'Global' render state settings for the shader
    NSBD3D10StateGroup* m_pkRenderStateGroup;
    //  'Global' vertex shader mapping for the shader
    NiTPrimitiveSet<NSBD3D10ConstantMap*> m_kVertexShaderConstantMaps;
    //  'Global' geometry shader mapping for the shader
    NiTPrimitiveSet<NSBD3D10ConstantMap*> m_kGeometryShaderConstantMaps;
    //  'Global' pixel shader mapping for the shader
    NiTPrimitiveSet<NSBD3D10ConstantMap*> m_kPixelShaderConstantMaps;

    NiTPrimitiveArray<NSBD3D10Pass*> m_kPasses;

    NiD3D10BinaryShaderPtr m_spShader;

    bool m_bSoftwareVP;
};

#include "NSBD3D10Implementation.inl"

#endif  //NSBD3D10IMPLEMENTATION_H
