#pragma once

#include "m4c0.hpp"

#include <string.h>

int LoadTGA(Spriteset & tiles, const char * filename) {
  FILE * file;
  unsigned char byte, crap[16], id_length;
  int n, width, height, channels, x, y;
  file = fopen(filename, "rb");
  if (!file) return -1;
  fread(&id_length, 1, 1, file);
  fread(crap, 1, 11, file);
  width = 0;
  height = 0;
  fread(&width, 1, 2, file);  // width
  fread(&height, 1, 2, file); // height
  fread(&byte, 1, 1, file);   // bits
  channels = byte / 8;
  fread(&byte, 1, 1, file); // image descriptor byte (per-bit info)
  for (n = 0; n < id_length; n++)
    fread(&byte, 1, 1, file); // image description
  tiles.data = (DWORD *)malloc(width * height * sizeof(DWORD));
  for (y = height - 1; y >= 0; y--)
    for (x = 0; x < width; x++) {
      DWORD pixel = 0;
      fread(&byte, 1, 1, file);
      pixel |= byte;
      fread(&byte, 1, 1, file);
      pixel |= byte << 8;
      fread(&byte, 1, 1, file);
      pixel |= byte << 16;
      tiles.data[y * width + x] = pixel;
    }
  fclose(file);
  tiles.height = height;
  tiles.width = height;
  tiles.pitch = width;

  return 0;
}

void ClearScreen(DWORD color) {
  draw_bar(0, 0, 640, 480, color);
}

void DrawBar(int sx, int sy, int w, int h, DWORD color) {
  draw_bar(sx, sy, w, h, color);
}

void DrawSprite(Spriteset & sprites, int sx, int sy, int i, DWORD color) {
  for (int y = 0; y < sprites.height; y++) {
    int spoffset = y * sprites.pitch + i * sprites.width;
    for (int x = 0; x < sprites.width; x++) {
      auto p = sprites.data[spoffset++];
      if (p != 0x300030) draw_bar(sx + x, sy + y, 1, 1, color);
    }
  }
}

void DrawText(int sx, int sy, DWORD color, const char * string) {
  auto len = strlen(string);
  for (int i = 0; i < len; i++)
    DrawSprite(font, sx + i * 8, sy, string[i] - ' ', color);
}

bool MouseInBox(int x, int y, int w, int h) {
  return mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h;
}
