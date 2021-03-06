#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <QTimer>
#include <QMutex>


class NetSocket : public QUdpSocket
{
    Q_OBJECT

public:
    int myPort;
    int sendPort;   // port where I sent to
    int recvPort;   // port where I received from
    QHostAddress HostAddress;
    NetSocket(QObject *parent);
    QString mOriginName;
    // Bind this socket to a P2Papp-specific default port.
    bool bind();
    void writeUdp(const QVariantMap &map, int index);
    // bool readUdp(QVariantMap *map);
    int genRandNum();
	int getWritePort();
    ~NetSocket();

private:
    int myPortMin, myPortMax;
};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
    ChatDialog();

    void addToMessageList(int seqNo, const QString &text, QString &origin);
    
public slots:
	void gotReturnPressed();
    void gotReadyRead();
    void timeoutHandler();
    void antiEntropyHandler();

private:
    QMutex mutex;
    QMutex mutex2;
	QTextEdit *textview;
	QLineEdit *textline;
    NetSocket *mSocket;
    QTimer *mEntropyTimer;
    QTimer *mTimeoutTimer;
    QVariantMap mLastMsg;
    QMap<QString, QMap<quint32, QString> > mMessageList;
    QMap<QString, quint32> mLocalWants;
    void writeRumor(QString &origin, int seqNo, QString &text, bool append = true);
    //void processMessages(QVariantMap &map);
	void processRumor(QVariantMap &map);
    void processStatus(QMap<QString, QMap<QString, quint32> > &map);
	void appendToMessageList(QVariantMap &map);
	void writeStatus(int port);
};


#endif // P2PAPP_MAIN_HH
