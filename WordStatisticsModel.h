#ifndef WORDSTATISTICSMODEL_H
#define WORDSTATISTICSMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#include <QPair>

class WordStatisticsModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit WordStatisticsModel(QObject *parent = nullptr);

    // 重写QAbstractTableModel的虚函数
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // 更新数据
    void updateData();
    int getTotalQueries() const { return m_totalQueries; }

private:
    QList<QPair<QString, int>> m_topWords;  // 单词和次数的对
    QStringList m_headers = {"排名", "单词", "查询次数", "占比"};
    int m_totalQueries = 0;  // 总查询次数
};

#endif // WORDSTATISTICSMODEL_H
