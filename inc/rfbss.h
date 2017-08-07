#ifndef RFBSS_H
#define RFBSS_H

#include <QMainWindow>


//#define NOMINMAX

#include <libssh/libssh.h>
#include <QImage>
#ifndef QT_NO_PRINTER
#include <QPrinter>
#endif

class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;


namespace Ui {
class RFBSS;
}

class RFBSS : public QMainWindow
{
    Q_OBJECT

public:
    explicit RFBSS(QWidget *parent = 0);
    ~RFBSS();

private:
    Ui::RFBSS *ui;

    ssh_session  m_con;
    int send_remote_command(ssh_session session, QString &, QByteArray & p_result);
    void initialChecks();
    void loadProfiles();
    void loadProfile(const QString & p_name);
    void createActions();
    void createMenus();
    void updateActions();
    bool saveFile(const QString &fileName);

    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);
    void appendToImageList(const QImage &newImage);
    bool fileExists(QString path);
    QImage image;
    QLabel *imageLabel;
    QScrollArea *scrollArea;
    double scaleFactor;

    static void  Connect(RFBSS * p_parent);

    static void TakeSnapshot(RFBSS * p_parent);

#ifndef QT_NO_PRINTER
    QPrinter printer;
#endif

    QAction *saveAsAct;
    QAction *printAct;
    QAction *copyAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *normalSizeAct;
    QAction *fitToWindowAct;

    private slots:
    void onConnect_clicked();
    void open();
    void saveAs();
    void print();
    void copy();
    void paste();
    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow();
    void about();

    void onTakeSnapShot_clicked();
    void onshotPreview_clicked();

    void onLogResult_received(QString l_string);

    bool loadFile(const QString &);
    void setImage(const QImage &newImage, const bool &  appendToList = true);
    void onShowMessageBox_received(int p_type, QString p_title, QString p_message) ;

    void onNewProfile_clicked();
    void onLoadProfile_clicked();
    void onSaveProfile_clicked();

signals:
    void LogResult( QString);
    void setImageSignal(QImage,bool);
    void ShowMessageBox(int,QString,QString);
};

#endif // RFBSS_H
