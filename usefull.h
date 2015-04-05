//---------------------------------------------------------------------------
#ifndef UsefullH
#define UsefullH

#include "dumb3d.h"
#include "Logs.h"

//#define	B1(t)     (t*t*t)
//#define	B2(t)     (3*t*t*(1-t))
//#define	B3(t)     (3*t*(1-t)*(1-t))
//#define	B4(t)     ((1-t)*(1-t)*(1-t))
// Ra: to jest mocno nieoptymalne: 10+3*4=22 mnożenia, 6 odejmowań, 3*3=9 dodawań
// Ra: po przeliczeniu współczynników mamy: 3*3=9 mnożeń i 3*3=9 dodawań
//#define	Interpolate(t,p1,cp1,cp2,p2)     (B4(t)*p1+B3(t)*cp1+B2(t)*cp2+B1(t)*p2)

// Ra: "delete NULL" nic nie zrobi, więc "if (a!=NULL)" jest zbędne
//#define SafeFree(a) if (a!=NULL) free(a)
#define SafeDelete(a) \
    {                 \
        delete (a);   \
        a = NULL;     \
    }
#define SafeDeleteArray(a) \
    {                      \
        delete[](a);       \
        a = NULL;          \
    }

#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define DegToRad(a) ((M_PI / 180.0) * (a)) //(a) w nawiasie, bo może być dodawaniem
#define RadToDeg(r) ((180.0 / M_PI) * (r))

#define Fix(a, b, c) \
    {                \
        if (a < b)   \
            a = b;   \
        if (a > c)   \
            a = c;   \
    }

#define asModelsPath AnsiString("models\\")
#define asSceneryPath AnsiString("scenery\\")
//#define asTexturePath AnsiString("textures\\")
//#define asTextureExt AnsiString(".bmp")
#define szSceneryPath "scenery\\"
#define szTexturePath "textures\\"
//#define szDefaultTextureExt ".dds"

//#define DevelopTime     //FIXME
//#define EditorMode

//---------------------------------------------------------------------------
#endif
