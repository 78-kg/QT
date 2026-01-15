#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QTimer>
#include <QTextCursor>
#include <QShortcut>
#include <QStandardPaths>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DatabaseManager.h"
#include <QTimer>
#include <QDebug>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_historyModel(new HistoryModel(this))
    , m_networkManager(new NetworkManager(this))
    , m_wordStatsModel(new WordStatisticsModel(this))
{
    ui->setupUi(this);

    // 基本初始化
    this->setWindowTitle("智能词典工具 v2.0");
    initDatabase();

    ui->searchHistoryLineEdit->setPlaceholderText("搜索历史记录");

    connect(ui->queryLineEdit, &QLineEdit::returnPressed,
            this, &MainWindow::on_searchButton_clicked);

    // 设置历史表格
    ui->historyTableView->setModel(m_historyModel);
    ui->historyTableView->horizontalHeader()->setStretchLastSection(true);
    ui->historyTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 设置常用词汇表格
    if (m_wordStatsModel) {
        ui->wordStatisticsTableView->setModel(m_wordStatsModel);
        ui->wordStatisticsTableView->horizontalHeader()->setStretchLastSection(true);
        ui->wordStatisticsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->wordStatisticsTableView->setAlternatingRowColors(true);
    }

    // 连接网络管理器
    connect(m_networkManager, &NetworkManager::translationFinished,
            this, &MainWindow::onTranslationFinished);

    // 安全定时器
    QTimer* safetyTimer = new QTimer(this);
    connect(safetyTimer, &QTimer::timeout, this, [this]() {
        if (!ui->searchButton->isEnabled()) {
            ui->searchButton->setEnabled(true);
            ui->statusbar->showMessage("已自动恢复查询功能", 2000);
        }
    });
    safetyTimer->start(10000);

    // 设置初始文本
    ui->resultTextEdit->setPlaceholderText("翻译结果将显示在这里...");
    ui->statusbar->showMessage("就绪");
    ui->searchHistoryLineEdit->clear();

    // 设置焦点
    ui->queryLineEdit->setFocus();

    // 初始自动更新统计
    QTimer::singleShot(100, this, [this]() {
        if (m_wordStatsModel) {
            m_wordStatsModel->updateData();
        }
    });
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initDatabase()
{
    if (DatabaseManager::instance().initDatabase()) {
        ui->statusbar->showMessage("数据库初始化成功", 3000);
    } else {
        ui->statusbar->showMessage("数据库初始化失败", 3000);
    }
}

void MainWindow::on_searchButton_clicked()
{
    QString word = ui->queryLineEdit->text().trimmed();
    if (word.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入要查询的内容！");
        ui->queryLineEdit->setFocus();
        return;
    }

    m_currentQuery = word;
    m_currentExamples.clear();

    m_examplesRequested = true;
    m_examplesReceived = false;

    // 设置例句获取超时检查
    QTimer::singleShot(5000, this, [this]() {
        if (m_examplesRequested && !m_examplesReceived) {
            ui->statusbar->showMessage("例句获取超时，使用示例句子", 3000);



            updateExampleDisplay();
        }
    });

    int index = ui->langComboBox->currentIndex();
    QString fromLang, toLang;
    QString displayDirection;  // 用于在界面上显示的方向

    if (index == 0) {  // 英文->中文
        fromLang = "en";
        toLang = "zh";
        displayDirection = "英文->中文";
    } else if (index == 1) {  // 中文->英文
        fromLang = "zh";
        toLang = "en";
        displayDirection = "中文->英文";
    } else if (index == 2) {  // 自动检测（关键修改！）
        // 智能检测：根据输入内容判断语言
        if (isChineseText(word)) {
            fromLang = "zh";  // 中文->英文
            toLang = "en";
            displayDirection = "中文->英文 (自动检测)";
        } else {
            fromLang = "en";  // 英文->中文
            toLang = "zh";
            displayDirection = "英文->中文 (自动检测)";
        }
    } else {
        fromLang = "auto";
        toLang = "zh";
        displayDirection = "自动检测";
    }

    qDebug() << "翻译设置: from" << fromLang << "to" << toLang;

    // 4. 显示查询状态
    ui->resultTextEdit->setText(
        "🔍 正在查询: " + word + "\n"
                                 "🌐 方向: " + ui->langComboBox->currentText() + "\n"
                                            "⏰ 时间: " + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") + "\n\n"
                                                                         "📊 正在获取以下信息:\n"
                                                                         "  1. 翻译结果...\n"
                                                                         "  2. 真实例句...\n\n"
                                                                         "请稍候..."
        );

    ui->statusbar->showMessage("正在查询: " + word);
    ui->searchButton->setEnabled(false);  // 禁用按钮防止重复点击

    // 5. 更新单词统计
    DatabaseManager::instance().incrementWordCount(word);

    // 6. 同时发起两个网络请求（并行）

    // 请求1: 百度翻译API（获取翻译结果）
    m_networkManager->translateBaidu(word, fromLang, toLang);


    // 7. 设置查询超时保护
    QTimer::singleShot(10000, this, [this]() {
        if (!ui->searchButton->isEnabled()) {
            ui->searchButton->setEnabled(true);
            ui->statusbar->showMessage("查询超时，请重试", 3000);
        }
    });

    // 8. 记录查询日志
    qDebug() << "=== 开始查询 ===";
    qDebug() << "查询词:" << word;
    qDebug() << "源语言:" << fromLang;
    qDebug() << "目标语言:" << toLang;
    qDebug() << "时间:" << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
}

void MainWindow::onTranslationFinished(const QString& result, bool success, const QString& error)
{
    // 1. 重新启用查询按钮
    ui->searchButton->setEnabled(true);

    if (success) {
        // 2. 显示翻译结果
        QString langDirection = ui->langComboBox->currentText();
        QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

        // 构建基本结果
        QString resultText = QString("🔍 查询: %1\n"
                                     "🌐 方向: %2\n"
                                     "⏰ 时间: %3\n\n"
                                     "📖 翻译结果:\n"
                                     "%4\n\n")
                                 .arg(m_currentQuery)
                                 .arg(langDirection)
                                 .arg(currentTime)
                                 .arg(result);

        // 3. 直接使用翻译结果生成例句（不再等待例句API）
        m_currentExamples = generateExamplesFromTranslation(m_currentQuery, result);
        resultText += formatExampleSentences();

        ui->resultTextEdit->setText(resultText);

        // 4. 保存到历史记录
        if (DatabaseManager::instance().addHistory(m_currentQuery, result, "auto", langDirection)) {
            updateHistoryView();
        }

        // 5. 更新状态栏
        ui->statusbar->showMessage("翻译完成 ✓", 2000);

        // 6. 滚动到顶部
        ui->resultTextEdit->moveCursor(QTextCursor::Start);
    } else {
        // 错误处理
        showErrorMessage(error);
        ui->statusbar->showMessage("翻译失败: " + error, 5000);

        // 使用模拟翻译作为后备
        QString langDirection = ui->langComboBox->currentText();
        QString fromLang = (langDirection == "中文 -> 英文") ? "zh" : "auto";
        QString toLang = (langDirection == "中文 -> 英文") ? "en" : "zh";

        QString mockTranslation = m_networkManager->translateMock(m_currentQuery, fromLang, toLang);

        // 显示模拟结果
        QString resultText = QString("🔍 查询: %1\n"
                                     "🌐 方向: %2\n"
                                     "⏰ 时间: %3\n\n"
                                     "⚠️ API翻译失败，使用模拟翻译:\n"
                                     "%4\n\n")
                                 .arg(m_currentQuery)
                                 .arg(langDirection)
                                 .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                                 .arg(mockTranslation);

        // 添加默认例句
        m_currentExamples = getDefaultExamples(m_currentQuery);
        resultText += formatExampleSentences();

        ui->resultTextEdit->setText(resultText);
    }
}

void MainWindow::showTranslationResult(const QString& query, const QString& translation)
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    // 构建漂亮的显示结果
    QString resultText = QString("🔍 查询: %1\n"
                                 "🌐 方向: %2\n"
                                 "⏰ 时间: %3\n\n"
                                 "📖 翻译结果:\n"
                                 "%4\n\n"
                                 "📚 例句:\n"
                                 "1. This is an example sentence for '%1'.\n"
                                 "2. Another example using the word '%1'.")
                             .arg(query)
                             .arg(m_displayDirection)  // 使用保存的显示方向
                             .arg(currentTime)
                             .arg(translation);

    ui->resultTextEdit->setText(resultText);

    // 保存到历史记录
    if (DatabaseManager::instance().addHistory(query, translation, "auto", m_displayDirection)) {
        updateHistoryView();  // 更新历史显示
    }

    // 滚动到顶部
    ui->resultTextEdit->moveCursor(QTextCursor::Start);
}

