/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Malte Starostik <malte@kde.org>
   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pixmaploader.h"
#include "keramikimage.h"

#include <QGuiApplication>
#include <QPainter>
#include <QPixmapCache>

#include <cmath>

#include "pixmaps.keramik"

using namespace Keramik;

qreal
Keramik::paintScale(const QPainter *p)
{
	const qreal dpr = p->device() ? p->device()->devicePixelRatio() : 1.0;
	return dpr * qAbs(p->worldTransform().m11());
}

// Integer scale factor the tiles are rendered at for the painter.
// Fractional ratios are rounded up, and the result gets downscaled by Qt.
static int
deviceScale(const QPainter *p)
{
	return qBound(1, int(std::ceil(paintScale(p) - 0.001)), 6);
}

PixmapLoader::PixmapLoader()
{
	for (int c = 0; c < 256; c++)
		clamp[c] = static_cast<unsigned char>(c);

	for (int c = 256; c < 540; c++)
		clamp[c] = 255;
}

PixmapLoader &
PixmapLoader::the()
{
	static PixmapLoader instance;
	return instance;
}

QImage
PixmapLoader::getDisabled(int name, const QColor &color)
{
	const KeramikEmbedImage *edata = KeramikGetDbImage(name);
	if (!edata)
		return QImage();

	// Like getColored, but desaturate a bit, and lower gamma..

	QImage img(edata->width, edata->height, QImage::Format_ARGB32);

	quint32 i = qGray(color.rgb());
	quint32 r = (3 * color.red() + i) >> 2;
	quint32 g = (3 * color.green() + i) >> 2;
	quint32 b = (3 * color.blue() + i) >> 2;

	const int size = img.width() * img.height();
	QRgb *write = reinterpret_cast<QRgb *>(img.bits());

	if (edata->haveAlpha) {
		for (int pos = 0; pos < size * 3; pos += 3) {
			quint32 scale = edata->data[pos];
			quint32 add = (edata->data[pos + 1] * i + 127) >> 8;
			quint32 alpha = edata->data[pos + 2];

			quint32 rr = clamp[qMin(((r * scale + 127) >> 8) + add, 539u)];
			quint32 rg = clamp[qMin(((g * scale + 127) >> 8) + add, 539u)];
			quint32 rb = clamp[qMin(((b * scale + 127) >> 8) + add, 539u)];

			*write++ = qRgba(rr, rg, rb, alpha);
		}
	} else {
		for (int pos = 0; pos < size * 2; pos += 2) {
			quint32 scale = edata->data[pos];
			quint32 add = (edata->data[pos + 1] * i + 127) >> 8;
			quint32 rr = clamp[qMin(((r * scale + 127) >> 8) + add, 539u)];
			quint32 rg = clamp[qMin(((g * scale + 127) >> 8) + add, 539u)];
			quint32 rb = clamp[qMin(((b * scale + 127) >> 8) + add, 539u)];
			*write++ = qRgb(rr, rg, rb);
		}
	}

	return img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

QImage
PixmapLoader::getColored(int name, const QColor &color)
{
	const KeramikEmbedImage *edata = KeramikGetDbImage(name);
	if (!edata)
		return QImage();

	QImage img(edata->width, edata->height, QImage::Format_ARGB32);

	quint32 r = color.red() + 2;
	quint32 g = color.green() + 2;
	quint32 b = color.blue() + 2;

	const int size = img.width() * img.height();
	QRgb *write = reinterpret_cast<QRgb *>(img.bits());

	if (edata->haveAlpha) {
		for (int pos = 0; pos < size * 3; pos += 3) {
			quint32 scale = edata->data[pos];
			quint32 add = edata->data[pos + 1];
			quint32 alpha = edata->data[pos + 2];
			if (scale != 0)
				add = add * 5 / 4;

			quint32 rr = clamp[qMin(((r * scale + 127) >> 8) + add, 539u)];
			quint32 rg = clamp[qMin(((g * scale + 127) >> 8) + add, 539u)];
			quint32 rb = clamp[qMin(((b * scale + 127) >> 8) + add, 539u)];

			*write++ = qRgba(rr, rg, rb, alpha);
		}
	} else {
		for (int pos = 0; pos < size * 2; pos += 2) {
			quint32 scale = edata->data[pos];
			quint32 add = edata->data[pos + 1];
			if (scale != 0)
				add = add * 5 / 4;

			quint32 rr = clamp[qMin(((r * scale + 127) >> 8) + add, 539u)];
			quint32 rg = clamp[qMin(((g * scale + 127) >> 8) + add, 539u)];
			quint32 rb = clamp[qMin(((b * scale + 127) >> 8) + add, 539u)];

			*write++ = qRgb(rr, rg, rb);
		}
	}

	return img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

QPixmap
PixmapLoader::pixmap(int name, const QColor &color, bool disabled, int scale)
{
	return this->scale(name, 0, 0, color, disabled, scale);
}

QPixmap
PixmapLoader::scale(int name, int width, int height, const QColor &color,
					bool disabled, int scale)
{
	const QString key = QStringLiteral("keramik-%1-%2-%3-%4-%5-%6")
		.arg(name).arg(width).arg(height).arg(color.rgb(), 0, 16).arg(disabled).arg(scale);

	QPixmap result;
	if (QPixmapCache::find(key, &result))
		return result;

	QImage img = disabled ? getDisabled(name, color) : getColored(name, color);
	if (img.isNull())
		return result;

	const QSize target((width ? width : img.width()) * scale,
					   (height ? height : img.height()) * scale);

	// Blow the tile up to device pixels first, so that it stays crisp, then
	// stretch it to the requested size like the original QImage::scale() did.
	if (scale > 1)
		img = img.scaled(img.size() * scale, Qt::IgnoreAspectRatio, Qt::FastTransformation);
	if (img.size() != target)
		img = img.scaled(target, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

	result = QPixmap::fromImage(img);
	result.setDevicePixelRatio(scale);
	QPixmapCache::insert(key, result);
	return result;
}

QSize
PixmapLoader::size(int id)
{
	const KeramikEmbedImage *edata = KeramikGetDbImage(id);
	if (!edata)
		return QSize(0, 0);
	return QSize(edata->width, edata->height);
}

void
TilePainter::draw(QPainter *p, int x, int y, int width, int height,
				  const QColor &color, bool disabled)
{
	const int dscale = deviceScale(p);
	if (qFuzzyCompare(paintScale(p), qreal(dscale))) {
		drawTiles(p, x, y, width, height, color, disabled, dscale);
		return;
	}

	// Fixed tiles are drawn whole even when they don't fit
	PixmapLoader &loader = PixmapLoader::the();
	int fullWidth = 0, fullHeight = 0;
	for (unsigned int col = 0; col < columns(); ++col)
		if (columnMode(col) == Fixed)
			fullWidth += loader.size(absTileName(col, 0)).width();
	for (unsigned int row = 0; row < rows(); ++row)
		if (rowMode(row) == Fixed)
			fullHeight += loader.size(absTileName(0, row)).height();
	fullWidth = qMax(fullWidth, width);
	fullHeight = qMax(fullHeight, height);
	if (fullWidth <= 0 || fullHeight <= 0)
		return;

	// At a fractional scale, the borders between the tiles fall between
	// device pixels and show up as seams. Assemble the tiles at integer
	// scale instead, and scale the result as a whole.
	QString key = QStringLiteral("keramik-tiles-%1-%2-%3-%4-%5")
		.arg(width).arg(height).arg(color.rgb(), 0, 16).arg(disabled).arg(dscale);
	for (unsigned int row = 0; row < rows(); ++row)
		for (unsigned int col = 0; col < columns(); ++col)
			key += QStringLiteral("-%1:%2%3").arg(absTileName(col, row)).arg(columnMode(col)).arg(rowMode(row));

	QPixmap pix;
	if (!QPixmapCache::find(key, &pix)) {
		pix = QPixmap(fullWidth * dscale, fullHeight * dscale);
		pix.setDevicePixelRatio(dscale);
		pix.fill(Qt::transparent);
		QPainter p2(&pix);
		drawTiles(&p2, 0, 0, width, height, color, disabled, dscale);
		p2.end();
		QPixmapCache::insert(key, pix);
	}

	p->save();
	p->setRenderHint(QPainter::SmoothPixmapTransform);
	p->drawPixmap(QRect(x, y, fullWidth, fullHeight), pix);
	p->restore();
}

void
TilePainter::drawTiles(QPainter *p, int x, int y, int width, int height,
					   const QColor &color, bool disabled, int dscale)
{
	PixmapLoader &loader = PixmapLoader::the();

	unsigned int scaledColumns = 0, scaledRows = 0, lastScaledColumn = 0, lastScaledRow = 0;
	int scaleWidth = width, scaleHeight = height;

	for (unsigned int col = 0; col < columns(); ++col)
		if (columnMode(col) != Fixed) {
			scaledColumns++;
			lastScaledColumn = col;
		} else
			scaleWidth -= loader.size(absTileName(col, 0)).width();
	for (unsigned int row = 0; row < rows(); ++row)
		if (rowMode(row) != Fixed) {
			scaledRows++;
			lastScaledRow = row;
		} else
			scaleHeight -= loader.size(absTileName(0, row)).height();
	if (scaleWidth < 0)
		scaleWidth = 0;
	if (scaleHeight < 0)
		scaleHeight = 0;

	int ypos = y;
	if (scaleHeight && !scaledRows)
		ypos += scaleHeight / 2;
	for (unsigned int row = 0; row < rows(); ++row) {
		int xpos = x;
		if (scaleWidth && !scaledColumns)
			xpos += scaleWidth / 2;
		int h = rowMode(row) == Fixed ? 0 : scaleHeight / scaledRows;
		if (scaledRows && row == lastScaledRow)
			h += scaleHeight - scaleHeight / scaledRows * scaledRows;

		int realH = h ? h : loader.size(absTileName(0, row)).height();

		// Skip non-fitting tiles, too.
		if (rowMode(row) != Fixed && h == 0)
			continue;

		if (rowMode(row) == Tiled)
			h = 0;

		for (unsigned int col = 0; col < columns(); ++col) {
			int w = columnMode(col) == Fixed ? 0 : scaleWidth / scaledColumns;

			int tileW = loader.size(absTileName(col, row)).width();

			if (scaledColumns && col == lastScaledColumn)
				w += scaleWidth - scaleWidth / scaledColumns * scaledColumns;
			int realW = w ? w : tileW;

			// M.O.: Check me: Just skip the tile if it can't fit...
			if (columnMode(col) != Fixed && w == 0)
				continue;

			if (columnMode(col) == Tiled)
				w = 0;

			if (tileW) {
				if (w || h)
					p->drawTiledPixmap(xpos, ypos, realW, realH,
									   loader.scale(absTileName(col, row), w, h, color, disabled, dscale));
				else
					p->drawTiledPixmap(xpos, ypos, realW, realH,
									   loader.pixmap(absTileName(col, row), color, disabled, dscale));
			}
			xpos += realW;
		}
		ypos += realH;
	}
}

RectTilePainter::RectTilePainter(int name, bool scaleH, bool scaleV,
								 unsigned int columns, unsigned int rows)
	: TilePainter(name), m_scaleH(scaleH), m_scaleV(scaleV)
{
	m_columns = columns;
	m_rows = rows;

	TileMode mh = m_scaleH ? Scaled : Tiled;
	TileMode mv = m_scaleV ? Scaled : Tiled;
	for (int c = 0; c < 4; c++) {
		if (c != 1)
			colMde[c] = Fixed;
		else
			colMde[c] = mh;
	}

	for (int c = 0; c < 4; c++) {
		if (c != 1)
			rowMde[c] = Fixed;
		else
			rowMde[c] = mv;
	}
}

int
RectTilePainter::tileName(unsigned int column, unsigned int row) const
{
	return row * 3 + column;
}

ActiveTabPainter::ActiveTabPainter(bool bottom)
	: RectTilePainter(bottom ? keramik_tab_bottom_active : keramik_tab_top_active, false),
	  m_bottom(bottom)
{
	m_rows = 2;
	if (m_bottom) {
		rowMde[0] = rowMde[2] = rowMde[3] = Scaled;
		rowMde[1] = Fixed;
	} else {
		rowMde[0] = rowMde[2] = rowMde[3] = Fixed;
		rowMde[1] = Scaled;
	}
}

int
ActiveTabPainter::tileName(unsigned int column, unsigned int row) const
{
	if (m_bottom)
		return RectTilePainter::tileName(column, row + 1);
	return RectTilePainter::tileName(column, row);
}

InactiveTabPainter::InactiveTabPainter(Mode mode, bool bottom)
	: RectTilePainter(bottom ? keramik_tab_bottom_inactive : keramik_tab_top_inactive, false),
	  m_mode(mode), m_bottom(bottom)
{
	m_rows = 2;
	if (m_bottom) {
		rowMde[0] = rowMde[2] = rowMde[3] = Scaled;
		rowMde[1] = Fixed;
	} else {
		rowMde[0] = rowMde[2] = rowMde[3] = Fixed;
		rowMde[1] = Scaled;
	}

	Mode check = QGuiApplication::isRightToLeft() ? First : Last;
	m_columns = (m_mode == check ? 3 : 2);
}

int
InactiveTabPainter::tileName(unsigned int column, unsigned int row) const
{
	Mode check = QGuiApplication::isRightToLeft() ? Last : First;
	if (column == 0 && m_mode != check)
		return KeramikTileSeparator;
	if (m_bottom)
		return RectTilePainter::tileName(column, row + 1);
	return RectTilePainter::tileName(column, row);
}

ScrollBarPainter::ScrollBarPainter(int type, int count, bool horizontal)
	: TilePainter(name(horizontal)), m_type(type), m_count(count), m_horizontal(horizontal)
{
	for (int c = 0; c < 5; c++) {
		if (!m_horizontal || !(c % 2))
			colMde[c] = Fixed;
		else
			colMde[c] = Tiled;

		if (m_horizontal || !(c % 2))
			rowMde[c] = Fixed;
		else
			rowMde[c] = Tiled;
	}

	m_columns = m_horizontal ? m_count : 1;
	m_rows = m_horizontal ? 1 : m_count;
}

int
ScrollBarPainter::name(bool horizontal)
{
	return horizontal ? keramik_scrollbar_hbar : keramik_scrollbar_vbar;
}

int
ScrollBarPainter::tileName(unsigned int column, unsigned int row) const
{
	unsigned int num = (column ? column : row) + 1;
	if (m_count == 5) {
		if (num == 3)
			num = 4;
		else if (num == 4)
			num = 2;
		else if (num == 5)
			num = 3;
	}

	return m_type + (num - 1) * 16;
}
