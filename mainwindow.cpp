#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDebug>
#include <QDateTime>
#include <QTimer>  // 添加这行！必须包含QTimer头文件
#include <QTextCursor>

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

    // 连接网络管理器的信号 - 确保连接成功
    bool connected = connect(m_networkManager, &NetworkManager::translationFinished,
                             this, &MainWindow::onTranslationFinished);

    if (!connected) {
        qDebug() << "警告：translationFinished信号连接失败！";
    }

    // 添加一个定时器备用，防止按钮永远禁用
    QTimer* safetyTimer = new QTimer(this);
    connect(safetyTimer, &QTimer::timeout, this, [this]() {
        if (!ui->searchButton->isEnabled()) {
            ui->searchButton->setEnabled(true);
            ui->statusbar->showMessage("已自动恢复查询功能", 2000);
        }
    });
    safetyTimer->start(10000);  // 10秒后自动恢复

    // 设置初始文本
    ui->resultTextEdit->setPlaceholderText("翻译结果将显示在这里...\n\n"
                                           "当前使用模式：模拟翻译\n"
                                           "（如需真实翻译，请配置API密钥）");

    // 设置状态栏
    ui->statusbar->showMessage("就绪 - 模拟翻译模式");
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

    // 修改这里：强制使用模拟翻译，避免API密钥问题
    QString translation = m_networkManager->translateMock(word, fromLang, toLang);

    // 立即显示结果，不等待网络信号
    showTranslationResult(word, translation);
    ui->searchButton->setEnabled(true);  // 重新启用按钮

    // 如果你想使用真实API，需要先配置密钥
    // 暂时注释掉这行：
    // m_networkManager->translateBaidu(word, fromLang, toLang);
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