void MainWindow::showErrorMessage(const QString& error)
{
    QString errorText = QString("❌ 翻译失败\n\n"
                                "错误信息:\n"
                                "%1\n\n"
                                "建议:\n"
                                "1. 检查网络连接\n"
                                "2. 重新尝试查询\n"
                                "3. 使用模拟翻译模式").arg(error);

    ui->resultTextEdit->setText(errorText);
}

void MainWindow::updateHistoryView()
{
    if (m_historyModel) {
        m_historyModel->updateData();
    }
}

void MainWindow::on_clearButton_clicked()
{
    ui->queryLineEdit->clear();
    ui->resultTextEdit->clear();
    ui->queryLineEdit->setFocus();
    ui->statusbar->showMessage("已清除", 2000);
}

void MainWindow::on_historyButton_clicked()
{
    updateHistoryView();
    ui->historyTableView->scrollToTop();
    ui->statusbar->showMessage("历史记录已刷新", 2000);
}

void MainWindow::on_favoriteButton_clicked()
{
    QString word = ui->queryLineEdit->text().trimmed();
    if (word.isEmpty()) {
        QMessageBox::warning(this, "提示", "当前没有可收藏的内容");
        return;
    }

    // 这里可以获取当前翻译结果
    QString translation = "[需要获取翻译结果]";

    if (DatabaseManager::instance().addFavorite(word, translation, "")) {
        QMessageBox::information(this, "收藏", QString("已收藏: %1").arg(word));
        ui->statusbar->showMessage("收藏成功: " + word, 3000);
    } else {
        QMessageBox::warning(this, "收藏", "收藏失败");
    }
}

