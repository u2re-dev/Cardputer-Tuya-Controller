#pragma once

//
#include <std/std.hpp>

//
void RGBtoHSV(float& fR, float& fG, float fB, float& fH, float& fS, float& fV) {
    float fCMax = std::max(std::max(fR, fG), fB);
    float fCMin = std::min(std::min(fR, fG), fB);
    float fDelta = fCMax - fCMin;
    
    if (fDelta > 0) {
        if(fCMax == fR) {
            fH = 60 * (fmod(((fG - fB) / fDelta), 6));
        } else if(fCMax == fG) {
            fH = 60 * (((fB - fR) / fDelta) + 2);
        } else if(fCMax == fB) {
            fH = 60 * (((fR - fG) / fDelta) + 4);
        }
        
        if(fCMax > 0) {
            fS = fDelta / fCMax;
        } else {
            fS = 0;
        }
        
        fV = fCMax;
    } else {
        fH = 0;
        fS = 0;
        fV = fCMax;
    }

    if(fH < 0) { fH = 360 + fH; }
}

//
void HSVtoRGB(float& fR, float& fG, float& fB, float& fH, float& fS, float& fV) {
    float fC = fV * fS; // Chroma
    float fHPrime = fmod(fH / 60.0, 6);
    float fX = fC * (1 - fabs(fmod(fHPrime, 2) - 1));
    float fM = fV - fC;
    
    if(0 <= fHPrime && fHPrime < 1) {
        fR = fC;
        fG = fX;
        fB = 0;
    } else if(1 <= fHPrime && fHPrime < 2) {
        fR = fX;
        fG = fC;
        fB = 0;
    } else if(2 <= fHPrime && fHPrime < 3) {
        fR = 0;
        fG = fC;
        fB = fX;
    } else if(3 <= fHPrime && fHPrime < 4) {
        fR = 0;
        fG = fX;
        fB = fC;
    } else if(4 <= fHPrime && fHPrime < 5) {
        fR = fX;
        fG = 0;
        fB = fC;
    } else if(5 <= fHPrime && fHPrime < 6) {
        fR = fC;
        fG = 0;
        fB = fX;
    } else {
        fR = 0;
        fG = 0;
        fB = 0;
    }
    
    fR += fM;
    fG += fM;
    fB += fM;
}

//
#include <std/string.hpp>

// DP=24
#ifdef ENABLE_ARDUINO
String HSV_TO_HEX_B(float H, float S, float V) {
    _String_<12> _HEX_ = "000000000000";
    _StringWrite_ _H_(_HEX_.bytes() + 0, 4);
    _StringWrite_ _S_(_HEX_.bytes() + 4, 4);
    _StringWrite_ _V_(_HEX_.bytes() + 8, 4);

    //
    _H_.atEnd(String(int(H), HEX));
    _S_.atEnd(String(int(S*1000.0), HEX));
    _V_.atEnd(String(int(V*1000.0), HEX));

    //
    return _HEX_.toString();
}
#endif
