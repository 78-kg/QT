#include "DatabaseManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QSqlError>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{
}

DatabaseManager::~DatabaseManager()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool DatabaseManager::initDatabase()
{
    // 设置数据库路径
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString dbPath = dataPath + "/dictionary.db";

    qDebug() << "Database path:" << dbPath;

    // 连接数据库
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        qDebug() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }

    return createTables();
}


bool DatabaseManager::addHistory(const QString& query, const QString& translation,
                                 const QString& sourceLang, const QString& targetLang)
{
    QSqlQuery sqlQuery;
    sqlQuery.prepare("INSERT INTO history (query_text, translation, source_lang, target_lang) "
                     "VALUES (?, ?, ?, ?)");
    sqlQuery.addBindValue(query);
    sqlQuery.addBindValue(translation);
    sqlQuery.addBindValue(sourceLang);
    sqlQuery.addBindValue(targetLang);

    if (!sqlQuery.exec()) {
        qDebug() << "Failed to add history:" << sqlQuery.lastError().text();
        return false;
    }
    return true;
}

QList<QStringList> DatabaseManager::getHistory(int limit)
{
    QList<QStringList> historyList;

    QSqlQuery query;
    query.prepare("SELECT query_time, query_text, translation FROM history "
                  "ORDER BY query_time DESC LIMIT ?");
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            QStringList record;
            record << query.value(0).toString();  // 时间
            record << query.value(1).toString();  // 查询词
            record << query.value(2).toString();  // 翻译
            historyList.append(record);
        }
    } else {
        qDebug() << "Failed to get history:" << query.lastError().text();
    }

    return historyList;
}

bool DatabaseManager::clearHistory()
{
    QSqlQuery query("DELETE FROM history");
    if (!query.exec()) {
        qDebug() << "Failed to clear history:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::addFavorite(const QString& word, const QString& translation,
                                  const QString& notes)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO favorites (word, translation, notes) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(word);
    query.addBindValue(translation);
    query.addBindValue(notes);

    if (!query.exec()) {
        qDebug() << "Failed to add favorite:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::removeFavorite(const QString& word)
{
    QSqlQuery query;
    query.prepare("DELETE FROM favorites WHERE word = ?");
    query.addBindValue(word);

    if (!query.exec()) {
        qDebug() << "Failed to remove favorite:" << query.lastError().text();
        return false;
    }
    return query.numRowsAffected() > 0;
}

QList<QStringList> DatabaseManager::getFavorites()
{
    QList<QStringList> favoritesList;

    QSqlQuery query("SELECT word, translation, add_time, notes FROM favorites ORDER BY add_time DESC");

    if (query.exec()) {
        while (query.next()) {
            QStringList record;
            record << query.value(0).toString();  // 单词
            record << query.value(1).toString();  // 翻译
            record << query.value(2).toString();  // 收藏时间
            record << query.value(3).toString();  // 备注
            favoritesList.append(record);
        }
    } else {
        qDebug() << "Failed to get favorites:" << query.lastError().text();
    }

    return favoritesList;
}

bool DatabaseManager::isFavorite(const QString& word)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM favorites WHERE word = ?");
    query.addBindValue(word);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}


bool DatabaseManager::createTables()
{
    QSqlQuery query;

    // 1. 创建历史记录表
    QString createHistoryTable =
        "CREATE TABLE IF NOT EXISTS history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "query_text TEXT NOT NULL, "
        "translation TEXT NOT NULL, "
        "source_lang TEXT, "
        "target_lang TEXT, "
        "query_time DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")";

    if (!query.exec(createHistoryTable)) {
        qDebug() << "Failed to create history table:" << query.lastError().text();
        return false;
    }

    // 2. 创建收藏表
    QString createFavoriteTable =
        "CREATE TABLE IF NOT EXISTS favorites ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "word TEXT NOT NULL UNIQUE, "
        "translation TEXT NOT NULL, "
        "add_time DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "notes TEXT"
        ")";

    if (!query.exec(createFavoriteTable)) {
        qDebug() << "Failed to create favorites table:" << query.lastError().text();
        return false;
    }

    // 3. 新增：创建单词统计表
    return createStatisticsTable();
}

// 新增：创建单词统计表
bool DatabaseManager::createStatisticsTable()
{
    QSqlQuery query;

    QString createStatisticsTable =
        "CREATE TABLE IF NOT EXISTS word_statistics ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "word TEXT NOT NULL UNIQUE, "
        "count INTEGER DEFAULT 1, "
        "last_query_time DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")";

    if (!query.exec(createStatisticsTable)) {
        qDebug() << "Failed to create statistics table:" << query.lastError().text();
        return false;
    }

    return true;
}

// 新增：增加单词查询次数
void DatabaseManager::incrementWordCount(const QString& word)
{
    if (word.trimmed().isEmpty()) {
        return;
    }

    QString cleanWord = word.trimmed().toLower();  // 转换为小写，统一统计

    QSqlQuery query;

    // 使用INSERT OR REPLACE来更新或插入
    query.prepare(
        "INSERT OR REPLACE INTO word_statistics (word, count, last_query_time) "
        "VALUES (?, "
        "COALESCE((SELECT count FROM word_statistics WHERE word = ?), 0) + 1, "
        "CURRENT_TIMESTAMP)"
        );
    query.addBindValue(cleanWord);
    query.addBindValue(cleanWord);

    if (!query.exec()) {
        qDebug() << "Failed to update word count:" << query.lastError().text();
    } else {
        qDebug() << "增加单词统计:" << cleanWord;
    }
}

// 新增：获取单词查询次数
int DatabaseManager::getWordCount(const QString& word)
{
    QString cleanWord = word.trimmed().toLower();

    QSqlQuery query;
    query.prepare("SELECT count FROM word_statistics WHERE word = ?");
    query.addBindValue(cleanWord);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

// 新增：获取常用词排名
QList<QPair<QString, int>> DatabaseManager::getTopWords(int limit)
{
    QList<QPair<QString, int>> topWords;

    QSqlQuery query;
    query.prepare(
        "SELECT word, count FROM word_statistics "
        "ORDER BY count DESC, last_query_time DESC "
        "LIMIT ?"
        );
    query.addBindValue(limit);

    if (query.exec()) {
        while (query.next()) {
            QString word = query.value(0).toString();
            int count = query.value(1).toInt();
            topWords.append(qMakePair(word, count));
        }
    } else {
        qDebug() << "Failed to get top words:" << query.lastError().text();
    }

    return topWords;
}

// 新增：清空单词统计
bool DatabaseManager::clearWordStatistics()
{
    QSqlQuery query("DELETE FROM word_statistics");
    if (query.exec()) {
        qDebug() << "单词统计已清空";
        return true;
    } else {
        qDebug() << "Failed to clear word statistics:" << query.lastError().text();
        return false;
    }
}
