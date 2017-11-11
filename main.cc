
#include <unistd.h>
#include <QHostInfo>
#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QHostAddress>
#include <QtGlobal>
#include <QDateTime>

#include "main.hh"

ChatDialog::ChatDialog()
{
	setWindowTitle("P2Papp");

	// Read-only text box where we display messages from everyone.
	// This widget expands both horizontally and vertically.
	textview = new QTextEdit(this);
	textview->setReadOnly(true);

	// Small text-entry box the user can enter messages.
	// This widget normally expands only horizontally,
	// leaving extra vertical space for the textview widget.
	//
	// You might change this into a read/write QTextEdit,
	// so that the user can easily enter multi-line messages.
	textline = new QLineEdit(this);

	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(textview);
	layout->addWidget(textline);
	setLayout(layout);

	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));

    // User code starts here
    // Make sure Dialog has access to socket
    mSocket = new NetSocket(this);
    if (!mSocket->bind())
        exit(1);
    mLocalWants.insert(mSocket->mOriginName, 0);
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(gotReadyRead()));
}

// Slot for receiving messages
void ChatDialog::gotReadyRead()
{
    QVariantMap qMap;
    mSocket->readUdp(&qMap);
    QVariantMap::iterator i;
    for (i = qMap.begin(); i != qMap.end(); ++i)
    {
        qDebug() << i.key() << ", " << i.value().toString();
        QString oStr = i.key() + "," + i.value().toString();
        textview->append(oStr);
    }
}

void ChatDialog::writeRumor(QString &origin, int seqNo, const QString &text)
{
    QVariantMap qMap;
    qMap["ChatText"] = text;
    qMap["Origin"] = origin;
    qMap["SeqNo"] = seqNo;
    // Update tracking variables
    if (mMessageList.contains(origin))
    {
        if (!mMessageList[origin].contains(seqNo))
        {
            // Only add to messagelist if it hasn't been written
            mMessageList[origin].insert(seqNo, text);
        }
    }
    else
    {
        QMap<quint32, QString> tMap;
        tMap.insert(seqNo, text);
        mMessageList.insert(origin, tMap);
    }
    int index = (mSocket->genRandNum()) % 2;
    mSocket->writeUdp(qMap, index);
}

// Slot for enter messages
void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...
	qDebug() << "FIX: send message to other peers: " << textline->text();
	textview->append(textline->text());

    // Spread user enetered rumors
    writeRumor(mSocket->mOriginName, mLocalWants[mSocket->mOriginName], textline->text());
    mLocalWants[mSocket->mOriginName]++;

    // Clear the textline to get ready for the next input message.
    textline->clear();
}

int NetSocket::genRandNum()
{
    QDateTime current = QDateTime::currentDateTime();
    uint msecs = current.toTime_t();
    qsrand(msecs);
    return qrand();
}

NetSocket::NetSocket(QObject *parent = NULL) : QUdpSocket(parent)
{
	// Pick a range of four UDP ports to try to allocate by default,
	// computed based on my Unix user ID.
	// This makes it trivial for up to four P2Papp instances per user
	// to find each other on the same host,
	// barring UDP port conflicts with other applications
	// (which are quite possible).
	// We use the range from 32768 to 49151 for this purpose.
	myPortMin = 32768 + (getuid() % 4096)*4;
	myPortMax = myPortMin + 3;

    // Get host address
    mHostAddress = QHostAddress(QHostAddress::LocalHost);
    qDebug() << mHostAddress.toString();
    QHostInfo info;
    mOriginName = info.localHostName() + "-" + QString::number(genRandNum());
    qDebug() << mOriginName;
}

NetSocket::~NetSocket()
{
    //delete mHostAddress;
}

bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
            if (p == myPortMin)
            {
                mPeerPorts.append(p + 1);
            }
            else if (p == myPortMax)
            {
                mPeerPorts.append(p - 1);
            }
            else
            {
                mPeerPorts.append(p - 1);
                mPeerPorts.append(p + 1);
            }
			qDebug() << "bound to UDP port " << p;
            mPort = p;
            for (int i = 0; i < mPeerPorts.size(); i++)
            {
                qDebug() << "Peer Port: " << mPeerPorts[i];
            }
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}

// Serializes and writes to UDP socket
// @param map - readonly ref to map to be written
void NetSocket::writeUdp(const QVariantMap &map, int index)
{
    QByteArray wBytes;
    QDataStream out(&wBytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);
    out << map;
    if (mPeerPorts.size() == 1)
    {
        this->writeDatagram(wBytes, mHostAddress, mPeerPorts[0]);
    }
    else
    {
        this->writeDatagram(wBytes, mHostAddress, mPeerPorts[index]);
    }
}


// Deserialize and read from UDP socket
// @param map - pointer to map to store data
void NetSocket::readUdp(QVariantMap* map)
{
    //while(this->hasPendingDatagrams())
    //{
    QByteArray buf(this->pendingDatagramSize(), Qt::Uninitialized);
    QDataStream str(&buf, QIODevice::ReadOnly);
    this->readDatagram(buf.data(), buf.size(), &(this->mHostAddress), &(this->mPort));
    str >> (*map);
    //}
}


int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

    // Create a UDP network socket
    //NetSocket sock;
    //if (!sock.bind())
    //    exit(1);

	// Create an initial chat dialog window
    ChatDialog dialog;
    dialog.show();

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

