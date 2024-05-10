// Copyright (c) 1997-2024 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// curvemap.h -- include file for curve and performance map

#ifndef _CURVEMAP_H
#define _CURVEMAP_H

class PMACCESS
{
public:
    PMACCESS();
    ~PMACCESS();
    RC pa_Init(const PERFORMANCEMAP* pPM, record* pParent, const char* tag, double capRef);

    enum { pmSPEEDMIN, pmSPEEDRATED, pmSPEEDMAX };
    double pa_GetSpeedF(int s) const { return s==pmSPEEDMIN ? pa_speedFMin : s==pmSPEEDMAX ? 1. : pa_speedFRated; }
    double pa_GetSpeedFMin() const { return pa_GetSpeedF(pmSPEEDMIN); };

    RC pa_GetCapInp(float tdb, float speedF, float& cap, float& inp);
    RC pa_GetCapRatEIR(float tdb, float speedF, double& capRat, double& eir);
    RC pa_GetRatedCapCOP(float tdb, float& cap, float& COP, int whichSpeed = pmSPEEDRATED);

    const class PERFORMANCEMAP* pa_pPERFORMANCEMAP;
    class Btwxt::RegularGridInterpolator* pa_pRGI; // associated Btwxt interpolator

    std::vector<double> pa_vTarget;
    std::vector<double> pa_vResult;

    double pa_capRef;       // reference capacity, Btuh
    double pa_speedFMin;    // minimum speed fraction
    double pa_speedFRated;  // speed fraction for rated values
};      // class PMACCESS


#endif	// _CURVEMAP_H

