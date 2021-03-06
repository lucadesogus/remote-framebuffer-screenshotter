#include "rfbss.h"
#include "ui_rfbss.h"

#ifndef QT_NO_PRINTER
#include <QPrintDialog>
#endif
#include "qdebug.h"
#include <QtWidgets>
#include "clickable_label.h"

#include <QSettings>
#include <algorithm>
#include <thread>

RFBSS::RFBSS(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RFBSS),
    m_con(NULL),
    scaleFactor(1),
    m_progressBar(NULL)
{
    ui->setupUi(this);

   // QObject::connect(ui->actionConnect, SIGNAL(triggered()), this, SLOT(onConnect_clicked()));
    QObject::connect(ui->actionCapture_FB, SIGNAL(triggered()), this, SLOT(onTakeSnapShot_clicked()));
    QObject::connect(this, SIGNAL(LogResult(QString)), this, SLOT(onLogResult_received(QString)));
    QObject::connect(this, SIGNAL(setImageSignal(QImage,bool)), this, SLOT(setImage(const QImage &,const bool &)));
    QObject::connect(this,SIGNAL(ShowMessageBox(int,QString,QString)),this,SLOT(onShowMessageBox_received(int,QString,QString)));
    QObject::connect(ui->btnLoadProfile,SIGNAL(clicked()),this,SLOT(onLoadProfile_clicked()));
    QObject::connect(ui->btnSaveProfile,SIGNAL(clicked()),this,SLOT(onSaveProfile_clicked()));
    QObject::connect(ui->btnNewProfile,SIGNAL(clicked()),this,SLOT(onNewProfile_clicked()));
    QObject::connect(ui->btnConnectSSH,SIGNAL(clicked()),this,SLOT(onConnectSSH_clicked()));

    ui->listProfiles->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->listProfiles, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showProfileContextMenu(QPoint)));
    connect(ui->listProfiles->itemDelegate(), SIGNAL(closeEditor(QWidget*, QAbstractItemDelegate::EndEditHint)), this, SLOT(profileNameEditEnd(QWidget*, QAbstractItemDelegate::EndEditHint)));

    QObject::connect(this,SIGNAL(DetectedWidthHeight(int,int)),this,SLOT(onDetectedWidthHeight_received(int,int)));

    QObject::connect(this, SIGNAL(ShowStatusbarMessage(QString)), this, SLOT(onShowStatusbarMessage_received(QString)));

     QObject::connect(this, SIGNAL(ConnectionStatus(const conn_status)), this, SLOT(onConnectionStatus_received(const conn_status)));

    QObject::connect(this, SIGNAL(ShowProgressBar(const bool &)), this, SLOT(onShowProgressBar_received(const bool & )));



     //	imageLabel = ui->imglabel;
    imageLabel = new QLabel;
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setScaledContents(true);
    //imageLabel->setAttribute(Qt::WA_TranslucentBackground);
    imageLabel->setStyleSheet("background-color: rgba(0,0,0,0%)");



    //scrollArea = ui->scrollArea;
    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    //scrollArea->setVisible(false);

    setCentralWidget(scrollArea);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumHeight(20);
    m_progressBar->setMaximumWidth(250);
    m_progressBar->setValue(0);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);
    m_progressBar->setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    m_progressBar->setStyleSheet("QProgressBar {border: 2px solid grey; border-radius: 5px;}"
                             "QProgressBar::chunk {background-color: #30729C;width: 25px;}");


    QComboBox * l_cmbBox = ui->cmbBoxBufType;
    l_cmbBox->addItem("ARGB 32", QImage::Format_ARGB32);
    l_cmbBox->addItem("ARGB 32 Premultiplied", QImage::Format_ARGB32_Premultiplied);
    l_cmbBox->addItem("ARGB 4444 Premultiplied", QImage::Format_ARGB4444_Premultiplied);
    l_cmbBox->addItem("ARGB 6666 Premultiplied", QImage::Format_ARGB6666_Premultiplied);
    l_cmbBox->addItem("ARGB 8555 Premultiplied", QImage::Format_ARGB8555_Premultiplied);
    l_cmbBox->addItem("ARGB 8565 Premultiplied", QImage::Format_ARGB8565_Premultiplied);
    l_cmbBox->addItem("Indexed 8", QImage::Format_Indexed8);
    l_cmbBox->addItem("Mono", QImage::Format_Mono);
    l_cmbBox->addItem("Mono LSB", QImage::Format_MonoLSB);
    l_cmbBox->addItem("RGB 16", QImage::Format_RGB16);
    l_cmbBox->addItem("RGB 32", QImage::Format_RGB32);
    l_cmbBox->addItem("RGB 444", QImage::Format_RGB444);
    l_cmbBox->addItem("RGB 555", QImage::Format_RGB555);
    l_cmbBox->addItem("RGB 666", QImage::Format_RGB666);
    l_cmbBox->addItem("RGB 888", QImage::Format_RGB888);
    l_cmbBox->addItem("RGBA 8888", QImage::Format_RGBA8888);
    l_cmbBox->addItem("RGBA 8888 Premultiplied", QImage::Format_RGBA8888_Premultiplied);
    l_cmbBox->addItem("RGBX 8888", QImage::Format_RGBX8888);


    l_cmbBox = ui->cmbBoxAutosaveFormat;
    l_cmbBox->addItems(QStringList()<<"jpeg"<< "bmp"<< "png");

