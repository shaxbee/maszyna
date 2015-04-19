#ifndef _World_H_
#define _World_H_

#include "commons.h"
#include "commons_usr.h"
#include "cstring.h"
class TWorld
{
public:
    void  InOutKey();
    bool  Init();
	bool  Resize(int x, int y);
	bool  Render(double dt, int id);
	bool  Update(double dt);
	void  OnKeyDown(int cKey, int scancode, int action, int mode, std::string command);
	void  OnKeyUp(int cKey, int scancode, int action, int mode, std::string command);
	void  OnMouseMove(double x, double y);
	void  OnMouseWheel(float zDelta);
	void  OnRButtonDown(int X, int Y);
	void  OnLButtonDown(int X, int Y);
	bool  TWorld::RenderLoader(HDC hDC, int node, std::string text);
	GLvoid glPrint(CString fmt);
    TWorld();
   ~TWorld();
  
private:

    bool  RenderX();

public:
	
};

#endif
