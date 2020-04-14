// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divepicturemodel.h"
#include "core/metrics.h"
#include "core/divelist.h" // for mark_divelist_changed()
#include "core/dive.h"
#include "core/imagedownloader.h"
#include "core/picture.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/divelistnotifier.h"

#include <QFileInfo>
#include <QPainter>

DivePictureModel *DivePictureModel::instance()
{
	static DivePictureModel *self = new DivePictureModel();
	return self;
}

DivePictureModel::DivePictureModel() : zoomLevel(0.0)
{
	connect(Thumbnailer::instance(), &Thumbnailer::thumbnailChanged,
		this, &DivePictureModel::updateThumbnail, Qt::QueuedConnection);
	connect(&diveListNotifier, &DiveListNotifier::pictureOffsetChanged,
		this, &DivePictureModel::pictureOffsetChanged);
}

void DivePictureModel::setZoomLevel(int level)
{
	zoomLevel = level / 10.0;
	// zoomLevel is bound by [-1.0 1.0], see comment below.
	if (zoomLevel < -1.0)
		zoomLevel = -1.0;
	if (zoomLevel > 1.0)
		zoomLevel = 1.0;
	updateZoom();
	layoutChanged();
}

void DivePictureModel::updateZoom()
{
	size = Thumbnailer::thumbnailSize(zoomLevel);
}

void DivePictureModel::updateThumbnails()
{
	updateZoom();
	for (PictureEntry &entry: pictures)
		entry.image = Thumbnailer::instance()->fetchThumbnail(QString::fromStdString(entry.filename), false);
}

void DivePictureModel::updateDivePictures()
{
	beginResetModel();
	if (!pictures.isEmpty()) {
		pictures.clear();
		Thumbnailer::instance()->clearWorkQueue();
	}

	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->selected) {
			int first = pictures.count();
			FOR_EACH_PICTURE(dive)
				pictures.push_back({ dive->id, picture->filename, {}, picture->offset.seconds, {.seconds = 0}});

			// Sort pictures of this dive by offset.
			// Thus, the list will be sorted by (diveId, offset).
			std::sort(pictures.begin() + first, pictures.end(),
				  [](const PictureEntry &a, const PictureEntry &b) { return a.offsetSeconds < b.offsetSeconds; });
		}
	}

	updateThumbnails();
	endResetModel();
}

int DivePictureModel::columnCount(const QModelIndex&) const
{
	return 2;
}

QVariant DivePictureModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	const PictureEntry &entry = pictures.at(index.row());
	if (index.column() == 0) {
		switch (role) {
		case Qt::ToolTipRole:
			return QString::fromStdString(entry.filename);
		case Qt::DecorationRole:
			return entry.image.scaled(size, size, Qt::KeepAspectRatio);
		case Qt::DisplayRole:
			return QFileInfo(QString::fromStdString(entry.filename)).fileName();
		case Qt::DisplayPropertyRole:
			return QFileInfo(QString::fromStdString(entry.filename)).filePath();
		case Qt::UserRole:
			return entry.diveId;
		case Qt::UserRole + 1:
			return entry.offsetSeconds;
		case Qt::UserRole + 2:
			return entry.length.seconds;
		}
	} else if (index.column() == 1) {
		switch (role) {
		case Qt::DisplayRole:
			return QString::fromStdString(entry.filename);
		}
	}
	return QVariant();
}

// Return true if we actually removed a picture
static bool removePictureFromSelectedDive(const char *fileUrl)
{
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		if (dive->selected && remove_picture(&dive->pictures, fileUrl)) {
			invalidate_dive_cache(dive);
			return true;
		}
	}
	return false;
}