//	QObject::connect(l_cmbBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onImageType_changed(int)));

    createActions();

   // resize(QGuiApplication::primaryScreen()->availableSize());// * 3 / 5);
    showMaximized();
    emit ConnectionStatus(conn_status::DISCONNECTED);
    initialChecks();
    loadProfiles();
}

RFBSS::~RFBSS()
{
    delete ui;
}

void RFBSS::initialChecks()
{
    //check if profiles folder exists, else create it with default profile default.profile
    QDir dir("profiles");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    if(!fileExists("profiles/default.profile"))
    {
        QSettings l_prof(QApplication::applicationDirPath() + "/profiles/default.profile", QSettings::IniFormat);
        //QString l_s (QApplication::applicationDirPath() + "/profiles/default.profile");
        l_prof.setValue("ip", "0.0.0.0");
        l_prof.setValue("port", 22);
        l_prof.setValue("username", "root");
        l_prof.setValue("password", "root");
        l_prof.setValue("FB", 0);
        l_prof.setValue("type", 0);
        l_prof.setValue("width", 620);
        l_prof.setValue("height", 480);
        l_prof.setValue("autodetect_wh", false);
        l_prof.setValue("autodetect_FB", 0);
        l_prof.setValue("autosave_screenshot", false);
        l_prof.setValue("autosave_screenshot_format", 0);
        l_prof.sync();
    }

    //check if screenshots folder exists, else create it
    QDir dir_ss("screenshots");
    if (!dir_ss.exists()) {
        dir_ss.mkpath(".");
    }
}

void RFBSS::loadProfiles(const QString & p_sel)
{
 QDir dir("profiles");
 QStringList l_list = dir.entryList(QDir::Files);
 l_list.erase(std::remove_if(l_list.begin(),
                           l_list.end(),
                           []( QString  x){return !x.endsWith(".profile");}),
            l_list.end());

 QStringList l_finalList;
 for ( const QString & l_item: l_list)
    l_finalList.append(l_item.mid(0,l_item.lastIndexOf('.')));
ui->listProfiles->clear();
 ui->listProfiles->addItems(l_finalList);

 loadProfile(p_sel);
 for(auto x = 0; x != l_finalList.size() ;++x)
 {
     ui->listProfiles->item(x)->setFlags (ui->listProfiles->item(x)->flags () | Qt::ItemIsEditable);
     ui->listProfiles->item(x)->setData(Qt::UserRole + 1 ,ui->listProfiles->item(x)->text()); //original name
     if(l_finalList.at(x) == p_sel)
     {
         ui->listProfiles->setItemSelected(ui->listProfiles->item(x),true);
        // break;
     }
 }

}

void RFBSS::loadProfile(const QString & p_name)
{
    QSettings l_profile (QApplication::applicationDirPath() + "/profiles/"+ p_name+ ".profile", QSettings::IniFormat);
    ui->txt_host->setText(l_profile.value("ip", "0.0.0.0").toString());
    ui->txt_port->setText(QString::number(l_profile.value("port", 22).toInt()));
    ui->txt_user->setText(l_profile.value("username", "root").toString());
    ui->txt_pw->setText(l_profile.value("password", "root").toString());
    ui->spnBox_FBnum->setValue(l_profile.value("FB", 0).toInt());
    ui->cmbBoxBufType->setCurrentIndex(l_profile.value("type", 0).toInt());
    ui->spnBoxWidth->setValue(l_profile.value("width", 620).toInt());
    ui->spnBoxHeight->setValue(l_profile.value("height", 480).toInt());
    ui->chkBoxAutodetectWH->setChecked(l_profile.value("autodetect_wh", false).toBool());
    ui->spnBox_FB_autodetectWH->setValue(l_profile.value("autodetect_FB", 0).toInt());
    ui->chkBoxAutosaveScreenshot->setChecked(l_profile.value("autosave_screenshot", false).toBool());
    ui->cmbBoxAutosaveFormat->setCurrentIndex(l_profile.value("autosave_screenshot_format", 0).toInt());
}


