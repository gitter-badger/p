#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "modelingwindow.hpp"
#include "modelingwizard.hpp"
#include "db.hpp"
#include "imagestorage.hpp"
#include <QtConcurrent/QtConcurrent>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

/*!
 * \brief MainWindow::MainWindow
 * \param parent
 */
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->showMaximized();
    ui->widget_3->setTitle("Pressure");
    ui->widget_3->setAxisTitle(ui->widget_3->xBottom, "Frame");
    ui->widget_3->setAxisTitle(ui->widget_3->yLeft, "Pressure [kPa]");
    ui->widget_3->setAxisAutoScale(ui->widget_3->xBottom, true);
    ui->widget_3->setAxisAutoScale(ui->widget_3->yLeft, true);
    zoom = new QwtPlotZoomer(ui->widget_3->canvas());
    zoom->setRubberBandPen(QPen(Qt::white));
    QPen pen = QPen(Qt::red);
    curve = new QwtPlotCurve;
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->setPen(pen);
    curve->attach(ui->widget_3);
    
    ui->widget_4->setTitle("Pressure");
    ui->widget_4->setAxisTitle(ui->widget_4->xBottom, "Pressure [kPa]");
    ui->widget_4->setAxisTitle(ui->widget_4->yLeft, "Pixels");
    ui->widget_4->setAxisAutoScale(ui->widget_4->xBottom, true);
    ui->widget_4->setAxisAutoScale(ui->widget_4->yLeft, true);
    zoom0 = new QwtPlotZoomer(ui->widget_4->canvas());
    zoom0->setRubberBandPen(QPen(Qt::white));
    QPen pen0 = QPen(Qt::red);
    curve0 = new QwtPlotCurve();
    curve0->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve0->setPen(pen0);
    curve0->attach(ui->widget_4);
    
    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);
    ui->widget_3->hide();
    ui->pushButton_2->hide();
    
    player = new QMediaPlayer;
    player->setVideoOutput(ui->widget_2);
    connect(ui->imageArea, SIGNAL(viewUpdated(Display)), this, SLOT(imageAreaUpdated(Display)));
}

/*!
 * \brief MainWindow::runCalibration
 */
void MainWindow::runCalibration()
{
    ui->mainToolBar->hide();
    ui->splitter->widget(0)->hide();
    ui->splitter->widget(2)->hide();
    //ui->menuBar->hide();
    
    on_actionCalibrate_triggered();
}

/*!
 * \brief MainWindow::runMeasurements
 */
void MainWindow::runMeasurements()
{
    QList<int> list;
    list << 0 << ui->splitter->widget(0)->width()+ui->splitter->widget(1)->width() << ui->splitter->widget(2)->width();
    ui->splitter->setSizes(list);
    //ui->widget->setSizePolicy(QSizePolicy::Expanding, ui->widget->sizePolicy().verticalPolicy());
    // on_actionLoad_triggered();
    // on_actionLoad_2_triggered();
}

QPair<ModelingData, bool> MainWindow::preModel()
{
    
    //    ModelingWizard* wizard = new ModelingWizard(this);
    //    wizard->setIs_integrated(true);
    //    wizard->exec();
    //    if(wizard->result() == QWizard::Rejected){
    //        return QPair<ModelingData,bool>(wizard->getDataRef(), false);
    //    }
    //    return QPair<ModelingData, bool>(wizard->getDataRef(), true);
    QString item = QInputDialog::getItem(this, "Select sensor", "Please select sensor name", DB::getInstance().getSensorNames(),0,false);
    int cid, did;
    ModelingData data = DB::getInstance().getSensor(item, cid, did);
    Config conf = DB::getInstance().getConfig(cid);
    p->setConfig(conf);
    auto a = DB::getInstance().getData(did);
    p->setDatum(a.first, a.second);
    return QPair<ModelingData, bool>(data, true);
}

/*!
 * \brief MainWindow::model
 */
