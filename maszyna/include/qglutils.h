#ifndef qglutilsH
#define qglutilsH


#include "../commons.h"
#include "../commons_usr.h"

// *****************************************************************************
//
// *****************************************************************************
inline void ENTEX(bool state)
{

	if (state == false) glDisable(GL_TEXTURE_2D);
	 else glEnable(GL_TEXTURE_2D);

}

//---------------------------------------------------------------------------
#endif
