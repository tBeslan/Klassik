/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Malte Starostik <malte@kde.org>
   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>
   SPDX-FileCopyrightText: 2001-2002 Karol Szwed <gallium@kde.org>
   SPDX-FileCopyrightText: 2001-2002 Fredrik Höglund <fredrik@kde.org>
   SPDX-FileCopyrightText: 2000 Daniel M. Duley <mosfet@kde.org>
   SPDX-FileCopyrightText: 2000 Dirk Mueller <mueller@kde.org>
   SPDX-FileCopyrightText: 2001 Martijn Klingens <klingens@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "keramikstyle.h"
#include "gradients.h"
#include "keramikimage.h"
#include "pixmaploader.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QPainter>
#include <QProgressBar>
#include <QStyleOption>
#include <QTimerEvent>
#include <QToolBar>

#include <qdrawutil.h>

#include <cmath>

#define loader Keramik::PixmapLoader::the()

namespace
{
	const int itemFrame       = 2;
	const int itemHMargin     = 6;
	const int itemVMargin     = 0;
	const int arrowHMargin    = 6;
	const int rightBorder     = 12;

	const int smallButMaxW    = 27;
	const int smallButMaxH    = 20;

	// Line segments (x1, y1, x2, y2), relative to the arrow center
	const int keramik_combo_arrow[] = {
		-4, -5, 4, -5,
		-2, -2, 2, -2,
		-2, -1, 2, -1,
		-2,  0, 2,  0,
		-4,  1, 4,  1,
		-3,  2, 3,  2,
		-2,  3, 2,  3,
		-1,  4, 1,  4,
		 0,  5, 0,  5
	};

	const int keramik_up_arrow[] = {
		 0, -4, 0, -4,
		-1, -3, 1, -3,
		-2, -2, 2, -2,
		-3, -1, 3, -1,
		-4,  0, 4,  0,
		-2,  1, 2,  1,
		-2,  2, 2,  2,
		-2,  3, 2,  3,
		-2,  4, 2,  4
	};

	const int keramik_down_arrow[] = {
		 0,  4, 0,  4,
		-1,  3, 1,  3,
		-2,  2, 2,  2,
		-3,  1, 3,  1,
		-4,  0, 4,  0,
		-2, -1, 2, -1,
		-2, -2, 2, -2,
		-2, -3, 2, -3,
		-2, -4, 2, -4
	};

	const int keramik_right_arrow[] = {
		 4,  0,  4, 0,
		 3, -1,  3, 1,
		 2, -2,  2, 2,
		 1, -3,  1, 3,
		 0, -4,  0, 4,
		-1, -2, -1, 2,
		-2, -2, -2, 2,
		-3, -2, -3, 2,
		-4, -2, -4, 2
	};

	const int keramik_left_arrow[] = {
		-4,  0, -4, 0,
		-3, -1, -3, 1,
		-2, -2, -2, 2,
		-1, -3, -1, 3,
		 0, -4,  0, 4,
		 1, -2,  1, 2,
		 2, -2,  2, 2,
		 3, -2,  3, 2,
		 4, -2,  4, 2
	};

	// All the lines Keramik draws are horizontal or vertical. Drawing them
	// as rectangles keeps them pixel aligned when the painter is scaled.
	void drawLine(QPainter *p, int x1, int y1, int x2, int y2, const QColor &color)
	{
		p->fillRect(QRect(QPoint(qMin(x1, x2), qMin(y1, y2)),
						  QPoint(qMax(x1, x2), qMax(y1, y2))), color);
	}

	void drawPoint(QPainter *p, int x, int y, const QColor &color)
	{
		p->fillRect(QRect(x, y, 1, 1), color);
	}

	template<int N>
	void drawSegments(QPainter *p, const int (&segments)[N], int dx, int dy, const QColor &color)
	{
		for (int i = 0; i < N; i += 4)
			drawLine(p, segments[i] + dx, segments[i + 1] + dy,
					 segments[i + 2] + dx, segments[i + 3] + dy, color);
	}

	void drawKeramikArrow(QPainter *p, const QPalette &pal, const QRect &r,
						  QStyle::PrimitiveElement pe, bool down, bool enabled)
	{
		auto draw = [&](int dx, int dy, const QColor &color) {
			switch (pe) {
			case QStyle::PE_IndicatorArrowUp:
				drawSegments(p, keramik_up_arrow, dx, dy, color);
				break;
			case QStyle::PE_IndicatorArrowDown:
				drawSegments(p, keramik_down_arrow, dx, dy, color);
				break;
			case QStyle::PE_IndicatorArrowLeft:
				drawSegments(p, keramik_left_arrow, dx, dy, color);
				break;
			default:
				drawSegments(p, keramik_right_arrow, dx, dy, color);
				break;
			}
		};

		if (enabled) {
			//CHECKME: Why is the -1 needed?
			draw(r.x() + r.width() / 2 - 1, r.y() + r.height() / 2,
				 down ? pal.button().color() : pal.buttonText().color());
		} else {
			draw(r.x() + r.width() / 2, r.y() + r.height() / 2 + 1, pal.light().color());
			draw(r.x() + r.width() / 2 - 1, r.y() + r.height() / 2, pal.mid().color());
		}
	}

	// The frame around the list of a non-editable combobox popup
	bool isComboBoxPopup(const QWidget *widget)
	{
		return widget && widget->inherits("QComboBoxPrivateContainer");
	}

	bool isBusy(const QProgressBar *bar)
	{
		return bar->minimum() == 0 && bar->maximum() == 0;
	}

	// Busy progress bars move their block every busyInterval ms,
	// by busyStep pixels
	const int busyInterval = 30;
	const int busyStep = 2;

	// Rectangle at the origin that a control of rect r gets drawn in, when
	// the painter is scaled by k
	QRect reducedRect(const QRect &r, qreal k)
	{
		return QRect(0, 0, int(std::ceil(r.width() / k - 0.01)), int(std::ceil(r.height() / k - 0.01)));
	}

	// Maps a rectangle in the reduced coordinates of a control of rect r back
	QRect enlargedRect(const QRect &reduced, const QRect &r, qreal k)
	{
		const int left = r.x() + qRound(reduced.x() * k);
		const int top = r.y() + qRound(reduced.y() * k);
		const int right = r.x() + qRound((reduced.x() + reduced.width()) * k);
		const int bottom = r.y() + qRound((reduced.y() + reduced.height()) * k);
		return QRect(left, top, right - left, bottom - top);
	}

	bool isOnToolBar(const QWidget *widget)
	{
		return widget && qobject_cast<const QToolBar *>(widget->parentWidget());
	}

	bool isOnHorizontalToolBar(const QWidget *widget)
	{
		const QToolBar *toolbar = widget ? qobject_cast<const QToolBar *>(widget->parentWidget()) : nullptr;
		return !toolbar || toolbar->orientation() == Qt::Horizontal;
	}
}

KeramikStyle::KeramikStyle()
	: KlassikStyle(KlassikStyle::HighColor)
{
}

qreal
KeramikStyle::sizeScale() const
{
	const QFont font = QApplication::font();
	if (m_sizeScale == 0 || font != m_scaleFont) {
		m_scaleFont = font;
		m_fontHeight = QFontMetrics(font).height();
		// The lines of the fonts KDE 3 used were about 14 pixels high.
		// Quarter steps keep the tiles from getting too blurry.
		const qreal ratio = m_fontHeight / 14.0;
		m_sizeScale = qMax(qreal(1), qRound(ratio * 4) / qreal(4));
	}
	return m_sizeScale;
}

int
KeramikStyle::extraHeight(qreal fraction) const
{
	sizeScale(); // Updates m_fontHeight
	return qRound(m_fontHeight * fraction);
}

bool
KeramikStyle::enlarging() const
{
	return m_reducedDepth == 0 && sizeScale() != 1;
}

template<typename Option, typename Draw>
void
KeramikStyle::drawEnlarged(const Option *opt, QPainter *p, Draw draw) const
{
	const qreal k = sizeScale();
	Option reduced(*opt);
	reduced.rect = reducedRect(opt->rect, k);

	p->save();
	p->translate(opt->rect.topLeft());
	p->scale(k, k);
	++m_reducedDepth;
	draw(&reduced);
	--m_reducedDepth;
	p->restore();
}

template<typename Fn>
bool
KeramikStyle::withReducedOption(ComplexControl control, const QStyleOptionComplex *opt, Fn fn) const
{
	auto call = [&](const auto *typed) {
		auto reduced = *typed;
		reduced.rect = reducedRect(opt->rect, sizeScale());
		++m_reducedDepth;
		fn(&reduced);
		--m_reducedDepth;
		return true;
	};

	switch (control) {
	case CC_ScrollBar:
	case CC_Slider:
		if (const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt))
			return call(slider);
		break;
	case CC_SpinBox:
		if (const QStyleOptionSpinBox *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(opt))
			return call(spin);
		break;
	case CC_ComboBox:
		if (const QStyleOptionComboBox *combo = qstyleoption_cast<const QStyleOptionComboBox *>(opt))
			return call(combo);
		break;
	default:
		break;
	}
	return false;
}

void
KeramikStyle::polish(QWidget *widget)
{
	if (qobject_cast<QComboBox *>(widget))
		widget->setAttribute(Qt::WA_Hover, true);

	if (isComboBoxPopup(widget)) {
		// The bottom corners of the list frame are rounded
		widget->setAttribute(Qt::WA_TranslucentBackground, true);
		widget->installEventFilter(this);
	}

	if (QProgressBar *bar = qobject_cast<QProgressBar *>(widget)) {
		if (!m_busySteps.contains(bar)) {
			m_busySteps.insert(bar, 0);
			connect(bar, &QObject::destroyed, this, [this](QObject *object) {
				m_busySteps.remove(object);
			});
		}
	}

	KlassikStyle::polish(widget);
}

