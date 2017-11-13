
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

    // User code starts here
    // Make sure Dialog has access to socket
    mSocket = new NetSocket(this);
    if (!mSocket->bind())
        exit(1);

    // initialize timers
    mTimeoutTimer = new QTimer(this);
    mEntropyTimer = new QTimer(this);
    mEntropyTimer->start(10 * 1000);

    // attached SIGNALS to SLOTS
    // Timer based events
    connect(mTimeoutTimer, SIGNAL(timeout()),
            this, SLOT(timeoutHandler()));
    connect(mEntropyTimer, SIGNAL(timeout()),
            this, SLOT(antiEntropyHandler()));

    // Input and output based events
    connect(textline, SIGNAL(returnPressed()),
            this, SLOT(gotReturnPressed()));
    connect(mSocket, SIGNAL(readyRead()),
            this, SLOT(gotReadyRead()));\
}

// if timer fires, send out status message
void ChatDialog::antiEntropyHandler()
{
    qDebug() << "AntiEntropyHandler called";
    writeStatus(mSocket->getWritePort());
    mEntropyTimer->start(10 * 1000);
}

// if timer fires, resend last message
void ChatDialog::timeoutHandler()
{
    qDebug() << "TimeoutHandler called";
    if (!mLastMsg.isEmpty() && mLastMsg.contains("ChatText"))
    {
        mSocket->writeUdp(mLastMsg, mSocket->sendPort);
        mTimeoutTimer->start(1000);
    }
}

// Slot for receiving messages
void ChatDialog::gotReadyRead()
{
    QVariantMap qMap;
    QMap<QString, QMap<QString, quint32> > sMap;
    QHostAddress sendingAddr;
    quint16 temPort;
    do
    {
        //pending = mSocket->readUdp(&qMap);
        QByteArray buf(mSocket->pendingDatagramSize(), Qt::Uninitialized);
        mSocket->readDatagram(buf.data(), buf.size(), &sendingAddr, &temPort);
        QDataStream str(&buf, QIODevice::ReadOnly);
        mSocket->recvPort = temPort;
        str >> (qMap);
        if (qMap.contains("Want"))
        {
            QDataStream str2(&buf, QIODevice::ReadOnly);
            str2 >> (sMap);
            // received status message stop
            qDebug() << sMap;
            qDebug() << "Received a status message";
            processStatus(sMap);
        }
        else if(qMap.contains("ChatText"))
        {
            qDebug() << "Received a rumor message";
            processRumor(qMap);
        }
        else
        {
            qDebug() << "Datagram contains neither messages";
            qDebug() << qMap;
        }
        mTimeoutTimer->stop();
    }
    while(mSocket->hasPendingDatagrams());
}

//void ChatDialog::processMessages(QVariantMap &qMap)
//{
//}

void ChatDialog::processRumor(QVariantMap &rMap)
{
    QString origin = rMap["Origin"].toString();
    quint32 seqNo = rMap["SeqNo"].toInt();
    QString text = rMap["ChatText"].toString();

    // Only process message that doesn't belong to us
    if (origin != mSocket->mOriginName)
    {
        if (mLocalWants.contains(origin))
        {
            // Only append if it is the seqNo we want
            if (seqNo == mLocalWants[origin])
            {
                mLocalWants[origin]++;
                writeRumor(origin, seqNo, text);

            }
            // Discard seen or out of order packets
        }
        else
        {
            // If it is a new host just add the newest msg
            // Insert new host address to map
            //mLocalWants.insert(origin, seqNo + 1);
            mLocalWants.insert(origin, 0);
            writeRumor(origin, seqNo, text, false);
        }
        writeStatus(mSocket->recvPort);
    }

    //mTimeoutTimer->stop();
    //writeStatus(mSocket->myPort);
}

void ChatDialog::processStatus(QMap<QString, QMap<QString, quint32> > &sMap)
{
    QVariantMap rumorMsg;
    qDebug() << "sMap" << sMap;
    QMap<QString, quint32> remStat = sMap["Want"];
    qDebug() << remStat;
    qDebug() << mLocalWants;

    // use statusFlag to determin if:
    // 0 - okay
    // 1 - local has more msgs
    // 2 - remote has more msgs
    int statusFlag = 0;
    if (remStat.contains("uninit") && mLocalWants.isEmpty())
    {
        return;
    }

    // Loop through localwants to see if there is anything missing
    QMap<QString, quint32>::const_iterator iter = mLocalWants.constBegin();
    for (; iter != mLocalWants.constEnd(); iter++)
    {
        if (remStat.contains("uninit") || !remStat.contains(iter.key()))
        {
            qDebug() << "mLocalWants has keys remote doesn't";
            statusFlag = 1;
            rumorMsg["Origin"] = iter.key();
            rumorMsg["SeqNo"] = 0;
            rumorMsg["ChatText"] = mMessageList[iter.key()][0];
            break;
        }
        // remote key has smaller seq number than I have
        else if (remStat[iter.key()] < mLocalWants[iter.key()])
        {
            qDebug() << "mLocalWant's sequence number is ahead";
            statusFlag = 1;
            rumorMsg["Origin"] = iter.key();
            rumorMsg["SeqNo"] = remStat[iter.key()];
            rumorMsg["ChatText"] = mMessageList[iter.key()][remStat[iter.key()]];
            break;
        }
        // remote's sequence number is ahead of mine
        else if (remStat[iter.key()] > mLocalWants[iter.key()])
        {
            qDebug() << "mLocalWant's sequence number is behind";
            statusFlag = 2;
            break;
        }
    }

    // Only look for difference if none have been found
    if (statusFlag == 0)
    {
        iter = remStat.constBegin();
        for (; iter != remStat.constEnd(); iter++)
        {
            // remote has keys I don't have
            if (mLocalWants.isEmpty() || !mLocalWants.contains(iter.key()))
            {
                qDebug() << "remote has keys mLocalWants doesn't";
                statusFlag = 2;
                break;
            }
        }
    }

    // Process results
    switch (statusFlag)
    {
        // If status is in sync, 50% to spread rumor
        case 0:
            qDebug() << "Status messages are in sync";
            if (mSocket->genRandNum() % 2 == 0)
            {
                QString origin = mLastMsg["Origin"].toString();
                int seqNo = mLastMsg["SeqNo"].toInt();
                QString text = mLastMsg["ChatText"].toString();
                writeRumor(origin, seqNo, text, false);
            }
            break;
        // If status is ahead of remote
        case 1:
            qDebug() << "Status mesage is ahead";
            mSocket->writeUdp(rumorMsg, mSocket->recvPort);
            break;
        // If status is behind remote
        case 2:
            qDebug() << "Status message is behind";
            writeStatus(mSocket->recvPort);
            break;
    }
}

