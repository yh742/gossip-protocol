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

    // Bind this socket to a P2Papp-specific default port.
    bool bind();
    void writeUdp(const QVariantMap &map);
    void readUdp(QVariantMap *map);
    ~NetSocket();

private:
    int myPortMin, myPortMax;
    quint16 mPort;
    //QHostAddress *mHostAddress;
    QString mOriginName;
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
};


#endif // P2PAPP_MAIN_HH