void
KeramikStyle::unpolish(QWidget *widget)
{
	if (isComboBoxPopup(widget)) {
		widget->removeEventFilter(this);
		widget->setAttribute(Qt::WA_TranslucentBackground, false);
	}

	if (QProgressBar *bar = qobject_cast<QProgressBar *>(widget)) {
		disconnect(bar, &QObject::destroyed, this, nullptr);
		m_busySteps.remove(bar);
	}

	KlassikStyle::unpolish(widget);
}

bool
KeramikStyle::eventFilter(QObject *object, QEvent *event)
{
	QWidget *popup = qobject_cast<QWidget *>(object);
	if (event->type() == QEvent::Show && isComboBoxPopup(popup)) {
		// Like in KDE 3, the list slightly overlaps the bottom of the combobox
		// and is narrower than it. The geometry is still changeable here, as
		// the native window gets shown after the show event.
		const QWidget *combo = popup->parentWidget();
		if (combo && popup->y() >= combo->mapToGlobal(QPoint(0, combo->height())).y() - 1)
			popup->setGeometry(popup->geometry().adjusted(4, -4, -6, 4));
	}

	return KlassikStyle::eventFilter(object, event);
}

void
KeramikStyle::timerEvent(QTimerEvent *event)
{
	if (event->timerId() != m_busyTimer.timerId()) {
		KlassikStyle::timerEvent(event);
		return;
	}

	bool visible = false;
	for (auto it = m_busySteps.begin(); it != m_busySteps.end(); ++it) {
		QProgressBar *bar = static_cast<QProgressBar *>(const_cast<QObject *>(it.key()));
		if (!bar->isVisible() || !bar->isEnabled() || !isBusy(bar))
			continue;
		visible = true;
		it.value() += busyStep;
		bar->update();
	}

	// Restarted when a busy progress bar gets painted again
	if (!visible)
		m_busyTimer.stop();
}

void
KeramikStyle::drawLineEditFrame(QPainter *p, const QRect &r, const QPalette &pal) const
{
	int x, y, w, h;
	r.getRect(&x, &y, &w, &h);

	drawLine(p, x, y, x + w - 1, y, pal.dark().color());
	drawLine(p, x, y, x, y + h - 1, pal.dark().color());
	drawLine(p, x + 1, y + 1, x + w - 1, y + 1, pal.mid().color());
	drawLine(p, x + 1, y + 1, x + 1, y + h - 1, pal.mid().color());
	drawLine(p, x + w - 1, y, x + w - 1, y + h - 1, pal.light().color());
	drawLine(p, x, y + h - 1, x + w - 1, y + h - 1, pal.light().color());
	drawLine(p, x + w - 2, y + 1, x + w - 2, y + h - 2, pal.light().color().darker(110));
	drawLine(p, x + 1, y + h - 2, x + w - 2, y + h - 2, pal.light().color().darker(110));
}

void
KeramikStyle::drawFrameShadow(QPainter *p, const QRect &r, const QPalette &pal) const
{
	Keramik::RectTilePainter(keramik_frame_shadow, false, false, 2, 2)
		.draw(p, r, pal.color(QPalette::Normal, QPalette::Button));
}

