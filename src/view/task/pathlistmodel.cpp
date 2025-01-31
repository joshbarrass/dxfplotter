#include <pathlistmodel.h>

#include <QIcon>

namespace view::task
{

PathListModel::PathListModel(model::Task &task, QObject *parent)
	:QAbstractListModel(parent),
	m_task(task)
{
}

QVariant PathListModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	const model::Path &path = m_task.pathAt(index.row());

	switch (role) {
		case Qt::DisplayRole:
		case Qt::ToolTipRole:
		{
			switch (index.column()) {
				case 0:
				{
					return QString::fromStdString(path.name());
					break;
				}
			}
			break;
		}
		case Qt::DecorationRole:
		{
			switch (index.column()) {
				case 1:
				{
					if (path.globallyVisible()) {
						return QIcon::fromTheme("object-visible");
					}
					else {
						return QIcon::fromTheme("object-hidden");
					}
					break;
				}
			}
			break;
		}
	}

	return QVariant();
}

int PathListModel::rowCount([[maybe_unused]] const QModelIndex& parent) const
{
	return m_task.pathCount();
}

int PathListModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
{
	return 2;
}

Qt::ItemFlags PathListModel::flags(const QModelIndex &index) const
{
	if (index.column() == 0) {
		return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	}

	const model::Path &path = m_task.pathAt(index.row());
	return (path.layer().visible()) ? Qt::ItemIsEnabled : Qt::NoItemFlags;
}

QModelIndex PathListModel::movePath(const QModelIndex &index, model::Task::MoveDirection direction)
{
	const int row = index.row();
	const int newRow = row + direction;

	if (index.isValid() && 0 <= newRow && newRow < rowCount(index)) {
		// Mistic qt move indexing
		const int newQtRow = (newRow > row) ? newRow + 1 : newRow;

		if (beginMoveRows(index, row, row, index, newQtRow)) {

			m_task.movePath(row, direction);

			endMoveRows();

			return this->index(newRow);
		}
	}

	return index;
}

void PathListModel::itemClicked(const QModelIndex& index)
{
	if ((index.flags() & Qt::ItemIsEnabled) == 0) {
		return;
	}

	switch (index.column()) {
		case 1:
		{
			model::Path &path = m_task.pathAt(index.row());
			path.toggleVisible();
			
			emit dataChanged(index, index);
		}
	}
}

void PathListModel::updateItemSelection(const model::Path &path, QItemSelectionModel::SelectionFlag flag, QItemSelectionModel *selectionModel)
{
	const int row = m_task.pathIndexFor(path);
	selectionModel->select(index(row, 0), flag);
}

void PathListModel::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	for (const QModelIndex &index : selected.indexes()) {
		model::Path &path = m_task.pathAt(index.row());
		path.setSelected(true);
	}

	for (const QModelIndex &index : deselected.indexes()) {
		model::Path &path = m_task.pathAt(index.row());
		path.setSelected(false);
	}
}

}
