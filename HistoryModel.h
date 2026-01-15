#ifndef HISTORYMODEL_H
#define HISTORYMODEL_H

#include <QAbstractTableModel>
#include <QStringList>

class HistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit HistoryModel(QObject *parent = nullptr);

    // 重写QAbstractTableModel的虚函数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 更新数据
    void updateData();
    void clearHistory();

    //搜索功能
    void searchHistory(const QString& keyword);
    void clearSearch();

private:
    QList<QStringList> m_historyData;      // 所有历史数据
    QList<QStringList> m_filteredData;     // 过滤后的数据（用于搜索）
    QStringList m_headers = {"时间", "查询内容", "翻译结果"};
    QString m_searchKeyword;               // 当前搜索关键词
    bool m_isSearching = false;            // 是否处于搜索状态
};

#endif // HISTORYMODEL_H
