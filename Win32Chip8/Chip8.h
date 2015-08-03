#include <stdio.h>
#include <stdlib.h>
#include <map>

#pragma once

class Chip8
{
public:
	unsigned char drawFlag;

	Chip8();
	~Chip8();

	void initialize();
	void loadGame(PWSTR game);
	void emulateCycle();
	void OnKeyDown(WPARAM wParam);
	void OnKeyUp(WPARAM wParam);
	void DrawScreen(HDC hdc);
	void decrementTimers();
	void SetHwnd(HWND hWnd);
	void UpdateScreen();
	
private:
	//0x000 - 0x1FF - Chip 8 interpreter(contains font set in emu)
	//0x050 - 0x0A0 - Used for the built in 4x5 pixel font set(0 - F)
	//0x200 - 0xFFF - Program ROM and work RAM
	unsigned short I;
	unsigned short pc;
	unsigned short opcode;
	unsigned char memory[4096];
	unsigned char V[16];
	unsigned char gfx[64 * 32]; 
	unsigned char delay_timer;
	unsigned char sound_timer;
	unsigned short stack[16];
	unsigned short sp; 
	unsigned char key[16];
	unsigned char keyPressed;

	const void(*cpuTable[16])();
	const void(*cpuTable0000[2])();
	const void(*cpuTable8000[9])();
	const void(*cpuTableE000[2])();
	const void(*cpuTableF000[9])();

	const void _0000();
	const void _8000();
	const void _E000();
	const void _F000();

	const void _0000();
	const void _1NNN();
	const void _2NNN();
	const void _3XNN();
	const void _4XNN();
	const void _5XY0();
	const void _6XNN();
	const void _7XNN();
	const void _8000();
	const void _9XY0();
	const void _ANNN();
	const void _BNNN();
	const void _CXNN();
	const void _DXYN();
	const void _E000();
	const void _F000();

	HWND hWnd;

	static const std::map<unsigned char, int> char_map;

	static const unsigned char chip8_fontset[80];
};

