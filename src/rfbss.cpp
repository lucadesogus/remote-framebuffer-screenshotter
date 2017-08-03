#include "rfbss.h"
#include "ui_rfbss.h"

#ifndef QT_NO_PRINTER
#include <QPrintDialog>
#endif
#include "qdebug.h"
#include <QtWidgets>
#include "clickable_label.h"

RFBSS::RFBSS(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RFBSS),
    m_con(NULL),
    scaleFactor(1)
{
    ui->setupUi(this);

    QObject::connect(ui->btn_connect, SIGNAL(clicked()), this, SLOT(onConnect_clicked()));
    QObject::connect(ui->btn_takeSnapshot, SIGNAL(clicked()), this, SLOT(onTakeSnapShot_clicked()));
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

//	QObject::connect(l_cmbBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onImageType_changed(int)));

    createActions();

    resize(QGuiApplication::primaryScreen()->availableSize() * 3 / 5);
}

RFBSS::~RFBSS()
{
    delete ui;
}


int RFBSS::send_remote_command(ssh_session session, QString & p_command ,QByteArray & p_result)
{
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
    int rc;
    char *password;
    int port = ui->txt_port->text().toInt();
    int verbosity = SSH_LOG_PROTOCOL;
    m_con = ssh_new();
    if (m_con == NULL)
    {
        ui->txt_result->appendHtml("NULL connection object. \n");
        return;
    }
    ssh_options_set(m_con, SSH_OPTIONS_HOST, ui->txt_host->text().toStdString().c_str());
    ssh_options_set(m_con, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    ssh_options_set(m_con, SSH_OPTIONS_PORT, &port);
    // Connect to server
    rc = ssh_connect(m_con);
    if (rc != SSH_OK)
    {
        //fprintf(stderr, "Error connecting to localhost: %s\n",
        //ssh_get_error(m_con));
        ui->txt_result->appendHtml("Error: " + QString::fromLatin1(ssh_get_error(m_con)) + " \n");
        //const char * l = ssh_get_error(m_con);
        ssh_free(m_con);
        return;
        //exit(-1);
    }

    rc = ssh_userauth_password(m_con,
        ui->txt_user->text().toStdString().c_str(),
        ui->txt_pw->text().toStdString().c_str()
        );
    if (rc != SSH_AUTH_SUCCESS)
    {
        //fprintf(stderr, "Authentication failed: %s\n",
        //	ssh_get_error(m_con));
        ui->txt_result->appendHtml("Authentication failed: " + QString::fromLatin1(ssh_get_error(m_con)) + " \n");
        ssh_free(m_con);
        return;
        //exit(-1);
    }
    else
    {

        //fprintf(stderr,"Authentication OK ");
        ui->txt_result->appendHtml("Connected. \n");
    }
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
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
            tr("Cannot write %1: %2")
            .arg(QDir::toNativeSeparators(fileName)), writer.errorString());
        return false;
    }
    const QString message = tr("Wrote \"%1\"").arg(QDir::toNativeSeparators(fileName));
    statusBar()->showMessage(message);
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
    QString l_cmd("cat /dev/fb0");
    QByteArray l_result;
    send_remote_command(m_con, l_cmd, l_result);
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
    int l_itemIndex = ui->cmbBoxBufType->currentIndex();
    if (l_itemIndex == -1)
    {
        qDebug() << "cmbBox index not found";
        return;
    }
    if (l_result.isEmpty())
    {
        QMessageBox::warning(this, QGuiApplication::applicationDisplayName(),
            tr("1) Connect.\n2) Capture."));
        return;
    }
    QImage::Format l_imgFormat = (QImage::Format)ui->cmbBoxBufType->itemData(l_itemIndex).toInt();
    int l_w = ui->spnBoxWidth->value();
    int l_h = ui->spnBoxHeight->value();
    QImage l_image(l_w, l_h, l_imgFormat);
    qDebug() << "image size = " << QString::number(l_image.byteCount()) << "\n";
    unsigned char * l_ptr = l_image.bits();
    for (unsigned long j = 0; j< l_result.size() && j <l_image.byteCount(); ++j)
    {
        l_ptr[j] = l_result.at(j);
    }
    setImage(l_image);
    //imageLabel->setPixmap(QPixmap::fromImage(image));

    qDebug() << "result size = " << QString::number(l_result.size()) << "depth = " << QString::number(l_result.size() / (640 * 480)) << "\n";

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

//void RFBSS::adaptScaleToShownFullImage()
//         int l_scrAreaW = scrollArea->width();
  //          int l_imgLblW = imageLabel-

//}

#if 0

ssh root@192.168.0.2 "cat /dev/fb1" | convert - depth 8 - size 1280x480 rgba:-[0] - alpha off - separate - swap 0, 2 - combine $(date + '%Y-%m-%d-%H-%M-%S-%N').png

#endif
