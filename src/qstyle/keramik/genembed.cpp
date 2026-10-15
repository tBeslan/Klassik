/*
   SPDX-FileCopyrightText: 2026 neeeeow <https://github.com/neeeeow>

   SPDX-FileCopyrightText: 2002 Maksim Orlovich <mo002j@mail.rochester.edu>

   SPDX-License-Identifier: MIT
*/

// Build-time tool: converts the Keramik PNG tiles into recolorable data
// tables (pixmaps.keramik) and the tile id enumeration (keramikrc.h).
//
// Usage: genembed <pixmaps.keramik> <keramikrc.h> <png files...>

#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QMap>
#include <QTextStream>

#include <cmath>
#include <cstdio>

#include "keramikimage.h"

/**
Need to generate something like this:
TargetColorAlpha, GreyAdd, SrcAlpha;

so that one can do (R*T+GreyAdd, G*T+GreyAdd, B*T+GreyAdd, SrcAlpha) as pixel values
*/

static int
evalSuffix(const QString &suffix)
{
	if (suffix == QLatin1String("-tl"))
		return 0;
	if (suffix == QLatin1String("-tc"))
		return 1;
	if (suffix == QLatin1String("-tr"))
		return 2;
	if (suffix == QLatin1String("-cl"))
		return 3;
	if (suffix == QLatin1String("-cc"))
		return 4;
	if (suffix == QLatin1String("-cr"))
		return 5;
	if (suffix == QLatin1String("-bl"))
		return 6;
	if (suffix == QLatin1String("-bc"))
		return 7;
	if (suffix == QLatin1String("-br"))
		return 8;
	if (suffix == QLatin1String("-separator"))
		return KeramikTileSeparator;
	if (suffix == QLatin1String("-slider1"))
		return KeramikSlider1;
	if (suffix == QLatin1String("-slider2"))
		return KeramikSlider2;
	if (suffix == QLatin1String("-slider3"))
		return KeramikSlider3;
	if (suffix == QLatin1String("-slider4"))
		return KeramikSlider4;
	if (suffix == QLatin1String("-groove1"))
		return KeramikGroove1;
	if (suffix == QLatin1String("-groove2"))
		return KeramikGroove2;
	if (suffix == QLatin1String("-1"))
		return 1;
	if (suffix == QLatin1String("-2"))
		return 2;
	if (suffix == QLatin1String("-3"))
		return 3;

	return -1;
}

// Port of KImageEffect::contrastHSV(img, true) from kdefx
static void
contrastHSV(QImage &img)
{
	const double scale = 0.5000000000000001;
	const int sign = 1;

	for (int y = 0; y < img.height(); ++y) {
		QRgb *data = reinterpret_cast<QRgb *>(img.scanLine(y));
		for (int x = 0; x < img.width(); ++x) {
			QColor c(data[x]);
			int h, s, v;
			c.getHsv(&h, &s, &v);
			double brightness = v / 255.0;
			double theta = (brightness - 0.5) * M_PI;
			brightness += scale * (((scale * ((std::sin(theta) + 1.0))) - brightness) * sign);
			if (brightness > 1.0)
				brightness = 1.0;
			else if (brightness < 0)
				brightness = 0.0;
			v = int(brightness * 255);
			c.setHsv(h, s, v);
			data[x] = qRgba(c.red(), c.green(), c.blue(), qAlpha(data[x]));
		}
	}
}

struct ImageInfo
{
	bool haveAlpha;
	int width;
	int height;
	int id;
	QString name;
};

