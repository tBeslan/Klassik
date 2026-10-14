/*
   SPDX-FileCopyrightText: 2026 Klassik contributors

   SPDX-FileCopyrightText: 2002 Fredrik Höglund <fredrik@kde.org>

   SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <KDecoration3/Decoration>
#include <KDecoration3/DecorationButton>
#include <KDecoration3/DecorationButtonGroup>

#include <QVariant>
#include <QPixmap>

namespace Keramik
{

enum TilePixmap { TitleLeft = 0, TitleCenter, TitleRight,
				  CaptionSmallLeft, CaptionSmallCenter, CaptionSmallRight,
				  CaptionLargeLeft, CaptionLargeCenter, CaptionLargeRight,
				  GrabBarLeft, GrabBarCenter, GrabBarRight,
				  BorderLeft, BorderRight, NumTiles };

enum ButtonDeco { Menu = 0, OnAllDesktops, NotOnAllDesktops, Help, Minimize, Maximize,
				  Restore, Close, AboveOn, AboveOff, BelowOn, BelowOff, ShadeOn, ShadeOff,
				  NumButtonDecos };

struct Pixmaps
{
	// Tiles are stored at their original (unscaled) size, as in KDE 3,
	// and are scaled up by an integer factor when painted.
	QPixmap activeTiles[NumTiles];
	QPixmap inactiveTiles[NumTiles];

	// Button bevels: three frames side by side (normal, hover, pressed)
	QPixmap titleButtonRound;
	QPixmap titleButtonSquare;

	QPixmap buttonDecos[NumButtonDecos];

	const QPixmap &tile(TilePixmap t, bool active) const {
		return active ? activeTiles[t] : inactiveTiles[t];
	}
};

class Decoration : public KDecoration3::Decoration
{
	Q_OBJECT
public:
	explicit Decoration(QObject *parent = nullptr, const QVariantList &args = QVariantList());
	~Decoration() override = default;

	bool init() override;
	void paint(QPainter *p, const QRectF &repaintRegion) override;

	// Access pixmaps from outside of the decoration class (i.e. inside the button)
	const Pixmaps &pixmaps() const { return m_pixmaps; }

	// Integer factor that the original pixmaps are scaled up by
	int pixelScale() const { return m_pixelScale; }

	// Converts original pixmap units to logical decoration coordinates
	qreal toLogical(qreal units) const;

	int buttonSize() const { return m_pixmaps.titleButtonRound.height(); }
	int titleBaseY() const { return m_largeTitlebar ? 3 : 0; }

private:
	Pixmaps m_pixmaps;
	int m_pixelScale = 1;

	// Config (twinkeramikrc in KDE 3, klassikkeramikrc here)
	bool m_showIcons = true;
	bool m_shadowedText = true;
	bool m_smallCaptionBubbles = false;
	bool m_largeGrabBars = true;

	bool m_largeTitlebar = true;

	KDecoration3::DecorationButtonGroup *m_leftButtons = nullptr;
	KDecoration3::DecorationButtonGroup *m_rightButtons = nullptr;

	void readConfig();
	void reconfigure();
	void updateBorders();
	void updateButtonsGeometry();
	void createPixmaps();

	int titleBarHeight(bool large) const;
	int grabBarHeight() const { return m_pixmaps.activeTiles[GrabBarCenter].height(); }
	bool largeCaption() const;
	bool isMaximizedFully() const;
	QRect captionRect() const;
	QColor blendColor(bool active) const;
};


class Button : public KDecoration3::DecorationButton
{
	Q_OBJECT

public:
	explicit Button(KDecoration3::DecorationButtonType type,
					KDecoration3::Decoration *decoration,
					QObject *parent = nullptr);
	~Button() override = default;

	static Button *create(KDecoration3::DecorationButtonType type,
						  KDecoration3::Decoration *decoration,
						  QObject *parent);

	void paint(QPainter *p, const QRectF &repaintRegion) override;

private:
	bool isRound() const;
	ButtonDeco deco() const;
};

} // namespace Keramik
