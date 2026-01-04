#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "DatabaseManager.h"
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_historyModel(new HistoryModel(this))
{
    ui->setupUi(this);

    // 初始化
    this->setWindowTitle("智能词典工具 v1.0");

    // 初始化数据库
    initDatabase();

    // 设置历史表格的模型
    ui->historyTableView->setModel(m_historyModel);
    ui->historyTableView->horizontalHeader()->setStretchLastSection(true);

    // 设置结果区域
    ui->resultTextEdit->setPlaceholderText("翻译结果将显示在这里...\n\n"
                                           "示例用法：\n"
                                           "1. 输入要查询的单词或句子\n"
                                           "2. 选择翻译方向\n"
                                           "3. 点击'查询'按钮");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initDatabase()
{
    if (DatabaseManager::instance().initDatabase()) {
        qDebug() << "Database initialized successfully";
    } else {
        QMessageBox::warning(this, "警告", "数据库初始化失败！");
    }
}

void MainWindow::updateHistoryView()
{
    if (m_historyModel) {
        m_historyModel->updateData();
    }
}

void MainWindow::on_searchButton_clicked()
{
    QString word = ui->queryLineEdit->text().trimmed();
    if (word.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入要查询的单词！");
        ui->queryLineEdit->setFocus();
        return;
    }

    QString langDirection = ui->langComboBox->currentText();

    // 模拟翻译结果（后续替换为真实API调用）
    QString translation = QString("[模拟] %1 的翻译结果").arg(word);

    // 显示结果
    QString resultText = QString("查询: %1\n"
                                 "方向: %2\n"
                                 "时间: %3\n\n"
                                 "翻译结果:\n"
                                 "%4\n\n"
                                 "例句:\n"
                                 "1. This is an example sentence for '%1'.\n"
                                 "2. Another example using the word '%1'.")
                             .arg(word)
                             .arg(langDirection)
                             .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
                             .arg(translation);

    ui->resultTextEdit->setText(resultText);

    // 保存到历史记录
    if (DatabaseManager::instance().addHistory(word, translation, "auto", langDirection)) {
        updateHistoryView();  // 更新历史显示
    }

    // 滚动到顶部
    ui->resultTextEdit->moveCursor(QTextCursor::Start);
}

void MainWindow::on_clearButton_clicked()
{
    ui->queryLineEdit->clear();
    ui->resultTextEdit->clear();
    ui->queryLineEdit->setFocus();
}

void MainWindow::on_historyButton_clicked()
{
    // 可以在这里添加历史管理的额外功能
    updateHistoryView();  // 刷新历史记录
    ui->historyTableView->scrollToTop();  // 滚动到顶部
}

void MainWindow::on_langComboBox_currentIndexChanged(int index)
{
    // 语言选择变化时的处理
    qDebug() << "Language changed to index:" << index;
    // 这里可以添加语言切换的逻辑
}
