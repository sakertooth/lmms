/*
 * FileBrowserModel.cpp
 *
 * Copyright (c) 2026 saker <sakertooth@gmail.com>
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
#include <QFileInfo>
#include <QPixmap>

#include "embed.h"

namespace lmms {

FileBrowserModel::FileBrowserModel(const QStringList& rootPaths, RootPathsType rootPathsType, QObject* parent)
	: QAbstractItemModel(parent)
	, m_root(std::make_unique<Node>())
{
	switch (rootPathsType)
	{
	case RootPathsType::Directories:
		expand(m_root.get(), rootPaths);
		break;
	case RootPathsType::Items:
		insert(m_root.get(), rootPaths);
		break;
	}
}

void FileBrowserModel::expand(Node* node, const QStringList& paths)
{
	auto items = QStringList{};

	for (const auto& path : paths)
	{
		const auto dir = QDir{path};
		const auto entries = dir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot);

		for (const auto& entry : entries)
		{
			items.append(dir.absoluteFilePath(entry));
		}
	}

	insert(node, items);
}

void FileBrowserModel::insert(Node* node, const QStringList& paths)
{
	const auto firstRow = node->children.size();
	const auto lastRow = firstRow + paths.size() - 1;

	beginInsertRows(indexForNode(node), firstRow, lastRow);

	for (const auto& path : paths)
	{
		const auto type = determineType(path);
		if (type == Node::Type::Unknown) { continue; }

		const auto info = QFileInfo{path};

		auto child = std::make_unique<Node>();
		child->parent = node;
		child->name = info.fileName();
		child->path = path;
		child->type = type;
		child->row = node->children.size();

		node->children.emplace_back(std::move(child));
	}

	endInsertRows();
}

QModelIndex FileBrowserModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent)) { return {}; }

	const auto node = parent.isValid() ? static_cast<Node*>(parent.internalPointer()) : m_root.get();
	const auto child = static_cast<const void*>(node->children[row].get());

	// TODO (Qt 6): Remove use of const_cast
	return createIndex(row, column, const_cast<void*>(child));
}

QModelIndex FileBrowserModel::parent(const QModelIndex& child) const
{
	if (!child.isValid()) { return {}; }

	const auto childNode = static_cast<Node*>(child.internalPointer());
	if (childNode->parent == m_root.get()) { return {}; }

	const auto parentNode = childNode->parent;
	return createIndex(parentNode->row, 0, parentNode);
}

int FileBrowserModel::rowCount(const QModelIndex& parent) const
{
	if (!parent.isValid()) { return m_root->children.size(); }

	const auto node = static_cast<Node*>(parent.internalPointer());
	return node->children.size();
}

int FileBrowserModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent);
	return 1;
}

QVariant FileBrowserModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid()) { return {}; }

	const auto node = static_cast<Node*>(index.internalPointer());

	switch (role)
	{
	case Qt::DisplayRole:
		return node->name;
	case Qt::DecorationRole:
		return fetchPixmap(node);
	};

	return QVariant{};
}

QPixmap FileBrowserModel::fetchPixmap(Node* node)
{
	constexpr auto pixmapWidth = 16;
	constexpr auto pixmapHeight = 16;

	static auto s_projectFilePixmap = embed::getIconPixmap("project_file", pixmapWidth, pixmapHeight);
	static auto s_presetFilePixmap = embed::getIconPixmap("preset_file", pixmapWidth, pixmapHeight);
	static auto s_sampleFilePixmap = embed::getIconPixmap("sample_file", pixmapWidth, pixmapHeight);
	static auto s_soundfontFilePixmap = embed::getIconPixmap("soundfont_file", pixmapWidth, pixmapHeight);
	static auto s_vstPluginFilePixmap = embed::getIconPixmap("vst_plugin_file", pixmapWidth, pixmapHeight);
	static auto s_midiFilePixmap = embed::getIconPixmap("midi_file", pixmapWidth, pixmapHeight);
	static auto s_unknownFilePixmap = embed::getIconPixmap("unknown_file", pixmapWidth, pixmapHeight);
	static auto s_folderPixmap = embed::getIconPixmap("folder", pixmapWidth, pixmapHeight);

	switch (node->type)
	{
	case Node::Node::Type::Project:
		return s_projectFilePixmap;
	case Node::Type::Preset:
		return s_presetFilePixmap;
	case Node::Type::Sample:
	case Node::Type::Patch:
		return s_sampleFilePixmap;
	case Node::Type::SoundFont:
		return s_soundfontFilePixmap;
	case Node::Type::Midi:
		return s_midiFilePixmap;
	case Node::Type::VstPlugin:
		return s_vstPluginFilePixmap;
	case Node::Type::Directory:
		return s_folderPixmap;
	case Node::Type::Unknown:
		return s_unknownFilePixmap;
	};

	return QPixmap{};
}

auto FileBrowserModel::determineType(const QString& path) -> Node::Type
{
	const auto projectFilters = QStringList{".mmp", ".mpt", ".mmpz"};
	const auto presetFilters = QStringList{".xpf", ".xml", ".xiz", ".lv2"};
	const auto soundFontFilters = QStringList{".sf2", ".sf3"};
	const auto patchFilters = QStringList{".pat"};
	const auto midiFilters = QStringList{".mid", ".midi", ".rmi"};

	auto vstPluginFilters = QStringList{".dll"};
#ifdef LMMS_BUILD_LINUX
	vstPluginFilters.append(".so");
#endif

	auto audioFilters
		= QStringList{".wav", ".ogg", ".mp3", ".ds", ".flac", ".spx", ".voc", ".aif", ".aiff", ".au", ".raw"};

	const auto info = QFileInfo{path};
	const auto extension = info.completeSuffix();

	if (projectFilters.contains(extension, Qt::CaseInsensitive)) { return Node::Type::Project; }
	if (presetFilters.contains(extension, Qt::CaseInsensitive)) { return Node::Type::Preset; }
	if (soundFontFilters.contains(extension, Qt::CaseInsensitive)) { return Node::Type::SoundFont; }
	if (patchFilters.contains(extension, Qt::CaseInsensitive)) { return Node::Type::Patch; }
	if (midiFilters.contains(extension, Qt::CaseInsensitive)) { return Node::Type::Midi; }
	if (vstPluginFilters.contains(extension, Qt::CaseInsensitive)) { return Node::Type::VstPlugin; }
	if (audioFilters.contains(extension, Qt::CaseInsensitive)) { return Node::Type::Sample; }

	return Node::Type::Unknown;
}

auto FileBrowserModel::indexForNode(Node* node) -> QModelIndex
{
	if (node == m_root.get()) { return {}; }
	return index(node->row, 0, indexForNode(node->parent));
}

void FileBrowserModel::fetchMore(const QModelIndex& parent)
{
	if (!parent.isValid()) { return; }

	const auto node = static_cast<Node*>(parent.internalPointer());
	expand(node, QStringList{node->path});
}

auto FileBrowserModel::canFetchMore(const QModelIndex& parent) const -> bool
{
	if (!parent.isValid()) { return false; }

	const auto node = static_cast<Node*>(parent.internalPointer());
	return node->type == Node::Type::Directory && node->children.empty();
}

auto FileBrowserModel::hasChildren(const QModelIndex& parent) const -> bool
{
	if (!parent.isValid()) { return true; }

	const auto node = static_cast<Node*>(parent.internalPointer());
	return node->type == Node::Type::Directory;
}

} // namespace lmms