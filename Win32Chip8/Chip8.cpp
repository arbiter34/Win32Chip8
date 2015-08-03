#include "stdafx.h"
#include "Chip8.h"
#include <stdio.h>
#include <stdlib.h>

Chip8::Chip8()
{
}


Chip8::~Chip8()
{
}

void Chip8::initialize() {
	pc = 0x200;
	opcode = 0;
	I = 0;
	sp = 0;

	for (int i = 0; i < 80; i++) {
		memory[i] = chip8_fontset[i];
	}

	for (int i = 0; i < 32 * 64; i++) {
		gfx[i] = 0;
	}

	for (int i = 0; i < 16; i++) {
		key[i] = 0;
	}
}

void Chip8::loadGame(PWSTR game) {
	FILE *in;
	long size;
	size_t res;
	
	res = _wfopen_s(&in, game, L"rb");
	if (res != 0) {
		return;
	}

	fseek(in, 0, SEEK_END);
	size = ftell(in);
	rewind(in);

	if (size < (4096 - 0x200)) {
		res = fread(&(memory[0x200]), 1, size, in);
		if (res != size) {
			return;
		}
	}

	fclose(in);
	return;

}

void __cdecl odprintf(const char *format, ...)
{
	char    buf[4096], *p = buf;
	va_list args;
	int     n;

	va_start(args, format);
	n = _vsnprintf(p, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
	va_end(args);

	p += (n < 0) ? sizeof buf - 3 : n;

	while (p > buf  &&  isspace(p[-1]))
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p = '\0';

	OutputDebugStringA(buf);
}

void Chip8::emulateCycle() {
	// Fetch Opcode
	opcode = memory[pc] << 8 | memory[pc + 1];
	odprintf("Opcode: 0x%x\n", opcode);
	// Decode Opcode
	switch (opcode & 0xF000) {
		case 0x0000:
			switch (opcode & 0x000F) {
				case 0x0000:			// 0x00E0: Clears the screen
					for (int i = 0; i < 32; i++) {
						for (int j = 0; j < 64; j++) {
							gfx[i * 64 + j] = 0;
						}
					}
					drawFlag = true;
					break;
				case 0x000E:			// 0x00EE: Returns from subroutine
					sp--;
					pc = stack[sp];
					break;
				default:
					break;
			}
			break;

		case 0x1000:				//1NNN: Jumps to address NNN
			pc = opcode & 0x0FFF; 
			pc -= 2;
			break;

		case 0x2000:				//2NNN: Calls subroutine at NNN
			stack[sp] = pc;
			sp++;
			pc = opcode & 0x0FFF;
			pc -= 2;
			break;

		case 0x3000:				//3XNN: If Vx == NN, skip next instruction 
			pc += V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF) ? 2 : 0;
			break;

		case 0x4000:				//4XNN: If Vx == NN, skip next instruction  
			pc += V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF) ? 2 : 0;
			break;

		case 0x5000:				//5XY0: If Vx == Vy, skip next instruction 
			pc += V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4] ? 2 : 0;
			break;

		case 0x6000:				//6XNN: Sets Vx to NN
			V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
			break;

		case 0x7000:				//7XNN: Adds NN to Vx
			V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
			break;

		case 0x8000:				
			switch (opcode & 0x000F) {
				case 0x0000:		//8XY0: Sets Vx to the value of Vy
					V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
					break;

				case 0x0001:		//8XY1: Sets Vx to Vx | Vy
					V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];
					break;

				case 0x0002:		//8XY2: Sets Vx to Vx & Vy
					V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
					break;

				case 0x0003:		//8XY3: Sets Vx to Vx ^ Vy
					V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
					break; 

				case 0x0004:		//8XY4: Adds Vy to Vx. Vf is set to 1 when there's a carry, and to 0 when there isn't
					if ((V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4]) > (1 << 8)) {
						V[0xF] = 1;
						V[(opcode & 0x0F00) >> 8] = (V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4]) - (1 << 8);
					}
					else {
						V[0xF] = 0;
						V[(opcode & 0x0F00) >> 8] += V[(opcode & 0x00F0) >> 4];
					}
					break;

				case 0x0005:		//8XY5: Vy is subtracted from Vx. Vf is set to 0 when there's a borrow, 1 when there isn't
					if (V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4]) {
						V[0xF] = 1;
					}
					else {
						V[0xF] = 0;
					}
					V[(opcode & 0x0F00) >> 8] -= V[(opcode & 0x00F0) >> 4];
					break;

				case 0x0006:		//8XY6: Shifts Vx right by one. V is set to the value of the least significant bit of Vx before the shift
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x0001;
					V[(opcode & 0x0F00) >> 8] >>= 1;
					break;

				case 0x0007:		//8XY7: Vy is subtracted from Vx. Vf is set to 0 when there's a borrow, 1 when there isn't
					if (V[(opcode & 0x00F0) >> 4] > V[(opcode & 0x0F00) >> 8]) {
						V[0xF] = 1;
					}
					else {
						V[0xF] = 0;
					}
					V[(opcode & 0x00F0) >> 4] -= V[(opcode & 0x0F00) >> 8];
					break;

				case 0x000E:		//8XYE: Shifts Vx left by one. V is set to the value of the most significant bit of Vx before the shift
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x8000;
					V[(opcode & 0x0F00) >> 8] <<= 1;
					break;
			}
			break;

		case 0x9000:				//9XY0: Skips the next instruction if Vx != Vy
			pc += V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4] ? 2 : 0;
			break;

		case 0xA000:				//ANNN: Sets I to Address NNN
			I = opcode & 0x0FFF;
			break;

		case 0xB000:				//BNNN: Jumps to the address NNN + V[0]
			pc = (opcode & 0x0FFF) + V[0];
			pc -= 2;
			break;

		case 0xC000:				//CXNN: Sets Vx to a random number, masked by NN
			V[(opcode & 0x0F00) >> 8] = (rand() % (1 << 8)) & (opcode & 0x00FF);
			break;

		case 0xD000: {				//DXYN: Sprites stored in memory at location in index register (i), maximum 8 bits wide.  Wraps around the screen.  
						 //If when drawn, clears a pixel, register VF is set to 1, else 0.  All drawing is XOR drawing (it toggles the screen pixels)
						 unsigned short x = V[(opcode & 0x0F00) >> 8];
						 unsigned short y = V[(opcode & 0x00F0) >> 4];
						 unsigned short n = (opcode & 0x000F);
						 unsigned char row;

						 V[0xF] = 0;

						 for (int i = 0; i < n; i++) {
							 row = memory[I + i];
							 for (int j = 0; j < 8; j++) {
								 int xcoord = x + j >= 64 ? ((x + j) - 64) : x + j;
								 int ycoord = y + i >= 32 ? ((y + i) - 32) : y + i;
								 if ((gfx[((ycoord)* 64) + (xcoord)] == 1) && (row & (0x80 >> j)) != 0) {
									 V[0xF] = 1;
								 }
								 gfx[((ycoord)* 64) + (xcoord)] ^= (row & (0x80 >> j)) != 0;
							 }
						 }
						 drawFlag = true;
						 break;
		}
		case 0xE000:
			switch (opcode & 0x000F) {
				case 0x000E:		//EX9E: Skips the next instruction if the key stored in VX is pressed.
					pc += key[V[(opcode & 0x0F00) >> 8]] != 0 ? 2 : 0;
					//key[V[(opcode & 0x0F00) >> 8]] = 0;
					break;

				case 0x0001:		//EXA1: Skips the next instruction if the key stored in VX isn't pressed.
					pc += key[V[(opcode & 0x0F00) >> 8]] == 0 ? 2 : 0;
					//key[V[(opcode & 0x0F00) >> 8]] = 0;
					break;
			}
			break;

		case 0xF000:
			switch (opcode & 0x00FF) {
				case 0x0007:		//FX07: Sets VX to the value of the delay timer
					V[(opcode & 0x0F00) >> 8] = delay_timer;
					break;

				case 0x000A:		//FX0A	A key press is awaited, and then stored in VX.
					keyPressed = 0;
					while (keyPressed == 0) {}
					V[(opcode & 0x0F00) >> 8] = keyPressed;					
					break;

				case 0x0015:		//FX15	Sets the delay timer to VX.
					delay_timer = V[(opcode & 0x0F00) >> 8];
					break;

				case 0x0018:		//FX18	Sets the sound timer to VX.
					sound_timer = V[(opcode & 0x0F00) >> 8];
					break;

				case 0x001E:		//FX1E	Adds VX to I
					I += V[(opcode & 0x0F00) >> 8];
					break;

				case 0x0029:		//FX29	Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
					I = (V[(opcode & 0x0F00) >> 8]) * 5;
					break;

				case 0x0033:		//FX33	Stores the Binary-coded decimal representation of VX
					memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
					memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
					break;

				case 0x0055:		//FX55	Stores V0 to VX in memory starting at address I
					for (int i = 1; i < ((opcode & 0x0F00) >> 8); i++) {
						memory[I + i] = V[i];
					}
					break;

				case 0x0065:		//FX65	Fills V0 to VX with values from memory starting at address I
					for (int i = 1; i <= ((opcode & 0x0F00) >> 8); i++) {
						V[i] = memory[I + i];
					}
					break;
			}
	}
	pc += 2;

}

