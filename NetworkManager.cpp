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
    // 填入你的真实API密钥
    m_baiduAppId = "20260104002532859";  // 你的百度App ID
    m_baiduSecretKey = "kFnYilHeW0sp6p9Lcovj";  // 你的百度密钥
    qDebug() << "API密钥已加载，启用百度翻译";
}

void NetworkManager::translateBaidu(const QString& text, const QString& from, const QString& to)
{
    qDebug() << "=== 开始百度翻译调用 ===";
    qDebug() << "查询文本:" << text;
    qDebug() << "源语言:" << from;
    qDebug() << "目标语言:" << to;
    qDebug() << "App ID:" << m_baiduAppId;


    if (from == "zh" && to == "zh") {
        qDebug() << "警告：源语言和目标语言相同！";
    }


    if (m_baiduAppId.isEmpty() || m_baiduSecretKey.isEmpty()) {
        qDebug() << "错误：API密钥为空，使用模拟翻译";
        QString mockResult = translateMock(text, from, to);
        emit translationFinished(mockResult, true, "模拟翻译模式");
        return;
    }

    qint64 salt = QRandomGenerator::global()->generate();
    QString sign = generateBaiduSign(text, salt);

    qDebug() << "生成salt:" << salt;
    qDebug() << "生成sign:" << sign;

    QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");
    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", from);
    query.addQueryItem("to", to);
    query.addQueryItem("appid", m_baiduAppId);
    query.addQueryItem("salt", QString::number(salt));
    query.addQueryItem("sign", sign);

    url.setQuery(query);

    qDebug() << "请求URL:" << url.toString(QUrl::RemoveUserInfo);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onBaiduReplyFinished(reply);
    });

    qDebug() << "网络请求已发送";
}

void NetworkManager::onBaiduReplyFinished(QNetworkReply* reply)
{
    qDebug() << "=== 收到百度API响应 ===";

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "网络错误:" << reply->errorString();
        emit translationFinished("", false, "网络错误: " + reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QString responseStr = QString::fromUtf8(data);

    qDebug() << "API响应数据:" << responseStr;
    qDebug() << "响应状态码:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        qDebug() << "错误：API返回的不是有效的JSON对象";
        emit translationFinished("", false, "API返回格式错误");
        reply->deleteLater();
        return;
    }

    QJsonObject obj = doc.object();

    // 检查错误码
    if (obj.contains("error_code")) {
        int errorCode = obj["error_code"].toInt();
        QString errorMsg = obj["error_msg"].toString();
        qDebug() << "API返回错误码:" << errorCode << "错误信息:" << errorMsg;
        emit translationFinished("", false, QString("API错误 %1: %2").arg(errorCode).arg(errorMsg));
        reply->deleteLater();
        return;
    }

    if (!obj.contains("trans_result")) {
        qDebug() << "错误：API响应缺少trans_result字段";
        qDebug() << "完整响应:" << obj;
        emit translationFinished("", false, "API返回数据格式错误");
        reply->deleteLater();
        return;
    }

    QJsonArray transResult = obj["trans_result"].toArray();
    QString result;

    for (const auto& item : transResult) {
        QJsonObject transObj = item.toObject();
        QString src = transObj["src"].toString();
        QString dst = transObj["dst"].toString();
        qDebug() << "原文:" << src << "-> 译文:" << dst;
        result += dst + "\n";
    }

    if (!result.isEmpty()) {
        qDebug() << "翻译成功，结果:" << result.trimmed();
        emit translationFinished(result.trimmed(), true);
    } else {
        qDebug() << "错误：翻译结果为空";
        emit translationFinished("", false, "翻译结果为空");
    }

    reply->deleteLater();
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

