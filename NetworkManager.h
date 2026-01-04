#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);

    // 百度翻译API
    void translateBaidu(const QString& text, const QString& from, const QString& to);

    // 有道翻译API
    void translateYoudao(const QString& text, const QString& from, const QString& to);

    // 模拟翻译（无网络时使用）
    QString translateMock(const QString& text, const QString& from, const QString& to);

signals:
    void translationFinished(const QString& result, bool success, const QString& error = "");
    void networkError(const QString& error);

private slots:
    void onBaiduReplyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_baiduAppId;      // 百度API ID
    QString m_baiduSecretKey;  // 百度密钥
    QString m_youdaoAppKey;    // 有道App Key
    QString m_youdaoAppSecret; // 有道App Secret

    void initKeys();  // 初始化API密钥
    QString generateBaiduSign(const QString& query, qint64 salt);
    QString generateYoudaoSign(const QString& query, qint64 salt);
};

#endif // NETWORKMANAGER_H