void Chip8::OnKeyDown(WPARAM wParam)
{
	if (char_map.find(wParam) != char_map.end()) {
		key[char_map.at(wParam)] = 1;
		keyPressed = char_map.at(wParam);
	}
}

void Chip8::OnKeyUp(WPARAM wParam)
{
	if (char_map.find(wParam) != char_map.end()) {
		key[char_map.at(wParam)] = 0;
	}
}


void Chip8::DrawScreen(HDC hdc)
{
	if (!drawFlag) {
		return;
	}
	//drawFlag = false;
	RECT rect;

	int border = 10;

	//Paint Background 
	GetClientRect(WindowFromDC(hdc), &rect);
	int width = abs(rect.left - rect.right);
	int height = abs(rect.top - rect.bottom);
	int widthFactor = width / 64;
	int heightFactor = height / 32;

	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);

	HANDLE hOld = SelectObject(hdcMem, hbmMem);

	HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
	FillRect(hdcMem, &rect, brush);

	DeleteObject(brush);

	brush = CreateSolidBrush(RGB(255, 255, 255));

	for (int i = 0; i < 32; i++) {
		for (int j = 0; j < 64; j++) {
			if (gfx[i*64 + j] != 0) {
				rect.left = border + j * widthFactor;
				rect.top = border + i * heightFactor;
				rect.bottom = rect.top + heightFactor;
				rect.right = rect.left + widthFactor;
				FillRect(hdcMem, &rect, brush);
			}
		}
	}
	BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
	SelectObject(hdcMem, hOld);
	DeleteObject(hbmMem);
	DeleteDC(hdcMem);
	DeleteObject(brush);
}

const std::map<unsigned char, int> Chip8::char_map = {
	{ 0x31, 0 },
	{ 0x32, 1 },
	{ 0x33, 2 },
	{ 0x34, 3 },
	{ 0x51, 4 },
	{ 0x57, 5 },
	{ 0x45, 6 },
	{ 0x52, 7 },
	{ 0x41, 8 },
	{ 0x53, 9 },
	{ 0x44, 10 },
	{ 0x46, 11 },
	{ 0x5A, 12 },
	{ 0x58, 13 },
	{ 0x43, 14 },
	{ 0x56, 15 },

};

const unsigned char Chip8::chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


void Chip8::decrementTimers()
{
	delay_timer -= delay_timer > 0 ? 1 : 0;
	sound_timer -= sound_timer > 0 ? 1 : 0;
	return;
}


void Chip8::SetHwnd(HWND hWnd)
{
	this->hWnd = hWnd;
}


void Chip8::UpdateScreen()
{
	InvalidateRect(hWnd, NULL, FALSE);
}
