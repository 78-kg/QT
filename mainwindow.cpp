#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QTimer>  // 添加这行！必须包含QTimer头文件
#include <QTextCursor>
#include <QShortcut>        // 添加这行
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
    , m_wordStatsModel(new WordStatisticsModel(this))  // 确保这行存在且正确
{
    ui->setupUi(this);

    // 基本初始化
    this->setWindowTitle("智能词典工具 v2.0");
    initDatabase();

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

    qDebug() << "翻译设置: " << displayDirection << "(" << fromLang << "->" << toLang << ")";

    // 保存显示方向，用于结果展示

    // 显示查询状态
    ui->resultTextEdit->setText("正在查询中...\n请稍候");
    ui->statusbar->showMessage("正在翻译: " + word);
    ui->searchButton->setEnabled(false);

    m_networkManager->translateBaidu(word, fromLang, toLang);
}

void MainWindow::onTranslationFinished(const QString& result, bool success, const QString& error)
{
    // 确保按钮被重新启用
    ui->searchButton->setEnabled(true);

    if (success) {
        showTranslationResult(m_currentQuery, result);
        ui->statusbar->showMessage("翻译完成", 3000);
    } else {
        // 显示错误但不阻止后续查询
        showErrorMessage(error);
        ui->statusbar->showMessage("翻译失败: " + error, 5000);

        // 错误时也使用模拟翻译作为后备
        QString langDirection = ui->langComboBox->currentText();
        QString fromLang = (langDirection == "中文 -> 英文") ? "zh" : "auto";
        QString toLang = (langDirection == "中文 -> 英文") ? "en" : "zh";

        QString mockTranslation = m_networkManager->translateMock(m_currentQuery, fromLang, toLang);
        showTranslationResult(m_currentQuery, mockTranslation);
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
