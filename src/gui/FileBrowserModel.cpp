/*
 * FileBrowserModel.h - Qt model for FileBrowser
 *
 * Copyright (c) 2023 saker <sakertooth@gmail.com>
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

#include "FileBrowserModel.h"

#include <QDir>

#include "PluginFactory.h"
#include "embed.h"

namespace lmms::gui {
FileBrowserModel::FileBrowserModel(const QStringList& directories)
	: m_directories(directories)
{
}

auto FileBrowserModel::index(int row, int column, const QModelIndex& parent) const -> QModelIndex
{
	if (!hasIndex(row, column, parent)) { return QModelIndex{}; }
	const auto parentNode = parent.isValid() ? static_cast<Node*>(parent.internalPointer()) : &m_rootNode;
	const auto childNode = (*parentNode)[row];
	return childNode ? createIndex(row, column, childNode) : QModelIndex{};
}

auto FileBrowserModel::parent(const QModelIndex& index) const -> QModelIndex
{
	if (!index.isValid()) { return QModelIndex{}; }

	auto childNode = static_cast<Node*>(index.internalPointer());
	auto parentNode = childNode->parent;

	if (childNode->parent == &m_rootNode) { return QModelIndex{}; }
	return createIndex(parentNode->row(), 0, parentNode);
}

auto FileBrowserModel::rowCount(const QModelIndex& parent) const -> int
{
	const auto parentNode = parent.isValid() ? static_cast<Node*>(parent.internalPointer()) : &m_rootNode;
	return parentNode->children.size();
}

auto FileBrowserModel::columnCount(const QModelIndex& parent) const -> int
{
	return 1;
}

auto FileBrowserModel::data(const QModelIndex& index, int role) const -> QVariant
{
	if (!index.isValid()) { return QVariant{}; }

	const auto childNode = static_cast<Node*>(index.internalPointer());
	switch (role)
	{
	case Qt::DisplayRole:
	case static_cast<int>(Role::NameRole):
		return childNode->name;
	case static_cast<int>(Role::PathRole):
		return childNode->path;
	case Qt::DecorationRole:
		return childNode->pixmap();
	default:
		return QVariant{};
	}
}

auto FileBrowserModel::canFetchMore(const QModelIndex& parent) const -> bool
{
	const auto paths
		= parent.isValid() ? QStringList{static_cast<Node*>(parent.internalPointer())->path} : m_directories;

	auto dir = QDir{};
	auto numEntries = 0;
	for (const auto& path : m_directories)
	{
		dir.setPath(path);
		numEntries += dir.entryList().size();
	}

	return rowCount(parent) < numEntries;
}

auto FileBrowserModel::fetchMore(const QModelIndex& parent) -> void
{
	const auto paths
		= parent.isValid() ? QStringList{static_cast<Node*>(parent.internalPointer())->path} : m_directories;

	beginResetModel();

	auto dir = QDir{};
	for (const auto& path : m_directories)
	{
		dir.setPath(path);
		insertPaths(dir.entryList(dirFilters(), sortFlags()));
	}

	endResetModel();
}

auto FileBrowserModel::insertPath(const QString& path) -> void
{
	auto relativePath = path;
	auto currentDir = QDir{};
	for (const auto& basePath : m_directories)
	{
		if (relativePath.startsWith(basePath))
		{
			relativePath.remove(basePath);
			currentDir.setPath(basePath);
			break;
		}
	}

	if (relativePath == path) { return; }

	const auto parts = QDir::cleanPath(relativePath).split('/');
	auto currentNode = &m_rootNode;
	for (const auto& part : parts)
	{
		currentDir.cd(part);

		auto childNode = std::find_if(currentNode->children.begin(), currentNode->children.end(),
			[&part](const Node& node) { return node.name == part; });

		if (childNode == currentNode->children.end())
		{
			currentNode->children.emplace_back(part, currentDir.dirName(), currentNode);
			currentNode = &currentNode->children.back();
			continue;
		}

		currentNode = &*childNode;
	}
}

auto FileBrowserModel::insertPaths(const QStringList& paths) -> void
{
	for (const auto& path : paths)
	{
		insertPath(path);
	}
}

auto FileBrowserModel::Node::type() -> Type
{
	const auto info = QFileInfo{path};
	if (info.isDir()) { return info.isReadable() ? Type::Directory : Type::UnreadableDirectory; }
	else if (info.isFile())
	{
		const auto extension = info.suffix();
		if (extension == "mmp" || extension == "mpt" || extension == "mmpz") { return Type::ProjectFile; }
		else if (extension == "xpf" || extension == "xml" || extension == "lv2") { return Type::PresetFile; }
		else if (extension == "sf2" || extension == "sf3") { return Type::SoundFontFile; }
		else if (extension == "pat") { return Type::PatchFile; }
		else if (extension == "mid" || extension == "midi") { return Type::MidiFile; }
		else if (extension == "xiz" && getPluginFactory()->pluginSupportingExtension(extension).isNull())
		{
			return Type::PresetFile;
		}
		else if (extension == "dll"
#ifdef LMMS_BUILD_LINUX
			|| extension == "so"
#endif
		)
		{
			return Type::VstPluginFile;
		}
		else if (auto audioExtensions = audioFilters().remove("*.").split(' '); audioExtensions.contains(extension))
		{
			return Type::SampleFile;
		}
	}

	return Type::Unknown;
}

auto FileBrowserModel::Node::pixmap() -> QPixmap
{
	switch (type())
	{
	case Type::Directory:
		return embed::getIconPixmap("folder");
	case Type::UnreadableDirectory:
		return embed::getIconPixmap("folder_locked");
	case Type::ProjectFile:
		return embed::getIconPixmap("project_file", 16, 16);
	case Type::PresetFile:
		return embed::getIconPixmap("preset_file", 16, 16);
	case Type::SampleFile:
		return embed::getIconPixmap("sample_file", 16, 16);
	case Type::SoundFontFile:
		return embed::getIconPixmap("soundfont_file", 16, 16);
	case Type::VstPluginFile:
		return embed::getIconPixmap("vst_plugin_file", 16, 16);
	case Type::MidiFile:
		return embed::getIconPixmap("midi_file", 16, 16);
	case Type::Unknown:
		return embed::getIconPixmap("unknown_file");
	default:
		return QPixmap{};
	}
	return QPixmap{};
}

auto FileBrowserModel::audioFilters() -> QString
{
	auto filters
		= QStringList{"*.wav", "*.ogg", "*.ds", "*.flac", "*.spx", "*.voc", "*.aif", "*.aiff", "*.au", "*.raw"};
#ifdef LMMS_HAVE_SNDFILE_MP3
	filters.append("*.mp3");
#endif
	return filters.join(' ');
}

} // namespace lmms::gui