void MainWindow::on_langComboBox_currentIndexChanged(int index)
{
    QString displayText;
    if (index == 0) {
        displayText = "英文->中文";
    } else if (index == 1) {
        displayText = "中文->英文";
    } else if (index == 2) {
        displayText = "自动检测";
    } else {
        displayText = ui->langComboBox->itemText(index);
    }

    ui->statusbar->showMessage("已切换翻译方向: " + displayText, 2000);
}

// 计算字符串中中文字符的比例
double MainWindow::getChineseCharacterRatio(const QString& text)
{
    if (text.isEmpty()) return 0.0;

    int chineseCount = 0;
    int totalCount = 0;

    for (int i = 0; i < text.length(); i++) {
        QChar ch = text.at(i);

        // 统计中文字符（包括汉字、中文标点等）
        ushort unicode = ch.unicode();
        if ((unicode >= 0x4E00 && unicode <= 0x9FFF) ||   // 常用汉字
            (unicode >= 0x3400 && unicode <= 0x4DBF) ||   // 扩展A
            (unicode >= 0x20000 && unicode <= 0x2A6DF) || // 扩展B
            (unicode >= 0x3000 && unicode <= 0x303F) ||   // 中文标点符号
            (unicode >= 0xFF00 && unicode <= 0xFFEF)) {   // 全角字符
            chineseCount++;
        }

        // 只统计可见字符
        if (!ch.isSpace() && ch.category() != QChar::Other_Control) {
            totalCount++;
        }
    }

    if (totalCount == 0) return 0.0;
    return static_cast<double>(chineseCount) / totalCount;
}

// 判断文本是否是中文
bool MainWindow::isChineseText(const QString& text)
{
    if (text.isEmpty()) return false;

    // 规则1：如果包含常见中文字符，很可能是中文
    double ratio = getChineseCharacterRatio(text);

    // 规则2：检查是否有中文特有的词语
    QString commonChineseWords[] = {"的", "是", "了", "在", "和", "有", "我", "你", "他", "她", "它"};
    bool hasCommonChinese = false;
    for (const QString& word : commonChineseWords) {
        if (text.contains(word)) {
            hasCommonChinese = true;
            break;
        }
    }

    // 规则3：检查是否是纯英文（不含中文字符）
    bool isPureEnglish = true;
    for (int i = 0; i < text.length(); i++) {
        QChar ch = text.at(i);
        ushort unicode = ch.unicode();
        if ((unicode >= 0x4E00 && unicode <= 0x9FFF) ||  // 中文字符范围
            (unicode >= 0x3000 && unicode <= 0x303F)) {  // 中文标点
            isPureEnglish = false;
            break;
        }
    }

    // 决策逻辑
    if (ratio > 0.3) {  // 超过30%的中文字符
        return true;
    } else if (hasCommonChinese) {  // 包含常见中文词汇
        return true;
    } else if (isPureEnglish) {  // 纯英文
        return false;
    } else if (ratio > 0.1) {  // 有一定比例的中文字符
        return true;
    } else {
        // 默认认为是英文
        return false;
    }
}