int RFBSS::send_remote_command(ssh_session session, QString & p_command ,QByteArray & p_result)
{
    std::lock_guard<std::mutex> l_guard(m_send_cmd_mutex);
    qDebug() << "send_remote_command \n";
    if (session == NULL)
        return -1;
    ssh_channel channel;
    int rc;
    char buffer[256];
    unsigned int nbytes;
    channel = ssh_channel_new(session);
    if (channel == NULL)
        return SSH_ERROR;
    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK)
    {
        ssh_channel_free(channel);
        return rc;
    }
    rc = ssh_channel_request_exec(channel, p_command.toStdString().c_str());
    if (rc != SSH_OK)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return rc;
    }
    nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    while (nbytes > 0)
    {
    //	ui->txt_result->insertPlainText(buffer);
        p_result.append(buffer, nbytes);
        nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    }

    if (nbytes < 0)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return SSH_ERROR;
    }
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    qDebug() << "send_remote_command end\n";
    return SSH_OK;
}

void RFBSS::onConnect_clicked()
{
    qDebug() << "onConnect_clicked \n";
    std::thread (Connect,this).detach();
}

void  RFBSS::Connect(RFBSS * p_parent)
{
    qDebug() << "Connect \n";
    emit p_parent->ConnectionStatus(conn_status::IN_PROGRESS);
    QThread::msleep(500);
     p_parent->m_conn_status_mutex.lock();
    int rc;
    char *password;
    int port = p_parent->ui->txt_port->text().toInt();
    int verbosity = SSH_LOG_PROTOCOL;
    if(p_parent->m_con == NULL)
    {
        ssh_free(p_parent->m_con);
        p_parent->m_con = NULL;
    }
    p_parent->m_con = ssh_new();
    if (p_parent->m_con == NULL)
    {
        emit p_parent->LogResult("NULL connection object. \n");
        //ui->txt_result->appendHtml("NULL connection object. \n");
         emit p_parent->ConnectionStatus(conn_status::DISCONNECTED);
         p_parent->m_conn_status_mutex.unlock();
        return;
    }
    ssh_options_set(p_parent->m_con, SSH_OPTIONS_HOST, p_parent->ui->txt_host->text().toStdString().c_str());
    ssh_options_set(p_parent->m_con, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(p_parent->m_con, SSH_OPTIONS_PORT, &port);
    // Connect to server
    rc = ssh_connect(p_parent->m_con);
    if (rc != SSH_OK)
    {
        //fprintf(stderr, "Error connecting to localhost: %s\n",
        //ssh_get_error(m_con));
        //ui->txt_result->appendHtml("Error: " + QString::fromLatin1(ssh_get_error(m_con)) + " \n");
        emit p_parent->LogResult("Error: " + QString::fromLatin1(ssh_get_error(p_parent->m_con)) + " \n");
        //const char * l = ssh_get_error(m_con);
        ssh_free(p_parent->m_con);
        p_parent->m_con = NULL;
        emit p_parent->ConnectionStatus(conn_status::DISCONNECTED);
        p_parent->m_conn_status_mutex.unlock();
        return;
        //exit(-1);
    }

    rc = ssh_userauth_password(p_parent->m_con,
        p_parent->ui->txt_user->text().toStdString().c_str(),
       p_parent-> ui->txt_pw->text().toStdString().c_str()
        );
    if (rc != SSH_AUTH_SUCCESS)
    {
        //fprintf(stderr, "Authentication failed: %s\n",
        //	ssh_get_error(m_con));
        emit p_parent->LogResult("Authentication failed: " + QString::fromLatin1(ssh_get_error(p_parent->m_con)) + " \n");
        //ui->txt_result->appendHtml("Authentication failed: " + QString::fromLatin1(ssh_get_error(m_con)) + " \n");
        ssh_free(p_parent->m_con);
        p_parent->m_con = NULL;
         emit p_parent->ConnectionStatus(conn_status::DISCONNECTED);
        p_parent->m_conn_status_mutex.unlock();
        return;
        //exit(-1);
    }
    else
    {

        //fprintf(stderr,"Authentication OK ");
        emit p_parent->LogResult("Connected. \n");
       // ui->txt_result->appendHtml("Connected. \n");

        if(p_parent->ui->chkBoxAutodetectWH->isChecked())
        {
            //detect w and h
            int l_fbd = p_parent->ui->spnBox_FB_autodetectWH->value();
            QString l_cmd("cat /sys/class/graphics/fb"+
                          QString::number(l_fbd)+
                          "/virtual_size");
            QByteArray l_res;
            if(0 == p_parent->send_remote_command(p_parent->m_con,l_cmd,l_res))
            {
                QString l_str(l_res);
                QStringList l_wh = l_str.split(',');
                if(l_wh.size() == 2)
                {
                    int l_detW = l_wh.first().toInt();
                    int l_detH = l_wh.back().toInt();
                    emit p_parent->LogResult("Detected FB"+ QString::number(l_fbd) + " res.: "+ l_str.replace(",","x"));
                    emit p_parent->DetectedWidthHeight(l_detW,l_detH);
                }
            }
        }
        emit p_parent->ConnectionStatus(conn_status::CONNECTED);
        // thread that check the connection status
        std::thread (checkConnectionStatus_Thread,p_parent).detach();
    }
    p_parent->m_conn_status_mutex.unlock();
}

bool RFBSS::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    //reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if (newImage.isNull()) {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
            tr("Cannot load %1: %2")
            .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    setImage(newImage);

    setWindowFilePath(fileName);

    const QString message = tr("Opened \"%1\", %2x%3, Depth: %4")
        .arg(QDir::toNativeSeparators(fileName)).arg(image.width()).arg(image.height()).arg(image.depth());
    statusBar()->showMessage(message);
    return true;
}

