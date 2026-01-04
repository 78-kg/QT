#include "WordStatisticsModel.h"
#include "DatabaseManager.h"
#include <QDebug>
#include <QFont>
#include <QColor>

WordStatisticsModel::WordStatisticsModel(QObject *parent)
    : QAbstractTableModel(parent)
    , m_totalQueries(0)
{
    updateData();
}

int WordStatisticsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_topWords.size();
}

int WordStatisticsModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_headers.size();
}

QVariant WordStatisticsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_topWords.size())
        return QVariant();

    if (role == Qt::DisplayRole) {
        const auto& wordPair = m_topWords.at(index.row());

        switch (index.column()) {
        case 0:  // 排名
            return QString("%1").arg(index.row() + 1);
        case 1:  // 单词
            return wordPair.first;
        case 2:  // 查询次数
            return wordPair.second;
        case 3:  // 占比
            if (m_totalQueries > 0) {
                double percentage = (wordPair.second * 100.0) / m_totalQueries;
                return QString("%1%").arg(QString::number(percentage, 'f', 1));
            }
            return "0%";
        default:
            return QVariant();
        }
    }

    // 设置前三名的背景色
    if (role == Qt::BackgroundRole) {
        if (index.row() == 0) return QColor(255, 223, 186);  // 第一名：金色
        if (index.row() == 1) return QColor(230, 230, 250);  // 第二名：银色
        if (index.row() == 2) return QColor(255, 218, 185);  // 第三名：铜色
    }

    // 设置前三名的字体加粗
    if (role == Qt::FontRole && index.row() < 3) {
        QFont font;
        font.setBold(true);
        return font;
    }

    // 居中对齐
    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    return QVariant();
}

QVariant WordStatisticsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        if (section < m_headers.size())
            return m_headers.at(section);
    }

    return QVariant();
}

void WordStatisticsModel::updateData()
{
    beginResetModel();
    m_topWords = DatabaseManager::instance().getTopWords(10);

    // 计算总查询次数
    m_totalQueries = 0;
    for (const auto& pair : m_topWords) {
        m_totalQueries += pair.second;
    }

    endResetModel();

    qDebug() << "常用词汇统计更新，共" << m_topWords.size() << "个单词";
    qDebug() << "总查询次数:" << m_totalQueries;
}
