#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "HistoryModel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_searchButton_clicked();
    void on_clearButton_clicked();
    void on_historyButton_clicked();  // 你新增的查询历史按钮
    void on_langComboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    HistoryModel *m_historyModel;  // 历史数据模型
    void initDatabase();  // 初始化数据库
    void updateHistoryView();  // 更新历史显示
};
#endif // MAINWINDOW_H
