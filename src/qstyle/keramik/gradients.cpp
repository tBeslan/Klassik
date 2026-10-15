/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Malte Starostik <malte@kde.org>
   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "gradients.h"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPixmapCache>

#include <cmath>

using namespace Keramik;

QColor
ColorUtil::lighten(const QColor &in, int factor)
{
	if (factor > 100) {
		int h, s, v;
		in.getHsv(&h, &s, &v);

		float mShare = v / 230.0;
		if (mShare > 1)
			mShare = 1;

		mShare *= mShare;

		int diff = factor - 100;
		int hd = int(mShare * diff);
		int delta = int((diff - hd) * 7.55);

		QColor wrk = in.lighter(100 + hd);

		int r = qMin(wrk.red() + delta, 255);
		int g = qMin(wrk.green() + delta, 255);
		int b = qMin(wrk.blue() + delta, 255);

		return QColor(r, g, b);
	}

	return in;
}

// Port of KImageEffect::gradient() for the vertical/horizontal cases, with
// the same fixed point stepping.
static QImage
gradient(const QSize &size, const QColor &ca, const QColor &cb, bool vertical)
{
	QImage image(size, QImage::Format_RGB32);
	if (size.isEmpty())
		return image;

	int rca = ca.red(), gca = ca.green(), bca = ca.blue();
	int rDiff = cb.red() - rca;
	int gDiff = cb.green() - gca;
	int bDiff = cb.blue() - bca;

	int rl = rca << 16;
	int gl = gca << 16;
	int bl = bca << 16;

	const int steps = vertical ? size.height() : size.width();
	const int rcdelta = ((1 << 16) / steps) * rDiff;
	const int gcdelta = ((1 << 16) / steps) * gDiff;
	const int bcdelta = ((1 << 16) / steps) * bDiff;

	for (int i = 0; i < steps; i++) {
		rl += rcdelta;
		gl += gcdelta;
		bl += bcdelta;

		const QRgb rgb = qRgb(rl >> 16, gl >> 16, bl >> 16);
		if (vertical) {
			QRgb *line = reinterpret_cast<QRgb *>(image.scanLine(i));
			for (int x = 0; x < size.width(); x++)
				line[x] = rgb;
		} else {
			for (int y = 0; y < size.height(); y++)
				reinterpret_cast<QRgb *>(image.scanLine(y))[i] = rgb;
		}
	}

	return image;
}

void
GradientPainter::renderGradient(QPainter *p, const QRect &r, const QColor &c,
								bool horizontal, bool menu, int px, int py,
								int pwidth, int pheight)
{
	int width = r.width(), height = r.height();
	if (pwidth != -1)
		width = pwidth;
	if (pheight != -1)
		height = pheight;

	if (horizontal)
		width = 18;
	else
		height = 18;

	if (width <= 0 || height <= 0)
		return;

	const qreal dpr = p->device() ? p->device()->devicePixelRatio() : 1.0;
	const int scale = qBound(1, int(std::ceil(dpr - 0.001)), 4);

	const QString key = QStringLiteral("keramik-gradient-%1-%2-%3-%4-%5")
		.arg(width).arg(height).arg(c.rgb(), 0, 16).arg(menu).arg(scale);

	QPixmap pix;
	if (!QPixmapCache::find(key, &pix)) {
		// The gradient is computed in device pixels, so that it stays smooth
		const int sw = width * scale, sh = height * scale;
		QImage img(sw, sh, QImage::Format_RGB32);
		QPainter p2(&img);

		if (horizontal) {
			if (menu) {
				p2.drawImage(QRect(0, 0, sw, sh),
							 gradient(QSize(4, sh), c.lighter(93), ColorUtil::lighten(c, 109), true));
			} else {
				int h1 = 3 * sh / 4;
				int h2 = sh - h1;

				p2.drawImage(QRect(0, 0, sw, h1),
							 gradient(QSize(4, h1), ColorUtil::lighten(c, 110), c.lighter(94), true));
				p2.drawImage(QRect(0, h1, sw, h2),
							 gradient(QSize(4, h2), c.lighter(94), ColorUtil::lighten(c, 109), true));
			}
		} else {
			int h1 = 3 * sw / 4;
			int h2 = sw - h1;

			p2.drawImage(QRect(0, 0, h1, sh),
						 gradient(QSize(h1, 4), ColorUtil::lighten(c, 110), c.lighter(94), false));
			p2.drawImage(QRect(h1, 0, h2, sh),
						 gradient(QSize(h2, 4), c.lighter(94), ColorUtil::lighten(c, 109), false));
		}
		p2.end();

		pix = QPixmap::fromImage(img);
		pix.setDevicePixelRatio(scale);
		QPixmapCache::insert(key, pix);
	}

	p->drawTiledPixmap(r, pix, horizontal ? QPoint(0, py) : QPoint(px, 0));
}