void
KeramikStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption *opt,
							QPainter *p, const QWidget *widget) const
{
	if (enlarging() && (pe == PE_IndicatorCheckBox || pe == PE_IndicatorItemViewItemCheck ||
						pe == PE_IndicatorRadioButton)) {
		drawEnlarged(opt, p, [&](const QStyleOption *reduced) {
			drawPrimitive(pe, reduced, p, widget);
		});
		return;
	}

	const QPalette &pal = opt->palette;
	const QRect &r = opt->rect;
	int x, y, w, h;
	r.getRect(&x, &y, &w, &h);
	const int x2 = x + w - 1;
	const int y2 = y + h - 1;

	const bool down = opt->state & State_Sunken;
	const bool on = opt->state & State_On;
	const bool disabled = !(opt->state & State_Enabled);

	// Depending on the Qt version, the frame of a combobox popup is drawn
	// as PE_Frame or as PE_PanelMenu
	if ((pe == PE_Frame || pe == PE_PanelMenu) && isComboBoxPopup(widget)) {
		// The top of the frame fades into the window background; the shadow
		// at the right and the rounded bottom stay translucent
		p->fillRect(QRect(x, y, w - 2, h - 6), pal.window());
		Keramik::RectTilePainter(keramik_combobox_list, false, false)
			.draw(p, r, pal.button().color());
		return;
	}

	switch (pe) {
	// BUTTONS
	// -------------------------------------------------------------------
	case PE_FrameDefaultButton: {
		bool sunken = on || down;

		int id;
		if (sunken)
			id = keramik_pushbutton_default_pressed;
		else
			id = keramik_pushbutton_default;

		if ((opt->state & State_MouseOver) && id == keramik_pushbutton_default)
			id = keramik_pushbutton_default_hov;

		Keramik::RectTilePainter(id, false).draw(p, r, pal.button().color(), disabled);
		break;
	}

	case PE_IndicatorButtonDropDown:
	case PE_PanelButtonTool: {
		// Tool buttons outside of toolbars behave like push buttons. Like in
		// KDE 3, that includes flat ones when raised, e.g. in Qt Quick.
		if (!(opt->state & State_AutoRaise) || !isOnToolBar(widget)) {
			proxy()->drawPrimitive(PE_PanelButtonCommand, opt, p, widget);
			break;
		}

		if (on) {
			Keramik::RectTilePainter(keramik_toolbar_clk).draw(p, r, pal.button().color());
			drawLine(p, x, y, x2, y, pal.dark().color());
			drawLine(p, x, y, x, y2, pal.dark().color());
		} else if (down) {
			Keramik::RectTilePainter(keramik_toolbar_clk).draw(p, r, pal.button().color());
		} else {
			const QColor light = Keramik::ColorUtil::lighten(pal.button().color(), 115);
			Keramik::GradientPainter::renderGradient(p, r, light, isOnHorizontalToolBar(widget), false);

			const QColor dark = pal.button().color().lighter(70);
			drawLine(p, x, y, x2 - 1, y, dark);
			drawLine(p, x, y, x, y2 - 1, dark);
			drawLine(p, x + 2, y2 - 1, x2 - 1, y2 - 1, dark);
			drawLine(p, x2 - 1, y + 2, x2 - 1, y2 - 2, dark);

			drawLine(p, x + 1, y + 1, x2 - 1, y + 1, light);
			drawLine(p, x + 1, y + 1, x + 1, y2, light);
			drawLine(p, x, y2, x2, y2, light);
			drawLine(p, x2, y, x2, y2, light);
		}
		break;
	}

	// PUSH BUTTON
	// -------------------------------------------------------------------
	case PE_PanelButtonCommand: {
		bool sunken = on || down;

		int name;
		if (w <= smallButMaxW || h <= smallButMaxH)
			name = sunken ? keramik_pushbutton_small_pressed : keramik_pushbutton_small;
		else
			name = sunken ? keramik_pushbutton_pressed : keramik_pushbutton;

		if ((opt->state & State_MouseOver) && name == keramik_pushbutton)
			name = keramik_pushbutton_hov;

		Keramik::RectTilePainter(name, false).draw(p, r, pal.button().color(), disabled);
		break;
	}

	// BEVELS
	// -------------------------------------------------------------------
	case PE_PanelButtonBevel: {
		bool sunken = on || down;

		// Outer frame
		drawLine(p, x, y, x2, y, pal.shadow().color());
		drawLine(p, x, y2, x2, y2, pal.shadow().color());
		drawLine(p, x, y, x, y2, pal.shadow().color());
		drawLine(p, x2, y, x2, y2, pal.shadow().color());

		// Bevel
		const QColor topLeft = sunken ? pal.mid().color() : pal.light().color();
		const QColor bottomRight = sunken ? pal.light().color() : pal.mid().color();
		drawLine(p, x + 1, y + 1, x2 - 1, y + 1, topLeft);
		drawLine(p, x + 1, y + 1, x + 1, y2 - 1, topLeft);
		drawLine(p, x + 2, y2 - 1, x2 - 1, y2 - 1, bottomRight);
		drawLine(p, x2 - 1, y + 2, x2 - 1, y2 - 1, bottomRight);

		if (w > 4 && h > 4) {
			if (sunken)
				p->fillRect(x + 2, y + 2, w - 4, h - 4, pal.button());
			else
				Keramik::GradientPainter::renderGradient(p, QRect(x + 2, y + 2, w - 4, h - 4),
														 pal.button().color(), opt->state & State_Horizontal);
		}
		break;
	}

	// HEADER ARROW
	// -------------------------------------------------------------------
	case PE_IndicatorHeaderArrow: {
		const QStyleOptionHeader *header = qstyleoption_cast<const QStyleOptionHeader *>(opt);
		QStyleOption arrow(*opt);
		arrow.state = State_Enabled;
		if (header && header->sortIndicator == QStyleOptionHeader::SortUp)
			proxy()->drawPrimitive(PE_IndicatorArrowUp, &arrow, p, widget);
		else
			proxy()->drawPrimitive(PE_IndicatorArrowDown, &arrow, p, widget);
		break;
	}

	// CHECKBOX (indicator)
	// -------------------------------------------------------------------
	case PE_IndicatorCheckBox:
	case PE_IndicatorItemViewItemCheck: {
		int name;
		if (opt->state & State_On)
			name = keramik_checkbox_on;
		else if (opt->state & State_NoChange)
			name = keramik_checkbox_tri;
		else
			name = keramik_checkbox_off;

		Keramik::ScaledPainter(name).draw(p, r, pal.button().color(), disabled);
		break;
	}

	// RADIOBUTTON (exclusive indicator)
	// -------------------------------------------------------------------
	case PE_IndicatorRadioButton: {
		Keramik::ScaledPainter(on ? keramik_radiobutton_on : keramik_radiobutton_off)
			.draw(p, r, pal.button().color(), disabled);
		break;
	}

	// LINE EDIT FRAME
	// -------------------------------------------------------------------
	case PE_PanelLineEdit: {
		const QStyleOptionFrame *panel = qstyleoption_cast<const QStyleOptionFrame *>(opt);
		if (!panel)
			break;

		const int lw = panel->lineWidth;
		p->fillRect(r.adjusted(lw, lw, -lw, -lw), pal.base());
		if (lw > 0)
			drawLineEditFrame(p, r, pal);

		// Keramik draws a soft shadow over every line edit
		if (lw > 0 || qobject_cast<const QLineEdit *>(widget))
			drawFrameShadow(p, r, pal);
		break;
	}

	case PE_FrameLineEdit: {
		drawLineEditFrame(p, r, pal);
		break;
	}

	// SPLITTER/DOCKWINDOW HANDLES
	// -------------------------------------------------------------------
	case PE_IndicatorDockWidgetResizeHandle: {
		drawLine(p, x, y, x2, y, pal.dark().color());
		drawLine(p, x, y2, x2, y2, pal.dark().color());
		drawLine(p, x, y, x, y2, pal.dark().color());
		drawLine(p, x2, y, x2, y2, pal.dark().color());
		drawPoint(p, x, y, pal.window().color());
		drawPoint(p, x2, y, pal.window().color());
		drawPoint(p, x, y2, pal.window().color());
		drawPoint(p, x2, y2, pal.window().color());
		drawLine(p, x + 1, y + 1, x + 1, y2 - 1, pal.light().color());
		drawLine(p, x + 1, y + 1, x2 - 1, y + 1, pal.light().color());
		drawLine(p, x + 2, y + 2, x + 2, y2 - 2, pal.midlight().color());
		drawLine(p, x + 2, y + 2, x2 - 2, y + 2, pal.midlight().color());
		drawLine(p, x2 - 1, y + 1, x2 - 1, y2 - 1, pal.mid().color());
		drawLine(p, x + 1, y2 - 1, x2 - 1, y2 - 1, pal.mid().color());
		p->fillRect(x + 3, y + 3, w - 5, h - 5, pal.window());
		break;
	}

	// FOCUS RECT
	// -------------------------------------------------------------------
	case PE_FrameFocusRect: {
		const QStyleOptionFocusRect *fropt = qstyleoption_cast<const QStyleOptionFocusRect *>(opt);
		if (!fropt || r.isEmpty())
			break;

		// A dotted line in the inverse of the background color, made of
		// whole logical pixels so that it stays visible when scaled
		QColor bg = fropt->backgroundColor;
		if (!bg.isValid())
			bg = p->background().color();
		const QColor color(bg.red() ^ 0xff, bg.green() ^ 0xff, bg.blue() ^ 0xff);

		// Every other pixel of a checkerboard anchored at the top left
		for (int i = x; i <= x2; i++) {
			if ((i - x) % 2 == 0)
				drawPoint(p, i, y, color);
			if ((i - x + h - 1) % 2 == 0)
				drawPoint(p, i, y2, color);
		}
		for (int j = y + 1; j < y2; j++) {
			if ((j - y) % 2 == 0)
				drawPoint(p, x, j, color);
			if ((j - y + w - 1) % 2 == 0)
				drawPoint(p, x2, j, color);
		}
		break;
	}

	// MENU / TOOLBAR PANEL
	// -------------------------------------------------------------------
	case PE_PanelMenu: {
		p->fillRect(r, pal.window().color().lighter(105));
		break;
	}

	case PE_PanelMenuBar: {
		Keramik::GradientPainter::renderGradient(p, r, pal.button().color(), true, true);

		const QStyleOptionFrame *frame = qstyleoption_cast<const QStyleOptionFrame *>(opt);
		if (frame && frame->lineWidth)
			drawLine(p, x2, y, x2, y2, pal.dark().color());
		break;
	}

	case PE_PanelToolBar: {
		Keramik::GradientPainter::renderGradient(p, r, pal.button().color(), w > h);
		break;
	}

	// TOOLBAR HANDLE
	// -------------------------------------------------------------------
	case PE_IndicatorToolBarHandle: {
		const QColor light = pal.light().color();
		const QColor mid = pal.mid().color();

		if (opt->state & State_Horizontal) {
			Keramik::GradientPainter::renderGradient(p, r, pal.button().color(), true);
			drawLine(p, x + 1, y + 4, x + 1, y2 - 4, light);
			drawLine(p, x + 3, y + 4, x + 3, y2 - 4, light);
			drawLine(p, x + 5, y + 4, x + 5, y2 - 4, light);

			drawLine(p, x + 2, y + 4, x + 2, y2 - 4, mid);
			drawLine(p, x + 4, y + 4, x + 4, y2 - 4, mid);
			drawLine(p, x + 6, y + 4, x + 6, y2 - 4, mid);
		} else {
			Keramik::GradientPainter::renderGradient(p, r, pal.button().color(), false);
			drawLine(p, x + 4, y + 1, x2 - 4, y + 1, light);
			drawLine(p, x + 4, y + 3, x2 - 4, y + 3, light);
			drawLine(p, x + 4, y + 5, x2 - 4, y + 5, light);

			drawLine(p, x + 4, y + 2, x2 - 4, y + 2, mid);
			drawLine(p, x + 4, y + 4, x2 - 4, y + 4, mid);
			drawLine(p, x + 4, y + 6, x2 - 4, y + 6, mid);
		}
		break;
	}

	// TOOLBAR SEPARATOR
	// -------------------------------------------------------------------
	case PE_IndicatorToolBarSeparator: {
		Keramik::GradientPainter::renderGradient(p, r, pal.button().color(),
												 opt->state & State_Horizontal);
		if (!(opt->state & State_Horizontal)) {
			drawLine(p, x + 4, y + h / 2, x + w - 5, y + h / 2, pal.mid().color());
			drawLine(p, x + 4, y + h / 2 + 1, x + w - 5, y + h / 2 + 1, pal.light().color());
		} else {
			drawLine(p, x + w / 2, y + 4, x + w / 2, y + h - 5, pal.mid().color());
			drawLine(p, x + w / 2 + 1, y + 4, x + w / 2 + 1, y + h - 5, pal.light().color());
		}
		break;
	}

	default:
		KlassikStyle::drawPrimitive(pe, opt, p, widget);
		break;
	}
}