void DivePictureModel::removePictures(const QVector<QString> &fileUrlsIn)
{
	// Transform vector of QStrings into vector of std::strings
	std::vector<std::string> fileUrls;
	fileUrls.reserve(fileUrlsIn.size());
	std::transform(fileUrlsIn.begin(), fileUrlsIn.end(), std::back_inserter(fileUrls),
		       [] (const QString &s) { return s.toStdString(); });

	bool removed = false;
	for (const std::string &fileUrl: fileUrls)
		removed |= removePictureFromSelectedDive(fileUrl.c_str());
	if (!removed)
		return;
	copy_dive(current_dive, &displayed_dive);
	mark_divelist_changed(true);

	for (int i = 0; i < pictures.size(); ++i) {
		// Find range [i j) of pictures to remove
		if (std::find(fileUrls.begin(), fileUrls.end(), pictures[i].filename) == fileUrls.end())
			continue;
		int j;
		for (j = i + 1; j < pictures.size(); ++j) {
			if (std::find(fileUrls.begin(), fileUrls.end(), pictures[j].filename) == fileUrls.end())
				break;
		}

		// Qt's model-interface is surprisingly idiosyncratic: you don't pass [first last), but [first last] ranges.
		// For example, an empty list would be [0 -1].
		beginRemoveRows(QModelIndex(), i, j - 1);
		pictures.erase(pictures.begin() + i, pictures.begin() + j);
		endRemoveRows();
	}
	emit picturesRemoved(fileUrlsIn);
}

int DivePictureModel::rowCount(const QModelIndex&) const
{
	return pictures.count();
}

int DivePictureModel::findPictureId(const std::string &filename)
{
	for (int i = 0; i < pictures.size(); ++i)
		if (pictures[i].filename == filename)
			return i;
	return -1;
}

static void addDurationToThumbnail(QImage &img, duration_t duration)
{
	int seconds = duration.seconds;
	if (seconds < 0)
		return;
	QString s = seconds >= 3600 ?
		QStringLiteral("%1:%2:%3").arg(seconds / 3600, 2, 10, QChar('0'))
					  .arg((seconds % 3600) / 60, 2, 10, QChar('0'))
					  .arg(seconds % 60, 2, 10, QChar('0')) :
		QStringLiteral("%1:%2").arg(seconds / 60, 2, 10, QChar('0'))
				       .arg(seconds % 60, 2, 10, QChar('0'));

	QFont font(system_divelist_default_font, 30);
	QFontMetrics metrics(font);
	QSize size = metrics.size(Qt::TextSingleLine, s);
	QSize imgSize = img.size();
	int x = imgSize.width() - size.width();
	int y = imgSize.height() - size.height() + metrics.descent();
	QPainter painter(&img);
	painter.setBrush(Qt::white);
	painter.setPen(Qt::NoPen);
	painter.drawRect(x, y, size.width(), size.height() - metrics.descent());
	painter.setFont(font);
	painter.setPen(Qt::black);
	painter.drawText(x, imgSize.height(), s);
}

void DivePictureModel::updateThumbnail(QString filename, QImage thumbnail, duration_t duration)
{
	int i = findPictureId(filename.toStdString());
	if (i >= 0) {
		if (duration.seconds > 0) {
			addDurationToThumbnail(thumbnail, duration);	// If we know the duration paint it on top of the thumbnail
			pictures[i].length = duration;
		}
		pictures[i].image = thumbnail;
		emit dataChanged(createIndex(i, 0), createIndex(i, 1));
	}
}

void DivePictureModel::pictureOffsetChanged(dive *d, const QString filenameIn, offset_t offset)
{
	std::string filename = filenameIn.toStdString();

	// Find the pictures of the given dive.
	auto from = std::find_if(pictures.begin(), pictures.end(), [d](const PictureEntry &e) { return e.diveId == d->id; });
	auto to = std::find_if(from, pictures.end(), [d](const PictureEntry &e) { return e.diveId != d->id; });

	// Find picture with the given filename
	auto oldPos = std::find_if(from, to, [filename](const PictureEntry &e) { return e.filename == filename; });
	if (oldPos == to)
		return;

	// Find new position
	auto newPos = std::find_if(from, to, [offset](const PictureEntry &e) { return e.offsetSeconds > offset.seconds; });

	// Update the offset here and in the backend
	oldPos->offsetSeconds = offset.seconds;
	copy_dive(current_dive, &displayed_dive); // TODO: remove once profile can display arbitrary dives

	// Henceforth we will work with indices instead of iterators
	int oldIndex = oldPos - pictures.begin();
	int newIndex = newPos - pictures.begin();
	if (oldIndex == newIndex || oldIndex + 1 == newIndex)
		return;
	beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), newIndex);
	moveInVector(pictures, oldIndex, oldIndex + 1, newIndex);
	endMoveRows();
}
