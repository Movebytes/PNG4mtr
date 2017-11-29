#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QComboBox>
#include <QDir>

enum { absoluteFileNameRole = Qt::UserRole + 1 };

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

private slots:
    void browse();
    void find();
    void quit();
    void openFileOfItem(int row, int column);
    void contextMenu(const QPoint &pos);
    void procceed();

private:
    QStringList findFiles(const QStringList &files, const QString &text);
    void showFiles(const QStringList &files);
    void enableProggressBar(bool);

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QDir currentDir;
    QString FILE_FORMAT;
};

#endif // MAINWINDOW_H