void
KeramikStyle::drawControl(ControlElement control, const QStyleOption *opt,
						  QPainter *p, const QWidget *widget) const
{
	// The labels are drawn separately, at their normal size
	if (enlarging()) {
		const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
		if (control == CE_PushButtonBevel && btn) {
			drawEnlarged(btn, p, [&](const QStyleOptionButton *reduced) {
				drawControl(control, reduced, p, widget);
			});
			return;
		}
		const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
		if (control == CE_TabBarTabShape && tab) {
			drawEnlarged(tab, p, [&](const QStyleOptionTab *reduced) {
				drawControl(control, reduced, p, widget);
			});
			return;
		}
	}

	const QPalette &pal = opt->palette;
	const QRect &r = opt->rect;
	int x, y, w, h;
	r.getRect(&x, &y, &w, &h);

	const bool disabled = !(opt->state & State_Enabled);

	switch (control) {
	// PUSHBUTTON
	// -------------------------------------------------------------------
	case CE_PushButtonBevel: {
		const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
		if (!btn)
			break;

		if (btn->features & QStyleOptionButton::Flat) {
			bool sunken = btn->state & (State_On | State_Sunken);
			int name;
			if (w <= smallButMaxW || h <= smallButMaxH)
				name = sunken ? keramik_pushbutton_small_pressed : keramik_pushbutton_small;
			else
				name = sunken ? keramik_pushbutton_pressed : keramik_pushbutton;
			if ((btn->state & State_MouseOver) && name == keramik_pushbutton)
				name = keramik_pushbutton_hov;

			Keramik::ScaledPainter(name + KeramikTileCC, Keramik::ScaledPainter::Vertical)
				.draw(p, r, pal.button().color(), disabled);

			const QColor line = pal.button().color().lighter(75);
			drawLine(p, x, y, x + w - 1, y, line);
			drawLine(p, x, y, x, y + h - 1, line);
			drawLine(p, x, y + h - 1, x + w - 1, y + h - 1, line);
			drawLine(p, x + w - 1, y, x + w - 1, y + h - 1, line);
		} else if (btn->features & QStyleOptionButton::DefaultButton)
			proxy()->drawPrimitive(PE_FrameDefaultButton, opt, p, widget);
		else
			proxy()->drawPrimitive(PE_PanelButtonCommand, opt, p, widget);
		break;
	}

	// PUSHBUTTON LABEL
	// -------------------------------------------------------------------
	case CE_PushButtonLabel: {
		const QStyleOptionButton *button = qstyleoption_cast<const QStyleOptionButton *>(opt);
		if (!button)
			break;

		QStyle::State flags = button->state;
		const bool active = flags & (State_On | State_Sunken);

		// Shift button contents if pushed.
		if (active) {
			x += proxy()->pixelMetric(PM_ButtonShiftHorizontal, opt, widget);
			y += proxy()->pixelMetric(PM_ButtonShiftVertical, opt, widget);
			flags |= State_Sunken;
		}

		// Does the button have a popup menu?
		if (button->features & QStyleOptionButton::HasMenu) {
			int dx = proxy()->pixelMetric(PM_MenuButtonIndicator, opt, widget);
			QStyleOption arrow(*opt);
			arrow.state = flags;
			arrow.rect = visualRect(button->direction, r, QRect(x + w - dx - 8, y + 2, dx, h - 4));
			proxy()->drawPrimitive(PE_IndicatorArrowDown, &arrow, p, widget);
			w -= dx;
		}

		// Draw the icon if there is one
		if (!button->icon.isNull()) {
			QIcon::Mode mode = QIcon::Disabled;
			QIcon::State state = QIcon::Off;

			if (button->state & State_Enabled)
				mode = (button->state & State_HasFocus) ? QIcon::Active : QIcon::Normal;
			if (button->state & State_On)
				state = QIcon::On;

			const qreal dpr = p->device() ? p->device()->devicePixelRatio() : 1.0;
			QPixmap pixmap = button->icon.pixmap(button->iconSize, dpr, mode, state);
			const int pw = pixmap.width() / pixmap.devicePixelRatio();
			const int ph = pixmap.height() / pixmap.devicePixelRatio();

			QRect iconRect;
			if (button->text.isEmpty())
				iconRect = QRect(x + w / 2 - pw / 2, y + h / 2 - ph / 2, pw, ph);
			else
				iconRect = QRect(x + 4, y + h / 2 - ph / 2, pw, ph);
			p->drawPixmap(visualRect(button->direction, r, iconRect), pixmap);

			x += pw + 4;
			w -= pw + 4;
		}

		int tf = Qt::AlignCenter | Qt::TextShowMnemonic;
		if (!proxy()->styleHint(SH_UnderlineShortcut, button, widget))
			tf |= Qt::TextHideMnemonic;

		proxy()->drawItemText(p, visualRect(button->direction, r, QRect(x, y, w, h)), tf, pal,
							  button->state & State_Enabled, button->text, QPalette::ButtonText);
		break;
	}

	// TOOLBUTTON LABEL
	// -------------------------------------------------------------------
	case CE_ToolButtonLabel: {
		if (!(opt->state & State_AutoRaise) && w > smallButMaxW && h > smallButMaxH) {
			// Account for the shadow of the push button tiles
			QStyleOptionToolButton tb;
			if (const QStyleOptionToolButton *tool = qstyleoption_cast<const QStyleOptionToolButton *>(opt))
				tb = *tool;
			else
				tb.QStyleOption::operator=(*opt);
			tb.rect.setWidth(r.width() - 2);
			KlassikStyle::drawControl(control, &tb, p, widget);
		} else
			KlassikStyle::drawControl(control, opt, p, widget);
		break;
	}

	// HEADER SECTION
	// -------------------------------------------------------------------
	case CE_HeaderSection: {
		if (opt->state & State_Sunken)
			Keramik::RectTilePainter(keramik_listview_pressed, false).draw(p, r, pal.button().color());
		else
			Keramik::RectTilePainter(keramik_listview, false).draw(p, r, pal.button().color());
		break;
	}

	// TABBAR TAB
	// -------------------------------------------------------------------
	case CE_TabBarTabShape: {
		const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(opt);
		if (!tab)
			break;

		bool bottom;
		if (tab->shape == QTabBar::RoundedNorth || tab->shape == QTabBar::TriangularNorth)
			bottom = false;
		else if (tab->shape == QTabBar::RoundedSouth || tab->shape == QTabBar::TriangularSouth)
			bottom = true;
		else {
			KlassikStyle::drawControl(control, opt, p, widget);
			break;
		}

		if (tab->state & State_Selected)
			Keramik::ActiveTabPainter(bottom).draw(p, r, pal.button().color(), disabled);
		else {
			Keramik::InactiveTabPainter::Mode mode;
			if (tab->position == QStyleOptionTab::Beginning || tab->position == QStyleOptionTab::OnlyOneTab)
				mode = Keramik::InactiveTabPainter::First;
			else if (tab->position == QStyleOptionTab::End)
				mode = Keramik::InactiveTabPainter::Last;
			else
				mode = Keramik::InactiveTabPainter::Middle;

			if (bottom) {
				Keramik::InactiveTabPainter(mode, bottom).draw(p, x, y, w, h - 4, pal.button().color(), disabled);
				drawLine(p, x, y, x + w, y, pal.dark().color());
			} else {
				Keramik::InactiveTabPainter(mode, bottom).draw(p, x, y + 4, w, h - 4, pal.button().color(), disabled);
				drawLine(p, x, y + h - 1, x + w, y + h - 1, pal.light().color());
			}
		}
		break;
	}

	// MENUBAR
	// -------------------------------------------------------------------
	case CE_MenuBarEmptyArea: {
		Keramik::GradientPainter::renderGradient(p, r, pal.button().color(), true, true);
		break;
	}

	case CE_MenuBarItem: {
		const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
		if (!mi)
			break;

		// Only the item of the open menu gets the sunken panel
		if ((mi->state & State_Selected) && (mi->state & State_Sunken))
			qDrawShadePanel(p, r, pal, true, 1, &pal.brush(QPalette::Midlight));
		else
			Keramik::GradientPainter::renderGradient(p, r, pal.button().color(), true, true,
													 0, r.y() - mi->menuRect.y(), -1,
													 mi->menuRect.height());

		int tf = Qt::AlignCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
		if (!proxy()->styleHint(SH_UnderlineShortcut, mi, widget))
			tf |= Qt::TextHideMnemonic;

		proxy()->drawItemText(p, r, tf, pal, mi->state & State_Enabled, mi->text, QPalette::ButtonText);
		break;
	}

	// POPUPMENU ITEM
	// -------------------------------------------------------------------
	case CE_MenuEmptyArea: {
		p->fillRect(r, pal.window().color().lighter(105));
		break;
	}

	case CE_MenuItem: {
		const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
		if (!mi)
			break;

		const QRect main = r;
		const int tab = mi->reservedShortcutWidth;
		const bool checkable = mi->checkType != QStyleOptionMenuItem::NotCheckable;
		int checkcol = mi->maxIconWidth;
		const bool enabled = mi->state & State_Enabled;
		const bool active = mi->state & State_Selected;
		const bool etchtext = proxy()->styleHint(SH_EtchDisabledText, opt, widget);
		const bool reverse = mi->direction == Qt::RightToLeft;
		if (checkable)
			checkcol = qMax(checkcol, 20);

		// Draw the menu item background
		if (active && enabled)
			Keramik::RowPainter(keramik_menuitem).draw(p, main, pal.highlight().color());
		else
			p->fillRect(main, pal.window().color().lighter(105));

		// Are we a menu item separator?
		if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
			drawLine(p, main.x() + 5, main.y() + 1, main.width() - 10, main.y() + 1, pal.mid().color());
			drawLine(p, main.x() + 5, main.y() + 2, main.width() - 10, main.y() + 2, pal.light().color());
			break;
		}

		if (active && !enabled) {
			QStyleOptionFocusRect focus;
			focus.QStyleOption::operator=(*opt);
			proxy()->drawPrimitive(PE_FrameFocusRect, &focus, p, widget);
		}

		QRect cr = visualRect(mi->direction, r, QRect(x + 2, y + 2, checkcol - 1, h - 4));
		const bool checked = checkable && mi->checked;

		// Do we have an icon?
		if (!mi->icon.isNull()) {
			QIcon::Mode mode = enabled ? (active ? QIcon::Active : QIcon::Normal) : QIcon::Disabled;

			// Do we have an icon and are checked at the same time?
			// Then draw a "pressed" background behind the icon
			if (checked)
				qDrawShadePanel(p, cr, pal, true, 1, &pal.brush(QPalette::Midlight));

			const int size = proxy()->pixelMetric(PM_SmallIconSize, opt, widget);
			const qreal dpr = p->device() ? p->device()->devicePixelRatio() : 1.0;
			QPixmap pixmap = mi->icon.pixmap(QSize(size, size), dpr, mode);
			QRect pmr(QPoint(0, 0), pixmap.size() / pixmap.devicePixelRatio());
			pmr.moveCenter(cr.center());
			p->drawPixmap(pmr.topLeft(), pixmap);
		}
		// Are we checked? (This time without an icon)
		else if (checked) {
			qDrawShadePanel(p, cr, pal, true, 1, &pal.brush(QPalette::Midlight));

			// Draw the checkmark
			QStyleOption check(*opt);
			check.rect = cr;
			check.state = active ? State_Enabled : State_On;
			proxy()->drawPrimitive(PE_IndicatorMenuCheckMark, &check, p, widget);
		}

		// Time to draw the menu item label...
		int xm = itemFrame + checkcol + itemHMargin; // X position margin

		int xp = reverse ? // X position
			x + tab + rightBorder + itemHMargin + itemFrame - 1 :
			x + xm;

		int offset = reverse ? -1 : 1; // Shadow offset for etched text

		// Label width (minus the width of the accelerator portion)
		int tw = w - xm - tab - arrowHMargin - itemHMargin * 3 - itemFrame + 1;

		const QColor discol = pal.mid().color();
		const QColor draw = (active && enabled) ? pal.highlightedText().color() : pal.text().color();

		QString s = mi->text;
		if (!s.isEmpty()) {
			qsizetype t = s.indexOf(u'\t');
			const int m = itemVMargin;
			int text_flags = Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine;
			if (!proxy()->styleHint(SH_UnderlineShortcut, mi, widget))
				text_flags |= Qt::TextHideMnemonic;
			text_flags |= reverse ? Qt::AlignRight : Qt::AlignLeft;

			p->save();

			// Does the menu item have a tabstop? (for the accelerator text)
			if (t >= 0) {
				int tabx = reverse ? x + rightBorder + itemHMargin + itemFrame :
					x + w - tab - rightBorder - itemHMargin - itemFrame;

				// Draw the right part of the label (accelerator text)
				if (etchtext && !enabled) {
					// Draw etched text if we're inactive and the menu item is disabled
					p->setPen(pal.light().color());
					p->drawText(tabx + offset, y + m + 1, tab, h - 2 * m, text_flags, s.mid(t + 1));
					p->setPen(discol);
				} else
					p->setPen(draw);
				p->drawText(tabx, y + m, tab, h - 2 * m, text_flags, s.mid(t + 1));
				s = s.left(t);
			}

			// Draw the left part of the label (or the whole label
			// if there's no accelerator)
			if (etchtext && !enabled) {
				// Etched text again for inactive disabled menu items...
				p->setPen(pal.light().color());
				p->drawText(xp + offset, y + m + 1, tw, h - 2 * m, text_flags, s);
				p->setPen(discol);
			} else
				p->setPen(draw);

			p->drawText(xp, y + m, tw, h - 2 * m, text_flags, s);
			p->restore();
		}

		// Does the menu item have a submenu?
		if (mi->menuItemType == QStyleOptionMenuItem::SubMenu) {
			PrimitiveElement arrow = reverse ? PE_IndicatorArrowLeft : PE_IndicatorArrowRight;
			int dim = proxy()->pixelMetric(PM_MenuButtonIndicator, opt, widget) - itemFrame;
			QStyleOption arrowOpt(*opt);
			arrowOpt.rect = visualRect(mi->direction, r,
									   QRect(x + w - arrowHMargin - itemFrame - dim,
											 y + h / 2 - dim / 2, dim, dim));
			arrowOpt.state = enabled ? State_Enabled : State_None;
			proxy()->drawPrimitive(arrow, &arrowOpt, p, widget);
		}
		break;
	}

	// PROGRESSBAR
	// -------------------------------------------------------------------
	case CE_ProgressBarGroove: {
		qDrawShadePanel(p, r, pal, true, 1);
		break;
	}

	case CE_ProgressBarContents: {
		const QStyleOptionProgressBar *pb = qstyleoption_cast<const QStyleOptionProgressBar *>(opt);
		if (!pb)
			break;

		if (!(pb->state & State_Horizontal)) {
			KlassikStyle::drawControl(control, opt, p, widget);
			break;
		}

		const QRect &cr = r;
		const bool reverse = (pb->direction == Qt::RightToLeft) != pb->invertedAppearance;
		const bool busy = pb->minimum == 0 && pb->maximum == 0;
		const qint64 steps = qint64(pb->maximum) - pb->minimum;
		const qint64 progress = qint64(pb->progress) - pb->minimum;

		if (!cr.isValid())
			break;

		if (progress > 0 || busy) {
			double pg = busy ? 0.1 : double(progress) / steps;
			int width = qMin(cr.width(), int(pg * cr.width()));
			if (busy) { // Busy indicator
				width = qMin(width, 20); // Don't cross squares
				if (width < 1)
					width = 1; // A busy indicator with width 0 is kind of useless

				int remWidth = cr.width() - width; // Never disappear completely
				if (remWidth <= 0)
					remWidth = 1; // Do something non-crashy when too small...

				if (widget && m_busySteps.contains(widget) && !m_busyTimer.isActive())
					m_busyTimer.start(busyInterval, const_cast<KeramikStyle *>(this));

				int pstep = m_busySteps.value(widget) % (2 * remWidth);
				if (pstep > remWidth) {
					// Bounce about..
					pstep = -(pstep - 2 * remWidth);
				}

				if (reverse)
					Keramik::RowPainter(keramik_progressbar).draw(p, cr.x() + cr.width() - width - pstep, cr.y(),
																  width, cr.height(), pal.highlight().color());
				else
					Keramik::RowPainter(keramik_progressbar).draw(p, cr.x() + pstep, cr.y(),
																  width, cr.height(), pal.highlight().color());
				break;
			}

			if (reverse)
				Keramik::ProgressBarPainter(keramik_progressbar, reverse)
					.draw(p, cr.x() + (cr.width() - width), cr.y(), width, cr.height(), pal.highlight().color());
			else
				Keramik::ProgressBarPainter(keramik_progressbar, reverse)
					.draw(p, cr.x(), cr.y(), width, cr.height(), pal.highlight().color());
		}
		break;
	}

	case CE_ProgressBarLabel: {
		p->save();
		QFont font = p->font();
		font.setBold(true);
		p->setFont(font);
		KlassikStyle::drawControl(control, opt, p, widget);
		p->restore();
		break;
	}

	// COMBOBOX LABEL
	// -------------------------------------------------------------------
	case CE_ComboBoxLabel: {
		const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
		if (!cb)
			break;

		// QComboBox draws the text with the painter's current pen
		p->save();
		if (!cb->editable && (cb->state & State_HasFocus))
			p->setPen(pal.highlightedText().color());
		else
			p->setPen(pal.text().color());
		QCommonStyle::drawControl(control, opt, p, widget);
		p->restore();
		break;
	}

	default:
		KlassikStyle::drawControl(control, opt, p, widget);
		break;
	}
}

