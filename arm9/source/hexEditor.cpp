#include "hexEditor.h"

#include "date.h"
#include "tonccpy.h"
#include "file_browse.h"

#include <algorithm>
#include <nds.h>
#include <stdio.h>

extern PrintConsole bottomConsole, bottomConsoleBG, topConsole;

extern void reinitConsoles(void);

u32 jumpToOffset(u32 offset) {
	consoleSelect(&bottomConsoleBG);
	consoleClear();
	consoleSelect(&bottomConsole);
	consoleClear();

	u8 cursorPosition = 0;
	u16 pressed = 0, held = 0;
	while(1) {
		printf("\x1B[9;6H\x1B[47m-------------------");
		printf("\x1B[10;8H\x1B[47mJump to Offset");
		printf("\x1B[12;11H\x1B[37m%08lX", offset);
		printf("\x1B[12;%dH\x1B[41m%lX", 17 - cursorPosition, (offset >> ((cursorPosition + 1) * 4)) & 0xF);
		printf("\x1B[13;6H\x1B[47m-------------------");

		consoleSelect(&topConsole);
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
			printf("\x1B[30m\x1B[0;26H %s", RetTime().c_str()); // Print time
		} while(!held);
		consoleSelect(&bottomConsole);

		if(held & KEY_UP) {
			offset = (offset & ~(0xF0 << cursorPosition * 4)) | ((offset + (0x10 << (cursorPosition * 4))) & (0xF0 << cursorPosition * 4));
		} else if(held & KEY_DOWN) {
			offset = (offset & ~(0xF0 << cursorPosition * 4)) | ((offset - (0x10 << (cursorPosition * 4))) & (0xF0 << cursorPosition * 4));
		} else if(held & KEY_LEFT) {
			if(cursorPosition < 6)
				cursorPosition++;
		} else if(held & KEY_RIGHT) {
			if(cursorPosition > 0)
				cursorPosition--;
		} else if(pressed & (KEY_A | KEY_B)) {
			return offset;
		}
	}
}