void RFBSS::setImage(const QImage &newImage, const bool & appendToList)
{
    image = newImage;
    imageLabel->setPixmap(QPixmap::fromImage(image));
    //imageLabel->set.scaled(scrollArea->width(),scrollArea->height(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    scaleFactor = 1.0;
    scrollArea->setVisible(true);
    printAct->setEnabled(true);
    fitToWindowAct->setEnabled(true);
    updateActions();
    if (!fitToWindowAct->isChecked())
        imageLabel->adjustSize();

   /* while (scrollArea->width() < imageLabel->width())
    {
        scaleImage(0.9);
    }*/
    int l_scrAreaW = scrollArea->width();
    int l_imgLblW = imageLabel->width();
    if(l_scrAreaW< l_imgLblW)
    {
        double l_div = 1.0/((double)l_imgLblW / (double)l_scrAreaW);
        scaleImage(l_div);
    }
    if (appendToList)
        appendToImageList(newImage);
}


bool RFBSS::saveFile(const QString &fileName)
{
    QImageWriter writer(fileName);

    if (!writer.write(image)) {
        emit ShowMessageBox(QMessageBox::Information,
                            QGuiApplication::applicationDisplayName(),
                            "Cannot write "+ QDir::toNativeSeparators(fileName) + " :" +writer.errorString());
        //QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
         //   tr("Cannot write %1: %2")
         //   .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    emit ShowStatusbarMessage(message);
   // statusBar()->showMessage(message);
    return true;
}


static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QList<QByteArray> supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
        ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach(const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/jpeg");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("jpg");
}

void RFBSS::open()
{
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void RFBSS::saveAs()
{
    QFileDialog dialog(this, tr("Save File As"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptSave);

    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first())) {}
}

void RFBSS::print()
{
    Q_ASSERT(imageLabel->pixmap());
#if !defined(QT_NO_PRINTER) && !defined(QT_NO_PRINTDIALOG)
    QPrintDialog dialog(&printer, this);
    if (dialog.exec()) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = imageLabel->pixmap()->size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(imageLabel->pixmap()->rect());
        painter.drawPixmap(0, 0, *imageLabel->pixmap());
    }
#endif
}

void RFBSS::copy()
{
#ifndef QT_NO_CLIPBOARD
    QGuiApplication::clipboard()->setImage(image);
#endif // !QT_NO_CLIPBOARD
}

#ifndef QT_NO_CLIPBOARD
static QImage clipboardImage()
{
    if (const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData()) {
        if (mimeData->hasImage()) {
            const QImage image = qvariant_cast<QImage>(mimeData->imageData());
            if (!image.isNull())
                return image;
        }
    }
    return QImage();
}
#endif // !QT_NO_CLIPBOARD

void RFBSS::paste()
{
#ifndef QT_NO_CLIPBOARD
    const QImage newImage = clipboardImage();
    if (newImage.isNull()) {
        statusBar()->showMessage(tr("No image in clipboard"));
    }
    else {
        setImage(newImage);
        setWindowFilePath(QString());
        const QString message = tr("Obtained image from clipboard, %1x%2, Depth: %3")
            .arg(newImage.width()).arg(newImage.height()).arg(newImage.depth());
        statusBar()->showMessage(message);
    }
#endif // !QT_NO_CLIPBOARD
}