void MainWindow::model(ModelingData res)
{
    ui->widget->setSizePolicy(QSizePolicy::Expanding, ui->widget->sizePolicy().verticalPolicy());
    if(core) core->deleteLater();
    core = new ModelingCore(this);
    res.setPressure(p->currImage().getPressure()/25*1000);
    res.setRay_number(100000);
    core->setData(res);
    QProgressDialog* pd = new QProgressDialog("Modeling","Stop",0,100, this);
    pd->setWindowTitle("Progress");
    pd->setModal(true);
    pd->setMinimumSize(pd->size());
    pd->setMaximumSize(pd->size());
    connect(core, SIGNAL(lil(int)), pd, SLOT(setValue(int)), Qt::QueuedConnection);
    connect(core, SIGNAL(sendImage(QImage)), ui->widget, SLOT(setImage(QImage)), Qt::QueuedConnection);
    connect(pd, SIGNAL(canceled()), core, SLOT(Stop()));
    QtConcurrent::run(core,&ModelingCore::run);    
    pd->exec();
}

/*!
 * \brief MainWindow::~MainWindow
 */
MainWindow::~MainWindow()
{
    emit death();
    ImageStorage::getInstance().clear();
    ImageStorage::getInstance().getCurrNumRef() = 0u;
    delete ui;
}

/*!
 * \brief MainWindow::disableUi
 * \param b
 */
void MainWindow::disableUi(bool b)
{
    if (b) {
        ui->splitter->setDisabled(b);
        ui->mainToolBar->setDisabled(b);
        ui->menuBar->setDisabled(b);
    } else {
        ui->splitter->setEnabled(!b);
        ui->mainToolBar->setEnabled(!b);
        ui->menuBar->setEnabled(!b);
    }
}

/*!
 * \brief MainWindow::Update
 * \param dis
 */
void MainWindow::Update(Display dis)
{
    disableUi(false);  
    ui->imageArea->setDisplay(dis);
    ui->pressureLabel->setText(QString::number(dis.im.getPressure()));
    ui->sumLabel->setText(QString::number(dis.im.getSum()));
    ui->averageLabel->setText(QString::number(dis.im.getSum() / 4.0));
    ui->rightLabel->setText(QString::number(dis.im.getBoundCounter()[0]));
    ui->leftLabel->setText(QString::number(dis.im.getBoundCounter()[1]));
    ui->topLabel->setText(QString::number(dis.im.getBoundCounter()[2]));
    ui->bottomLabel->setText(QString::number(dis.im.getBoundCounter()[3]));
    scene->addPixmap(QPixmap(dis.im.getFileName()));
}

/*!
 * \brief MainWindow::imageAreaUpdated
 * \param dis
 */
void MainWindow::imageAreaUpdated(Display dis)
{
    p->setDisplay(dis);
}

/*!
 * \brief MainWindow::Error
 * \param a
 * \param b
 */
void MainWindow::Error(QString a, QString b)
{
    QMessageBox::warning(this, a, b);
}

/*!
 * \brief MainWindow::plot
 * \param t
 * \param res
 */
void MainWindow::plot(QVector<double> res)
{
    QVector<QPointF> points(res.size());
    quint32 counter = 0;
    auto pointsIt = points.begin();
    
    for (auto ri = res.constBegin(); ri != res.constEnd(); ++ri, ++pointsIt, ++counter) {
        if (ri != res.constBegin()) {
            (*pointsIt) = QPointF(counter, (*ri + *(ri - 1)) / 2.0);
        } else {
            (*pointsIt) = QPointF(counter, (*ri));
        }
    }
    
    QwtPointSeriesData* data = new QwtPointSeriesData(points);
    
    ui->widget_3->detachItems(QwtPlotItem::Rtti_PlotCurve, false);
    ui->widget_3->replot();
    curve->setData(data);
    curve->attach(ui->widget_3);
    ui->widget_3->replot();
}

/*!
 * \brief MainWindow::plot
 * \param res0
 * \param res
 */