u32 search(u32 offset, FILE *file) {
	consoleSelect(&bottomConsoleBG);
	consoleClear();
	consoleSelect(&bottomConsole);
	consoleClear();

	u8 cursorPosition = 0;
	u16 pressed = 0, held = 0;
	while(1) {
		printf("\x1B[9;4H\x1B[47m-----------------------");
		printf("\x1B[10;5H%c Search for String %c", cursorPosition == 0 ? '>' : ' ', cursorPosition == 0 ? '<' : ' ');
		printf("\x1B[11;5H%c  Search for Data  %c", cursorPosition == 1 ? '>' : ' ', cursorPosition == 1 ? '<' : ' ');
		printf("\x1B[12;4H-----------------------");

		consoleSelect(&topConsole);
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
			printf("\x1B[30m\x1B[0;26H %s", RetTime().c_str()); // Print time
		} while(!held);
		consoleSelect(&bottomConsole);

		if(held & (KEY_UP | KEY_DOWN)) {
			cursorPosition ^= 1;
		} else if(pressed & KEY_A) {
			break;
		} else if(pressed & KEY_B) {
			return offset;
		}
	}

	char str[64] = {0};
	size_t strLen = 1;

	if(cursorPosition == 0) {
		consoleDemoInit();
		Keyboard *kbd = keyboardDemoInit();
		kbd->OnKeyPressed = OnKeyPressed;

		// keyboardShow();
		printf("Search for:\n");
		fgets(str, sizeof(str), stdin);
		keyboardHide();
		consoleClear();

		reinitConsoles();
		consoleSelect(&bottomConsole);

		BG_PALETTE_SUB[0x1F] = 0x9CF7;
		BG_PALETTE_SUB[0x2F] = 0xB710;
		BG_PALETTE_SUB[0x3F] = 0xAE8D;
		BG_PALETTE_SUB[0x7F] = 0xEA2D;

		strLen = strlen(str) - 1;
		if(strLen == 0)
			return offset;

		str[strLen] = 0; // Remove ending \n that fgets has
	} else {
		consoleClear();

		cursorPosition = 0;
		while(1) {
			printf("\x1B[9;6H\x1B[47m------------------");
			printf("\x1B[10;9HEnter value:");
			u8 pos = 15 - strLen;
			for(size_t i = 0; i < strLen * 2; i++) {
				printf("\x1B[12;%dH\x1B[%dm%X", pos + i, (i == cursorPosition ? 31 : ((i / 2 % 2) ? 33 : 32)), str[i / 2] >> (!(i % 2) * 4) & 0xF);
			}
			printf("\x1B[13;6H\x1B[47m------------------");

			consoleSelect(&topConsole);
			do {
				swiWaitForVBlank();
				scanKeys();
				pressed = keysDown();
				held = keysDownRepeat();
				printf("\x1B[30m\x1B[0;26H %s", RetTime().c_str()); // Print time
			} while(!held);
			consoleSelect(&bottomConsole);

			if(held & KEY_UP) {
				char val = str[cursorPosition / 2];
				u8 shift = !(cursorPosition % 2) * 4;
				str[cursorPosition / 2] = (val & (0xF0 >> shift)) | ((val + (1 << shift)) & (0xF << shift));
			} else if(held & KEY_DOWN) {
				char val = str[cursorPosition / 2];
				u8 shift = !(cursorPosition % 2) * 4;
				str[cursorPosition / 2] = (val & (0xF0 >> shift)) | ((val - (1 << shift)) & (0xF << shift));
			} else if(held & KEY_LEFT) {
				if(cursorPosition > 0)
					cursorPosition--;
			} else if(held & KEY_RIGHT) {
				if(cursorPosition < strLen * 2 - 1) {
					cursorPosition++;
				} else if(strLen < 8) {
					strLen++;
					cursorPosition++;
				}
			} else if(pressed & KEY_A) {
				break;
			} else if(pressed & KEY_B) {
				return offset;
			} else if(pressed & KEY_X) {
				if(strLen > 1) {
					str[strLen - 1] = 0;
					strLen--;
					if(cursorPosition > strLen * 2 - 1)
						cursorPosition -= 2;
					consoleClear();
				}
			}
		}
	}

	consoleClear();
	printf("\x1B[9;6H\x1B[47m---------------------");
	printf("\x1B[10;12HSearching");
	printf("\x1B[14;8HPress B to cancel");
	printf("\x1B[15;6H---------------------");

	size_t len = 32 << 10, pos = offset;
	fseek(file, 0, SEEK_END);
	size_t fileLen = ftell(file);
	char *buf = new char[len];
	do {
		scanKeys();
		if(keysDown() & KEY_B) {
			delete[] buf;
			return offset;
		}

		printf("\x1B[12;6H%10d/%d", pos, fileLen);

		if(fseek(file, pos, SEEK_SET) != 0)
			break;
		len = fread(buf, 1, len, file);

		for(size_t i = 0; i < len - strLen && len >= strLen; i++) {
			if(memcmp(buf + i, str, strLen) == 0) {
				delete[] buf;
				return (pos + i) & ~7;
			}
		}

		pos += len;
	} while(len == 32 << 10);
	delete[] buf;

	consoleClear();
	printf("\x1B[9;5H\x1B[47m---------------------");
	printf("\x1B[10;6HReached end of file");
	printf("\x1B[11;8Hwith no results");
	printf("\x1B[12;5H---------------------");

	do {
		swiWaitForVBlank();
		scanKeys();
		printf("\x1B[30m\x1B[0;26H %s", RetTime().c_str()); // Print time
	} while(!keysDown());

	return offset;
}