QRect
KeramikStyle::subElementRect(SubElement element, const QStyleOption *opt,
							 const QWidget *widget) const
{
	switch (element) {
	case SE_PushButtonContents:
		return opt->rect;

	case SE_PushButtonFocusRect: {
		const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);
		// Keep the focus rect inside the enlarged bevel
		if (btn && enlarging()) {
			const qreal k = sizeScale();
			QStyleOptionButton reduced(*btn);
			reduced.rect = reducedRect(btn->rect, k);
			++m_reducedDepth;
			const QRect r = subElementRect(element, &reduced, widget);
			--m_reducedDepth;
			return enlargedRect(r, btn->rect, k);
		}
		const QRect &wrect = opt->rect;
		QRect r;
		if (btn && (btn->features & (QStyleOptionButton::DefaultButton | QStyleOptionButton::AutoDefaultButton)))
			r = QRect(wrect.x() + 6, wrect.y() + 5, wrect.width() - 12, wrect.height() - 10);
		else
			r = QRect(wrect.x() + 3, wrect.y() + 5, wrect.width() - 8, wrect.height() - 10);
		return visualRect(opt->direction, wrect, r);
	}

	case SE_ComboBoxFocusRect: {
		if (const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt))
			return subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
		break;
	}

	case SE_ProgressBarGroove:
		return opt->rect;

	case SE_ProgressBarContents:
	case SE_ProgressBarLabel:
		return opt->rect.adjusted(2, 2, -2, -2);

	default:
		break;
	}

	return KlassikStyle::subElementRect(element, opt, widget);
}

void
KeramikStyle::drawScrollBarSlider(QPainter *p, const QRect &r, const QPalette &pal,
								  bool horizontal, bool active) const
{
	unsigned int count = 3;

	if (horizontal) {
		if (r.width() > (loader.size(keramik_scrollbar_hbar + KeramikSlider1).width() +
						 loader.size(keramik_scrollbar_hbar + KeramikSlider4).width() +
						 loader.size(keramik_scrollbar_hbar + KeramikSlider3).width() + 2))
			count = 5;
	} else if (r.height() > (loader.size(keramik_scrollbar_vbar + KeramikSlider1).height() +
							 loader.size(keramik_scrollbar_vbar + KeramikSlider4).height() +
							 loader.size(keramik_scrollbar_vbar + KeramikSlider3).height() + 2))
		count = 5;

	QColor col = pal.highlight().color();
	if (active)
		col = Keramik::ColorUtil::lighten(col, 110);

	Keramik::ScrollBarPainter(KeramikSlider1, count, horizontal).draw(p, r, col);
}

void
KeramikStyle::drawScrollBarSubLine(QPainter *p, const QRect &r, const QPalette &pal,
								   bool horizontal, bool down, bool disabled) const
{
	const QColor color = down ? pal.buttonText().color() : pal.button().color();

	if (horizontal) {
		Keramik::CenteredPainter(keramik_scrollbar_hbar_arrow1).draw(p, r, color, disabled);
		drawKeramikArrow(p, pal, r, PE_IndicatorArrowLeft, down, !disabled);
	} else {
		Keramik::CenteredPainter(keramik_scrollbar_vbar_arrow1).draw(p, r, color, disabled);
		drawKeramikArrow(p, pal, r, PE_IndicatorArrowUp, down, !disabled);
	}
}