void MainWindow::plot(QVector<double> res0, QVector<double> res)
{
    ui->widget_4->detachItems(QwtPlotItem::Rtti_PlotCurve, false);
    ui->widget_4->replot();
    
    QVector<QPointF> points(res.size());
    auto pointsIt = points.begin();
    
    for (int i = 0; i < res.size(); ++i) {
        (*(pointsIt + i)) = QPointF(res[i], res0[i]);
    }
    
    QwtPointSeriesData* data = new QwtPointSeriesData(points);
    curve0->setData(data);
    curve0->attach(ui->widget_4);
    ui->widget_4->replot();
}

/*!
 * \brief MainWindow::on_actionOpen_Image_s_triggered
 */
void MainWindow::on_actionOpen_Image_s_triggered()
{
    ui->pushButton_2->hide();
    ui->widget_3->hide();
    ui->widget->show();
    ui->tabWidget_2->show();
    ui->splitter->widget(2)->show();
    disableUi();
    QStringList names = QFileDialog::getOpenFileNames(this, "Open Image(s)", ".", "Images (*.bmp)");
    if(names.isEmpty()) {
        disableUi(false);
        return;
    }
    QtConcurrent::run(p, &Processor::openImage, names);
    if(i_am_mad){
        QMessageBox::information(this, "Info", "The second step - crop image by right mouse button to the size of the sensor. Then set 3 points on the left and right top and the right bottom corners of the central hole of the sensor. Then click Image->Align.");
    }
}

/*!
 * \brief MainWindow::on_actionAlign_triggered
 */
void MainWindow::on_actionAlign_triggered()
{
    disableUi();
    QtConcurrent::run(p, &Processor::align);
    if(i_am_mad){
        QMessageBox::information(this, "Info", "Set 3 points again and then click Config->Save");
    }
}

/*!
 * \brief MainWindow::on_actionRun_triggered
 */
void MainWindow::on_actionRun_triggered()
{
    disableUi();
    QtConcurrent::run(p, &Processor::run, true);
}

/*!
 * \brief MainWindow::on_actionReset_triggered
 */
void MainWindow::on_actionReset_triggered()
{
    disableUi();
    QtConcurrent::run(p, &Processor::reset);
}

/*!
 * \brief MainWindow::on_actionAutorun_triggered
 */
void MainWindow::on_actionAutorun_triggered()
{
    disableUi();
    auto md = preModel();
    if(md.second){
        if(p->getDetect()){
            p->detectAutorun();
            model(md.first);
        }else{
            p->autorun(true);
            model(md.first);
        }
    }
}

/*!
 * \brief MainWindow::on_actionPrev_triggered
 */
void MainWindow::on_actionPrev_triggered()
{
    disableUi();
    QtConcurrent::run(p, &Processor::prev);
}

/*!
 * \brief MainWindow::on_actionNext_triggered
 */
void MainWindow::on_actionNext_triggered()
{
    disableUi();
    QtConcurrent::run(p, &Processor::next);
}

/*!
 * \brief MainWindow::on_actionExit_triggered
 */
void MainWindow::on_actionExit_triggered()
{
    this->close();
}

/*!
 * \brief MainWindow::on_actionLoad_triggered
 */
void MainWindow::on_actionLoad_triggered()
{
    QString name = QFileDialog::getOpenFileName(this, tr("Open config"), "", tr("Config files (*.conf)"));
    if(name.isEmpty()) return;
    QtConcurrent::run(p, &Processor::loadConf, name);
}

/*!
 * \brief MainWindow::on_actionSave_triggered
 */
void MainWindow::on_actionSave_triggered()
{
    QString name;
    name = QFileDialog::getSaveFileName(this, tr("Save config"), "", tr("Config files (*.conf)"));
    if(name.isEmpty()){
        i_am_mad = false;
        return;
    }
    QtConcurrent::run(p, &Processor::saveConf, name);
    if(i_am_mad){
        saved_config_name = name;
        on_actionCalibrate_triggered();
    }
}

/*!
 * \brief MainWindow::on_actionLoad_2_triggered
 */
