#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "HistoryModel.h"
#include "NetworkManager.h"
#include "WordStatisticsModel.h"

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
    void on_historyButton_clicked();
    void on_favoriteButton_clicked();  // 收藏按钮
    void on_langComboBox_currentIndexChanged(int index);

    // 新增：翻译结果返回的槽函数
    void onTranslationFinished(const QString& result, bool success, const QString& error);

    void on_searchHistoryButton_clicked();
    void on_searchHistoryLineEdit_returnPressed();  // 确保这行存在！
    void on_clearHistoryButton_clicked();

    void on_updateStatsButton_clicked();
    void on_clearStatsButton_clicked();
    void onExampleSentencesReceived(const QStringList& examples, bool success, const QString& error);


private:
    Ui::MainWindow *ui;
    HistoryModel *m_historyModel;
    NetworkManager *m_networkManager;  // 网络管理器
    QString m_currentQuery;  // 当前查询的词
    QString m_currentFromLang;  // 添加这两个
    QString m_currentToLang;
    QString m_displayDirection;
    WordStatisticsModel *m_wordStatsModel;


    QStringList m_currentExamples;  // 当前查询词的例句

    QString formatExampleSentences();  // 添加这行！

    void updateExampleDisplay();    // 更新例句显示

    bool isChineseText(const QString& text);
    double getChineseCharacterRatio(const QString& text);

    void initDatabase();
    void updateHistoryView();
    void showTranslationResult(const QString& query, const QString& translation);
    void showErrorMessage(const QString& error);

    bool m_examplesRequested = false;
    bool m_examplesReceived = false;

    QStringList generateExamplesFromTranslation(const QString& word, const QString& translation);

    QStringList getDefaultExamples(const QString& word);  // 获取默认例句

};
#endif // MAINWINDOW_H