void MainWindow::on_searchHistoryButton_clicked()
{
    QString keyword = ui->searchHistoryLineEdit->text().trimmed();

    qDebug() << "搜索历史记录，关键词：" << keyword;

    if (keyword.isEmpty()) {
        // 如果搜索框为空，显示所有记录
        m_historyModel->clearSearch();
        ui->statusbar->showMessage("已显示全部历史记录", 2000);
        return;
    }

    // 执行搜索
    if (m_historyModel) {
        // 调用HistoryModel的搜索功能
        m_historyModel->searchHistory(keyword);

        int resultCount = m_historyModel->rowCount();
        qDebug() << "找到" << resultCount << "条相关记录";

        if (resultCount > 0) {
            // 滚动到顶部
            QModelIndex topLeft = m_historyModel->index(0, 0);
            ui->historyTableView->scrollTo(topLeft);

            ui->statusbar->showMessage(
                QString("找到 %1 条包含 \"%2\" 的历史记录").arg(resultCount).arg(keyword),
                3000
                );
        } else {
            ui->statusbar->showMessage(
                QString("未找到包含 \"%1\" 的历史记录").arg(keyword),
                3000
                );

            // 可以添加声音或视觉提示
            QApplication::beep();  // 提示音
        }
    }
}

void MainWindow::on_searchHistoryLineEdit_returnPressed()
{
    // 当用户在搜索框中按回车键时，自动执行搜索
    on_searchHistoryButton_clicked();
}


void MainWindow::on_clearHistoryButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "清空历史记录",
                                  "确定要清空所有历史记录吗？此操作不可恢复！",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_historyModel->clearHistory();
        ui->statusbar->showMessage("历史记录已清空", 3000);
    }
}

// 修改刷新统计按钮的功能
void MainWindow::on_updateStatsButton_clicked()
{
    // 1. 从历史记录中重新统计常用词汇
    ui->statusbar->showMessage("正在统计常用词汇...", 2000);

    // 2. 获取所有历史记录
    QList<QStringList> allHistory = DatabaseManager::instance().getHistory(1000); // 获取1000条记录

    // 3. 清空现有统计
    DatabaseManager::instance().clearWordStatistics();

    // 4. 重新统计
    int countedWords = 0;
    for (const QStringList& record : allHistory) {
        if (record.size() > 1) {  // 确保有查询内容
            QString word = record[1];  // 查询内容
            if (!word.trimmed().isEmpty()) {
                DatabaseManager::instance().incrementWordCount(word);
                countedWords++;
            }
        }
    }

    // 5. 刷新显示
    if (m_wordStatsModel) {
        m_wordStatsModel->updateData();

        int resultCount = m_wordStatsModel->rowCount();
        if (resultCount > 0) {
            ui->statusbar->showMessage(
                QString("统计完成！共统计 %1 条历史记录，发现 %2 个常用词汇").arg(countedWords).arg(resultCount),
                5000
                );
        } else {
            ui->statusbar->showMessage("统计完成，但未找到常用词汇", 3000);
        }
    }
}

// 清空统计按钮点击事件
void MainWindow::on_clearStatsButton_clicked()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this,
        "清空词汇统计",
        "确定要清空所有词汇统计吗？\n"
        "这将删除所有单词的查询次数记录，操作不可恢复！",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No  // 默认选择"否"
        );

    if (reply == QMessageBox::Yes) {
        if (DatabaseManager::instance().clearWordStatistics()) {
            // 刷新统计显示
            if (m_wordStatsModel) {
                m_wordStatsModel->updateData();
            }

            ui->statusbar->showMessage("词汇统计已清空", 3000);
            qDebug() << "词汇统计已清空";
        } else {
            QMessageBox::warning(this, "错误", "清空统计失败，请检查数据库连接");
        }
    } else {
        ui->statusbar->showMessage("已取消清空操作", 2000);
    }
}

