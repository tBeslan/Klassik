/*
   SPDX-FileCopyrightText: 2026 Klassik contributors

   SPDX-License-Identifier: GPL-3.0-or-later

   Recreation of the KDE 3.1 splash screen (kdebase/ksplash, by Matthias
   Ettrich and Matthias Elter). The images are the original KDE 3.1 ones.
*/

import QtQuick

Rectangle {
	id: root

	// Set by ksplashqml: 1 (initial) .. 6 (desktop ready, the splash closes)
	property int stage

	// KDE 3 progress state: 0..6 light up the icons one by one, 7 means done
	readonly property int splashState: Math.min(7, Math.round((stage - 1) * 7 / 5))

	readonly property var stateTexts: [
		"Установка межпрограммных связей",
		"Инициализация системных служб",
		"Инициализация периферийных устройств",
		"Загрузка оконного менеджера",
		"Загрузка рабочего стола",
		"Загрузка панели",
		"Восстановление сеанса",
		"KDE запущен и готов к работе"
	]

	// The desktop background of the KDE 3.1 default setup
	gradient: Gradient {
		GradientStop { position: 0.0; color: "#4185aa" }
		GradientStop { position: 1.0; color: "#9aadbb" }
	}

	// Where the inactive part of the icon bar starts, see KSplash::makePixmap()
	function barOffset(state) {
		let offset = state * 58;
		if (state === 3)
			offset += 8;
		if (state === 6)
			offset -= 8;
		return Math.min(offset, bar.width);
	}

	// The icon of the current stage blinks, as in KDE 3
	property bool blinkOn: true
	Timer {
		interval: 400
		repeat: true
		running: root.splashState < 7
		onTriggered: root.blinkOn = !root.blinkOn
	}

	Column {
		id: splash
		anchors.centerIn: parent

		Image {
			source: "images/splash_top.png"
			smooth: true
		}

		Item {
			id: bar
			width: activeBar.width
			height: activeBar.height

			readonly property int offset: root.barOffset(root.splashState + (root.blinkOn ? 1 : 0))

			Image {
				id: activeBar
				source: "images/splash_active_bar.png"
				smooth: true
			}

			Item {
				x: bar.offset
				width: bar.width - bar.offset
				height: bar.height
				clip: true

				Image {
					x: -bar.offset
					source: "images/splash_inactive_bar.png"
					smooth: true
				}
			}
		}

		Rectangle {
			id: bottom
			width: bar.width
			height: Math.max(16, statusText.implicitHeight + 4)
			color: "black"

			Image {
				anchors.fill: parent
				source: "images/splash_bottom.png"
				fillMode: Image.Stretch
				smooth: true
			}

			Text {
				id: statusText
				anchors {
					left: parent.left
					leftMargin: 5
					right: progress.visible ? progress.left : parent.right
					rightMargin: 4
					verticalCenter: parent.verticalCenter
				}
				text: root.stateTexts[root.splashState]
				color: "white"
				font.bold: true
				elide: Text.ElideRight
			}

			// Plasma doesn't report how far the session restore is, so the
			// progress bar only appears filled once the session is restored
			Rectangle {
				id: progress
				visible: root.splashState >= 6
				anchors {
					right: parent.right
					rightMargin: 4
					top: parent.top
					topMargin: 2
					bottom: parent.bottom
					bottomMargin: 2
				}
				width: 120
				color: "#01008a"

				Text {
					anchors.centerIn: parent
					text: "100%"
					color: "white"
					font.bold: true
					font.pixelSize: parent.height - 2
				}
			}
		}
	}
}
