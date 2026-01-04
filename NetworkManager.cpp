#include "NetworkManager.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QCryptographicHash>
#include <QRandomGenerator>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, [this](QNetworkReply* reply) {
                // 这里处理通用网络错误
                if (reply->error() != QNetworkReply::NoError) {
                    emit networkError(reply->errorString());
                }
            });

    initKeys();
}

void NetworkManager::initKeys()
{
    // 这里初始化API密钥（实际使用时从配置文件读取）
    // 百度翻译API（需要自己去百度翻译开放平台注册）
    m_baiduAppId = "";         // 你的百度App ID
    m_baiduSecretKey = "";     // 你的百度密钥

    // 有道翻译API（需要去有道智云注册）
    m_youdaoAppKey = "";       // 你的有道App Key
    m_youdaoAppSecret = "";    // 你的有道App Secret

    qDebug() << "NetworkManager initialized. API keys loaded.";
}

void NetworkManager::translateBaidu(const QString& text, const QString& from, const QString& to)
{
    if (m_baiduAppId.isEmpty() || m_baiduSecretKey.isEmpty()) {
        emit translationFinished("请配置百度翻译API密钥", false, "API密钥未配置");
        return;
    }

    qint64 salt = QRandomGenerator::global()->generate();
    QString sign = generateBaiduSign(text, salt);

    QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", from);
    query.addQueryItem("to", to);
    query.addQueryItem("appid", m_baiduAppId);
    query.addQueryItem("salt", QString::number(salt));
    query.addQueryItem("sign", sign);

    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onBaiduReplyFinished(reply);
    });
}

void NetworkManager::onBaiduReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit translationFinished("", false, "网络错误: " + reply->errorString());
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        emit translationFinished("", false, "API返回格式错误");
        return;
    }

    QJsonObject obj = doc.object();

    if (obj.contains("error_code")) {
        int errorCode = obj["error_code"].toInt();
        QString errorMsg = obj["error_msg"].toString();
        emit translationFinished("", false, QString("API错误 %1: %2").arg(errorCode).arg(errorMsg));
        return;
    }

    if (!obj.contains("trans_result")) {
        emit translationFinished("", false, "API返回数据格式错误");
        return;
    }

    QJsonArray transResult = obj["trans_result"].toArray();
    QString result;

    for (const auto& item : transResult) {
        QJsonObject transObj = item.toObject();
        result += transObj["dst"].toString() + "\n";
    }

    if (!result.isEmpty()) {
        emit translationFinished(result.trimmed(), true);
    } else {
        emit translationFinished("", false, "翻译结果为空");
    }
}

QString NetworkManager::generateBaiduSign(const QString& query, qint64 salt)
{
    QString signStr = m_baiduAppId + query + QString::number(salt) + m_baiduSecretKey;
    QByteArray hash = QCryptographicHash::hash(signStr.toUtf8(), QCryptographicHash::Md5);
    return QString(hash.toHex());
}

// 模拟翻译（用于测试，不依赖网络）
QString NetworkManager::translateMock(const QString& text, const QString& from, const QString& to)
{
    QString result = QString("[模拟翻译] %1 -> %2: \n").arg(from).arg(to);

    // 简单的模拟翻译逻辑
    if (from == "en" && to == "zh") {
        if (text.toLower() == "hello") return "你好";
        if (text.toLower() == "world") return "世界";
        if (text.toLower() == "you") return "你";
        if (text.toLower() == "can") return "能";
        return QString("这是'%1'的中文翻译").arg(text);
    } else if (from == "zh" && to == "en") {
        if (text.contains("你好")) return "Hello";
        if (text.contains("世界")) return "World";
        return QString("This is English translation of '%1'").arg(text);
    } else {
        return QString("Translation from %1 to %2: %3").arg(from).arg(to).arg(text);
    }
}