// 新增：格式化例句显示
QString MainWindow::formatExampleSentences()
{
    if (m_currentExamples.isEmpty()) {
        qDebug() << "formatExampleSentences: 当前没有例句";
        return "📚 真实例句:\n   暂无相关例句\n";
    }

    qDebug() << "formatExampleSentences: 格式化" << m_currentExamples.size() << "个例句";

    QString examplesText = "📚 真实例句:\n";

    for (int i = 0; i < qMin(m_currentExamples.size(), 5); i++) {
        QString example = m_currentExamples[i];

        // 清理示例文本
        example = example.trimmed();

        // 添加编号
        examplesText += QString("   %1. %2").arg(i + 1).arg(example);

        // 如果不是最后一个例句，添加换行
        if (i < qMin(m_currentExamples.size(), 5) - 1) {
            examplesText += "\n";
        }

        qDebug() << "  例句" << i+1 << ":" << example.left(50) << "...";
    }

    // 显示例句总数
    if (m_currentExamples.size() > 5) {
        examplesText += QString("\n\n   ... 还有 %1 个例句").arg(m_currentExamples.size() - 5);
    }

    examplesText += "\n";  // 添加最后的换行

    return examplesText;
}

void MainWindow::updateExampleDisplay()
{
    QString currentText = ui->resultTextEdit->toPlainText();

    if (currentText.isEmpty()) {
        qDebug() << "当前文本为空，不更新例句";
        return;
    }

    qDebug() << "updateExampleDisplay: 开始更新例句显示";
    qDebug() << "当前有例句数量:" << m_currentExamples.size();

    // 检查文本中是否包含翻译结果
    if (!currentText.contains("📖 翻译结果:")) {
        qDebug() << "文本中未找到翻译结果，等待翻译完成";
        return;
    }

    // 构建新的例句部分
    QString examplesSection = formatExampleSentences();

    // 查找并替换例句部分
    int exampleStart = currentText.indexOf("📚 正在获取真实例句");
    if (exampleStart == -1) {
        exampleStart = currentText.indexOf("📚 示例句子");
    }
    if (exampleStart == -1) {
        exampleStart = currentText.indexOf("📚 真实例句:");
    }

    if (exampleStart != -1) {
        // 找到例句结束位置
        int exampleEnd = currentText.indexOf("\n\n", exampleStart);
        if (exampleEnd == -1) {
            exampleEnd = currentText.length();
        }

        // 替换例句部分
        QString newText = currentText.left(exampleStart) + examplesSection;
        ui->resultTextEdit->setText(newText);
        qDebug() << "例句显示已更新";

        // 滚动到顶部
        ui->resultTextEdit->moveCursor(QTextCursor::Start);
    } else {
        qDebug() << "未找到例句部分，尝试在末尾添加";

        // 在末尾添加例句
        QString newText = currentText;
        if (!newText.endsWith("\n")) newText += "\n";
        newText += examplesSection;
        ui->resultTextEdit->setText(newText);
    }
}

QStringList MainWindow::getDefaultExamples(const QString& word)
{
    QString queryWord = word.toLower().trimmed();
    QStringList examples;

    qDebug() << "为单词生成默认例句:" << queryWord;

    // 常见单词的预设例句库
    static QMap<QString, QStringList> exampleDB = {
        {"looking", {
                        "She is looking for her keys.",
                        "He is looking at the painting.",
                        "They are looking forward to the trip."
                    }},
        {"hello", {
                      "Hello, how are you?",
                      "She said hello to everyone.",
                      "Say hello to your new colleague."
                  }},
        {"world", {
                      "Hello, world!",
                      "She travels around the world.",
                      "The whole world is watching."
                  }},
        {"love", {
                     "I love you.",
                     "She loves reading books.",
                     "Love makes the world go round."
                 }},
        {"time", {
                     "What time is it?",
                     "Time flies when you're having fun.",
                     "It's time to go."
                 }},
        {"water", {
                      "I need to drink some water.",
                      "The water is very clear.",
                      "Don't forget to water the plants."
                  }},
        {"book", {
                     "This is an interesting book.",
                     "I need to book a flight.",
                     "She is writing a new book."
                 }}
    };

    // 检查是否有预设例句
    if (exampleDB.contains(queryWord)) {
        examples = exampleDB[queryWord];
        qDebug() << "使用预设例句库，找到" << examples.size() << "个例句";
    } else {
        // 动态生成例句
        qDebug() << "为单词动态生成例句:" << queryWord;

        // 根据单词特征生成不同例句
        if (queryWord.endsWith("ing")) {  // 动名词
            examples << QString("I am %1 right now.").arg(queryWord)
                     << QString("She enjoys %1 in her free time.").arg(queryWord)
                     << QString("%1 is good for your health.").arg(queryWord);
        }
        else if (queryWord.endsWith("ed")) {  // 过去式
            examples << QString("Yesterday, I %1 to the store.").arg(queryWord)
                     << QString("She has %1 that movie before.").arg(queryWord)
                     << QString("They %1 the project successfully.").arg(queryWord);
        }
        else if (queryWord.length() <= 3) {  // 短单词
            examples << QString("The word '%1' is very short.").arg(word)
                     << QString("Can you spell '%1'?").arg(word)
                     << QString("'%1' is a common word in English.").arg(word);
        }
        else {  // 一般单词
            examples << QString("This is an example of using '%1'.").arg(word)
                     << QString("Can you make a sentence with '%1'?").arg(word)
                     << QString("The meaning of '%1' is important to understand.").arg(word);
        }

        qDebug() << "动态生成了" << examples.size() << "个例句";
    }

    return examples;
}