void RFBSS::zoomIn()
{
    scaleImage(1.25);
}

void RFBSS::zoomOut()
{
    scaleImage(0.8);
}

void RFBSS::normalSize()
{
    imageLabel->adjustSize();
    scaleFactor = 1.0;
}

void RFBSS::fitToWindow()
{
    bool fitToWindow = fitToWindowAct->isChecked();
    scrollArea->setWidgetResizable(fitToWindow);
    if (!fitToWindow)
        normalSize();
    updateActions();
}

void RFBSS::about()
{
    QMessageBox::about(this, tr("About RemoteFrameBufferScreenShotter"),
        tr("<p><b>RemoteFrameBufferScreenShotter</b> is Freeware, and as such, you should not have been charged to obtain this software</p>"

        "<p>You may redistribute RemoteFrameBufferScreenShotter as long as it is not modified in any way.</p>"
        "<p>The author accepts no responsibility for damages resulting from the use of this product and makes no warranty or representation, either express or implied, including but not limited to, any implied warranty of merchantability or fitness for a particular purpose.</p>"
        "<p>This software is provided \"AS IS\", and you, its user, assume all risks when using it.</p>"
        "<p>Author: Luca Desogus  &lt;luca.desogus@outlook.com&gt;"
        "</p>"));
}

void RFBSS::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAct = fileMenu->addAction(tr("&Open..."));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));
    openAct->setShortcut(QKeySequence::Open);

    saveAsAct = fileMenu->addAction(tr("&Save As..."));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(saveAs()));
    saveAsAct->setEnabled(false);

    printAct = fileMenu->addAction(tr("&Print..."));
    connect(printAct, SIGNAL(triggered()), this, SLOT(print()));
    printAct->setShortcut(QKeySequence::Print);
    printAct->setEnabled(false);

    fileMenu->addSeparator();

    QAction *exitAct = fileMenu->addAction(tr("E&xit"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));
    exitAct->setShortcut(tr("Ctrl+Q"));

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    copyAct = editMenu->addAction(tr("&Copy"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(copy()));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);

    QAction *pasteAct = editMenu->addAction(tr("&Paste"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(paste()));
    pasteAct->setShortcut(QKeySequence::Paste);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    zoomInAct = viewMenu->addAction(tr("Zoom &In (25%)"));
    connect(zoomInAct, SIGNAL(triggered()), this, SLOT(zoomIn()));
    zoomInAct->setShortcut(QKeySequence::ZoomIn);
    zoomInAct->setEnabled(false);

    zoomOutAct = viewMenu->addAction(tr("Zoom &Out (25%)"));
    connect(zoomOutAct, SIGNAL(triggered()), this, SLOT(zoomOut()));
    zoomOutAct->setShortcut(QKeySequence::ZoomOut);
    zoomOutAct->setEnabled(false);

    normalSizeAct = viewMenu->addAction(tr("&Normal Size"));
    connect(normalSizeAct, SIGNAL(triggered()), this, SLOT(normalSize()));
    normalSizeAct->setShortcut(tr("Ctrl+S"));
    normalSizeAct->setEnabled(false);

    viewMenu->addSeparator();

    fitToWindowAct = viewMenu->addAction(tr("&Fit to Window"));
    connect(fitToWindowAct, SIGNAL(triggered()), this, SLOT(fitToWindow()));
    fitToWindowAct->setEnabled(false);
    fitToWindowAct->setCheckable(true);
    fitToWindowAct->setShortcut(tr("Ctrl+F"));

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction * l_aboutAct = helpMenu->addAction(tr("&About"));
    connect(l_aboutAct, SIGNAL(triggered()), this, SLOT(about()));
    QAction * l_aboutQTAct = helpMenu->addAction(tr("About &Qt"));
    connect(l_aboutQTAct, SIGNAL(triggered()), QApplication::instance(), SLOT(aboutQt()));


}

void RFBSS::updateActions()
{
    saveAsAct->setEnabled(!image.isNull());
    copyAct->setEnabled(!image.isNull());
    zoomInAct->setEnabled(!fitToWindowAct->isChecked());
    zoomOutAct->setEnabled(!fitToWindowAct->isChecked());
    normalSizeAct->setEnabled(!fitToWindowAct->isChecked());
}

