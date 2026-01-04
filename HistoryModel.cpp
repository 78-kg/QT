#include "HistoryModel.h"
#include "DatabaseManager.h"
#include <QDebug>

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    updateData();
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_historyData.size();
}

int HistoryModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_headers.size();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_historyData.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (index.column() < m_historyData.at(index.row()).size()) {
            return m_historyData.at(index.row()).at(index.column());
        }
    }

    return QVariant();
}

QVariant HistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        if (section < m_headers.size())
            return m_headers.at(section);
    }

    return QVariant();
}

void HistoryModel::updateData()
{
    beginResetModel();
    m_historyData = DatabaseManager::instance().getHistory();
    endResetModel();
}

void HistoryModel::clearHistory()
{
    if (DatabaseManager::instance().clearHistory()) {
        beginResetModel();
        m_historyData.clear();
        endResetModel();
    }
}
