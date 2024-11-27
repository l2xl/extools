#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    mMarketView = std::make_unique<MarketWidget>(this);
    this->setCentralWidget(mMarketView.get());


    const static std::deque<std::array<double, 4>> sample_data {
        {75,100,50,90},
        {90,50,100,60},
        {60,55,95,65},
        {65,65,90,85},
        {85,85,55,55},
        {53,55,30,45}};

    mMarketView->SetMarketData(sample_data);

}

MainWindow::~MainWindow()
{
    delete ui;
}