void hexEditor(const char *path, int drive) {
	// Custom palettes
	BG_PALETTE_SUB[0x1F] = 0x9CF7;
	BG_PALETTE_SUB[0x2F] = 0xB710;
	BG_PALETTE_SUB[0x3F] = 0xAE8D;
	BG_PALETTE_SUB[0x7F] = 0xEA2D;

	FILE *file = fopen(path, drive < 4 ? "rb+" : "rb");

	if(!file)
		return;

	consoleClear();

	fseek(file, 0, SEEK_END);
	u32 fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	u8 maxLines = std::min(23lu, fileSize / 8 + (fileSize % 8 != 0));
	u32 maxSize = ((fileSize - 8 * maxLines) & ~7) + (fileSize & 7 ? 8 : 0);

	u8 cursorPosition = 0, mode = 0;
	u16 pressed = 0, held = 0;
	u32 offset = 0;

	char data[8 * maxLines];
	fseek(file, offset, SEEK_SET);
	fread(data, 1, sizeof(data), file);

	while(1) {
		consoleSelect(&bottomConsoleBG);
		printf ("\x1B[0;0H\x1B[46m"); // Blue
		for(int i = 0; i < 4; i++)
			printf ("\2");
		printf ("\x1B[42m"); // Green
		for(int i = 0; i < 32 - 4; i++)
			printf ("\2");

		consoleSelect(&bottomConsole);

		printf("\x1B[0;11H\x1B[30mHex Editor");

		printf("\x1B[0;0H\x1B[30m%04lX", offset >> 0x10);

		if(mode < 2) {
			fseek(file, offset, SEEK_SET);
			toncset(data, 0, sizeof(data));
			fread(data, 1, std::min((u32)sizeof(data), fileSize - offset), file);
		}

		for(int i = 0; i < maxLines; i++) {
			printf("\x1B[%d;0H\x1B[37m%04lX", i + 1, (offset + i * 8) & 0xFFFF);
			for(int j = 0; j < 4; j++)
				printf("\x1B[%d;%dH\x1B[%dm%02X", i + 1, 5 + (j * 2), (mode > 0 && i * 8 + j == cursorPosition) ? (mode > 1 ? 30 : 31) : (offset + i * 8 + j >= fileSize ? 38 : 32 + (j % 2)), data[i * 8 + j]);
			for(int j = 0; j < 4; j++)
				printf("\x1B[%d;%dH\x1B[%dm%02X", i + 1, 14 + (j * 2), (mode > 0 && i * 8 + 4 + j == cursorPosition) ? (mode > 1 ? 30 : 31) : (offset + i * 8 + 4 + j >= fileSize ? 38 : 32 + (j % 2)), data[i * 8 + 4 + j]);
			char line[9] = {0};
			for(int j = 0; j < 8; j++) {
				char c = data[i * 8 + j];
				if(c < ' ' || c > 127)
					line[j] = ' ';
				else
					line[j] = c;
			}
			printf("\x1B[%d;23H\x1B[47m%.8s", i + 1, line);
			if(mode > 0 && cursorPosition / 8 == i) {
				printf("\x1B[%d;%dH\x1B[%dm%c", i + 1, 23 + cursorPosition % 8, mode > 1 ? 30 : 31, line[cursorPosition % 8]);
			}
		}


		consoleSelect(&topConsole);
		do {
			swiWaitForVBlank();
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
			printf("\x1B[30m\x1B[0;26H %s", RetTime().c_str()); // Print time
		} while(!held);
		consoleSelect(&bottomConsole);

		if(mode == 0) {
			if(keysHeld() & KEY_R && held & (KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT)) {
				if(held & KEY_UP) {
					offset = std::max((s64)offset - 0x1000, 0ll);
				} else if(held & KEY_DOWN) {
					offset = std::min(offset + 0x1000, maxSize);
				} else if(held & KEY_LEFT) {
					offset = std::max((s64)offset - 0x10000, 0ll);
				} else if(held & KEY_RIGHT) {
					offset = std::min(offset + 0x10000, maxSize);
				}
			} else if(held & KEY_UP) {
				if(offset >= 8)
					offset -= 8;
			} else if(held & KEY_DOWN) {
				if(offset < fileSize - 8 * maxLines && fileSize > 8 * maxLines)
					offset += 8;
			} else if(held & KEY_LEFT) {
				offset = std::max((s64)offset - 8 * maxLines, 0ll);
			} else if(held & KEY_RIGHT) {
				offset = std::min(offset + 8 * maxLines, maxSize);
			} else if(pressed & KEY_A) {
				mode = 1;
			} else if(pressed & KEY_B) {
				break;
			} else if(pressed & KEY_X) {
				offset = std::min(search(offset, file), maxSize);
				consoleClear();
			} else if(pressed & KEY_Y) {
				offset = std::min(jumpToOffset(offset), maxSize);
				consoleClear();
			}
		} else if(mode == 1) {
			if(held & KEY_UP) {
				if(cursorPosition >= 8)
					cursorPosition -= 8;
				else if(offset > 8)
					offset -= 8;
			} else if(held & KEY_DOWN) {
				if(cursorPosition < 8 * 22)
					cursorPosition += 8;
				else if(offset < fileSize - 8 * maxLines && fileSize > 8 * maxLines)
					offset += 8;
				cursorPosition = std::min(cursorPosition, (u8)(fileSize - offset - 1));
			} else if(held & KEY_LEFT) {
				if(cursorPosition > 0)
					cursorPosition--;
			} else if(held & KEY_RIGHT) {
				if(cursorPosition < 8 * maxLines - 1)
					cursorPosition = std::min((u8)(cursorPosition + 1), (u8)(fileSize - offset - 1));
			} else if(pressed & KEY_A) {
				if(drive < 4) {
					mode = 2;
					consoleSelect(&bottomConsoleBG);
					printf("\x1B[%d;%dH\x1B[%dm\2\2", 1 + cursorPosition / 8, 5 + (cursorPosition % 8 * 2) + (cursorPosition % 8 / 4), 31);
					printf("\x1B[%d;%dH\x1B[%dm\2", 1 + cursorPosition / 8, 23 + cursorPosition % 8, 31);
					consoleSelect(&bottomConsole);
				}
			} else if(pressed & KEY_B) {
				mode = 0;
			} else if(pressed & KEY_X) {
				offset = std::min(search(offset, file), maxSize);
				consoleClear();
			} else if(pressed & KEY_Y) {
				offset = std::min(jumpToOffset(offset), maxSize);
				consoleClear();
			}
		} else if(mode == 2) {
			if(held & KEY_UP) {
				data[cursorPosition]++;
			} else if(held & KEY_DOWN) {
				data[cursorPosition]--;
			} else if(held & KEY_LEFT) {
				data[cursorPosition] -= 0x10;
			} else if(held & KEY_RIGHT) {
				data[cursorPosition] += 0x10;
			} else if(pressed & (KEY_A | KEY_B)) {
				mode = 1;
				fseek(file, offset + cursorPosition, SEEK_SET);
				fwrite(data + cursorPosition, 1, 1, file);

				consoleSelect(&bottomConsoleBG);
				printf("\x1B[%d;%dH\x1B[%dm\2\2", 1 + cursorPosition / 8, 5 + (cursorPosition % 8 * 2) + (cursorPosition % 8 / 4), 30);
				printf("\x1B[%d;%dH\x1B[%dm\2", 1 + cursorPosition / 8, 23 + cursorPosition % 8, 30);
				consoleSelect(&bottomConsole);
			}
		}
	}

	fclose(file);

	// Restore color palette
	BG_PALETTE_SUB[0x1F] = 0x000F;
	BG_PALETTE_SUB[0x2F] = 0x01E0;
	BG_PALETTE_SUB[0x3F] = 0x3339;
	BG_PALETTE_SUB[0x7F] = 0x656A;

	consoleSelect(&bottomConsoleBG);
	consoleClear();
}