void ChatDialog::appendToMessageList(QVariantMap &qMap)
{
    QString origin = qMap["Origin"].toString();
    quint32 seqNo = qMap["SeqNo"].toInt();
    QString text = qMap["ChatText"].toString();
    if (text.isEmpty())
    {
        return;
    }
    if (mMessageList.contains(origin))
    {
        // host exists in message list already
        if (!mMessageList[origin].contains(seqNo))
        {
            // Only update values if it is new
            mMessageList[origin].insert(seqNo, text);
        }
    }
    else
    {
        // host doesn't exist in message list, add
        QMap<quint32, QString> tMap;
        tMap.insert(seqNo, text);
        mMessageList.insert(origin, tMap);
    }
    this->textview->append(origin + ">: " + text);
}

void ChatDialog::writeStatus(int port)
{
    QMap<QString, QMap<QString, quint32> > sMap;
    QMap<QString, quint32> tempMap;
    if (mLocalWants.isEmpty())
    {
        // if we send uninitialized QMap serialization fails completely
        tempMap.insert("uninit", 0);
        sMap.insert("Want", tempMap);
    }
    else
    {
        sMap.insert("Want", mLocalWants);
    }
    //QVariantMap sMap;
    //sMap.insert("Want", QVariant::fromValue<QIntMap>(mLocalWants));
    QByteArray wBytes;
    QDataStream out(&wBytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);
    out << sMap;
    qDebug() << "Writing Status: " << sMap;
    mSocket->writeDatagram(wBytes, mSocket->HostAddress, port);

    //mSocket->writeUdp(sMap, port);
    mTimeoutTimer->start(1000);
}

void ChatDialog::writeRumor(QString &origin, int seqNo, QString &text, bool append)
{
    QVariantMap qMap;
    qMap["ChatText"] = text;
    qMap["Origin"] = origin;
    qMap["SeqNo"] = seqNo;
    qDebug() << "Sending Text: " << text;
    // Update tracking variables
    if(append) appendToMessageList(qMap);
    int port = mSocket->getWritePort();
    mSocket->writeUdp(qMap, port);
    // Loopback test
    //mSocket->writeUdp(qMap, mSocket->myPort);
    mTimeoutTimer->start(1000);
    mLastMsg = QVariantMap(qMap);
}

// Slot for enter messages
void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...
    // qDebug() << "FIX: send message to other peers: " << textline->text();

    // Spread user enetered rumors
    QString origin = mSocket->mOriginName;
    int seqNo = mLocalWants[mSocket->mOriginName];
    QString text = textline->text();
    writeRumor(origin, seqNo, text);
    if (mLocalWants.size() == 0)
    {
        mLocalWants[mSocket->mOriginName] = 1;
    }
    else
    {
        mLocalWants[mSocket->mOriginName]++;
    }
    //textview->append(origin + ">: " + textline->text());

    // Clear the textline to get ready for the next input message.
    textline->clear();
}

int NetSocket::getWritePort()
{
    // Determine which port to send tof
    sendPort = myPort == myPortMin ? myPort + 1 :
        myPort == myPortMax ? myPort - 1 :
        (genRandNum() % 2) == 0 ? myPort + 1:
        myPort - 1;
    qDebug() << "Sending Port: " << QString::number(sendPort);
    return sendPort;
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
    HostAddress = QHostAddress(QHostAddress::LocalHost);
    qDebug() << HostAddress.toString();
    QHostInfo info;
    mOriginName = info.localHostName() + "-" + QString::number(genRandNum());
    qDebug() << mOriginName;
}

NetSocket::~NetSocket()
{
    //delete HostAddress;
}

bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
			qDebug() << "bound to UDP port " << p;
            myPort = p;
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}

// Serializes and writes to UDP socket
// @param map - readonly ref to map to be written
void NetSocket::writeUdp(const QVariantMap &map, int port)
{
    if (map.isEmpty())
    {
        return;
    }
    QByteArray wBytes;
    QDataStream out(&wBytes, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_8);
    out << map;
    qDebug() << "Writing Output UDP: " << map;
    this->writeDatagram(wBytes, HostAddress, port);
}


// Deserialize and read from UDP socket
// @param map - pointer to map to store data
// @return bool - if there is more datagrams to read
/*
bool NetSocket::readUdp(QVariantMap* map)
{
    QByteArray buf(this->pendingDatagramSize(), Qt::Uninitialized);
    QDataStream str(&buf, QIODevice::ReadOnly);
    QHostAddress sendingAddr;
    quint16 temPort;
    readDatagram(buf.data(), buf.size(), &sendingAddr, &temPort);
    recvPort = temPort;
    str >> (*map);
    if (this->hasPendingDatagrams())
    {
        return true;
    }
    else
    {
        return false;
    }
}
*/

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

