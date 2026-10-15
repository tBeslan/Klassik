/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Malte Starostik <malte@kde.org>
   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

struct KeramikEmbedImage
{
	bool haveAlpha;
	int width;
	int height;
	int id;
	const unsigned char *data;
};

extern const KeramikEmbedImage *KeramikGetDbImage(int id);

// Offsets of the tile variants from the id of their widget
constexpr int KeramikTileTL = 0;
constexpr int KeramikTileTC = 1;
constexpr int KeramikTileTR = 2;
constexpr int KeramikTileCL = 3;
constexpr int KeramikTileCC = 4;
constexpr int KeramikTileCR = 5;
constexpr int KeramikTileRL = 6;
constexpr int KeramikTileRC = 7;
constexpr int KeramikTileRR = 8;
constexpr int KeramikTileSeparator = 16;
constexpr int KeramikSlider1 = 32;
constexpr int KeramikSlider2 = 48;
constexpr int KeramikSlider3 = 64;
constexpr int KeramikSlider4 = 80;
constexpr int KeramikGroove1 = 96;
constexpr int KeramikGroove2 = 112;