void MainWindow::on_actionLoad_2_triggered()
{
    QString name = QFileDialog::getOpenFileName(this, tr("Load data"), "", tr("Data (*.dat)"));
    if(name.isEmpty()) return;
    QtConcurrent::run(p, &Processor::loadData, name);
}

/*!
 * \brief MainWindow::on_actionSave_2_triggered
 */
void MainWindow::on_actionSave_2_triggered()
{
    QString name = QFileDialog::getSaveFileName(this, tr("Save data"), "", tr("Data (*.dat)"));
    if(name.isEmpty()) return;
    QtConcurrent::run(p, &Processor::saveData, name);
}

/*!
 * \brief MainWindow::on_actionSave_as_Default_triggered
 */
void MainWindow::on_actionSave_as_Default_triggered()
{
    QtConcurrent::run(p, &Processor::saveConf, QString("default.conf"));
}

/*!
 * \brief MainWindow::on_action3D_triggered
 * \param checked
 */
void MainWindow::on_action3D_triggered(bool checked)
{
    Q_UNUSED(checked);
    //    if (checked) {
    //        ui->widget->setStep((float)ui->imageArea->getThreshold() / 255.0);
    //        ui->widget->setImage(ui->imageArea->getImage());
    //    } else {
    //        QImage empty;
    //        ui->widget->setStep(1.0f);
    //        ui->widget->setImage(empty);
    //    }
}

/*!
 * \brief MainWindow::on_actionCalibrate_triggered
 */
void MainWindow::on_actionCalibrate_triggered()
{
    QString name = saved_config_name;
    if(!i_am_mad){
        QString sname = QInputDialog::getText(this, "Name", "Enter name for sensor configuration");
        ModelingWizard * wiz = new ModelingWizard;
        wiz->exec();
        DB::getInstance().addSensor(sname, wiz->getDataRef());
        QStringList lst;
        lst << "Create" << "Open";
        bool ok;
        QString item = QInputDialog::getItem(this, "Choose an option", "Create or open config", lst, 1, false, &ok);
        if(ok){
            if(item == "Open"){
                name = QFileDialog::getOpenFileName(this, tr("Open config"), "", tr("Config files (*.conf)"));
                if(name.isEmpty()) return;
            }else{
                QMessageBox::information(this, "Info", "Create new config. First step - open the sample image");
                i_am_mad = true;
                on_actionOpen_Image_s_triggered();
                return;
            }
        }else{
            return;
        }
    }else{
        i_am_mad = false;
    }
    QStringList names = QFileDialog::getOpenFileNames(this, "Open Image(s)", ".", "Images (*.bmp)");
    if(names.isEmpty()) return;
    QString named = QFileDialog::getSaveFileName(this, tr("Save data"), "", tr("Data (*.dat)"));
    if(named.isEmpty()) return;
    disableUi();
    QtConcurrent::run(p, &Processor::calibrate, name, named, names);
}

/*!
 * \brief MainWindow::on_actionOpen_Video_triggered
 */
void MainWindow::on_actionOpen_Video_triggered()
{
    QString videoFileName = QFileDialog::getOpenFileName(this, tr("Open data file"), "", tr("Video files (*.avi)"));
    if (!videoFileName.isNull() && !videoFileName.isEmpty()) {
        ui->widget->hide();
        ui->widget_3->show();
        ui->pushButton_2->show();
        ui->tabWidget_2->hide();
        ui->splitter->widget(2)->hide();
        p->setVideoFileName(videoFileName);
        player->setMedia(QUrl::fromLocalFile(p->getVName()));
        QtConcurrent::run(p, &Processor::run);
    }
}

/*!
 * \brief MainWindow::saveResults
 * \param pol
 * \param res
 */