void RFBSS::scaleImage(double factor)
{
    Q_ASSERT(imageLabel->pixmap());
    scaleFactor *= factor;
    imageLabel->resize(scaleFactor * imageLabel->pixmap()->size());

    adjustScrollBar(scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(scrollArea->verticalScrollBar(), factor);

    zoomInAct->setEnabled(scaleFactor < 3.0);
    zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void RFBSS::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value()
        + ((factor - 1) * scrollBar->pageStep() / 2)));
}

void RFBSS::onTakeSnapShot_clicked()
{
    qDebug() << "onTakeSnapShot_clicked \n";
    std::thread (TakeSnapshot,this).detach();
}

void RFBSS::TakeSnapshot(RFBSS * p_parent)
{
    std::lock_guard<std::mutex> l_guard (p_parent->m_capture_image_mutex);
    qDebug() << "TakeSnapshot \n";
    emit p_parent->ShowProgressBar(true);
    QString l_cmd("cat /dev/fb" + QString::number(p_parent->ui->spnBox_FBnum->value()));
    QByteArray l_result;
    p_parent->send_remote_command(p_parent->m_con, l_cmd, l_result);
    emit p_parent->ShowProgressBar(false);
    //QImage l_img((unsigned char *)l_result.data_ptr(), 1280, 480, QImage::Format_RGB32);// = QImage::fromData(l_result);
    /*
    QImage image(1280, 480, QImage::Format_RGB32);

    unsigned char * l_ptr = image.bits();

    for (unsigned long i = 0; i < 1280; ++i)
    {
        for (unsigned long j = 0; j< 480; ++j)
        {
            unsigned long l_pos = i*j;
            l_ptr[i*j]= l_result.
        }
    }
    */
    int l_itemIndex = p_parent->ui->cmbBoxBufType->currentIndex();
    if (l_itemIndex == -1)
    {
        qDebug() << "cmbBox index not found";
        return;
    }
    if (l_result.isEmpty())
    {
        emit p_parent->ShowMessageBox(QMessageBox::Warning,QGuiApplication::applicationDisplayName(),
                           tr("1) Connect.\n2) Capture.") );
        return;
    }
    QImage::Format l_imgFormat = (QImage::Format)p_parent->ui->cmbBoxBufType->itemData(l_itemIndex).toInt();
    int l_w = p_parent->ui->spnBoxWidth->value();
    int l_h =p_parent-> ui->spnBoxHeight->value();
    QImage l_image(l_w, l_h, l_imgFormat);
    qDebug() << "image size = " << QString::number(l_image.byteCount()) << "\n";
    unsigned char * l_ptr = l_image.bits();
    for (long j = 0; j< l_result.size() && j <l_image.byteCount(); ++j)
    {
        l_ptr[j] = l_result.at(j);
    }
    emit p_parent->setImageSignal(l_image,true);
    //imageLabel->setPixmap(QPixmap::fromImage(image));

    qDebug() << "result size = " << QString::number(l_result.size()) << "depth = " << QString::number(l_result.size() / (640 * 480)) << "\n";

    if(p_parent->ui->chkBoxAutosaveScreenshot->isChecked())
    {
       QString l_filename = QApplication::applicationDirPath() + "/screenshots/" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmsszzz.") + p_parent->ui->cmbBoxAutosaveFormat->currentText();
        p_parent->saveFile(l_filename);
    }
}

void RFBSS::appendToImageList(const QImage &newImage)
{
    QVBoxLayout * l_lay = (QVBoxLayout*)ui->scrollAreaShotsList_widget->layout();
    ClickableLabel * l_lbl = new ClickableLabel;
    l_lbl->setScaledContents(true);
    l_lbl->setFixedWidth(150);
    l_lbl->setFixedHeight(100);
    l_lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    l_lbl->setPixmap(QPixmap::fromImage(image).scaled(150,100,Qt::KeepAspectRatio));
    l_lay->addWidget(l_lbl);
    l_lbl->setUserData(0, (QObjectUserData *)new QImage(newImage));
    QObject::connect(l_lbl, SIGNAL(clicked()), this, SLOT(onshotPreview_clicked()));
}

void RFBSS::onshotPreview_clicked()
{
    ClickableLabel * l_sender = (ClickableLabel*)QObject::sender();
    if (NULL != l_sender)
    {
        QImage * l_imgPtr = (QImage*)l_sender->userData(0);
        setImage(*l_imgPtr,false);
    }
}

bool RFBSS::fileExists(QString path) {
    QFileInfo check_file(path);
    // check if file exists and if yes: Is it really a file and no directory?
    if (check_file.exists() && check_file.isFile()) {
        return true;
    } else {
        return false;
    }
}

//void RFBSS::adaptScaleToShownFullImage()
//         int l_scrAreaW = scrollArea->width();
  //          int l_imgLblW = imageLabel-

