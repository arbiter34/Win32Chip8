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
}

void Chip8::loadGame(const char *game) {
	FILE *in;
	long size;
	size_t res;

	in = fopen(game, "rb");
	if (in == NULL) {
		return;
	}

	fseek(in, 0, SEEK_END);
	size = ftell(in);
	rewind(in);

	if (size < 4096) {
		res = fread(memory, 1, size, in);
		if (res != size) {
			return;
		}
	}

	fclose(in);
	return;

}

void Chip8::emulateCycle() {
	// Fetch Opcode
	opcode = memory[pc] << 8 | memory[pc + 1];

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
					break;
				case 0x000E:			// 0x00EE: Returns from subroutine
					pc = stack[sp];
					sp--;
					break;
				default:
					break;
			}
			break;

		case 0x1000:				//1NNN: Jumps to address NNN
			pc = opcode & 0x0FFF; 
			break;

		case 0x2000:				//2NNN: Calls subroutine at NNN
			stack[sp] = pc;
			sp++;
			pc = opcode & 0x0FFF;
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
					V[opcode & opcode] >>= 1;
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

				case 0x000E:		//8XYE: Shifts Vx right by one. V is set to the value of the most significant bit of Vx before the shift
					V[0xF] = V[(opcode & 0x0F00) >> 8] & 0x8000;
					V[opcode & opcode] <<= 1;
					break;
			}
			break;

		case 0x9000:				//9XY0: Skips the next instruction if Vx != Vy
			pc += V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4] ? 2 : 0;
			break;

		case 0xA000:				//ANNN: Sets I to Address NNN
			I = opcode & 0x0FFF;
			pc += 2;
			break;

		case 0xB000:				//BNNN: Jumps to the address NNN + V[0]
			pc = (opcode & 0x0FFF) + V[0];
			break;

		case 0xC000:				//CXNN: Sets Vx to a random number, masked by NN
			V[(opcode & 0x0F00) >> 8] = (rand() % (1 << 8)) & (opcode & 0x00FF);
			break;

		case 0xD000:				//DXYN: Sprites stored in memory at location in index register (i), maximum 8 bits wide.  Wraps around the screen.  
									//If when drawn, clears a pixel, register VF is set to 1, else 0.  All drawing is XOR drawing (it toggles the screen pixels)
			unsigned short x = V[(opcode & 0x0F00) >> 8];
			unsigned short y = V[(opcode & 0x00F0) >> 4];
			unsigned short n = (opcode & 0x000F);
			unsigned char row;

			V[0xF] = 0;

			for (int i = 0; i < n; i++) {
				row = memory[I + i];
				for (int j = 0; j < 8; j++) {
					int xcoord = x + j >= 64 ? (x + j) - 64 : x + j;
					int ycoord = y + i >= 32 ? (y + i) - 32 : y + i;
					if ((gfx[((ycoord)* 64) + (xcoord)] == 1) && (row & (0x80 >> j)) != 0) {
						V[0xF] = 1;
					}
					gfx[((ycoord)* 64) + (xcoord)] ^= (row & (0x80 >> j)) != 0;
				}
			}
			drawFlag = true;
			break;
			
		case 0xE000:
			switch (opcode & 0x000F) {
				case 0x000E:		//EX9E: Skips the next instruction if the key stored in VX is pressed.
					pc += key[V[(opcode & 0x0F00) >> 8]] != 0 ? 2 : 0;
					break;

				case 0x0001:		//EXA1: Skips the next instruction if the key stored in VX isn't pressed.
					pc += key[V[(opcode & 0x0F00) >> 8]] == 0 ? 2 : 0;
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
					while (keyPressed == 0) {
						V[(opcode & 0x0F00) >> 8] = keyPressed;
					}
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
					I = (V[(opcode & 0x0F00) >> 8]-1) * 5;
					break;

				case 0x0033:		//FX33	Stores the Binary-coded decimal representation of VX
					memory[I] = V[(opcode & 0x0F00) >> 8] / 100;
					memory[I + 1] = (V[(opcode & 0x0F00) >> 8] / 10) % 10;
					memory[I + 2] = (V[(opcode & 0x0F00) >> 8] % 100) % 10;
					pc += 2;
					break;

				case 0x0055:		//FX55	Stores V0 to VX in memory starting at address I
					for (int i = 1; i < ((opcode & 0x0F00) >> 8); i++) {
						memory[I + i] = V[i];
					}
					break;

				case 0x0065:		//FX65	Fills V0 to VX with values from memory starting at address I
					for (int i = 1; i < ((opcode & 0x0F00) >> 8); i++) {
						V[i] = memory[I + i];
					}
					break;
			}
			pc += 2;
	}


	// Update timers
}

void Chip8::OnKeyDown(WPARAM wParam)
{
	if (char_map.find(wParam) != char_map.end()) {
		key[char_map[wParam]] = 1;
		keyPressed = char_map[wParam];
	}
}

void Chip8::OnKeyUp(WPARAM wParam)
{
	if (char_map.find(wParam) != char_map.end()) {
		key[char_map[wParam]] = 0;
	}
}
