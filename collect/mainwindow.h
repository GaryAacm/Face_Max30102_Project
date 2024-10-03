#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QPixmap>
#include "MaxPlot.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onGenerateQRClicked();
    void onStartClicked();
    void onExitClicked();
    void sendExitMessage();

private:
    QPushButton *generateQRButton;
    QPushButton *startButton;
    QPushButton *exitButton;
    QPixmap backgroundPixmap; // 背景图片
    MaxPlot *maxPlotWindow;

};

#endif // MAINWINDOW_H