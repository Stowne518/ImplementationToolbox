#pragma once
#pragma comment(lib, "D3dx9")
#include <D3dx9tex.h>
#include <iostream>

// See imgui wiki for function example: https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples
bool displayCentralSquareLogo(LPDIRECT3DDEVICE9 g_pd3dDevice, const char* filename, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height);