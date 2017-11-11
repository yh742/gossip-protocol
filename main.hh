#ifndef P2PAPP_MAIN_HH
#define P2PAPP_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>


class NetSocket : public QUdpSocket
{
    Q_OBJECT

public:
    NetSocket(QObject *parent);
    QString mOriginName;
    // Bind this socket to a P2Papp-specific default port.
    bool bind();
    void writeUdp(const QVariantMap &map, int index);
    void readUdp(QVariantMap *map);
    int genRandNum();
    ~NetSocket();

private:
    int myPortMin, myPortMax;
    quint16 mPort;
    QHostAddress mHostAddress;
    QVector<int> mPeerPorts;
};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
    ChatDialog();

public slots:
	void gotReturnPressed();
    void gotReadyRead();

private:
	QTextEdit *textview;
	QLineEdit *textline;
    NetSocket *mSocket;
    QMap<QString, QMap<quint32, QString> > mMessageList;
    QMap<QString, quint32> mLocalWants;
    void writeRumor(QString &origin, int seqNo, const QString &text);
};


#endif // P2PAPP_MAIN_HH
