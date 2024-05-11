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


// Since this function is called per triangle in many cases, it is worthwhile
// to attempt to inline it. However, it is also a large function. It is known 
// that the Xbox 360 compiler will not inline the function, so it is forced 
// inline.
#if defined(_XENON)
__forceinline
#else
inline
#endif
bool NiCollisionUtils::IntersectTriangle(const NiPoint3& kOrigin, 
    const NiPoint3& kDir, const NiPoint3& kV1, const NiPoint3& kV2, 
    const NiPoint3& kV3, bool bCull, NiPoint3& kIntersect, float& r, float& s, 
    float& t)
{
    // All input quantities are in model space of the NiTriBasedGeom object.
    // Input:
    //     ray (kOrigin+T*kDir), 
    //     triangle vertices (kV1, kV2, kV3)
    //     backface culling is performed when bCull=true
    // Return value:  true iff the ray intersects the triangle
    // Output (valid when return value is true):
    //     intersection of ray and triangle (kIntersect)
    //     kIntersect = kOrigin+r*kDir
    //     kIntersect = kV1+s*(kV2-kV1)+t*(kV3-kV1)

    // From "Real-Time Rendering" which references Moller, Tomas & Trumbore, 
    // "Fast, Minimum Storage Ray-Triangle Intersection", Journal of Graphics 
    // Tools, vol. 2, no. 1, pp 21-28, 1997. With some modifications to the 
    // non-culling case by Michael Mounier.

    const float fTolerance = 1e-05f;

    NiPoint3 kEdge1 = kV2 - kV1;
    NiPoint3 kEdge2 = kV3 - kV1;

    NiPoint3 kPt = kDir.Cross(kEdge2);

    float fDet = kEdge1 * kPt;
    if (fDet >= fTolerance) // Determinant is positive.
    {
        NiPoint3 kS = kOrigin - kV1;
        s = kS * kPt;

        if (s < 0.0f || s > fDet)
            return false;

        NiPoint3 q = kS.Cross(kEdge1);
        t = kDir * q;

        if (t < 0.0f || s + t > fDet)
            return false;

        r = kEdge2 * q;

        if (r < 0.0f)
            return false;
    }
    else if (fDet <= -fTolerance && !bCull) // Determinant is negative.
    {
        NiPoint3 kS = kOrigin - kV1;
        s = kS * kPt;

        if (s > 0.0f || s < fDet)
            return false;

        NiPoint3 q = kS.Cross(kEdge1);
        t = kDir * q;

        if (t > 0.0f || s + t < fDet)
            return false;

        r = kEdge2 * q;

        if (r > 0.0f)
            return false;
    }
    else    // Parallel ray or culled.
    {
        return false;
    }

    float inv_det = 1.0f / fDet;

    s *= inv_det;
    t *= inv_det;
    r *= inv_det;

    kIntersect = kOrigin + r * kDir;

    return true;
}
