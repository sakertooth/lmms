/*
 * SampleFilePicker.cpp
 *
 * Copyright (c) 2024 saker
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#include "SampleFilePicker.h"

#include "ConfigManager.h"
#include "FileDialog.h"
#include "PathUtil.h"
#include "SampleDecoder.h"

namespace lmms::gui {
QString SampleFilePicker::openAudioFile(const QString& previousFile)
{
	auto openFileDialog = FileDialog(nullptr, QObject::tr("Open audio file"));
	auto dir = !previousFile.isEmpty() ? PathUtil::toAbsolute(previousFile) : ConfigManager::inst()->userSamplesDir();

	// change dir to position of previously opened file
	openFileDialog.setDirectory(dir);
	openFileDialog.setFileMode(FileDialog::ExistingFiles);

	// set filters
	auto fileTypes = QStringList{};
	auto allFileTypes = QStringList{};
	auto nameFilters = QStringList{};
	const auto& supportedAudioTypes = SampleDecoder::supportedAudioTypes();

	for (const auto& audioType : supportedAudioTypes)
	{
		const auto name = QString::fromStdString(audioType.name);
		const auto extension = QString::fromStdString(audioType.extension);
		const auto displayExtension = QString{"*.%1"}.arg(extension);
		fileTypes.append(QString{"%1 (%2)"}.arg(FileDialog::tr("%1 files").arg(name), displayExtension));
		allFileTypes.append(displayExtension);
	}

	nameFilters.append(QString{"%1 (%2)"}.arg(FileDialog::tr("All audio files"), allFileTypes.join(" ")));
	nameFilters.append(fileTypes);
	nameFilters.append(QString("%1 (*)").arg(FileDialog::tr("Other files")));

	openFileDialog.setNameFilters(nameFilters);

	if (!previousFile.isEmpty())
	{
		// select previously opened file
		openFileDialog.selectFile(QFileInfo{previousFile}.fileName());
	}

	if (openFileDialog.exec() == QDialog::Accepted)
	{
		if (openFileDialog.selectedFiles().isEmpty()) { return ""; }

		return PathUtil::toShortestRelative(openFileDialog.selectedFiles()[0]);
	}

	return "";
}

QString SampleFilePicker::openWaveformFile(const QString& previousFile)
{
	return openAudioFile(
		previousFile.isEmpty() ? ConfigManager::inst()->factorySamplesDir() + "waveforms/10saw.flac" : previousFile);
}

} // namespace lmms::gui