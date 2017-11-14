CS5450 Lab 3 Peer to Peer Network
Yu Hsiang Sean Hsu
yh742@cornell.edu

For a better format please use Google Doc Link

https://docs.google.com/document/d/12yZCBB_4SHSPtjGo4oGs-sa4QB7KMAbTIObO34639Ok/edit?usp=sharing

BUILD
qmake
make
./p2papp

SETUP

Finding a linux machine actually took a long time. Due to the lack of lab, I had to try using VirtualBox. Unfortunately, the performance on my 2014 MacBook Air was extremely choppy. I then tried installing ubuntu on a removable flash drive because I didn't want to mess up my PC's ESP/EFI partition. After several attempts, I was finally able to boot off a USB drive and use it as a filesystem

PART 1 QT

I downloaded Qt4.8 SDK which was archived somewhere on the qt website. I then installed qt creator to write code on in it.
The first part of this project required us to play around with the Qt4.8 framework. Since the usage of the connect() call has changed (SLOTS and SIGNALS) in qt5 I was hung up for a bit trying to get it compile. Otherwise, I have used QT a bit before so it was pretty straightforward.

PART 2 Event-Driven Network Programming

For the second part, I implemented a echo server to read back my own packets. I simply followed the implementation requested on the pdf document to create this function. The following class was used to serialize the data:
QByteArray - a buffer to store bytes underneath
QDataStream - a stream that can be used to convert maps to bytes
QVariantMap - a map that stores a QString as a key and QVariant (which can be any Qt Type)

PART 3 Gossip Protocol

To begin with, I read the RC-850 article here: http://www.ietf.org/rfc/rfc1036.txt
Then I started creating the serialization of both rumor and status messages
There are some issues with casting data types and serialization that I found that needs to be addressed
When a QMap<QString, quint32> needs to be casted into QVariant, we need to use:
Q_DECLARE_METATYPE as well as typedef to make the conversion. Unfortunately the code compiled but I couldn't get it to work (QVariant Save and Load errors)
Thus, I explicitly declared all status messages as QMap<QString, QMap<QString, quint32> >
When dealing with status messages, it seem like if we insert an empty QMap as the value of "Want" (this happens when the chat box is empty), the deserialization process seems to think it is empty. So instead, I placed a value of <"uninit", 0> to signify that the status message is empty.
For the unique id of my machine I used QHostInfo to get the local host name with a randomly generated number appended to it.
For neighboring nodes, I assigned the first bindable address. Each node is allowed to talk to the port above and below it. (except for max and min port ranged nodes)
There are 4 objects used in this program that have SIGNALS:
mTimeoutTimer - timeout is issued and the last rumor message is re-sent when a Rumor does not receive a corresponding status message back.
mEntropyTimer - timeout is issued every 10 second and a status message is propagated to a random neighbor.
textline - this signal is fired when the user presses enter on the chat box.
mSocket - this signal is fired when there is something to read in the socket.
Essentially the core algorithm works as follow for rumors (there are 2 scenarios):
User types a new message into the text box.
textline’s signal is fired and the receiving slot is returnPressed().
returnPressed() calls writeRumor() to append the line into textview as well as to the message history.
writeRumor() uses its parameters to construct a QVariantMap from QT types and serializes across the socket.
port destination is generated using getWritePort() which based on the location of your port generates a port to send datagrams to.
User receives a new message from the socket.
gotReadyRead() deserializes the data into a QVariantMap to check the keys to determine the type of messages.
If the message contains “ChatText” it is forwarded to processRumor(), 3 scenarios:
If we already have the rumor, discard it.
If this host is already in our table we only accept the rumor if it is the sequence number we want.
If the host is completely new we want to request all the messages start from sequence 0.
Finally the status table which is kept as mLocalWants is sent to the neighbor who sent us the new rumor message.
The algorithm for processing the status message is as follows:
gotReadyRead() deserializes the data into a QVariantMap to check the keys to determine the type of messages.
If the message contains “Want” as a key, the QByteArray is deserialized again into QMap<QString, QMap<QString, quint32> > (due to the QVariant bug described above).
The map is then passed to processStatus() the following cases are possible:
Both nodes have empty status’s (no messages), then just discard.
The local status has hosts that the remote status doesn’t.
Send rumor messages from the missing host start from sequence number 0.
  The local status’ sequence number is ahead of the remote status map.
Send rumor message corresponding to the remote status map.
The remote status is ahead of the local status map or the remote status has host names that the local status map doesn’t.
Send status back to remote neighbor’s port.
Otherwise the status message is insync and rumor is spread using the algorithm described in 9b.

Testing, Observations, and Improvements

Much of the initial testing is done by using tools found online such as netcat

Serialization and deserialization was especially cumbersome when dealing with some qt types so I used the debugger to step through the code and loopback node messages. I also used qDebug() method to dump messages maps to check its contents.

I was able to write a some client code to send different datagrams to my port to check if the right methods are being accessed using qDebug() to trace through the calls.

Finally, I opened up multiple(2-4) windows to type in the chat box to see if the messages get propagated properly. I test cases such as:
Opening a windows after a chat has been going on a while
Waiting a while before sending a message (see if sockets have been inundated with random status messages)
Check to see if the farthest nodes from each other can receive each other’s message
Sending really long messages
Tried communicating with one classmate’s program, it seems to work; we didn’t test it extensively

Using the node/port setup proposed in the pdf document, there are cases when the messages don’t get propagated properly. If a middle host suddenly drops off the network there is no nodes to facilitate communication between two nodes. There needs to be some rebinding or ports in order for this to work more robustly in future implementations.

The entropy timer of 10 seconds seems to take a while to fire causing delays for message to propagate to nodes. It might not also be optimal to just choose a random port to propagate new messages. If you know the port where you got new message, you should just send to the only other port. It doesn’t seem efficient to pick a random port as outlined in the project instructions.

Another improvement that could be made is the timeout mechanism, there should be a better way  to spam the sender with a status message until they respond. If the node is not active, there is no way to stop this.

Also while adding variables into global variables, I realized there might be concurrency issues so I used simply mutex locks for each shared resources. This might lead to degradation of performance, but provides data integrity. If there is a future implementation, I would structure the program a little bit differently to optimize for this fact.

There are some parts of the algorithm that wasn’t outlined such as when a new host joins a existing conversation. Should it get all the past messages as well?
In my implementation I chose to go this route by setting the status sequence number to 0.
Unfortunately the message received by be out or order. In future implementation, it might be good use a vector based map to achieve ordering of past messages.