void MainWindow::saveResults( const QVector<double>& pol,  const QVector<double>& res, QwtPlot* widget)
{
    if (!res.isEmpty()) {
        QString dataFileName = QFileDialog::getSaveFileName(this, tr("Save results"), "", tr("Data (*.dat)"));
        if(!dataFileName.isEmpty()){
            QFile file(dataFileName);
            if (file.open(QFile::WriteOnly)) {
                QTextStream s(&file);
                if(widget == ui->widget_3){
                    s << p->getImage().getThreshold() << "\n";
                    s << res.size() << "\n";
                }
                for (int i = 0; i < res.size(); ++i) {
                    s << res[i] << " " << pol[i] << "\n";
                }
            }
        }
        QString plotImageFileName = QFileDialog::getSaveFileName(this, tr("Save plot image"), "", tr("Images (*.png)"));
        if (!plotImageFileName.isEmpty()) {
            widget->grab().save(plotImageFileName);
        }
    }    
}

/*!
 * \brief MainWindow::closeEvent
 */
void MainWindow::closeEvent(QCloseEvent *)
{
    this->deleteLater();
}

/*!
 * \brief MainWindow::on_pushButton_2_clicked
 */
void MainWindow::on_pushButton_2_clicked()
{
    auto res = p->getPressureValues();
    auto pol = p->getPixelValues();
    saveResults(pol, res, ui->widget_3);
}

/*!
 * \brief MainWindow::on_pushButton_3_clicked
 */
void MainWindow::on_pushButton_3_clicked()
{
    auto res = p->getPreparedPixels();
    auto pol = p->getPreparedPressures();
    saveResults(pol, res, ui->widget_4);
}

/*!
 * \brief MainWindow::on_pushButton_clicked
 * \param checked
 */
void MainWindow::on_pushButton_clicked(bool checked)
{
    if (checked) {
        player->play();
    } else {
        player->pause();
    }
}

/*!
 * \brief MainWindow::on_horizontalSlider_valueChanged
 * \param value
 */
void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    player->setPosition(player->duration() / 100 * value);
}

/*!
 * \brief MainWindow::on_actionStop_triggered
 */
void MainWindow::on_actionStop_triggered()
{
    emit stop();
}

/*!
 * \brief MainWindow::on_actionAbout_triggered
 */
void MainWindow::on_actionAbout_triggered()
{
    QString cv;
#if defined(__GNUC__) || defined(__GNUG__)
    cv = "GCC " + QString::number(__GNUC__) + "." + QString::number(__GNUC_MINOR__) + "."+QString::number(__GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    cv = "MSVC " + QString::number(_MSC_FULL_VER);
#endif
    QMessageBox::about(this,"About", "Radiation sensor toolkit. © 2013-2015\nVersion 0.9.3\nQt version: " + QString(QT_VERSION_STR) + "\nCompiler Version: " + cv);
}

/*!
 * \brief MainWindow::on_actionHelp_triggered
 */
void MainWindow::on_actionHelp_triggered()
{
    QDesktopServices::openUrl(QUrl("manual.pdf"));
}

/*!
 * \brief MainWindow::on_actionModeling_triggered
 */
void MainWindow::on_actionModeling_triggered()
{
    ModelingWizard mw;
    mw.exec();
    ModelingData data = mw.getDataRef();
    ModelingWindow * window = new ModelingWindow(this);
    window->setData(data);
    window->show();
}

/*!
 * \brief MainWindow::on_actionTest_triggered
 */
void MainWindow::on_actionTest_triggered()
{
    ModelingWizard* wizard = new ModelingWizard(this);
    wizard->setIs_integrated(true);
    wizard->exec();
    auto res = wizard->getDataRef();
    res.setPressure(ui->pressureLabel->text().toDouble()/25*1000);
    ModelingWindow* mw = new ModelingWindow;
    mw->setData(res);
    mw->start();
    mw->show();
}

/*!
 * \brief MainWindow::on_actionSplash_triggered
 */
void MainWindow::on_actionSplash_triggered()
{
    QPixmap splash("splash.png");
    QSplashScreen ss(splash);
    ss.show();
}

void MainWindow::on_actionDBremove_triggered()
{
    QStringList names = DB::getInstance().getSensorNames();
    bool ok;
    QString name = QInputDialog::getItem(this, "DB remove", "Select sensor to delete", names, 0, false, &ok);
    if(ok){
        DB::getInstance().removeSensor(name);
    }
}
