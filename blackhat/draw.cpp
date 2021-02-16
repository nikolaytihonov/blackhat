#include "draw.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

Draw* g_pDraw = NULL;

void __stdcall EndScene_Hook(IDirect3DDevice9* pDevice)
{
	g_pDraw->Render(pDevice);
}

Draw::Draw()
	: m_pRenderClk(NULL)
{
}

void Draw::Init()
{
	g_pDraw = this;
	m_pDevice = NULL;

	HMODULE hD3D9 = GetModuleHandle("d3d9.dll");
	LPVOID pEndScene = (char*)hD3D9 + ENDSCENE_OFFSET;
	
	m_EndScene.set(Hook::HookFunction(pEndScene, EndScene_Hook, &m_pHook));
}

void Draw::Exit()
{
	m_pHook->DoUnHook();

	m_pLine->Release();
	m_pFont->Release();
}

void Draw::InitDirectX()
{
	D3DXCreateLine(m_pDevice, &m_pLine);
	D3DXCreateFontA(m_pDevice, DRAW_FONT_SIZE, 0, FW_DONTCARE, 0, FALSE, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"),
		&m_pFont);
}

void Draw::SetRenderCallback(IRenderCallback* pClk)
{
	m_pRenderClk = pClk;
}

void Draw::Render(IDirect3DDevice9* pDevice)
{
	if (!m_pDevice)
	{
		m_pDevice = pDevice;
		InitDirectX();
	}

	m_iPrintY = PRINT_START_Y;
	
	if (m_pRenderClk)
		m_pRenderClk->Render(pDevice);
	m_EndScene.get()(pDevice);
}

void Draw::DrawLines(D3DXVECTOR2* points, int nPoints, D3DCOLOR color)
{
	m_pLine->Begin();
	m_pLine->Draw(points, nPoints, color);
	m_pLine->End();
}

void Draw::DrawLine(int x1, int y1, int x2, int y2, D3DCOLOR color)
{
	D3DXVECTOR2 points[2];

	points[0] = D3DXVECTOR2((float)x1,(float)y1);
	points[1] = D3DXVECTOR2((float)x2, (float)y2);
	
	DrawLines(points, 2, color);
}

void Draw::GetFontDrawRect(LPRECT pRect, int x, int y, unsigned int nChars)
{
	SetRect(pRect, x, y, x + nChars * 32, y + DRAW_FONT_SIZE * 2);
}

void Draw::DrawTextA(int x, int y, const char* pText, D3DCOLOR color)
{
	RECT rect;
	INT nLen = strlen(pText);
	GetFontDrawRect(&rect, x, y, (unsigned int)nLen);
	m_pFont->DrawTextA(NULL, pText, nLen, &rect, DT_LEFT | DT_TOP, color);
}

void Draw::DrawTextW(int x, int y, const wchar_t* pText, D3DCOLOR color)
{
	RECT rect;
	INT nLen = wcslen(pText);
	GetFontDrawRect(&rect, x, y, (unsigned int)nLen);
	m_pFont->DrawTextW(NULL, pText, nLen, &rect, DT_LEFT | DT_TOP, color);
}

void Draw::Print(const char* pFmt, ...)
{
	char szLine[256];
	va_list ap;

	va_start(ap, pFmt);
	vsnprintf_s(szLine, sizeof(szLine), pFmt, ap);
	va_end(ap);

	DrawTextA(PRINT_START_X, m_iPrintY, szLine, PRINT_COLOR);
	m_iPrintY += DRAW_FONT_SIZE + 2;
}