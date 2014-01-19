#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include <QtMultimedia/QMediaPlayer>
#include <QVideoWidget>
#include <QtConcurrent/QtConcurrent>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->showMaximized();
    ui->horizontalSlider->hide();
    ui->groupBox->hide();
    connect(ui->widget,SIGNAL(giveImage(Image)),this,SLOT(getImage(Image)));
    ui->widget_2->setTitle("Pressure");
    ui->widget_2->setAxisTitle(ui->widget_2->xBottom, "Frame");
    ui->widget_2->setAxisTitle(ui->widget_2->yLeft,"Pressure");
    ui->widget_2->setAxisAutoScale( ui->widget_2->xBottom, true );
    ui->widget_2->setAxisAutoScale( ui->widget_2->yLeft, true );
    zoom = new QwtPlotZoomer(ui->widget_2->canvas());
    zoom->setRubberBandPen(QPen(Qt::white));
    QPen pen = QPen( Qt::red );
    curve.setRenderHint( QwtPlotItem::RenderAntialiased );
    curve.setPen( pen );
    curve.attach( ui->widget_2 );

    ui->widget_3->setTitle("Pressure");
    ui->widget_3->setAxisTitle(ui->widget_3->xBottom, "Pressure");
    ui->widget_3->setAxisTitle(ui->widget_3->yLeft,"Point of lights");
    ui->widget_3->setAxisAutoScale( ui->widget_3->xBottom, true );
    ui->widget_3->setAxisAutoScale( ui->widget_3->yLeft, true );
    zoom0 = new QwtPlotZoomer(ui->widget_3->canvas());
    zoom0->setRubberBandPen(QPen(Qt::white));
    QPen pen0 = QPen( Qt::red );
    curve0.setRenderHint( QwtPlotItem::RenderAntialiased );
    curve0.setPen( pen0 );
    curve0.attach( ui->widget_3 );

    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);
    player = new QMediaPlayer;
    player->setVideoOutput(ui->widget_4);
    connect(ui->widget, SIGNAL(giveFramesNumber(int)), this, SLOT(sliderAnalManipulation(int)));
    connect(ui->widget, SIGNAL(imageUpdated(QString)), this, SLOT(updateView(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    ui->widget->openImage();
    ui->horizontalSlider->hide();
}

void MainWindow::on_actionReset_triggered()
{
    ui->widget->reset();
}

void MainWindow::on_actionRun_triggered()
{
    ui->widget->run();
}

void MainWindow::on_actionAlign_triggered()
{
    ui->widget->align();
}

void MainWindow::on_action3D_triggered()
{
    ui->widget->switchMode();
    ui->horizontalSlider->hide();
}

void MainWindow::on_actionSave_triggered()
{
    //ui->widget->saveImage();
    ui->widget->saveConf(false);
}

void MainWindow::keyPressEvent(QKeyEvent *e){
    if(e->key() == Qt::Key_1){
        ui->widget->setMode(BOTTOM);
    }
    if(e->key() == Qt::Key_2){
        ui->widget->setMode(ISO);
    }
    if(e->key() == Qt::Key_3){
        ui->widget->setMode(LEFT);
    }
    if(e->key() == Qt::Key_4){
        ui->widget->setMode(RIGHT);
    }
    if(e->key() == Qt::Key_Escape){
        ui->widget->stop();
    }
}

void MainWindow::on_actionLoad_triggered()
{
    ui->widget->loadConf(false);
}

void MainWindow::on_actionCalibrate_triggered()
{
    ui->widget->calibrate();
    ui->horizontalSlider->hide();
}

void MainWindow::on_actionAutorun_triggered()
{
    ui->widget->autorun();
    displayResults(ui->widget->getRes(), ui->widget_2);
    displayResults(ui->widget->getRes(), ui->widget->getPol(), ui->widget_3);
}

void MainWindow::on_actionOpen_Video_triggered()
{
    ui->widget->openVideo();
    /*if(n){
        ui->horizontalSlider->show();
        ui->horizontalSlider->setMaximum(n-1);
        ui->horizontalSlider->setValue(0);
        ui->widget->getFrame(0);
    }*/
    player->setMedia(QUrl::fromLocalFile(ui->widget->getVName()));
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    ui->widget->getFrame(value);
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionSave_as_Default_triggered()
{
    ui->widget->saveConf(true);
}

void MainWindow::on_actionSave_2_triggered()
{
    ui->widget->saveData();
}

void MainWindow::on_actionLoad_2_triggered()
{
    ui->widget->loadData();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About"), tr("Danylo Lizanets © 2013"));
}


void MainWindow::on_actionBounds_triggered(bool checked)
{
    if(checked){
        ui->groupBox->show();
    }else{
        ui->groupBox->hide();
    }
}

void MainWindow::getImage(Image im)
{
    ui->thresh_sb->setValue(im.threshold);
    ui->pressure_l->setNum(im.sum);
    ui->left_l->setNum(static_cast<int>(im.bound_counter[2]));
    ui->top_l->setNum(static_cast<int>(im.bound_counter[1]));
    ui->right_l->setNum(static_cast<int>(im.bound_counter[0]));
    ui->bottom_l->setNum(static_cast<int>(im.bound_counter[3]));
    ui->ave_l->setNum(static_cast<int>((im.bound_counter[2]+im.bound_counter[1]+im.bound_counter[0]+im.bound_counter[3])/4));
    ui->sum_l->setNum(static_cast<int>(im.bound_counter[2]+im.bound_counter[1]+im.bound_counter[0]+im.bound_counter[3]));
}

void MainWindow::updateView(QString filename)
{
    scene->addPixmap(QPixmap(filename));
}

void MainWindow::on_actionPrev_triggered()
{
    ui->widget->prev();
}

void MainWindow::on_actionNext_triggered()
{
    ui->widget->next();
}

void MainWindow::displayResults(const QVector<double> &res, QwtPlot* widget_2)
{
    widget_2->detachItems( QwtPlotItem::Rtti_PlotCurve, false );
    widget_2->replot();

    QVector < QPointF > points( res.size() );
    quint32 counter = 0;
    auto pointsIt = points.begin();

    for ( auto ri = res.constBegin(); ri != res.constEnd(); ++ ri, ++ pointsIt, ++ counter ) {
        (*pointsIt) = QPointF( counter, (*ri) );
    }

    QwtPointSeriesData * data = new QwtPointSeriesData(points);
    curve.setData(data);
    curve.attach( widget_2 );
    widget_2->replot();
}

void MainWindow::displayResults(const QVector<double> &res, const QVector<double> &res0, QwtPlot* widget_2)
{


    widget_2->detachItems( QwtPlotItem::Rtti_PlotCurve, false );
    widget_2->replot();

    QVector < QPointF > points( res.size() );
    auto pointsIt = points.begin();

    for ( int i = 0; i < res.size(); ++i) {
        (*(pointsIt+i)) = QPointF( res[i], res0[i] );
    }

    QwtPointSeriesData * data = new QwtPointSeriesData(points);
    curve0.setData(data);
    curve0.attach( widget_2 );
    widget_2->replot();

}

void MainWindow::on_horizontalSlider_2_valueChanged(int value)
{
    player->setPosition(player->duration() / 100 * value);
}

void MainWindow::on_pushButton_toggled(bool checked)
{
    if(checked){
        player->play();
    }else{
        player->pause();
    }
}

void MainWindow::on_actionSave_3_triggered()
{
    ui->widget->saveResults();
}

void MainWindow::sliderAnalManipulation(int n)
{
    ui->horizontalSlider->show();
    ui->horizontalSlider->setMaximum(n-1);
    ui->horizontalSlider->setValue(0);
    ui->widget->getFrame(0);
}
