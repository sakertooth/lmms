/*
 * FileBrowserModel.h
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

#ifndef LMMS_FILE_BROWSER_MODEL_H
#define LMMS_FILE_BROWSER_MODEL_H

#include <QAbstractItemModel>

namespace lmms {

class FileBrowserModel : public QAbstractItemModel
{
	struct Node;

public:
	enum class RootPathsType
	{
		Directories, //< Treat paths as browsable roots
		Items		 //< Treat paths as direct child items
	};

	FileBrowserModel(const QStringList& rootPaths, RootPathsType rootPathsType, QObject* parent = nullptr);

	/**
	 * @brief Expands @a paths and adds the new entries as children of @a node.
	 *
	 * @param node
	 * @param paths
	 */
	void expand(Node* node, const QStringList& paths);

	/**
	 * @brief Insert the files (and folders) from the given @a paths as children of @a node.
	 * 
	 * @param node 
	 * @param paths 
	 */
	void insert(Node* node, const QStringList& paths);

	auto index(int row, int column, const QModelIndex& parent = QModelIndex()) const -> QModelIndex override;
	auto parent(const QModelIndex& child) const -> QModelIndex override;
	auto rowCount(const QModelIndex& parent = QModelIndex()) const -> int override;
	auto columnCount(const QModelIndex& parent = QModelIndex()) const -> int override;
	auto data(const QModelIndex& index, int role = Qt::DisplayRole) const -> QVariant override;

	void fetchMore(const QModelIndex& parent) override;
	auto canFetchMore(const QModelIndex& parent) const -> bool override;

	auto hasChildren(const QModelIndex& parent = QModelIndex()) const -> bool override;

private:
	struct Node
	{
		enum class Type
		{
			Project,
			Preset,
			Sample,
			SoundFont,
			Patch,
			Midi,
			VstPlugin,
			Directory,
			Unknown
		};

		Node* parent = nullptr;
		QString name;
		QString path;
		Type type = Type::Unknown;
		int row = 0;
		std::vector<std::unique_ptr<Node>> children;
	};

	auto indexForNode(Node* node) -> QModelIndex;
	static auto fetchPixmap(Node* node) -> QPixmap;
	static auto determineType(const QString& path) -> Node::Type;
	std::unique_ptr<Node> m_root;
};
} // namespace lmms

#endif // LMMS_FILE_BROWSER_MODEL_H