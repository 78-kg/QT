#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_historyModel(new HistoryModel(this))
    , m_networkManager(new NetworkManager(this))
{
    ui->setupUi(this);

    // 初始化
    this->setWindowTitle("智能词典工具 v1.0");

    // 初始化数据库
    initDatabase();

    // 设置历史表格
    ui->historyTableView->setModel(m_historyModel);
    ui->historyTableView->horizontalHeader()->setStretchLastSection(true);

    // 连接网络管理器的信号
    connect(m_networkManager, &NetworkManager::translationFinished,
            this, &MainWindow::onTranslationFinished);

    // 设置初始文本
    ui->resultTextEdit->setPlaceholderText("翻译结果将显示在这里...\n\n"
                                           "支持功能：\n"
                                           "1. 中英文互译\n"
                                           "2. 自动检测语言\n"
                                           "3. 历史记录保存\n"
                                           "4. 收藏功能");

    // 设置状态栏
    ui->statusbar->showMessage("就绪");
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

    m_currentQuery = word;  // 保存当前查询

    QString langDirection = ui->langComboBox->currentText();
    QString fromLang = "auto";
    QString toLang = "zh";

    // 解析语言方向
    if (langDirection == "英文 -> 中文") {
        fromLang = "en";
        toLang = "zh";
    } else if (langDirection == "中文 -> 英文") {
        fromLang = "zh";
        toLang = "en";
    } else if (langDirection == "自动检测") {
        fromLang = "auto";
        toLang = "zh";
    }

    // 显示查询状态
    ui->resultTextEdit->setText("正在查询中...\n请稍候");
    ui->statusbar->showMessage("正在翻译: " + word);
    ui->searchButton->setEnabled(false);  // 禁用按钮防止重复点击

    // 使用网络翻译（先使用模拟翻译测试）
    QString translation = m_networkManager->translateMock(word, fromLang, toLang);
    showTranslationResult(word, translation);

    // 实际使用API时取消下面的注释
    // m_networkManager->translateBaidu(word, fromLang, toLang);
}

void MainWindow::onTranslationFinished(const QString& result, bool success, const QString& error)
{
    ui->searchButton->setEnabled(true);  // 重新启用按钮

    if (success) {
        showTranslationResult(m_currentQuery, result);
        ui->statusbar->showMessage("翻译完成", 3000);
    } else {
        showErrorMessage(error);
        ui->statusbar->showMessage("翻译失败: " + error, 5000);
    }
}

void MainWindow::showTranslationResult(const QString& query, const QString& translation)
{
    QString langDirection = ui->langComboBox->currentText();
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
                             .arg(langDirection)
                             .arg(currentTime)
                             .arg(translation);

    ui->resultTextEdit->setText(resultText);

    // 保存到历史记录
    if (DatabaseManager::instance().addHistory(query, translation, "auto", langDirection)) {
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
    QString lang = ui->langComboBox->itemText(index);
    ui->statusbar->showMessage("已切换翻译方向: " + lang, 2000);
}