QStringList MainWindow::generateExamplesFromTranslation(const QString& word, const QString& translation)
{
    QStringList examples;

    qDebug() << "生成例句，单词:" << word << "翻译:" << translation;

    // 1. 首先添加翻译对
    examples << QString("%1 → %2").arg(word).arg(translation);

    // 2. 根据单词类型添加更多例句
    QString cleanWord = word.toLower().trimmed();

    // 常见单词的预设例句
    if (cleanWord == "hello") {
        examples << "Hello, how are you?"
                 << "She said hello to everyone."
                 << "Say hello to your new colleague.";
    }
    else if (cleanWord == "world") {
        examples << "Hello, world!"
                 << "She travels around the world."
                 << "The whole world is watching.";
    }
    else if (cleanWord == "looking") {
        examples << "She is looking for her keys."
                 << "He is looking at the painting."
                 << "They are looking forward to the trip.";
    }
    else if (cleanWord == "love") {
        examples << "I love you."
                 << "She loves reading books."
                 << "Love makes the world go round.";
    }
    else if (cleanWord == "time") {
        examples << "What time is it?"
                 << "Time flies when you're having fun."
                 << "It's time to go.";
    }
    else if (cleanWord == "water") {
        examples << "I need to drink some water."
                 << "The water is very clear."
                 << "Don't forget to water the plants.";
    }
    else if (cleanWord == "book") {
        examples << "This is an interesting book."
                 << "I need to book a flight."
                 << "She is writing a new book.";
    }
    // 根据单词特征生成例句
    else if (cleanWord.endsWith("ing")) {
        examples << QString("I am %1 right now.").arg(cleanWord)
        << QString("She enjoys %1 in her free time.").arg(cleanWord)
        << QString("%1 is good for your health.").arg(cleanWord);
    }
    else if (cleanWord.endsWith("ed")) {
        examples << QString("Yesterday, I %1 to the store.").arg(cleanWord)
        << QString("She has %1 that movie before.").arg(cleanWord)
        << QString("They %1 the project successfully.").arg(cleanWord);
    }
    else if (cleanWord.length() <= 3) {
        examples << QString("The word '%1' is very short.").arg(word)
        << QString("Can you spell '%1'?").arg(word)
        << QString("'%1' is a common word in English.").arg(word);
    }
    else {
        examples << QString("This is an example of using '%1'.").arg(word)
        << QString("Can you make a sentence with '%1'?").arg(word)
        << QString("The meaning of '%1' is important to understand.").arg(word);
    }

    // 确保不超过5个例句
    if (examples.size() > 5) {
        examples = examples.mid(0, 5);
    }

    qDebug() << "生成了" << examples.size() << "个例句";
    return examples;
}

void MainWindow::onExampleSentencesReceived(const QStringList& examples, bool success, const QString& error)
{
    qDebug() << "收到例句，数量:" << examples.size() << "成功:" << success;

    if (success && !examples.isEmpty()) {
        m_currentExamples = examples;
        updateExampleDisplay();
    } else {
        qDebug() << "例句获取失败:" << error;
        m_currentExamples = getDefaultExamples(m_currentQuery);
        updateExampleDisplay();
    }
}