int
main(int argc, char **argv)
{
	if (argc < 4) {
		std::fprintf(stderr, "usage: %s <pixmaps.keramik> <keramikrc.h> <png files...>\n", argv[0]);
		return 1;
	}

	QFile embedFile(QString::fromLocal8Bit(argv[1]));
	if (!embedFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		std::fprintf(stderr, "genembed: can't write %s\n", argv[1]);
		return 1;
	}
	QTextStream out(&embedFile);

	QList<ImageInfo> images;
	QMap<QString, int> assignID;
	int nextID = 0;

	out << "#include <QHash>\n\n";
	out << "#include \"keramikimage.h\"\n\n";

	for (int c = 3; c < argc; c++) {
		const QString path = QString::fromLocal8Bit(argv[c]);
		QImage input(path);
		if (input.isNull()) {
			std::fprintf(stderr, "genembed: can't load %s\n", argv[c]);
			return 1;
		}
		input = input.convertToFormat(QImage::Format_ARGB32);

		QString s = QFileInfo(path).completeBaseName();
		QString id = s;
		int readJustID = 0;

		int pos;
		if ((pos = s.lastIndexOf(QLatin1Char('-'))) != -1) {
			int suffix = evalSuffix(s.mid(pos));
			if (suffix != -1) {
				id = s.mid(0, pos);
				readJustID = suffix;
			}
		}

		if (!assignID.contains(id)) {
			assignID[id] = nextID;
			nextID += 256;
		}

		s.replace(QLatin1Char('-'), QLatin1Char('_'));

		if (s.contains(QLatin1String("button")))
			contrastHSV(input);

		int fullID = assignID[id] + readJustID; // Subwidget..

		bool highlights = true;
		bool shadows = true;
		float gamma = 1.0;
		int brightAdj = 0;

		// NOTE: the "tab-top-active" test never matches, since the dashes are
		// already replaced at this point. Kept as in the original for fidelity.
		if (s.contains(QLatin1String("toolbar")) || s.contains(QLatin1String("tab-top-active"))
			|| s.contains(QLatin1String("menubar"))) {
			gamma = 1 / 1.25f;
			shadows = false;
		}

		if (s.contains(QLatin1String("scrollbar")) && s.contains(QLatin1String("groove")))
			shadows = false;

		if (s.contains(QLatin1String("scrollbar")) && s.contains(QLatin1String("slider")))
			gamma = 1 / 0.7f;

		if (s.contains(QLatin1String("menuitem")))
			gamma = 1 / 0.6f;

		bool reallySolid = true;
		for (int y = 0; y < input.height(); ++y) {
			const QRgb *read = reinterpret_cast<const QRgb *>(input.constScanLine(y));
			for (int x = 0; x < input.width(); ++x)
				if (qAlpha(read[x]) != 255)
					reallySolid = false;
		}

		images.append({ !reallySolid, input.width(), input.height(), fullID, s });

		out << "static const unsigned char " << s << "[]={\n";

		int count = 0;
		for (int y = 0; y < input.height(); ++y) {
			const QRgb *read = reinterpret_cast<const QRgb *>(input.constScanLine(y));
			for (int x = 0; x < input.width(); ++x) {
				QRgb basePix = read[x];
				QColor clr(basePix);
				int h, sat, v;
				clr.getHsv(&h, &sat, &v);

				v = qGray(basePix);

				int targetColorAlpha = 0, greyAdd = 0;

				if (sat > 0 || v > 128) { // Non-shadow
					float fv = v / 255.0;
					fv = std::pow(fv, gamma);
					v = int(255.5 * fv);

					if (sat < 17 && highlights) { // A bit of a highligt..
						float effectPortion = (16 - sat) / 15.0;

						greyAdd = (int)(v / 4.0 * effectPortion * 1.2);
						targetColorAlpha = v - greyAdd;
					} else {
						targetColorAlpha = v;
						greyAdd = 0;
					}
				} else {
					if (shadows) {
						targetColorAlpha = 0;
						greyAdd = v;
					} else {
						targetColorAlpha = v;
						greyAdd = 0;
					}
				}

				greyAdd += brightAdj;

				if (reallySolid)
					out << targetColorAlpha << "," << greyAdd << ",";
				else
					out << targetColorAlpha << "," << greyAdd << "," << qAlpha(basePix) << ",";

				if (count++ % 8 == 7)
					out << "\n";
			}
		}

		out << "\n};\n\n";
	}

	out << "static const KeramikEmbedImage image_db[] = {\n";
	for (const ImageInfo &image : std::as_const(images))
		out << "\t{ " << (image.haveAlpha ? "true" : "false") << ", " << image.width << ", "
			<< image.height << ", " << image.id << ", " << image.name << " },\n";
	out << "\t{ false, 0, 0, 0, nullptr }\n";
	out << "};\n\n";

	out << "const KeramikEmbedImage *KeramikGetDbImage(int id)\n";
	out << "{\n";
	out << "\tstatic const QHash<int, const KeramikEmbedImage *> db = [] {\n";
	out << "\t\tQHash<int, const KeramikEmbedImage *> images;\n";
	out << "\t\tfor (int c = 0; image_db[c].width; c++)\n";
	out << "\t\t\timages.insert(image_db[c].id, &image_db[c]);\n";
	out << "\t\treturn images;\n";
	out << "\t}();\n";
	out << "\treturn db.value(id, nullptr);\n";
	out << "}\n";

	out.flush();
	embedFile.close();

	QFile rcFile(QString::fromLocal8Bit(argv[2]));
	if (!rcFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		std::fprintf(stderr, "genembed: can't write %s\n", argv[2]);
		return 1;
	}
	QTextStream ts(&rcFile);
	ts << "#ifndef KERAMIK_RC_H\n";
	ts << "#define KERAMIK_RC_H\n";
	ts << "enum KeramikWidget {\n";
	for (auto i = assignID.cbegin(); i != assignID.cend(); ++i) {
		QString name = QLatin1String("keramik_") + i.key();
		name.replace(QLatin1Char('-'), QLatin1Char('_'));
		ts << "\t" << name << " = " << i.value() << ",\n";
	}
	ts << "\tkeramik_last\n";
	ts << "};\n";
	ts << "#endif\n";

	return 0;
}
