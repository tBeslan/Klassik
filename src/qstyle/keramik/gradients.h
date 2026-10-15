/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Malte Starostik <malte@kde.org>
   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QColor>

class QPainter;
class QRect;

namespace Keramik
{
	class GradientPainter
	{
	public:
		static void renderGradient(QPainter *p, const QRect &r, const QColor &cr,
								   bool horizontal, bool menu = false,
								   int px = 0, int py = 0, int pwidth = -1, int pheight = -1);
	};

	class ColorUtil
	{
	public:
		static QColor lighten(const QColor &in, int factor);
	};
}
