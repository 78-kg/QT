#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDateTime>

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

private:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlDatabase m_database;
    bool createTables();  // 创建数据表
};

#endif // DATABASEMANAGER_H
