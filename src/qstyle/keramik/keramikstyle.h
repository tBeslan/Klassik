/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Malte Starostik <malte@kde.org>
   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "../klassikstyle.h"

#include <QBasicTimer>
#include <QHash>

class QStyleOptionComboBox;
class QStyleOptionSpinBox;

// Port of the KDE 3.1 Keramik widget style. Elements Keramik does not
// draw itself fall back to the KDE 3 HighColor style it was based on.
class KeramikStyle : public KlassikStyle
{
	Q_OBJECT
public:
	KeramikStyle();
	~KeramikStyle() override = default;

	using KlassikStyle::polish;
	void polish(QWidget *widget) override;
	using KlassikStyle::unpolish;
	void unpolish(QWidget *widget) override;

	bool eventFilter(QObject *object, QEvent *event) override;

	void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt,
					   QPainter *p, const QWidget *widget = nullptr) const override;

	void drawControl(ControlElement control, const QStyleOption *opt,
					 QPainter *p, const QWidget *widget = nullptr) const override;

	QRect subElementRect(SubElement element, const QStyleOption *opt,
						 const QWidget *widget = nullptr) const override;

	void drawComplexControl(ComplexControl control, const QStyleOptionComplex *opt,
							QPainter *p, const QWidget *widget = nullptr) const override;

	QRect subControlRect(ComplexControl control, const QStyleOptionComplex *opt,
						 SubControl sc, const QWidget *widget = nullptr) const override;

	SubControl hitTestComplexControl(ComplexControl control, const QStyleOptionComplex *opt,
									 const QPoint &pt, const QWidget *widget = nullptr) const override;

	int pixelMetric(PixelMetric metric, const QStyleOption *opt = nullptr,
					const QWidget *widget = nullptr) const override;

	QSize sizeFromContents(ContentsType contents, const QStyleOption *opt,
						   const QSize &contentsSize,
						   const QWidget *widget = nullptr) const override;

protected:
	void timerEvent(QTimerEvent *event) override;

private:
	void drawLineEditFrame(QPainter *p, const QRect &r, const QPalette &pal) const;
	void drawFrameShadow(QPainter *p, const QRect &r, const QPalette &pal) const;

	void drawScrollBarSlider(QPainter *p, const QRect &r, const QPalette &pal,
							 bool horizontal, bool active) const;
	void drawScrollBarSubLine(QPainter *p, const QRect &r, const QPalette &pal,
							  bool horizontal, bool down, bool disabled) const;
	void drawScrollBarAddLine(QPainter *p, const QRect &r, const QPalette &pal,
							  bool horizontal, bool down, bool disabled) const;

	// Combobox arrow area in left-to-right coordinates
	QRect comboArrowRect(const QStyleOptionComboBox *cb) const;
	// Area of the spinbox buttons, in left-to-right coordinates
	QRect spinButtonField(const QStyleOptionSpinBox *sb) const;

	// Position of the moving block of each busy progress bar
	QHash<const QObject *, int> m_busySteps;
	mutable QBasicTimer m_busyTimer;
};
