#ifndef __DRAW_H
#define __DRAW_H

#include <d3d9.h>
#include <d3dx9.h>
#include "hook.h"

#define DRAW_FONT_SIZE 16

#define PRINT_START_X 2
#define PRINT_START_Y 32
#define PRINT_COLOR D3DCOLOR_RGBA(0,255,0,255)

#define ENDSCENE_OFFSET 0x2279F

class IRenderCallback
{
public:
	virtual void Render(IDirect3DDevice9* pDevice) = 0;
};

class Draw : public IRenderCallback
{
public:
	Draw();

	void Init();
	void Exit();
	
	void InitDirectX();

	void SetRenderCallback(IRenderCallback* pClk);
	virtual void Render(IDirect3DDevice9* pDevice);

	void DrawLines(D3DXVECTOR2* points, int nPoints, D3DCOLOR color);
	void DrawLine(int x1, int y1, int x2, int y2, D3DCOLOR color);

	void GetFontDrawRect(LPRECT pRect, int x, int y, unsigned int nChars);

	void DrawTextA(int x, int y, const char* pText, D3DCOLOR color);
	void DrawTextW(int x, int y, const wchar_t* pText, D3DCOLOR color);
	
	void Print(const char* pFmt, ...);
private:
	IDirect3DDevice9* m_pDevice;
	IRenderCallback* m_pRenderClk;
	
	FunctionPtr<void(__stdcall*)(IDirect3DDevice9*)> m_EndScene;
	Hook* m_pHook;

	LPD3DXLINE m_pLine;
	LPD3DXFONT m_pFont;

	int m_iPrintY;
};

extern Draw* g_pDraw;

#endif