#include "HistoryModel.h"
#include "DatabaseManager.h"
#include <QDebug>

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_isSearching(false)
{
    updateData();
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // 根据搜索状态返回对应的数据行数
    if (m_isSearching) {
        return m_filteredData.size();
    } else {
        return m_historyData.size();
    }
}

int HistoryModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_headers.size();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // 根据搜索状态选择数据源
    const QList<QStringList>& currentData = m_isSearching ? m_filteredData : m_historyData;

    if (index.row() >= currentData.size())
        return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        if (index.column() < currentData.at(index.row()).size()) {
            return currentData.at(index.row()).at(index.column());
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
    m_isSearching = false;
    m_searchKeyword.clear();
    m_filteredData.clear();
    endResetModel();
}

void HistoryModel::clearHistory()
{
    if (DatabaseManager::instance().clearHistory()) {
        beginResetModel();
        m_historyData.clear();
        m_filteredData.clear();
        m_isSearching = false;
        m_searchKeyword.clear();
        endResetModel();
    }
}

// 新增：搜索历史记录
void HistoryModel::searchHistory(const QString& keyword)
{
    beginResetModel();

    m_searchKeyword = keyword.trimmed();

    if (m_searchKeyword.isEmpty()) {
        // 如果关键词为空，清除搜索状态
        m_isSearching = false;
        m_filteredData.clear();
    } else {
        m_isSearching = true;
        m_filteredData.clear();

        // 遍历所有历史记录，查找包含关键词的记录
        for (const QStringList& record : m_historyData) {
            if (record.size() >= 3) {  // 确保有足够的数据
                QString queryWord = record[1];  // 查询内容
                QString translation = record[2];  // 翻译结果

                // 不区分大小写搜索
                if (queryWord.contains(m_searchKeyword, Qt::CaseInsensitive) ||
                    translation.contains(m_searchKeyword, Qt::CaseInsensitive)) {
                    m_filteredData.append(record);
                }
            }
        }

        qDebug() << "搜索关键词:" << m_searchKeyword
                 << "，找到" << m_filteredData.size() << "条记录";
    }

    endResetModel();
}

// 新增：清除搜索状态
void HistoryModel::clearSearch()
{
    beginResetModel();
    m_isSearching = false;
    m_searchKeyword.clear();
    m_filteredData.clear();
    endResetModel();
}
