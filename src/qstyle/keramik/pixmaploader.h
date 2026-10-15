/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Malte Starostik <malte@kde.org>
   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QColor>
#include <QImage>
#include <QPixmap>
#include <QRect>

#include "keramikrc.h"

class QPainter;

namespace Keramik
{
	class PixmapLoader
	{
	public:
		PixmapLoader();

		// Returns the tile recolored with color. scale is the integer
		// device pixel ratio the pixmap is rendered for.
		QPixmap pixmap(int name, const QColor &color, bool disabled, int scale);

		// Like pixmap(), but scaled to width x height (logical pixels).
		// A width or height of 0 keeps the tile's own size.
		QPixmap scale(int name, int width, int height, const QColor &color,
					  bool disabled, int scale);

		// Size of the tile in logical pixels
		QSize size(int id);

		static PixmapLoader &the();

	private:
		QImage getColored(int id, const QColor &color);
		QImage getDisabled(int id, const QColor &color);

		unsigned char clamp[540];
	};

	class TilePainter
	{
	public:
		TilePainter(int name) : m_columns(1), m_rows(1), m_name(name) {}
		virtual ~TilePainter() = default;

		void draw(QPainter *p, int x, int y, int width, int height,
				  const QColor &color, bool disabled = false);
		void draw(QPainter *p, const QRect &rect, const QColor &color, bool disabled = false)
		{
			draw(p, rect.x(), rect.y(), rect.width(), rect.height(), color, disabled);
		}

	protected:
		enum TileMode { Fixed, Scaled, Tiled };

		unsigned int columns() const { return m_columns; }
		unsigned int rows() const { return m_rows; }

		virtual int tileName(unsigned int, unsigned int) const { return 0; }

		TileMode columnMode(unsigned int col) const { return colMde[col]; }
		TileMode rowMode(unsigned int row) const { return rowMde[row]; }

		TileMode colMde[5], rowMde[5];
		unsigned int m_columns;
		unsigned int m_rows;

	private:
		int absTileName(unsigned int column, unsigned int row) const
		{
			return m_name + tileName(column, row);
		}

		int m_name;
	};

	class ScaledPainter : public TilePainter
	{
	public:
		enum Direction { Horizontal = 1, Vertical = 2, Both = Horizontal | Vertical };
		ScaledPainter(int name, Direction direction = Both)
			: TilePainter(name), m_direction(direction)
		{
			colMde[0] = (m_direction & Horizontal) ? Scaled : Tiled;
			rowMde[0] = (m_direction & Vertical) ? Scaled : Tiled;
		}

	private:
		Direction m_direction;
	};

	class CenteredPainter : public TilePainter
	{
	public:
		CenteredPainter(int name) : TilePainter(name)
		{
			colMde[0] = colMde[1] = colMde[2] = colMde[3] = Fixed;
			rowMde[0] = rowMde[1] = rowMde[2] = rowMde[3] = Fixed;
		}
	};

	class RectTilePainter : public TilePainter
	{
	public:
		RectTilePainter(int name, bool scaleH = true, bool scaleV = true,
						unsigned int columns = 3, unsigned int rows = 3);

	protected:
		int tileName(unsigned int column, unsigned int row) const override;

	private:
		bool m_scaleH;
		bool m_scaleV;
	};

	class RowPainter : public TilePainter
	{
	public:
		RowPainter(int name) : TilePainter(name)
		{
			colMde[0] = colMde[2] = Fixed;
			colMde[1] = Tiled;
			rowMde[0] = Scaled;
			m_columns = 3;
		}

	protected:
		int tileName(unsigned int column, unsigned int /*row*/) const override
		{
			return column + 3; // So can use cl, cc, cr
		}
	};

	class ProgressBarPainter : public TilePainter
	{
	public:
		ProgressBarPainter(int name, bool reverse) : TilePainter(name), m_reverse(reverse)
		{
			// We use only of the tip bitmaps..
			if (reverse) {
				colMde[0] = Fixed;
				colMde[1] = Tiled;
			} else {
				colMde[0] = Tiled;
				colMde[1] = Fixed;
			}
			rowMde[0] = Scaled;

			m_columns = 2;
		}

	protected:
		int tileName(unsigned int column, unsigned int /*row*/) const override
		{
			if (m_reverse)
				return column + 3; // So can use cl, cc, cr
			else
				return column + 4; // So can use cl, cc, cr + we start from cc.
		}

		bool m_reverse;
	};

	class ActiveTabPainter : public RectTilePainter
	{
	public:
		ActiveTabPainter(bool bottom);

	protected:
		int tileName(unsigned int column, unsigned int row) const override;

	private:
		bool m_bottom;
	};

	class InactiveTabPainter : public RectTilePainter
	{
	public:
		enum Mode { First, Middle, Last };
		InactiveTabPainter(Mode mode, bool bottom);

	protected:
		int tileName(unsigned int column, unsigned int row) const override;

	private:
		Mode m_mode;
		bool m_bottom;
	};

	class ScrollBarPainter : public TilePainter
	{
	public:
		ScrollBarPainter(int type, int count, bool horizontal);

		static int name(bool horizontal);

	protected:
		int tileName(unsigned int column, unsigned int row) const override;

	private:
		int m_type;
		int m_count;
		bool m_horizontal;
	};

	class SpinBoxPainter : public TilePainter
	{
	public:
		SpinBoxPainter() : TilePainter(keramik_spinbox)
		{
			colMde[0] = colMde[2] = Fixed;
			colMde[1] = Scaled;
			rowMde[0] = rowMde[1] = rowMde[2] = Scaled;
			m_columns = 3;
		}

	protected:
		int tileName(unsigned int column, unsigned int /*row*/) const override
		{
			return column + 1;
		}
	};
}
