// Copyright (c) 1997-2024 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// curvemap.h -- include file for curve and performance map

#ifndef _CURVEMAP_H
#define _CURVEMAP_H

enum PMSPEED { MIN, RATED, MAX };

class PMACCESS
{
public:
    PMACCESS();
    ~PMACCESS();
    RC pa_Init(const PERFORMANCEMAP* pPM, record* pParent, const char* tag, double capRef);

    double pa_GetSpeedF(PMSPEED s) const { return s==PMSPEED::MIN ? pa_speedFMin : s==PMSPEED::MAX ? 1. : pa_speedFRated; }
    double pa_GetSpeedFMin() const { return pa_GetSpeedF(PMSPEED::MIN); };

    RC pa_GetCapInp(float tdb, float speedF, float& cap, float& inp);
    RC pa_GetCapInpRatios(float tdb, float speedF, double& capRat, double& inpRat);
    RC pa_GetRatedCapCOP(float tdb, float& cap, float& COP, PMSPEED whichSpeed = PMSPEED::RATED);
    double pa_GetRatedFanFlowFactor(float speedF);

    record* pa_pParent;     // owner 
    const class PERFORMANCEMAP* pa_pPERFORMANCEMAP;     // source PERFORMANCEMAP

    class Btwxt::RegularGridInterpolator* pa_pRGI; // associated Btwxt interpolator

    std::vector<double> pa_vTarget;
    std::vector<double> pa_vResult;

    double pa_capRef;       // reference capacity, Btuh

    double pa_tdbRated;

    double pa_speedFMin;    // minimum speed fraction
    double pa_speedFRated;  // speed fraction for rated values

};      // class PMACCESS


#endif	// _CURVEMAP_H