void
KeramikStyle::drawScrollBarAddLine(QPainter *p, const QRect &r, const QPalette &pal,
								   bool horizontal, bool down, bool disabled) const
{
	const QColor color = down ? pal.buttonText().color() : pal.button().color();

	if (horizontal) {
		Keramik::CenteredPainter(keramik_scrollbar_hbar_arrow2).draw(p, r, color, disabled);

		drawLine(p, r.x() + r.width() / 2 - 1, r.y() + r.height() / 2 - 3,
				 r.x() + r.width() / 2 - 1, r.y() + r.height() / 2 + 3, pal.buttonText().color());

		drawKeramikArrow(p, pal, QRect(r.x(), r.y(), r.width() / 2, r.height()),
						 PE_IndicatorArrowLeft, down, !disabled);
		drawKeramikArrow(p, pal, QRect(r.x() + r.width() / 2, r.y(), r.width() - r.width() / 2, r.height()),
						 PE_IndicatorArrowRight, down, !disabled);
	} else {
		Keramik::CenteredPainter(keramik_scrollbar_vbar_arrow2).draw(p, r, color, disabled);

		drawLine(p, r.x() + r.width() / 2 - 4, r.y() + r.height() / 2,
				 r.x() + r.width() / 2 + 2, r.y() + r.height() / 2, pal.buttonText().color());

		drawKeramikArrow(p, pal, QRect(r.x(), r.y(), r.width(), r.height() / 2),
						 PE_IndicatorArrowUp, down, !disabled);
		drawKeramikArrow(p, pal, QRect(r.x(), r.y() + r.height() / 2, r.width(), r.height() - r.height() / 2),
						 PE_IndicatorArrowDown, down, !disabled);
	}
}

void
KeramikStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *opt,
								 QPainter *p, const QWidget *widget) const
{
	if (enlarging()) {
		const qreal k = sizeScale();
		p->save();
		p->translate(opt->rect.topLeft());
		p->scale(k, k);
		const bool done = withReducedOption(control, opt, [&](const QStyleOptionComplex *reduced) {
			drawComplexControl(control, reduced, p, widget);
		});
		p->restore();
		if (done)
			return;
	}

	const QPalette &pal = opt->palette;
	const bool disabled = !(opt->state & State_Enabled);

	switch (control) {
	// COMBOBOX
	// -------------------------------------------------------------------
	case CC_ComboBox: {
		const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
		if (!cb)
			break;

		p->save();
		p->setClipRect(cb->rect, Qt::IntersectClip);

		QRect br = cb->rect;
		if (br.width() >= 28 && br.height() > 20)
			br.adjust(0, -2, 0, 0);

		if (cb->subControls & SC_ComboBoxFrame) {
			QStyleOption frame(*opt);
			frame.rect = br;
			frame.state &= ~(State_Sunken | State_On);
			proxy()->drawPrimitive(PE_PanelButtonCommand, &frame, p, widget);
		}

		if (cb->subControls & SC_ComboBoxArrow) {
			QRect ar = comboArrowRect(cb);
			ar.setWidth(ar.width() - 13);

			const int ripple = loader.size(keramik_ripple).width();
			QRect rr = visualRect(cb->direction, cb->rect,
								  QRect(ar.x(), ar.y() + 4, ripple, ar.height() - 8));
			ar = visualRect(cb->direction, cb->rect,
							QRect(ar.x() + ripple + 4, ar.y(), 11, ar.height()));

			drawSegments(p, keramik_combo_arrow, ar.x() + ar.width() / 2, ar.y() + ar.height() / 2,
						 pal.buttonText().color());

			Keramik::ScaledPainter(keramik_ripple).draw(p, rr, pal.button().color(), disabled);
		}

		if (cb->subControls & SC_ComboBoxEditField) {
			if (cb->editable) {
				QRect er = subControlRect(CC_ComboBox, cb, SC_ComboBoxEditField, widget);
				er.adjust(-2, -2, 2, 2);
				p->fillRect(er, pal.base());
				drawLineEditFrame(p, er, pal);
				drawFrameShadow(p, er, pal);
			} else if (cb->state & State_HasFocus) {
				QStyleOptionFocusRect focus;
				focus.QStyleOption::operator=(*cb);
				focus.rect = subElementRect(SE_ComboBoxFocusRect, cb, widget);
				focus.state |= State_FocusAtBorder;
				focus.backgroundColor = pal.highlight().color();
				p->fillRect(focus.rect, pal.highlight());
				proxy()->drawPrimitive(PE_FrameFocusRect, &focus, p, widget);
			}
		}

		p->restore();
		break;
	}

	// SPINBOX
	// -------------------------------------------------------------------
	case CC_SpinBox: {
		const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
		if (!sb)
			break;

		if ((sb->subControls & SC_SpinBoxFrame) && sb->frame)
			p->fillRect(sb->rect.adjusted(2, 2, -2, -2), pal.base());

		if (sb->buttonSymbols != QAbstractSpinBox::NoButtons
			&& (sb->subControls & (SC_SpinBoxUp | SC_SpinBoxDown))) {
			const QRect br = visualRect(sb->direction, sb->rect, spinButtonField(sb));
			const bool pressed = sb->state & State_Sunken;
			const bool upEnabled = !disabled && (sb->stepEnabled & QAbstractSpinBox::StepUpEnabled);
			const bool downEnabled = !disabled && (sb->stepEnabled & QAbstractSpinBox::StepDownEnabled);

			Keramik::SpinBoxPainter().draw(p, br, pal.button().color(), disabled);

			if (pressed && (sb->activeSubControls & SC_SpinBoxUp))
				Keramik::CenteredPainter(keramik_spinbox_pressed_arrow_up)
					.draw(p, br.x(), br.y() + 3, br.width(), br.height() / 2, pal.button().color());
			else
				Keramik::CenteredPainter(keramik_spinbox_arrow_up)
					.draw(p, br.x(), br.y() + 3, br.width(), br.height() / 2, pal.button().color(), !upEnabled);

			if (pressed && (sb->activeSubControls & SC_SpinBoxDown))
				Keramik::CenteredPainter(keramik_spinbox_pressed_arrow_down)
					.draw(p, br.x(), br.y() + br.height() / 2, br.width(), br.height() / 2 - 8, pal.button().color());
			else
				Keramik::CenteredPainter(keramik_spinbox_arrow_down)
					.draw(p, br.x(), br.y() + br.height() / 2, br.width(), br.height() / 2 - 8,
						  pal.window().color(), !downEnabled);
		}

		if ((sb->subControls & SC_SpinBoxFrame) && sb->frame)
			drawLineEditFrame(p, sb->rect, pal);

		break;
	}

	// SCROLLBAR
	// -------------------------------------------------------------------
	case CC_ScrollBar: {
		const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt);
		if (!sb)
			break;

		const bool horizontal = sb->orientation == Qt::Horizontal;
		const bool scrollDisabled = disabled || sb->minimum == sb->maximum;
		const SubControls active = (sb->state & State_Sunken) ? sb->activeSubControls : SC_None;

		const QRect slider = subControlRect(control, sb, SC_ScrollBarSlider, widget);
		const QRect subpage = subControlRect(control, sb, SC_ScrollBarSubPage, widget);
		const QRect addpage = subControlRect(control, sb, SC_ScrollBarAddPage, widget);
		const QRect subline = subControlRect(control, sb, SC_ScrollBarSubLine, widget);
		const QRect addline = subControlRect(control, sb, SC_ScrollBarAddLine, widget);

		if (sb->subControls & SC_ScrollBarSubLine)
			drawScrollBarSubLine(p, subline, pal, horizontal, active & SC_ScrollBarSubLine, scrollDisabled);

		QRegion clip;
		if (sb->subControls & SC_ScrollBarSubPage)
			clip |= subpage;
		if (sb->subControls & SC_ScrollBarAddPage)
			clip |= addpage;
		if (horizontal)
			clip |= QRect(slider.x(), sb->rect.y(), slider.width(), sb->rect.height());
		else
			clip |= QRect(sb->rect.x(), slider.y(), sb->rect.width(), slider.height());
		clip ^= slider;

		p->save();
		p->setClipRegion(clip, Qt::IntersectClip);
		Keramik::ScrollBarPainter(KeramikGroove1, 2, horizontal)
			.draw(p, slider | subpage | addpage, pal.button().color(), scrollDisabled);
		p->restore();

		if (sb->subControls & SC_ScrollBarSlider) {
			p->save();
			if (horizontal)
				p->setClipRect(slider.x(), slider.y(), addpage.right() - slider.x() + 1, slider.height(),
							   Qt::IntersectClip);
			else
				p->setClipRect(slider.x(), slider.y(), slider.width(), addpage.bottom() - slider.y() + 1,
							   Qt::IntersectClip);
			drawScrollBarSlider(p, slider, pal, horizontal, active & SC_ScrollBarSlider);
			p->restore();
		}

		if (sb->subControls & (SC_ScrollBarSubLine | SC_ScrollBarAddLine)) {
			drawScrollBarAddLine(p, addline, pal, horizontal, false, scrollDisabled);

			QRect half;
			if (active & SC_ScrollBarSubLine) {
				if (horizontal)
					half = QRect(addline.x(), addline.y(), addline.width() / 2, addline.height());
				else
					half = QRect(addline.x(), addline.y(), addline.width(), addline.height() / 2);
			} else if (active & SC_ScrollBarAddLine) {
				if (horizontal)
					half = QRect(addline.x() + addline.width() / 2, addline.y(), addline.width() / 2, addline.height());
				else
					half = QRect(addline.x(), addline.y() + addline.height() / 2, addline.width(), addline.height() / 2);
			}

			if (half.isValid()) {
				p->save();
				p->setClipRect(half, Qt::IntersectClip);
				drawScrollBarAddLine(p, addline, pal, horizontal, true, scrollDisabled);
				p->restore();
			}
		}
		break;
	}

	// SLIDER
	// -------------------------------------------------------------------
	case CC_Slider: {
		const QStyleOptionSlider *slider = qstyleoption_cast<const QStyleOptionSlider *>(opt);
		if (!slider)
			break;

		const bool horizontal = slider->orientation == Qt::Horizontal;
		const QRect groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
		const QRect handle = subControlRect(CC_Slider, slider, SC_SliderHandle, widget);

		if ((slider->subControls & SC_SliderGroove) && groove.isValid()) {
			if (horizontal)
				Keramik::RectTilePainter(keramik_slider_hgroove, false).draw(p, groove, pal.button().color(), disabled);
			else
				Keramik::RectTilePainter(keramik_slider_vgroove, true, false).draw(p, groove, pal.button().color(), disabled);

			// Draw the focus rect around the groove
			if (slider->state & State_HasFocus) {
				QStyleOptionFocusRect focus;
				focus.QStyleOption::operator=(*slider);
				focus.rect = groove;
				proxy()->drawPrimitive(PE_FrameFocusRect, &focus, p, widget);
			}
		}

		// Draw the tickmarks
		if (slider->subControls & SC_SliderTickmarks) {
			QStyleOptionSlider ticks(*slider);
			ticks.subControls = SC_SliderTickmarks;
			QCommonStyle::drawComplexControl(control, &ticks, p, widget);
		}

		// Draw the slider handle
		if ((slider->subControls & SC_SliderHandle) && handle.isValid()) {
			QColor hl = pal.highlight().color();
			if (!disabled && (slider->state & State_Sunken) && (slider->activeSubControls & SC_SliderHandle))
				hl = Keramik::ColorUtil::lighten(hl, 110);

			Keramik::ScaledPainter(horizontal ? keramik_slider : keramik_vslider)
				.draw(p, handle, disabled ? pal.button().color() : hl, disabled);
		}
		break;
	}

	default:
		KlassikStyle::drawComplexControl(control, opt, p, widget);
		break;
	}
}