//}

void RFBSS::onLogResult_received(QString l_string)
{
    ui->txt_result->appendHtml(l_string);
}


void RFBSS::onShowMessageBox_received(int p_type, QString p_title, QString p_message)
{
    switch (p_type) {
    case QMessageBox::Warning:
    {
        QMessageBox::warning(this, p_title, p_message);
    }
        break;
    default:
    {
        QMessageBox::information(this, p_title, p_message);
    }
        break;
    }

}

void RFBSS::onNewProfile_clicked()
{
    QDir dir("profiles");
    QStringList l_list = dir.entryList(QDir::Files);
    QString l_newItem ("new");
    int l_counter = 0;
    while(std::find(l_list.begin(),l_list.end(),l_newItem + ".profile") != l_list.end())
    {
        l_counter ++;
        l_newItem = "new_" + QString::number(l_counter);
    }

    QSettings l_prof(QApplication::applicationDirPath() + "/profiles/" + l_newItem +".profile", QSettings::IniFormat);
    l_prof.setValue("ip", ui->txt_host->text());
    l_prof.setValue("port", ui->txt_port->text());
    l_prof.setValue("username", ui->txt_user->text());
    l_prof.setValue("password", ui->txt_pw->text());
    l_prof.setValue("FB", ui->spnBox_FBnum->value());
    l_prof.setValue("type", ui->cmbBoxBufType->currentIndex());
    l_prof.setValue("width", ui->spnBoxWidth->value());
    l_prof.setValue("height", ui->spnBoxHeight->value());
    l_prof.setValue("autodetect_wh", ui->chkBoxAutodetectWH->isChecked());
    l_prof.setValue("autodetect_FB", ui->spnBox_FB_autodetectWH->value());
    l_prof.setValue("autosave_screenshot", ui->chkBoxAutosaveScreenshot->isChecked());
    l_prof.setValue("autosave_screenshot_format", ui->cmbBoxAutosaveFormat->currentIndex());
    l_prof.sync();

    loadProfiles(l_newItem);

}

void RFBSS::onLoadProfile_clicked()
{
    if(ui->listProfiles->currentItem() != NULL)
    {
         QString l_item = ui->listProfiles->currentItem()->text();
         loadProfile(l_item);
    }
    else
    {
        emit LogResult("Select profile.");
    }

}

void RFBSS::onSaveProfile_clicked()
{
    if(ui->listProfiles->currentItem() != NULL)
    {
         QString l_item = ui->listProfiles->currentItem()->text();
         QSettings l_prof(QApplication::applicationDirPath() + "/profiles/" + l_item +".profile", QSettings::IniFormat);
         l_prof.setValue("ip", ui->txt_host->text());
         l_prof.setValue("port", ui->txt_port->text());
         l_prof.setValue("username", ui->txt_user->text());
         l_prof.setValue("password", ui->txt_pw->text());
         l_prof.setValue("FB", ui->spnBox_FBnum->value());
         l_prof.setValue("type", ui->cmbBoxBufType->currentIndex());
         l_prof.setValue("width", ui->spnBoxWidth->value());
         l_prof.setValue("height", ui->spnBoxHeight->value());
         l_prof.setValue("autodetect_wh", ui->chkBoxAutodetectWH->isChecked());
         l_prof.setValue("autodetect_FB", ui->spnBox_FB_autodetectWH->value());
         l_prof.setValue("autosave_screenshot", ui->chkBoxAutosaveScreenshot->isChecked());
         l_prof.setValue("autosave_screenshot_format", ui->cmbBoxAutosaveFormat->currentIndex());
         l_prof.sync();
    }
    else
    {
        emit LogResult("Select profile.");
    }
}

void RFBSS::onDetectedWidthHeight_received(int p_w,int p_h)
{
    ui->spnBoxWidth->setValue(p_w);
    ui->spnBoxHeight->setValue(p_h);
}

void RFBSS::onShowStatusbarMessage_received(QString p_m)
{
    statusBar()->showMessage(p_m);
}


