#include "plugin.h"
#include "CCoronas.h"
#include "CCamera.h"
#include "CSprite.h"
#include "CDraw.h"
#include "H:\CParticlePort\CParticleVC\CParticleVC\debugmenu_public.h"
#include "CWeather.h"
#include <CGame.h>

using namespace plugin;

const int SPRITEBUFFERSIZE = 64;
const float SCREEN_WIDTH = 640.0f;
const float SCREEN_HEIGHT = 480.0f;

bool ToggleRainbow = false;
float m_f2DNearScreenZ;
float m_f2DFarScreenZ;
float m_fRecipNearClipPlane;
int32_t m_bFlushSpriteBufferSwitchZTest = 0;
int32_t nSpriteBufferIndex = 0;
RwIm2DVertex SpriteBufferVerts[SPRITEBUFFERSIZE * 6];
RwIm2DVertex verts[4];

#define RwIm2DGetNearScreenZ() (RWSRCGLOBAL(dOpenDevice).zBufferNear)
#define RwIm2DGetFarScreenZ() (RWSRCGLOBAL(dOpenDevice).zBufferFar)
#define RwV3dAdd(o, a, b) RwV3dAddMacro(o, a, b)

void InitSpriteBuffer() {
    m_f2DNearScreenZ = RwIm2DGetNearScreenZ();
    m_f2DFarScreenZ = RwIm2DGetFarScreenZ();
}

void FlushSpriteBuffer() {
    if (nSpriteBufferIndex > 0) {
        if (m_bFlushSpriteBufferSwitchZTest) {
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
            RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, SpriteBufferVerts, nSpriteBufferIndex * 6);
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
        } else {
            RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, SpriteBufferVerts, nSpriteBufferIndex * 6);
        }
        nSpriteBufferIndex = 0;
    }
}

void RenderBufferedOneXLUSprite(float x, float y, float z, float w, float h, uint8_t r, uint8_t g, uint8_t b, int16_t intens, float recipz, uint8_t a) {
    m_bFlushSpriteBufferSwitchZTest = 0;

    float xs[4];
    float ys[4];
    float us[4];
    float vs[4];
    int i;

    xs[0] = x - w; us[0] = 0.0f;
    xs[1] = x - w; us[1] = 0.0f;
    xs[2] = x + w; us[2] = 1.0f;
    xs[3] = x + w; us[3] = 1.0f;

    ys[0] = y - h; vs[0] = 0.0f;
    ys[1] = y + h; vs[1] = 1.0f;
    ys[2] = y + h; vs[2] = 1.0f;
    ys[3] = y - h; vs[3] = 0.0f;

    for (i = 0; i < 4; i++) {
        if (xs[i] < 0.0f) {
            us[i] = -xs[i] / (2.0f * w);
            xs[i] = 0.0f;
        }
        if (xs[i] > SCREEN_WIDTH) {
            us[i] = 1.0f - (xs[i] - SCREEN_WIDTH) / (2.0f * w);
            xs[i] = SCREEN_WIDTH;
        }
        if (ys[i] < 0.0f) {
            vs[i] = -ys[i] / (2.0f * h);
            ys[i] = 0.0f;
        }
        if (ys[i] > SCREEN_HEIGHT) {
            vs[i] = 1.0f - (ys[i] - SCREEN_HEIGHT) / (2.0f * h);
            ys[i] = SCREEN_HEIGHT;
        }
    }


 float screenz = m_f2DNearScreenZ + (z - CDraw::ms_fNearClipZ) * (m_f2DFarScreenZ - m_f2DNearScreenZ) * CDraw::ms_fFarClipZ /
        ((CDraw::ms_fFarClipZ - CDraw::ms_fNearClipZ) * z);

    RwIm2DVertex* vert = &SpriteBufferVerts[nSpriteBufferIndex * 6];
    static int indices[6] = { 0, 1, 2, 3, 0, 2 };
    for (i = 0; i < 6; i++) {
        RwIm2DVertexSetScreenX(&vert[i], xs[indices[i]]);
        RwIm2DVertexSetScreenY(&vert[i], ys[indices[i]]);
        RwIm2DVertexSetScreenZ(&vert[i], screenz);
        RwIm2DVertexSetCameraZ(&vert[i], z);
        RwIm2DVertexSetRecipCameraZ(&vert[i], recipz);
        RwIm2DVertexSetIntRGBA(&vert[i], r * intens >> 8, g * intens >> 8, b * intens >> 8, a);
        RwIm2DVertexSetU(&vert[i], us[indices[i]], recipz);
        RwIm2DVertexSetV(&vert[i], vs[indices[i]], recipz);
    }
    nSpriteBufferIndex++;
    if (nSpriteBufferIndex >= SPRITEBUFFERSIZE)
        FlushSpriteBuffer();
}

const CRGBA RAINBOW_LINES_COLOR[]{
    {30, 0, 0, 255},   
    {30, 15, 0, 255},  
    {30, 30, 0, 255},   
    {10, 30, 10, 255},  
    {0,  30, 30, 255},    
    {0,  0,  30, 255},   
    {30, 0, 30, 255}    
};

void Render_MaybeRenderRainbows() {
    float szx, szy;
    RwV3d screenpos;
    RwV3d worldpos;
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
    if (!CGame::CanSeeOutSideFromCurrArea())
        return;
    InitSpriteBuffer();
    RwV3d campos = TheCamera.GetPosition().ToRwV3d();
    // Радуга ->
    if (CWeather::Rainbow != 0.0f) {
        CVector2D rblineSizeScr;
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(gpCoronaTexture[0]));
        for (int i = 0; i < 7; i++) {
            const auto& clr = RAINBOW_LINES_COLOR[i];
            const auto offset = CVector{
                (float)i * 1.5f,
                100.f,
                5.f
            };
            RwV3dAdd(&worldpos, &campos, &offset);
            if (CSprite::CalcScreenCoors(worldpos, &screenpos, &rblineSizeScr.x, &rblineSizeScr.y, false, true))
                RenderBufferedOneXLUSprite(screenpos.x, screenpos.y, screenpos.z,
                    2.0f * rblineSizeScr.x, 50.0f * rblineSizeScr.y,
                    clr.r * CWeather::Rainbow, clr.g * CWeather::Rainbow, clr.b * CWeather::Rainbow,
                    255, 1.0f / screenpos.z, 255);
        }
        FlushSpriteBuffer();
    }
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}

class AllSevenRainbowColorsSA {
public:
    AllSevenRainbowColorsSA() {
        Events::initGameEvent += []() {
            //patch::RedirectJump(0x714371, Render_MaybeRenderRainbows);
            memset((void*)0x714371, 0x90, 5);
        };

        //	if (DebugMenuLoad()) {
        //		DebugMenuAddVarBool8("Rainbow", "Toggle rainbow", (int8_t*)&ToggleRainbow, nullptr);
        //		DebugMenuAddFloat32("Rainbow", "RainbowLevel", &CWeather::Rainbow, nullptr, 0.0f, 0.1f, 1.0f);
        //}

        CMovingThings__Render_BeforeClouds += []() {
            Render_MaybeRenderRainbows();
        };
    }
} allSevenRainbowColorsSA;