QRect
KeramikStyle::comboArrowRect(const QStyleOptionComboBox *cb) const
{
	const QRect &r = cb->rect;
	const int arrow = 11 + loader.size(keramik_ripple).width();
	return QRect(r.x() + r.width() - arrow - 14, r.y(), arrow + 13, r.height());
}

QRect
KeramikStyle::spinButtonField(const QStyleOptionSpinBox *sb) const
{
	const QRect &r = sb->rect;
	const int fw = sb->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, sb) : 0;

	int bh = r.height() / 2 - fw;
	if (bh < 8)
		bh = 8;
	const int bw = qMin(bh * 8 / 5, r.width() / 4);

	return QRect(r.x() + r.width() - fw - bw, r.y() + fw, bw, r.height() - 2 * fw);
}

QRect
KeramikStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *opt,
							 SubControl sc, const QWidget *widget) const
{
	if (enlarging()) {
		QRect reduced;
		if (withReducedOption(control, opt, [&](const QStyleOptionComplex *ropt) {
				reduced = subControlRect(control, ropt, sc, widget);
			}))
			return enlargedRect(reduced, opt->rect, sizeScale());
	}

	switch (control) {
	case CC_ComboBox: {
		const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
		if (!cb)
			break;

		const QRect &r = cb->rect;
		const int arrow = 11 + loader.size(keramik_ripple).width();
		QRect ret;
		switch (sc) {
		case SC_ComboBoxFrame:
			ret = r;
			break;
		case SC_ComboBoxArrow:
			ret = comboArrowRect(cb);
			break;
		case SC_ComboBoxEditField:
			if (r.width() < 36 || r.height() < 22)
				ret = QRect(4, 3, r.width() - arrow - 20, r.height() - 6);
			else if (cb->editable)
				ret = QRect(8, 4, r.width() - arrow - 26, r.height() - 11);
			else
				ret = QRect(6, 4, r.width() - arrow - 22, r.height() - 9);
			ret.translate(r.topLeft());
			break;
		default:
			return KlassikStyle::subControlRect(control, opt, sc, widget);
		}
		return visualRect(cb->direction, r, ret);
	}

	case CC_SpinBox: {
		const QStyleOptionSpinBox *sb = qstyleoption_cast<const QStyleOptionSpinBox *>(opt);
		if (!sb)
			break;

		const QRect &r = sb->rect;
		const int fw = sb->frame ? proxy()->pixelMetric(PM_SpinBoxFrameWidth, sb, widget) : 0;
		const bool buttons = sb->buttonSymbols != QAbstractSpinBox::NoButtons;
		const QRect field = spinButtonField(sb);
		const int bh = qMax(r.height() / 2 - fw, 8);

		QRect ret;
		switch (sc) {
		case SC_SpinBoxUp:
			if (!buttons)
				return QRect();
			ret = QRect(field.x(), field.y(), field.width(), bh);
			break;
		case SC_SpinBoxDown:
			if (!buttons)
				return QRect();
			ret = QRect(field.x(), field.y() + bh, field.width(), bh);
			break;
		case SC_SpinBoxEditField:
			if (buttons)
				ret = QRect(r.x() + fw, r.y() + fw, field.x() - r.x() - 2 * fw, r.height() - 2 * fw);
			else
				ret = r.adjusted(fw, fw, -fw, -fw);
			break;
		case SC_SpinBoxFrame:
			ret = r;
			break;
		default:
			return KlassikStyle::subControlRect(control, opt, sc, widget);
		}
		return visualRect(sb->direction, r, ret);
	}

	case CC_ScrollBar: {
		const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt);
		if (!sb)
			break;

		const QRect &r = sb->rect;
		const bool horizontal = sb->orientation == Qt::Horizontal;
		int addline, subline, sliderpos, sliderlen, maxlen, slidermin;
		if (horizontal) {
			subline = loader.size(keramik_scrollbar_hbar_arrow1).width();
			addline = loader.size(keramik_scrollbar_hbar_arrow2).width();
			maxlen = r.width() - subline - addline + 2;
		} else {
			subline = loader.size(keramik_scrollbar_vbar_arrow1).height();
			addline = loader.size(keramik_scrollbar_vbar_arrow2).height();
			maxlen = r.height() - subline - addline + 2;
		}

		if (sb->minimum != sb->maximum) {
			qint64 range = qint64(sb->maximum) - sb->minimum;
			sliderlen = int((qint64(sb->pageStep) * maxlen) / (range + sb->pageStep));
			slidermin = proxy()->pixelMetric(PM_ScrollBarSliderMin, sb, widget);
			if (sliderlen < slidermin)
				sliderlen = slidermin;
			if (sliderlen > maxlen)
				sliderlen = maxlen;
		} else
			sliderlen = maxlen;

		sliderpos = subline + sliderPositionFromValue(sb->minimum, sb->maximum, sb->sliderPosition,
													  maxlen - sliderlen, sb->upsideDown);

		const int sbw = r.width(), sbh = r.height();
		QRect ret;
		switch (sc) {
		case SC_ScrollBarGroove:
			ret = horizontal ? QRect(subline, 0, maxlen, sbh) : QRect(0, subline, sbw, maxlen);
			break;
		case SC_ScrollBarSlider:
			ret = horizontal ? QRect(sliderpos, 0, sliderlen, sbh) : QRect(0, sliderpos, sbw, sliderlen);
			break;
		case SC_ScrollBarSubLine:
			ret = horizontal ? QRect(0, 0, subline, sbh) : QRect(0, 0, sbw, subline);
			break;
		case SC_ScrollBarAddLine:
			ret = horizontal ? QRect(sbw - addline, 0, addline, sbh) : QRect(0, sbh - addline, sbw, addline);
			break;
		case SC_ScrollBarSubPage:
			ret = horizontal ? QRect(subline, 0, sliderpos - subline, sbh) : QRect(0, subline, sbw, sliderpos - subline);
			break;
		case SC_ScrollBarAddPage:
			ret = horizontal ? QRect(sliderpos + sliderlen, 0, sbw - addline - (sliderpos + sliderlen), sbh)
							 : QRect(0, sliderpos + sliderlen, sbw, sbh - addline - (sliderpos + sliderlen));
			break;
		default:
			return KlassikStyle::subControlRect(control, opt, sc, widget);
		}
		return ret.translated(r.topLeft());
	}

	case CC_Slider: {
		const QStyleOptionSlider *sl = qstyleoption_cast<const QStyleOptionSlider *>(opt);
		if (!sl)
			break;

		const QRect &r = sl->rect;
		const bool horizontal = sl->orientation == Qt::Horizontal;
		const QSlider::TickPosition ticks = sl->tickPosition;
		const int size = proxy()->pixelMetric(PM_SliderControlThickness, sl, widget);
		const int handleSize = proxy()->pixelMetric(PM_SliderThickness, sl, widget);
		const int len = proxy()->pixelMetric(PM_SliderLength, sl, widget);
		const int pos = sliderPositionFromValue(sl->minimum, sl->maximum, sl->sliderPosition,
												(horizontal ? r.width() : r.height()) - len,
												sl->upsideDown);
		QRect ret;
		switch (sc) {
		case SC_SliderGroove:
			if (horizontal) {
				if (ticks == QSlider::TicksBothSides)
					ret = QRect(0, (r.height() - size) / 2, r.width(), size);
				else if (ticks == QSlider::TicksAbove)
					ret = QRect(0, r.height() - size - (handleSize - size) / 2, r.width(), size);
				else
					ret = QRect(0, (handleSize - size) / 2, r.width(), size);
			} else {
				if (ticks == QSlider::TicksBothSides)
					ret = QRect((r.width() - size) / 2, 0, size, r.height());
				else if (ticks == QSlider::TicksAbove)
					ret = QRect(r.width() - size - (handleSize - size) / 2, 0, size, r.height());
				else
					ret = QRect((handleSize - size) / 2, 0, size, r.height());
			}
			break;
		case SC_SliderHandle:
			if (horizontal) {
				if (ticks == QSlider::TicksBothSides)
					ret = QRect(pos, (r.height() - handleSize) / 2, len, handleSize);
				else if (ticks == QSlider::TicksAbove)
					ret = QRect(pos, r.height() - handleSize, len, handleSize);
				else
					ret = QRect(pos, 0, len, handleSize);
			} else {
				if (ticks == QSlider::TicksBothSides)
					ret = QRect((r.width() - handleSize) / 2, pos, handleSize, len);
				else if (ticks == QSlider::TicksAbove)
					ret = QRect(r.width() - handleSize, pos, handleSize, len);
				else
					ret = QRect(0, pos, handleSize, len);
			}
			break;
		default:
			return KlassikStyle::subControlRect(control, opt, sc, widget);
		}
		return ret.translated(r.topLeft());
	}

	default:
		break;
	}

	return KlassikStyle::subControlRect(control, opt, sc, widget);
}

