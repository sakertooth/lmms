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

#ifndef LMMS_FILE_BROWSER_MODEL_H
#define LMMS_FILE_BROWSER_MODEL_H

#include <QAbstractItemModel>
#include <QDir>

namespace lmms::gui {
class FileBrowserModel : public QAbstractItemModel
{
public:
	enum class Role
	{
		PathRole = Qt::UserRole + 1,
		NameRole = Qt::UserRole + 2
	};

	struct Node
	{
		enum class Type
		{
			Directory,
			UnreadableDirectory,
			ProjectFile,
			PresetFile,
			SampleFile,
			SoundFontFile,
			PatchFile,
			VstPluginFile,
			MidiFile,
			Unknown
		};

		Node() = default;

		Node(const QString& name, const QString& path, Node* parent = nullptr)
			: name(name)
			, path(path)
			, parent(parent)
		{
		}

		Node* operator[](size_t index)
		{
			if (index < 0 || index >= children.size()) { return nullptr; }
			return &children[index];
		}

		auto row() const -> int
		{
			if (!parent) { return -1; }
			return this - parent->children.data();
		}

		auto type() -> Type;
		auto pixmap() -> QPixmap;

		QString name;
		QString path;
		Node* parent = nullptr;
		std::vector<Node> children;
	};

	FileBrowserModel(const QStringList& directories);

	auto index(int row, int column, const QModelIndex& parent = QModelIndex()) const -> QModelIndex override;
	auto parent(const QModelIndex& index) const -> QModelIndex override;
	auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
	auto columnCount(const QModelIndex& parent = QModelIndex()) const -> int override;
	auto data(const QModelIndex& index, int role = Qt::DisplayRole) const -> QVariant override;
	auto canFetchMore(const QModelIndex& parent) const -> bool override;
	auto fetchMore(const QModelIndex& parent) -> void override;

	static QString audioFilters();
	static QDir::Filters dirFilters() { return QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot; }
	static QDir::SortFlags sortFlags() { return QDir::LocaleAware | QDir::DirsFirst | QDir::Name | QDir::IgnoreCase; }

private:
	auto insertPath(const QString& path) -> void;
	auto insertPaths(const QStringList& paths) -> void;
	QStringList m_directories;
	mutable Node m_rootNode;
};
} // namespace lmms::gui

#endif // LMMS_FILE_BROWSER_MODEL_H