void RFBSS::checkConnectionStatus_Thread(RFBSS * p_parent)
{
    qDebug() << "checkConnectionStatus_Thread launched \n";
    if(p_parent == NULL)
    {
        qWarning() << "checkConnectionStatus_Thread:: p_parent == NULL";
        return;
    }
    for(;;)
    {
        p_parent->m_conn_status_mutex.lock();
        if(p_parent->m_con != NULL)
        {
            if(ssh_is_connected(p_parent->m_con))
            {
                qDebug() << "checkConnectionStatus_Thread:: ssh is connected \n";
                QByteArray l_result;
                QString l_cmd ("echo ping");
                p_parent->send_remote_command(p_parent->m_con, l_cmd, l_result);
                if (l_result.isEmpty() || !QString(l_result).startsWith("ping"))
                {
                    qDebug() << "checkConnectionStatus_Thread:: disconnession detected \n";
                    emit p_parent->LogResult("Disconnected. \n");
                    emit p_parent->ConnectionStatus(conn_status::DISCONNECTED);

                   break;
                }
                qDebug() << "checkConnectionStatus_Thread:: ssh is connected \n";
                p_parent->m_conn_status_mutex.unlock();
                QThread::msleep(2000);
            }
            else
            {
                 qDebug() << "checkConnectionStatus_Thread:: disconnession detected \n";
                break;
            }
        }
        else
        {
            qWarning() << "checkConnectionStatus_Thread:: p_parent->m_con == NULL";
            break;
        }
    }
    p_parent->m_conn_status_mutex.unlock();
}


void RFBSS::onConnectionStatus_received(const conn_status p_status)
{
    qDebug() << "onConnectionStatus_received \n";
    switch (p_status)
    {
        case conn_status::CONNECTED:
        {
            ui->btnConnectSSH->setEnabled(true);
            ui->btnConnectSSH->setText("Disconnect");
            ui->lbl_connStatus->setText("Connected");
            ui->lbl_connStatus->setStyleSheet("font:bold; color:darkgreen;");
            m_progressBar->hide();
        }
        break;
        case conn_status::DISCONNECTED:
        {
            ui->btnConnectSSH->setEnabled(true);
            ui->btnConnectSSH->setText("Connect");
            ui->lbl_connStatus->setText("Disconnected");
            ui->lbl_connStatus->setStyleSheet("font:bold; color:darkred;");
            m_progressBar->hide();
        }
        break;
        case conn_status::IN_PROGRESS:
        {
            ui->btnConnectSSH->setEnabled(false);
            emit ShowProgressBar(true);
        }
        break;
        default:
        break;
    }
}

void RFBSS::onConnectSSH_clicked()
{
     qDebug() << "onConnectSSH_clicked \n";
     m_conn_status_mutex.lock();
     if(m_con == NULL || !ssh_is_connected(m_con))
     {
         //connect ssh
         std::thread (Connect,this).detach();
     }
     else
     {
         //disconnect ssh
         ssh_disconnect(m_con);
         emit ConnectionStatus(conn_status::DISCONNECTED);
     }
     m_conn_status_mutex.unlock();
}

void RFBSS::onShowProgressBar_received(const bool & p_active)
{
    qDebug() << "onShowProgressBar_received \n";
    if(m_progressBar)
    {
        if(p_active)
        {
            QPoint l_p = (this->rect().center());
            m_progressBar->setGeometry(l_p.x() - 125,l_p.y() -10,250,20);
            m_progressBar->show();
        }
        else
        {
            m_progressBar->hide();
        }
    }
}

void RFBSS::showProfileContextMenu(const QPoint &pos)
{
    QPoint globalPos = ui->listProfiles->mapToGlobal(pos);
    QMenu myMenu;
    myMenu.addAction("Erase",  this, SLOT(eraseProfile()));
    myMenu.exec(globalPos);
}

void RFBSS::eraseProfile()
{

}

void RFBSS::profileNameEditEnd(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)
{
    if(editor)
    {
        QString  NewValue = reinterpret_cast<QLineEdit*>(editor)->text();
        qDebug()<<NewValue;
        bool l_isNewName = true;
        for (int i = 0; i != ui->listProfiles->count(); ++i)
        {
            if(!ui->listProfiles->item(i)->isSelected() && NewValue == ui->listProfiles->item(i)->text())
            {
                l_isNewName = false;
                break;
            }
        }

        auto * l_item = ui->listProfiles->selectedItems().back();
        if(l_item)
        {
            QString  oldValue = l_item->data(Qt::UserRole +1).toString();
            qDebug()<<oldValue;
            if(l_isNewName &&QFile::rename("profiles/" + oldValue + ".profile","profiles/" + NewValue + ".profile"))
            {
                l_item->setData(Qt::UserRole +1, NewValue);
            }
            else
            {
                l_item->setText(oldValue);
            }

        }
    }


}


#if 0

ssh root@192.168.0.2 "cat /dev/fb1" | convert - depth 8 - size 1280x480 rgba:-[0] - alpha off - separate - swap 0, 2 - combine $(date + '%Y-%m-%d-%H-%M-%S-%N').png

#endif