QStyle::SubControl
KeramikStyle::hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *opt,
									const QPoint &pt, const QWidget *widget) const
{
	if (enlarging()) {
		const qreal k = sizeScale();
		const QPoint reducedPt(int(std::floor((pt.x() - opt->rect.x()) / k)),
							   int(std::floor((pt.y() - opt->rect.y()) / k)));
		SubControl result = SC_None;
		if (withReducedOption(control, opt, [&](const QStyleOptionComplex *reduced) {
				result = hitTestComplexControl(control, reduced, reducedPt, widget);
			}))
			return result;
	}

	SubControl result = KlassikStyle::hitTestComplexControl(control, opt, pt, widget);

	// The upper half of the double arrow button scrolls back
	if (control == CC_ScrollBar && result == SC_ScrollBarAddLine) {
		if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(opt)) {
			QRect addline = subControlRect(control, sb, SC_ScrollBarAddLine, widget);
			if (sb->orientation == Qt::Horizontal) {
				if (pt.x() < addline.center().x())
					result = SC_ScrollBarSubLine;
			} else if (pt.y() < addline.center().y())
				result = SC_ScrollBarSubLine;
		}
	}
	return result;
}

int
KeramikStyle::pixelMetric(PixelMetric metric, const QStyleOption *opt,
						  const QWidget *widget) const
{
	if (enlarging()) {
		switch (metric) {
		case PM_SliderLength:
		case PM_SliderControlThickness:
		case PM_SliderThickness:
		case PM_ExclusiveIndicatorWidth:
		case PM_ExclusiveIndicatorHeight:
		case PM_IndicatorWidth:
		case PM_IndicatorHeight:
		case PM_ScrollBarExtent:
		case PM_ScrollBarSliderMin:
		case PM_TabBarTabHSpace:
		case PM_TabBarTabVSpace: {
			++m_reducedDepth;
			const int reduced = pixelMetric(metric, opt, widget);
			--m_reducedDepth;
			if (metric == PM_TabBarTabVSpace)
				return qRound(reduced * sizeScale()) + extraHeight(0.3);
			return qRound(reduced * sizeScale());
		}
		default:
			break;
		}
	}

	switch (metric) {
	// BUTTONS
	// -------------------------------------------------------------------
	case PM_ButtonMargin: // Space btw. frame and label
		return 4;

	case PM_ButtonDefaultIndicator:
		return 0;

	case PM_ButtonShiftHorizontal:
		return 0;
	case PM_ButtonShiftVertical: // Offset by 1
		return 1;

	// SLIDER
	// -------------------------------------------------------------------
	case PM_SliderLength:
		return 12;
	case PM_SliderControlThickness:
		return loader.size(keramik_slider).height() - 4;
	case PM_SliderThickness:
		return loader.size(keramik_slider).height();

	// CHECKBOXES / RADIO BUTTONS
	// -------------------------------------------------------------------
	case PM_ExclusiveIndicatorWidth: // Radiobutton size
		return loader.size(keramik_radiobutton_on).width();
	case PM_ExclusiveIndicatorHeight:
		return loader.size(keramik_radiobutton_on).height();
	case PM_IndicatorWidth: // Checkbox size
		return loader.size(keramik_checkbox_on).width();
	case PM_IndicatorHeight:
		return loader.size(keramik_checkbox_on).height();

	// SCROLLBAR
	// -------------------------------------------------------------------
	case PM_ScrollBarExtent:
		return loader.size(keramik_scrollbar_vbar + KeramikGroove1).width();
	case PM_ScrollBarSliderMin:
		return loader.size(keramik_scrollbar_vbar + KeramikSlider1).height() +
			loader.size(keramik_scrollbar_vbar + KeramikSlider3).height();

	// FRAMES
	// -------------------------------------------------------------------
	case PM_DefaultFrameWidth:
		// Keramik's line edit frame is 2 pixels wide
		if (qobject_cast<const QLineEdit *>(widget))
			return 2;
		// Room for the frame around the list of a combobox popup
		if (isComboBoxPopup(widget))
			return 4;
		return 1;

	case PM_SpinBoxFrameWidth:
		return 2;

	case PM_MenuBarPanelWidth:
		return 1;

	// GENERAL
	// -------------------------------------------------------------------
	case PM_MenuButtonIndicator:
		return 13;

	case PM_TabBarTabHSpace:
		return 24;

	case PM_TabBarTabVSpace:
		return 12;

	case PM_TabBarTabOverlap:
		return 0;

	case PM_TitleBarHeight:
		return 22;

	default:
		return KlassikStyle::pixelMetric(metric, opt, widget);
	}
}

QSize
KeramikStyle::sizeFromContents(ContentsType contents, const QStyleOption *opt,
							   const QSize &contentSize, const QWidget *widget) const
{
	// The contents keep their size, the room around them is enlarged.
	// Modern fonts also need more room above and below the text.
	if (enlarging() && (contents == CT_PushButton || contents == CT_ComboBox ||
						contents == CT_SpinBox || contents == CT_LineEdit)) {
		++m_reducedDepth;
		const QSize reduced = sizeFromContents(contents, opt, contentSize, widget);
		--m_reducedDepth;
		const qreal k = sizeScale();
		// Line edits have only a thin frame, give them a height close to
		// combos. Spinboxes get it from the size of their line edit.
		qreal extra = 0.3;
		if (contents == CT_LineEdit)
			extra = 0.45;
		else if (contents == CT_SpinBox)
			extra = 0;
		return QSize(contentSize.width() + qRound((reduced.width() - contentSize.width()) * k),
					 contentSize.height() + qRound((reduced.height() - contentSize.height()) * k) +
					 extraHeight(extra));
	}

	// Tool buttons outside of toolbars are drawn like buttons, at their normal size
	if (enlarging() && contents == CT_ToolButton && !isOnToolBar(widget)) {
		++m_reducedDepth;
		const QSize size = sizeFromContents(contents, opt, contentSize, widget);
		--m_reducedDepth;
		return size + QSize(0, extraHeight(0.3));
	}

	switch (contents) {
	// PUSHBUTTON SIZE
	// ------------------------------------------------------------------
	case CT_PushButton: {
		const QStyleOptionButton *btn = qstyleoption_cast<const QStyleOptionButton *>(opt);

		int w = contentSize.width() + 2 * proxy()->pixelMetric(PM_ButtonMargin, opt, widget);
		int h = contentSize.height() + 2 * proxy()->pixelMetric(PM_ButtonMargin, opt, widget);
		if (btn && btn->text.isEmpty() && contentSize.width() < 32)
			return QSize(w, h);

		return QSize(w + 30, h + 5); //MX: No longer blank space -- can make a bit smaller
	}

	case CT_ToolButton: {
		if (!isOnToolBar(widget)) {
			// Behaves like a button, so scale appropriately to the border
			return QSize(contentSize.width() + 12, contentSize.height() + 10);
		}
		return KlassikStyle::sizeFromContents(contents, opt, contentSize, widget);
	}

	case CT_ComboBox: {
		const QStyleOptionComboBox *cb = qstyleoption_cast<const QStyleOptionComboBox *>(opt);
		int arrow = 11 + loader.size(keramik_ripple).width();
		return QSize(contentSize.width() + arrow + ((cb && cb->editable) ? 26 : 22),
					 contentSize.height() + 10);
	}

	// POPUPMENU ITEM SIZE
	// -----------------------------------------------------------------
	case CT_MenuItem: {
		const QStyleOptionMenuItem *mi = qstyleoption_cast<const QStyleOptionMenuItem *>(opt);
		if (!mi)
			return contentSize;

		const bool checkable = mi->checkType != QStyleOptionMenuItem::NotCheckable;
		const int maxpmw = mi->maxIconWidth;
		int w = contentSize.width(), h = contentSize.height();

		if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
			w = 30; // Arbitrary
			h = 3;
		} else {
			// Ensure that the minimum height for text-only menu items
			// is the same as the icon size used by KDE.
			h = qMax(h, 16 + 2 * itemFrame);
			h = qMax(h, mi->fontMetrics.height() + 2 * itemVMargin + 2 * itemFrame);

			if (!mi->icon.isNull()) {
				const int iconExtent = proxy()->pixelMetric(PM_SmallIconSize, opt, widget);
				h = qMax(h, mi->icon.actualSize(QSize(iconExtent, iconExtent)).height() + 2 * itemFrame);
			}
		}

		if (mi->text.contains(u'\t'))
			w += 12;
		else if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
			w += 2 * arrowHMargin;

		if (maxpmw)
			w += maxpmw + 6;
		if (checkable && maxpmw < 20)
			w += 20 - maxpmw;
		if (checkable || maxpmw > 0)
			w += 12;

		w += rightBorder;

		return QSize(w, h);
	}

	default:
		return KlassikStyle::sizeFromContents(contents, opt, contentSize, widget);
	}
}
