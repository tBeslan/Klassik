/*
   SPDX-FileCopyrightText: 2026 Klassik contributors

   SPDX-FileCopyrightText: 2002 Fredrik Höglund <fredrik@kde.org>

   SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "keramik.h"

#include <KDecoration3/DecorationSettings>
#include <KDecoration3/DecoratedWindow>
#include <KPluginFactory>
#include <KIconEffect>
#include <KConfig>
#include <KConfigGroup>

#include <QPainter>
#include <QFontMetricsF>
#include <QPainterPath>
#include <QSvgRenderer>

K_PLUGIN_FACTORY_WITH_JSON(
	KeramikDecorationFactory,
	"metadata.json",
	registerPlugin<Keramik::Decoration>();
	)

namespace Keramik
{

// All of the following are in original (unscaled) pixmap units
static const int buttonMargin  = 13; // Distance between the window edge and the buttons
static const int buttonSpacing =  4; // Spacing between the titlebar buttons
static const int iconSpacing   =  5; // Spacing between the icon and the text label
static const int iconSize      = 16;
static const int decoSize      = 17; // Size of the button deco bitmaps

// Titlebar button bitmaps
static const uchar menu_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f, 0x00, 0xf0, 0x07, 0x00,
	0xe0, 0x03, 0x00, 0xc0, 0x01, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar on_all_desktops_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x01, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00, 0xf0, 0x0f, 0x00,
	0xf0, 0x0f, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar not_on_all_desktops_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00,
	0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar help_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x03, 0x00,
	0xf0, 0x07, 0x00, 0x30, 0x06, 0x00, 0x00, 0x07, 0x00, 0x80, 0x03, 0x00,
	0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar minimize_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar maximize_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x80, 0x01, 0x00, 0xc0, 0x03, 0x00, 0xe0, 0x07, 0x00, 0xf0, 0x0f, 0x00,
	0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar restore_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00,
	0xf0, 0x0f, 0x00, 0xe0, 0x07, 0x00, 0xc0, 0x03, 0x00, 0x80, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar close_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x30, 0x0c, 0x00, 0x70, 0x0e, 0x00, 0xe0, 0x07, 0x00, 0xc0, 0x03, 0x00,
	0xc0, 0x03, 0x00, 0xe0, 0x07, 0x00, 0x70, 0x0e, 0x00, 0x30, 0x0c, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar above_on_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x80, 0x01, 0x00, 0xe0, 0x07, 0x00, 0xc0, 0x03, 0x00, 0x80, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar above_off_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0xc0, 0x03, 0x00,
	0xe0, 0x07, 0x00, 0x80, 0x01, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar below_on_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0xc0, 0x03, 0x00,
	0xe0, 0x07, 0x00, 0x80, 0x01, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar below_off_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x80, 0x01, 0x00, 0xe0, 0x07, 0x00, 0xc0, 0x03, 0x00, 0x80, 0x01, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar shade_on_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x10, 0x08, 0x00, 0x10, 0x08, 0x00, 0x10, 0x08, 0x00, 0x10, 0x08, 0x00,
	0x10, 0x08, 0x00, 0x10, 0x08, 0x00, 0x10, 0x08, 0x00, 0xf0, 0x0f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static const uchar shade_off_bits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0xf0, 0x0f, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00};

static QImage
loadImage(const QString &name, const QColor &col)
{
	QImage img = QImage(QStringLiteral(":/keramik/pics/%1.png").arg(name)).convertToFormat(QImage::Format_ARGB32);
	if (col.isValid())
		KIconEffect::colorize(img, col, 1.0);
	return img;
}

static QPixmap
loadPixmap(const QString &name, const QColor &col)
{
	return QPixmap::fromImage(loadImage(name, col));
}

static QPixmap
renderSvg(const QString &name, const QSize &size)
{
	QPixmap pix(size);
	pix.fill(Qt::transparent);

	QSvgRenderer renderer(QStringLiteral(":/keramik/pics/%1.svg").arg(name));
	QPainter p(&pix);
	renderer.render(&p);
	p.end();

	return pix;
}

static QPixmap
composite(const QImage &over, const QImage &under)
{
	const int width = over.width();
	const int height = over.height();
	QImage dest(width, height, QImage::Format_ARGB32);
	dest.fill(0);

	// Copy the under image (bottom aligned) to the destination image
	for (int y1 = height - under.height(), y2 = 0; y1 < height; y1++, y2++) {
		if (y1 < 0)
			continue;
		QRgb *dst = reinterpret_cast<QRgb *>(dest.scanLine(y1));
		const QRgb *src = reinterpret_cast<const QRgb *>(under.constScanLine(y2));
		for (int x = 0; x < width && x < under.width(); x++)
			dst[x] = src[x];
	}

	// Blend the over image onto the destination
	for (int y = 0; y < height; y++) {
		QRgb *dst = reinterpret_cast<QRgb *>(dest.scanLine(y));
		const QRgb *src = reinterpret_cast<const QRgb *>(over.constScanLine(y));
		for (int x = 0; x < width; x++) {
			int r1 = qRed(dst[x]), g1 = qGreen(dst[x]), b1 = qBlue(dst[x]);
			int r2 = qRed(src[x]), g2 = qGreen(src[x]), b2 = qBlue(src[x]);
			int a  = qAlpha(src[x]);

			if (a == 0xff)
				dst[x] = src[x];
			else if (a != 0x00)
				dst[x] = qRgba(quint8(r1 + (((r2 - r1) * a) >> 8)),
							   quint8(g1 + (((g2 - g1) * a) >> 8)),
							   quint8(b1 + (((b2 - b1) * a) >> 8)),
							   0xff);
			else if (qAlpha(dst[x]) == 0x00)
				dst[x] = 0;
		}
	}

	return QPixmap::fromImage(dest);
}

static void
pretile(QPixmap &pix, int size, Qt::Orientation dir)
{
	if (pix.isNull())
		return;

	QPixmap newpix = (dir == Qt::Horizontal) ? QPixmap(size, pix.height()) : QPixmap(pix.width(), size);
	newpix.fill(Qt::transparent);

	QPainter p(&newpix);
	p.drawTiledPixmap(newpix.rect(), pix);
	p.end();

	pix = newpix;
}

static void
addWidth(int width, QPixmap &pix, bool left, const QPixmap &bottomPix)
{
	int w = pix.width() + width;
	int h = pix.height();

	QPixmap tmp(w, h);
	tmp.fill(Qt::white);
	QPainter p(&tmp);

	for (int i = 0; i < h; i++)
		p.drawPixmap(0, i, bottomPix, i % 2, 0, w, 1);

	if (left)
		p.drawPixmap(0, 0, pix);
	else
		p.drawPixmap(width, 0, pix);

	p.end();
	pix = tmp;
}

static void
addHeight(int height, QPixmap &pix)
{
	if (pix.isNull())
		return;

	int w = pix.width();
	int h = pix.height() + height;

	QPixmap tmp(w, h);
	tmp.fill(Qt::transparent);
	QPainter p(&tmp);
	if (pix.height() > 10) {
		p.drawPixmap(0, 0, pix, 0, 0, w, 11);
		for (int i = 0; i < height; i += 2)
			p.drawPixmap(0, 11 + i, pix, 0, 11, w, 2);
		p.drawPixmap(0, 11 + height, pix, 0, 11, w, pix.height() - 11);
	} else {
		int lines  = h - 3;
		int factor = pix.height() - 3;
		for (int i = 0; i < lines; i++)
			p.drawPixmap(0, i, pix, 0, i * factor / lines, w, 1);
		p.drawPixmap(0, lines, pix, 0, factor, w, 3);
	}
	p.end();

	pix = tmp;
}

// KDE 3 used a window mask to round off the titlebar corners and the top of
// the large caption bubble; here the masked out pixels are made transparent.
// Each entry is the number of pixels cut from the outer edge of a row.
static void
cutCorner(QPixmap &pix, std::initializer_list<int> rows, bool left)
{
	if (pix.isNull())
		return;

	QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
	int y = 0;
	for (int cut : rows) {
		if (y >= img.height())
			break;
		QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y++));
		for (int i = 0; i < cut && i < img.width(); i++)
			line[left ? i : img.width() - 1 - i] = 0;
	}
	pix = QPixmap::fromImage(img);
}

// When the tiles are scaled up, the stair-stepped corner of the original
// pixmap is replaced by a smooth, antialiased curve running through the
// middle of each step, so it keeps its KDE 3 shape without the jaggies.
static void
smoothCorner(QPixmap &pix, int factor, bool left)
{
	if (pix.isNull() || factor == 1)
		return;

	QImage img = pix.toImage().convertToFormat(QImage::Format_ARGB32);
	if (!left)
		img = img.flipped(Qt::Horizontal);

	// Find the steps: the number of transparent pixels at the start of each row
	auto transparentCount = [&img](int y) {
		const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));
		int cut = 0;
		while (cut < img.width() && qAlpha(line[cut]) < 128)
			cut++;
		return cut;
	};

	QPolygonF steps;
	for (int u = 0; u < img.height() / factor; u++) {
		int cut = transparentCount(u * factor + factor / 2);
		steps << QPointF(cut, (u + 0.5) * factor);
		if (cut == 0)
			break;
	}
	if (steps.size() < 3 || steps.last().x() != 0)
		return;

	const int boxWidth = std::min(img.width(), int(steps.first().x()) + factor);
	const int boxHeight = int(steps.last().y() + factor / 2.0);

	QPainterPath path;
	path.moveTo(steps.first().x(), 0);
	path.lineTo(steps.first());
	for (int i = 1; i < steps.size() - 1; i++)
		path.quadTo(steps[i], (steps[i] + steps[i + 1]) / 2);
	path.lineTo(steps.last());
	path.lineTo(0, boxHeight);
	path.lineTo(boxWidth, boxHeight);
	path.lineTo(boxWidth, 0);
	path.closeSubpath();

	QImage coverage(boxWidth, boxHeight, QImage::Format_ARGB32_Premultiplied);
	coverage.fill(Qt::transparent);
	QPainter p(&coverage);
	p.setRenderHint(QPainter::Antialiasing);
	p.fillPath(path, Qt::white);
	p.end();

	for (int y = 0; y < boxHeight; y++) {
		QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y));
		const QRgb *cov = reinterpret_cast<const QRgb *>(coverage.constScanLine(y));
		const int cut = transparentCount(y);
		if (cut >= img.width())
			continue;
		const QRgb edge = line[cut];
		for (int x = 0; x < boxWidth; x++) {
			const QRgb c = qAlpha(line[x]) < 128 ? edge : line[x];
			line[x] = qRgba(qRed(c), qGreen(c), qBlue(c), qAlpha(cov[x]));
		}
	}

	if (!left)
		img = img.flipped(Qt::Horizontal);
	pix = QPixmap::fromImage(img);
}

static QPixmap
scaled(const QPixmap &pix, int factor)
{
	if (factor == 1 || pix.isNull())
		return pix;
	return pix.scaled(pix.size() * factor, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

Decoration::Decoration(QObject *parent, const QVariantList &args) : KDecoration3::Decoration(parent, args)
{
}

bool
Decoration::init()
{
	readConfig();
	createPixmaps();
	updateBorders();

	// Create buttons
	auto createGroup = [this](KDecoration3::DecorationButtonGroup::Position pos) {
		return new KDecoration3::DecorationButtonGroup(pos, this, &Button::create);
	};
	m_leftButtons  = createGroup(KDecoration3::DecorationButtonGroup::Position::Left);
	m_rightButtons = createGroup(KDecoration3::DecorationButtonGroup::Position::Right);

	auto s = settings();

	// Signals requiring a simple update
	connect(window(), &KDecoration3::DecoratedWindow::activeChanged, this, qOverload<>(&Decoration::update));
	connect(window(), &KDecoration3::DecoratedWindow::captionChanged, this, qOverload<>(&Decoration::update));
	connect(window(), &KDecoration3::DecoratedWindow::iconChanged, this, qOverload<>(&Decoration::update));

	// Signals requiring full reconfiguration
	connect(s.get(), &KDecoration3::DecorationSettings::reconfigured, this, &Decoration::reconfigure);
	connect(s.get(), &KDecoration3::DecorationSettings::borderSizeChanged, this, &Decoration::reconfigure);
	connect(s.get(), &KDecoration3::DecorationSettings::fontChanged, this, &Decoration::reconfigure);
	connect(window(), &KDecoration3::DecoratedWindow::paletteChanged, this, &Decoration::reconfigure);
	connect(window(), &KDecoration3::DecoratedWindow::scaleChanged, this, &Decoration::reconfigure);

	// Signals requiring border size changes
	connect(window(), &KDecoration3::DecoratedWindow::maximizedChanged, this, &Decoration::updateBorders);
	connect(window(), &KDecoration3::DecoratedWindow::maximizedVerticallyChanged, this, &Decoration::updateBorders);

	// Signals requiring button and titlebar geometry recalculation
	connect(this, &KDecoration3::Decoration::bordersChanged, this, &Decoration::updateButtonsGeometry);
	connect(s.get(), &KDecoration3::DecorationSettings::decorationButtonsLeftChanged, this, &Decoration::updateButtonsGeometry);
	connect(s.get(), &KDecoration3::DecorationSettings::decorationButtonsRightChanged, this, &Decoration::updateButtonsGeometry);
	connect(window(), &KDecoration3::DecoratedWindow::sizeChanged, this, &Decoration::updateButtonsGeometry);
	connect(window(), &KDecoration3::DecoratedWindow::widthChanged, this, &Decoration::updateButtonsGeometry);
	connect(window(), &KDecoration3::DecoratedWindow::shadedChanged, this, &Decoration::updateButtonsGeometry);

	updateButtonsGeometry();
	update();

	return true;
}

qreal
Decoration::toLogical(qreal units) const
{
	return units * m_pixelScale / window()->scale();
}

void
Decoration::readConfig()
{
	KConfig config(QStringLiteral("klassikkeramikrc"));
	KConfigGroup group(&config, QStringLiteral("General"));
	m_showIcons = group.readEntry("ShowAppIcons", true);
	m_shadowedText = group.readEntry("UseShadowedText", true);
	m_smallCaptionBubbles = group.readEntry("SmallCaptionBubbles", false);
	m_largeGrabBars = group.readEntry("LargeGrabBars", true);
}

void
Decoration::reconfigure()
{
	readConfig();
	createPixmaps();
	updateBorders();
	updateButtonsGeometry();
}

bool
Decoration::isMaximizedFully() const
{
	return window()->isMaximized();
}

bool
Decoration::largeCaption() const
{
	return window()->isActive() && m_largeTitlebar;
}

int
Decoration::titleBarHeight(bool large) const
{
	return m_pixmaps.activeTiles[large ? CaptionLargeCenter : CaptionSmallCenter].height();
}

QColor
Decoration::blendColor(bool active) const
{
	// KDecoration3 doesn't expose the titlebar blend colour, so read it
	// directly from the colour scheme, like KDE 3's ColorTitleBlend
	KConfig config(QStringLiteral("kdeglobals"));
	KConfigGroup group(&config, QStringLiteral("WM"));
	QColor color = group.readEntry(active ? "activeBlend" : "inactiveBlend", QColor());
	if (color.isValid())
		return color;

	return window()->color(active ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive,
						   KDecoration3::ColorRole::Frame);
}

void
Decoration::updateBorders()
{
	m_largeTitlebar = !m_smallCaptionBubbles && !window()->isMaximizedVertically();

	const qreal scale = window()->scale();
	qreal left = m_pixmaps.activeTiles[BorderLeft].width();
	qreal right = m_pixmaps.activeTiles[BorderRight].width();
	qreal top = titleBarHeight(m_largeTitlebar);
	qreal bottom = grabBarHeight();

	if (isMaximizedFully())
		left = right = bottom = 0;

	setBorders(QMarginsF(left / scale, top / scale, right / scale, bottom / scale));
}

void
Decoration::updateButtonsGeometry()
{
	if (!m_leftButtons || !m_rightButtons)
		return;

	const qreal scale = window()->scale();
	const int n = m_pixelScale;
	const qreal btnSize = buttonSize() / scale;

	const auto buttons = m_leftButtons->buttons() + m_rightButtons->buttons();
	for (auto *button : buttons) {
		if (button->type() == KDecoration3::DecorationButtonType::Spacer)
			button->setGeometry(QRectF(0, 0, toLogical(buttonSpacing), btnSize));
		else
			button->setGeometry(QRectF(0, 0, btnSize, btnSize));
	}
	m_leftButtons->setSpacing(toLogical(buttonSpacing));
	m_rightButtons->setSpacing(toLogical(buttonSpacing));

	// Centre the buttons vertically below the top spacing, like the original layout
	const int topSpacing = m_largeTitlebar ? 4 : 1;
	const int rowHeight = titleBarHeight(m_largeTitlebar) / n - topSpacing;
	const qreal y = toLogical(topSpacing + (rowHeight - buttonSize() / n) / 2);

	m_leftButtons->setPos(QPointF(toLogical(buttonMargin), y));
	m_rightButtons->setPos(QPointF(size().width() - toLogical(buttonMargin - 1) - m_rightButtons->geometry().width(), y));

	// The top few pixels are left for resizing, as in KDE 3
	const qreal titleY = isMaximizedFully() ? 0 : toLogical(3);
	setTitleBar(QRectF(borderLeft(), titleY, size().width() - borderLeft() - borderRight(), borderTop() - titleY));

	update();
}

void
Decoration::createPixmaps()
{
	m_pixelScale = std::max(1, qRound(window()->scale()));

	int heightOffset;
	int widthOffset;
	{
		using enum KDecoration3::BorderSize;
		switch (settings()->borderSize()) {
		case Large:
			widthOffset = 4;
			heightOffset = 0;
			break;
		case VeryLarge:
			widthOffset = 8;
			heightOffset = 0;
			break;
		case Huge:
			widthOffset = 14;
			heightOffset = 0;
			break;
		case VeryHuge:
			widthOffset = 23;
			heightOffset = 10;
			break;
		case Oversized:
			widthOffset = 36;
			heightOffset = 25;
			break;
		case Normal:
		default:
			widthOffset = 0;
			heightOffset = 0;
		}
	}
	int fontHeight = QFontMetrics(settings()->font()).height();
	if (fontHeight > heightOffset + 20)
		heightOffset = fontHeight - 20;

	const QString size = (heightOffset < 8) ? QString() : (heightOffset < 20) ? QStringLiteral("-large") : QStringLiteral("-huge");

	const QString grabBarPrefix = m_largeGrabBars ? QStringLiteral("grabbar") : QStringLiteral("bottom");

	auto createTiles = [&](QPixmap *tiles, bool active) {
		const QColor captionColor = window()->color(active ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive,
													KDecoration3::ColorRole::TitleBar);
		const QColor titleColor = blendColor(active);

		// Load the titlebar corners
		tiles[TitleLeft]  = loadPixmap(QStringLiteral("titlebar-left"), titleColor);
		tiles[TitleRight] = loadPixmap(QStringLiteral("titlebar-right"), titleColor);

		// Load the titlebar center tile image (this will be used as
		// the background for the caption bubble tiles)
		const QImage titleCenter = loadImage(QStringLiteral("titlebar-center"), titleColor);

		// Create the caption bubble tiles (by blending the images onto the titlebar)
		tiles[CaptionSmallLeft]   = composite(loadImage(QStringLiteral("caption-small-left"), captionColor), titleCenter);
		tiles[CaptionSmallRight]  = composite(loadImage(QStringLiteral("caption-small-right"), captionColor), titleCenter);
		tiles[CaptionSmallCenter] = composite(loadImage(QStringLiteral("caption-small-center"), captionColor), titleCenter);

		// The large caption bubble is only ever shown on active windows
		if (active) {
			tiles[CaptionLargeLeft]   = composite(loadImage(QStringLiteral("caption-large-left"), captionColor), titleCenter);
			tiles[CaptionLargeRight]  = composite(loadImage(QStringLiteral("caption-large-right"), captionColor), titleCenter);
			tiles[CaptionLargeCenter] = composite(loadImage(QStringLiteral("caption-large-center"), captionColor), titleCenter);
		} else {
			tiles[CaptionLargeLeft] = tiles[CaptionLargeRight] = tiles[CaptionLargeCenter] = QPixmap();
		}

		tiles[TitleCenter] = QPixmap::fromImage(titleCenter);

		// Load the left & right border pixmaps
		tiles[BorderLeft]  = loadPixmap(QStringLiteral("border-left"), titleColor);
		tiles[BorderRight] = loadPixmap(QStringLiteral("border-right"), titleColor);

		// Load the bottom grabbar pixmaps
		tiles[GrabBarLeft]   = loadPixmap(grabBarPrefix + QStringLiteral("-left"), titleColor);
		tiles[GrabBarRight]  = loadPixmap(grabBarPrefix + QStringLiteral("-right"), titleColor);
		tiles[GrabBarCenter] = loadPixmap(grabBarPrefix + QStringLiteral("-center"), titleColor);

		// Pretile the center & border tiles
		pretile(tiles[CaptionSmallCenter], 64, Qt::Horizontal);
		pretile(tiles[CaptionLargeCenter], 64, Qt::Horizontal);
		pretile(tiles[TitleCenter], 64, Qt::Horizontal);
		pretile(tiles[GrabBarCenter], 128, Qt::Horizontal);
		pretile(tiles[BorderLeft], 128, Qt::Vertical);
		pretile(tiles[BorderRight], 128, Qt::Vertical);

		if (heightOffset > 0) {
			for (auto t : {TitleLeft, TitleCenter, TitleRight,
						   CaptionSmallLeft, CaptionSmallCenter, CaptionSmallRight,
						   CaptionLargeLeft, CaptionLargeCenter, CaptionLargeRight})
				addHeight(heightOffset, tiles[t]);
		}

		if (widthOffset > 0) {
			addWidth(widthOffset, tiles[BorderLeft], true, tiles[GrabBarCenter]);
			addWidth(widthOffset, tiles[BorderRight], false, tiles[GrabBarCenter]);

			int grabBarOffset = m_largeGrabBars ? widthOffset * 3 / 2 : widthOffset;
			for (auto t : {GrabBarLeft, GrabBarCenter, GrabBarRight})
				addHeight(grabBarOffset, tiles[t]);
		}

		// Round off the corners (see KeramikClient::updateMask() in KDE 3)
		cutCorner(tiles[TitleLeft], {8, 6, 4, 3, 2, 2, 1, 1}, true);
		cutCorner(tiles[TitleRight], {9, 7, 5, 4, 3, 2, 1, 1}, false);
		cutCorner(tiles[CaptionLargeLeft], {8, 6, 5}, true);
		cutCorner(tiles[CaptionLargeRight], {11, 9, 7}, false);

		// Finally scale the tiles up for HiDPI
		for (int i = 0; i < NumTiles; i++)
			tiles[i] = scaled(tiles[i], m_pixelScale);

		smoothCorner(tiles[TitleLeft], m_pixelScale, true);
		smoothCorner(tiles[TitleRight], m_pixelScale, false);
	};

	createTiles(m_pixmaps.activeTiles, true);
	createTiles(m_pixmaps.inactiveTiles, false);

	// Buttons
	if (m_pixelScale == 1) {
		m_pixmaps.titleButtonRound  = loadPixmap(QStringLiteral("titlebutton-round") + size, QColor());
		m_pixmaps.titleButtonSquare = loadPixmap(QStringLiteral("titlebutton-square") + size, QColor());
	} else {
		// Render the vector versions when scaling up, at the size of the matching pixmap
		const int buttonSize = loadImage(QStringLiteral("titlebutton-round") + size, QColor()).height();
		m_pixmaps.titleButtonRound  = renderSvg(QStringLiteral("titlebutton-round"), QSize(3 * buttonSize, buttonSize) * m_pixelScale);
		m_pixmaps.titleButtonSquare = renderSvg(QStringLiteral("titlebutton-square"), QSize(3 * buttonSize, buttonSize) * m_pixelScale);
	}

	// Button decos
	if (m_pixelScale == 1) {
		const uchar *decoBits[NumButtonDecos] = {
			menu_bits, on_all_desktops_bits, not_on_all_desktops_bits, help_bits,
			minimize_bits, maximize_bits, restore_bits, close_bits,
			above_on_bits, above_off_bits, below_on_bits, below_off_bits,
			shade_on_bits, shade_off_bits
		};
		for (int i = 0; i < NumButtonDecos; i++) {
			QImage img = QImage(decoBits[i], decoSize, decoSize, (decoSize + 7) / 8, QImage::Format_MonoLSB);
			img.setColorTable({qRgba(0, 0, 0, 0), qRgba(0, 0, 0, 255)});
			m_pixmaps.buttonDecos[i] = QPixmap::fromImage(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));
		}
	} else {
		// The vector decos are laid out side by side, in the order of the ButtonDeco enum
		const int cell = decoSize * m_pixelScale;
		const QPixmap decos = renderSvg(QStringLiteral("titlebutton-decos"), QSize(NumButtonDecos * cell, cell));
		for (int i = 0; i < NumButtonDecos; i++)
			m_pixmaps.buttonDecos[i] = decos.copy(i * cell, 0, cell, cell);
	}
}

QRect
Decoration::captionRect() const
{
	// Returns the caption bubble rect in device pixels
	const qreal scale = window()->scale();
	const int n = m_pixelScale;
	const int w = qRound(size().width() * scale);

	// The space between the button groups, see createLayout() in KDE 3
	int x1 = buttonMargin * n;
	if (m_leftButtons && m_leftButtons->geometry().width() > 0)
		x1 = qRound(m_leftButtons->geometry().right() * scale) + buttonSpacing * n;

	int x2 = w - (buttonMargin - 1 + 2 * buttonSpacing) * n;
	if (m_rightButtons && m_rightButtons->geometry().width() > 0)
		x2 = qRound(m_rightButtons->geometry().left() * scale) - 3 * buttonSpacing * n;

	QFontMetricsF fm(settings()->font());
	int cw = qCeil(fm.horizontalAdvance(window()->caption()) * scale) + 95 * n;
	if (m_showIcons)
		cw += (iconSize + 4) * n;
	cw = std::max(0, std::min(cw, x2 - x1));

	const bool large = largeCaption();
	return QRect(x1, large ? 0 : titleBaseY() * n, cw, titleBarHeight(large));
}

void
Decoration::paint(QPainter *p, const QRectF &repaintRegion)
{
	const qreal scale = window()->scale();
	const int n = m_pixelScale;
	const bool active = window()->isActive();
	const Pixmaps &px = m_pixmaps;

	// Paint in device pixels, so that the scaled-up tiles stay sharp
	p->save();
	p->setRenderHint(QPainter::Antialiasing, false);
	p->setRenderHint(QPainter::SmoothPixmapTransform, false);
	p->scale(1 / scale, 1 / scale);

	const int w = qRound(size().width() * scale);
	const int h = qRound(size().height() * scale);
	const int baseY = titleBaseY() * n;
	const int titleHeight = titleBarHeight(m_largeTitlebar);
	const int baseHeight = titleHeight - baseY;
	const int leftWidth = borderLeft() > 0 ? px.tile(BorderLeft, active).width() : 0;
	const int rightWidth = borderRight() > 0 ? px.tile(BorderRight, active).width() : 0;
	const int grabHeight = borderBottom() > 0 ? grabBarHeight() : 0;
	const QRect caption = captionRect();

	// Titlebar
	// -----------------------------------------------------------------------
	const QPixmap &titleLeft = px.tile(TitleLeft, active);
	const QPixmap &titleCenter = px.tile(TitleCenter, active);
	const QPixmap &titleRight = px.tile(TitleRight, active);

	// Top left corner
	p->drawPixmap(0, baseY, titleLeft);

	// Space between the top left corner and the caption bubble
	if (caption.left() > titleLeft.width())
		p->drawTiledPixmap(titleLeft.width(), baseY, caption.left() - titleLeft.width(), baseHeight, titleCenter);

	// Caption bubble
	if (caption.width() >= 25 * n) {
		const bool large = largeCaption();
		const QPixmap &left = px.tile(large ? CaptionLargeLeft : CaptionSmallLeft, active);
		const QPixmap &center = px.tile(large ? CaptionLargeCenter : CaptionSmallCenter, active);
		const QPixmap &right = px.tile(large ? CaptionLargeRight : CaptionSmallRight, active);

		p->drawPixmap(caption.x(), caption.y(), left);
		p->drawTiledPixmap(caption.x() + left.width(), caption.y(),
						   caption.width() - left.width() - right.width(), caption.height(), center);
		p->drawPixmap(caption.x() + caption.width() - right.width(), caption.y(), right);
	} else if (caption.width() > 0) {
		p->drawTiledPixmap(caption.x(), baseY, caption.width(), baseHeight, titleCenter);
	}

	// Space between the caption bubble and the top right corner
	if (w - titleRight.width() > caption.x() + caption.width())
		p->drawTiledPixmap(caption.x() + caption.width(), baseY,
						   w - titleRight.width() - caption.x() - caption.width(), baseHeight, titleCenter);

	// Top right corner
	p->drawPixmap(w - titleRight.width(), baseY, titleRight);

	// Borders
	// -----------------------------------------------------------------------
	if (h - grabHeight > titleHeight) {
		if (leftWidth > 0)
			p->drawTiledPixmap(0, titleHeight, leftWidth, h - grabHeight - titleHeight, px.tile(BorderLeft, active));
		if (rightWidth > 0)
			p->drawTiledPixmap(w - rightWidth, titleHeight, rightWidth, h - grabHeight - titleHeight, px.tile(BorderRight, active));
	}

	// Bottom grab bar
	// -----------------------------------------------------------------------
	if (grabHeight > 0) {
		const QPixmap &grabLeft = px.tile(GrabBarLeft, active);
		const QPixmap &grabRight = px.tile(GrabBarRight, active);

		p->drawPixmap(0, h - grabHeight, grabLeft);
		if (w - grabRight.width() > grabLeft.width())
			p->drawTiledPixmap(grabLeft.width(), h - grabHeight, w - grabLeft.width() - grabRight.width(),
							   grabHeight, px.tile(GrabBarCenter, active));
		p->drawPixmap(w - grabRight.width(), h - grabHeight, grabRight);
	}

	// Caption bubble contents
	// -----------------------------------------------------------------------
	if (caption.width() >= 25 * n) {
		QFontMetricsF fm(settings()->font());
		const QString text = window()->caption();
		const int tw = qCeil(fm.horizontalAdvance(text) * scale) + (m_showIcons ? (iconSize + iconSpacing) * n : 0);
		const int xpos = std::max((caption.width() - tw) / 3, 8 * n);
		QRect tr(caption.x() + xpos, caption.y() + n, caption.width() - xpos - 10 * n, caption.height() - 4 * n);

		// Application icon
		if (m_showIcons) {
			QPixmap icon = window()->icon().pixmap(QSize(iconSize * n, iconSize * n), 1.0);
			icon.setDevicePixelRatio(1.0);
			if (!active)
				KIconEffect::semiTransparent(icon);

			QRect iconRect(tr.x(), caption.y() + n + (caption.height() - 4 * n - iconSize * n) / 2,
						   iconSize * n, iconSize * n);
			QRect r(icon.rect());
			r.moveCenter(iconRect.center());

			if (tr.width() > iconSize * n)
				p->drawPixmap(r.topLeft(), icon);
			else if (tr.width() > 0)
				p->drawPixmap(r.topLeft(), icon, QRect(0, 0, tr.width(), icon.height()));

			tr.adjust((iconSize + iconSpacing) * n, 0, 0, 0);
		}

		// Draw the titlebar text in logical coordinates
		if (tr.width() > 0) {
			p->save();
			p->scale(scale, scale);
			p->setFont(settings()->font());

			const QRectF textRect(tr.x() / scale, tr.y() / scale, tr.width() / scale, tr.height() / scale);
			const QString elided = fm.elidedText(text, Qt::ElideRight, textRect.width());
			const int flags = Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine;
			const QColor fontColor = window()->color(active ? KDecoration3::ColorGroup::Active : KDecoration3::ColorGroup::Inactive,
													 KDecoration3::ColorRole::Foreground);

			if (m_shadowedText) {
				p->setPen(qGray(fontColor.rgb()) < 100 ? QColor(200, 200, 200) : QColor(Qt::black));
				p->drawText(textRect.translated(n / scale, n / scale), flags, elided);
			}

			p->setPen(fontColor);
			p->drawText(textRect, flags, elided);
			p->restore();
		}
	}

	if (m_leftButtons)
		m_leftButtons->paint(p, repaintRegion);
	if (m_rightButtons)
		m_rightButtons->paint(p, repaintRegion);

	p->restore();
}

Button::Button(KDecoration3::DecorationButtonType type,
			   KDecoration3::Decoration *decoration,
			   QObject *parent)
	: KDecoration3::DecorationButton(type, decoration, parent)
{
	setGeometry(QRectF(0, 0, decoSize, decoSize)); // default size as a backup
}

Button *
Button::create(KDecoration3::DecorationButtonType type,
			   KDecoration3::Decoration *decoration,
			   QObject *parent)
{
	if (auto d = qobject_cast<Decoration *>(decoration))
		return new Button(type, d, parent);
	return nullptr;
}

bool
Button::isRound() const
{
	using enum KDecoration3::DecorationButtonType;
	switch (type()) {
	case Menu:
	case ApplicationMenu:
	case OnAllDesktops:
	case ContextHelp:
		return true;
	default:
		return false;
	}
}

ButtonDeco
Button::deco() const
{
	using enum KDecoration3::DecorationButtonType;
	switch (type()) {
	case Menu:
	case ApplicationMenu:
		return Keramik::Menu;
	case OnAllDesktops:
		return isChecked() ? NotOnAllDesktops : Keramik::OnAllDesktops;
	case ContextHelp:
		return Help;
	case Minimize:
		return Keramik::Minimize;
	case Maximize:
		return isChecked() ? Restore : Keramik::Maximize;
	case Close:
		return Keramik::Close;
	case KeepAbove:
		return isChecked() ? AboveOn : AboveOff;
	case KeepBelow:
		return isChecked() ? BelowOn : BelowOff;
	case Shade:
		return isChecked() ? ShadeOn : ShadeOff;
	default:
		return NumButtonDecos;
	}
}

void
Button::paint(QPainter *p, const QRectF &repaintRegion)
{
	Q_UNUSED(repaintRegion);

	if (type() == KDecoration3::DecorationButtonType::Spacer)
		return;

	auto *deco = qobject_cast<Decoration *>(decoration());
	if (!deco)
		return;
	const Pixmaps &px = deco->pixmaps();
	const int n = deco->pixelScale();
	const qreal scale = deco->window()->scale();
	const bool active = deco->window()->isActive();

	const QPixmap &bevel = isRound() ? px.titleButtonRound : px.titleButtonSquare;
	const int size = bevel.height();
	const int x = qRound(geometry().x() * scale);
	const int y = qRound(geometry().y() * scale);

	// Draw the button background
	const QPixmap &background = px.tile(TitleCenter, active);
	const int sy = ((background.height() / n - size / n + 1) / 2) * n;
	p->drawPixmap(x, y, background, 0, sy, size, size);

	// Draw the bevel: normal, hover and pressed frames are side by side
	const int frame = isPressed() ? 2 : isHovered() ? 1 : 0;
	p->drawPixmap(x, y, bevel, frame * size, 0, size, size);

	// Draw the button deco on the bevel
	const ButtonDeco d = this->deco();
	if (d == NumButtonDecos)
		return;

	const QPixmap &decoPixmap = px.buttonDecos[d];
	const int offset = isPressed() ? n : 0;
	p->drawPixmap(x + (size - decoPixmap.width()) / 2 + offset, y + (size - decoPixmap.height()) / 2 + offset, decoPixmap);
}

} // namespace Keramik

#include "keramik.moc"
