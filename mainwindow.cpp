#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QDesktopServices>
#include <QMenu>
#include <QGuiApplication>
#include <QAction>
#include <QClipboard>
#include <QPainter>

static void findRecursion(const QString &path, const QString &pattern, QStringList *result)
{
    QDir currentDir(path);
    const QString prefix = path + QLatin1Char('/');
    foreach (const QString &match, currentDir.entryList(QStringList(pattern), QDir::Files | QDir::NoSymLinks)) {
        result->append(prefix + match);
    }
    foreach (const QString &dir, currentDir.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot)) {
        findRecursion(prefix + dir, pattern, result);
    }
}

static inline QString fileNameOfItem(const QTableWidgetItem *item)
{
    return item->data(absoluteFileNameRole).toString();
}

static inline void openFile(const QString &fileName)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    FILE_FORMAT("png")
{
    ui->setupUi(this);
    connect(ui->browseButton, &QAbstractButton::clicked, this, &MainWindow::browse);
    connect(ui->findButton, &QAbstractButton::clicked, this, &MainWindow::find);
    connect(ui->quitButton, &QAbstractButton::clicked, this, &MainWindow::quit);
    connect(ui->procceedButton, &QAbstractButton::clicked, this, &MainWindow::procceed);
    connect(ui->filesTable, &QTableWidget::customContextMenuRequested, this, &MainWindow::contextMenu);
    connect(ui->filesTable, &QTableWidget::cellActivated, this, &MainWindow::openFileOfItem);
}

void MainWindow::enableProggressBar(bool enable = true)
{
    ui->progressBar->setEnabled(enable);
    ui->progressBar->setValue(0);
}

void MainWindow::browse()
{
    QString directory = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Find PNGs"), QDir::currentPath()));
    if (!directory.isEmpty()) {
        if (ui->directoryComboBox->findText(directory) == -1) {
            ui->directoryComboBox->addItem(directory);
        }
        ui->directoryComboBox->setCurrentIndex(ui->directoryComboBox->findText(directory));
    }
}

void MainWindow::find()
{
    enableProggressBar();
    ui->filesTable->setRowCount(0);
    QString path = QDir::cleanPath(ui->directoryComboBox->currentText());
    currentDir = QDir(path);
    QStringList files;
    findRecursion(path, "*." + FILE_FORMAT, &files);
    ui->progressBar->setValue(50);
    showFiles(files);
    ui->progressBar->setValue(100);
    enableProggressBar(false);
}

void MainWindow::quit()
{
    close();
}

void MainWindow::procceed()
{
    // enable proggress
    enableProggressBar();
    int rows = ui->filesTable->rowCount();
    int multiple = ui->multipleBox->value();
    int procceeded = 0;
    bool centered = ui->centeredBox->isChecked();
    for (int i = 0; i < rows; i++)
    {
        if (ui->filesTable->item(i, ui->filesTable->columnCount() - 1)->checkState() != Qt::Checked) {
            continue;
        }
        QString fileName = fileNameOfItem(ui->filesTable->item(i, ui->filesTable->columnCount() - 1));
        QImage image(fileName, FILE_FORMAT.toLatin1().data());
        int leftWidth = image.width() % multiple;
        int leftHeight = image.height() % multiple;
        QImage newImage(image.width() + leftWidth, image.height() + leftHeight, image.format());
        newImage.fill(Qt::transparent);
        QPainter painter(&newImage);
        // Center the image if need
        int x = centered ? (int)((newImage.width() - image.width()) / 2) : 0;
        int y = centered ? (int)((newImage.height() - image.height()) / 2) : 0;
        painter.drawImage(x, y, image);
        painter.end();
        bool result = newImage.save(fileName, FILE_FORMAT.toLatin1().data());
        // update proggress bar
        ui->progressBar->setValue((i+1) * 100 / rows);
        if (result) {
            procceeded++;
            ui->filesTable->item(i, ui->filesTable->columnCount() - 1)->setCheckState(Qt::Unchecked);
            ui->filesTable->item(i, 0)->setBackgroundColor(Qt::green);
        }
    }
    ui->filesFoundLabel->setText(tr("%n file(s) procceeded", 0, procceeded));
    // disable proggress bar
    enableProggressBar(false);
}

void MainWindow::showFiles(const QStringList &files)
{
    for (int i = 0; i < files.size(); ++i) {
        const QString &fileName = files.at(i);
        QTableWidgetItem *statusItem = new QTableWidgetItem;
        statusItem->setData(absoluteFileNameRole, QVariant(fileName));
        statusItem->setBackgroundColor(Qt::gray);
        const QString toolTip = QDir::toNativeSeparators(fileName);
        const QString relativePath = QDir::toNativeSeparators(currentDir.relativeFilePath(fileName));
        const qint64 size = QFileInfo(fileName).size();
        QTableWidgetItem *fileNameItem = new QTableWidgetItem(relativePath);
        fileNameItem->setData(absoluteFileNameRole, QVariant(fileName));
        fileNameItem->setToolTip(toolTip);
        fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *sizeItem = new QTableWidgetItem(tr("%1 KB").arg(int((size + 1023) / 1024)));
        sizeItem->setData(absoluteFileNameRole, QVariant(fileName));
        sizeItem->setToolTip(toolTip);
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *checkItem = new QTableWidgetItem;
        checkItem->setCheckState(Qt::CheckState::Checked);
        checkItem->setData(absoluteFileNameRole, QVariant(fileName));
        int row = ui->filesTable->rowCount();
        ui->filesTable->insertRow(row);
        ui->filesTable->setItem(row, 0, statusItem);
        ui->filesTable->setItem(row, 1, fileNameItem);
        ui->filesTable->setItem(row, 2, sizeItem);
        ui->filesTable->setItem(row, 3, checkItem);
    }
    ui->filesFoundLabel->setText(tr("%n file(s) found", 0, files.size()));
    ui->filesFoundLabel->setWordWrap(true);
}

void MainWindow::openFileOfItem(int row, int /* column */)
{
    const QTableWidgetItem *item = ui->filesTable->item(row, 0);
    openFile(fileNameOfItem(item));
}

void MainWindow::contextMenu(const QPoint &pos)
{
    const QTableWidgetItem *item = ui->filesTable->itemAt(pos);
    if (!item)
        return;
    QMenu menu;
#ifndef QT_NO_CLIPBOARD
    QAction *copyAction = menu.addAction(tr("Copy name"));
#endif
    QAction *openAction = menu.addAction(tr("Open"));
    QAction *action = menu.exec(ui->filesTable->mapToGlobal(pos));
    if (!action)
        return;
    const QString fileName = fileNameOfItem(item);
    if (action == openAction)
        openFile(fileName);
#ifndef QT_NO_CLIPBOARD
    else if (action == copyAction)
        QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(fileName));
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}
