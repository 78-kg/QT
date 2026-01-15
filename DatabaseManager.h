#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>
#include <QPair>
#include <QStringList>

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    static DatabaseManager& instance();  // 单例模式
    bool initDatabase();  // 初始化数据库

    // 历史记录操作
    bool addHistory(const QString& query, const QString& translation,
                    const QString& sourceLang, const QString& targetLang);
    QList<QStringList> getHistory(int limit = 50);  // 获取历史记录
    bool clearHistory();  // 清空历史记录

    // 收藏夹操作
    bool addFavorite(const QString& word, const QString& translation,
                     const QString& notes = "");
    bool removeFavorite(const QString& word);
    QList<QStringList> getFavorites();
    bool isFavorite(const QString& word);

    // 新增：单词统计功能
    void incrementWordCount(const QString& word);  // 增加单词查询次数
    int getWordCount(const QString& word);         // 获取单词查询次数
    QList<QPair<QString, int>> getTopWords(int limit = 10);  // 获取常用词排名
    bool clearWordStatistics();  // 清空单词统计

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_database;
    bool createTables();  // 创建数据表
    bool createStatisticsTable();  // 新增：创建统计表
};

#endif // DATABASEMANAGER_H
