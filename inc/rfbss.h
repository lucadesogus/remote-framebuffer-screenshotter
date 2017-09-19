#ifndef RFBSS_H
#define RFBSS_H

#include <QMainWindow>


//#define NOMINMAX

#include <libssh/libssh.h>
#include <QImage>
#ifndef QT_NO_PRINTER
#include <QPrinter>
#endif

#include <mutex>
#include <QMetaType>
#include <QProgressBar>
#include <QAbstractItemDelegate>
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;


enum class conn_status
{
    DISCONNECTED,
    CONNECTED,
    IN_PROGRESS
};
Q_DECLARE_METATYPE(conn_status)

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
    void loadProfiles(const QString & = QString("default"));
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

    static void checkConnectionStatus_Thread(RFBSS * p_parent);

    std::mutex m_conn_status_mutex;

    std::mutex m_send_cmd_mutex;

    std::mutex m_capture_image_mutex;

    QProgressBar * m_progressBar;

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
    void onDetectedWidthHeight_received(int,int);
    void onShowStatusbarMessage_received(QString);
    void onConnectionStatus_received(const conn_status p_status);
    void onShowProgressBar_received(const bool & p_active);

    void onNewProfile_clicked();
    void onLoadProfile_clicked();
    void onSaveProfile_clicked();

    void onConnectSSH_clicked();

    void showProfileContextMenu(const QPoint &pos);

    void eraseProfile();

    void profileNameEditEnd(QWidget *editor, QAbstractItemDelegate::EndEditHint hint);

signals:
    void LogResult( QString);
    void setImageSignal(QImage,bool);
    void ShowMessageBox(int,QString,QString);
    void DetectedWidthHeight(int,int);
    void ShowStatusbarMessage(QString);
    void ConnectionStatus(const conn_status);
    void ShowProgressBar(const bool & p_active);
};



#endif // RFBSS_H
