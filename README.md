# gossip-protocol
## What is this?

A gossip protocol is a procedure or process of computer–computer communication that is based on the way social networks disseminate information or how epidemics spread. It is a communication protocol. Modern distributed systems often use gossip protocols to solve problems that might be difficult to solve in other ways, either because the underlying network has an inconvenient structure, is extremely large, or because gossip solutions are the most efficient ones available.

## How to build
qmake
make
./p2papp

## How this works

1. Essentially the core algorithm works as follow for rumors (there are 2 scenarios):
  * User types a new message into the text box.
    * textline’s signal is fired and the receiving slot is returnPressed().
    * returnPressed() calls writeRumor() to append the line into textview as well as to the message history.
    * writeRumor() uses its parameters to construct a QVariantMap from QT types and serializes across the socket.
    * port destination is generated using getWritePort() which based on the location of your port generates a port to send datagrams to.
  * User receives a new message from the socket.
    * gotReadyRead() deserializes the data into a QVariantMap to check the keys to determine the type of messages.
     * If the message contains “ChatText” it is forwarded to processRumor(), 3 scenarios: 
      * If we already have the rumor, discard it.
      * If this host is already in our table we only accept the rumor if it is the sequence number we want.
      * If the host is completely new we want to request all the messages start from sequence 0.
      * Finally the status table which is kept as mLocalWants is sent to the neighbor who sent us the new rumor message.
The algorithm for processing the status message is as follows:
  * gotReadyRead() deserializes the data into a QVariantMap to check the keys to determine the type of messages.
  * If the message contains “Want” as a key, the QByteArray is deserialized again into QMap<QString, QMap<QString, quint32> > (due to the QVariant bug described above).
  * The map is then passed to processStatus() the following cases are possible:
    * Both nodes have empty status’s (no messages), then just discard.
    * The local status has hosts that the remote status doesn’t.
      * Send rumor messages from the missing host start from sequence number 0.
    * The local status’ sequence number is ahead of the remote status map.
      * Send rumor message corresponding to the remote status map.
    * The remote status is ahead of the local status map or the remote status has host names that the local status map doesn’t.
      * Send status back to remote neighbor’s port.
* Otherwise the status message is insync and rumor is spread using the algorithm described in 9b.

## Implementation details

1. Download Qt4.8 SDK which was archived somewhere on the qt website.
2. Installed qt creator to write code on in it.
3. The following class was used to serialize the data:
   * QByteArray - a buffer to store bytes underneath
   * QDataStream - a stream that can be used to convert maps to bytes
   * QVariantMap - a map that stores a QString as a key and QVariant (which can be any Qt Type)
4. Reference the article here: http://www.ietf.org/rfc/rfc1036.txt
5. There are 4 objects used in this program that have SIGNALS:
    * mTimeoutTimer - timeout is issued and the last rumor message is re-sent when a Rumor does not receive a corresponding status message back.
    * mEntropyTimer - timeout is issued every 10 second and a status message is propagated to a random neighbor.
    * textline - this signal is fired when the user presses enter on the chat box.
    * mSocket - this signal is fired when there is something to read in the socket.

## Testing

Much of the initial testing is done by using tools found online such as netcat

Serialization and deserialization was especially cumbersome when dealing with some qt types so I used the debugger to step through the code and loopback node messages. I also used qDebug() method to dump messages maps to check its contents.

I was able to write a some client code to send different datagrams to my port to check if the right methods are being accessed using qDebug() to trace through the calls.

## Future improvements

* Using the node/port setup proposed in the pdf document, there are cases when the messages don’t get propagated properly. If a middle host suddenly drops off the network there is no nodes to facilitate communication between two nodes. There needs to be some rebinding or ports in order for this to work more robustly in future implementations.

* The entropy timer of 10 seconds seems to take a while to fire causing delays for message to propagate to nodes. It might not also be optimal to just choose a random port to propagate new messages. If you know the port where you got new message, you should just send to the only other port. It doesn’t seem efficient to pick a random port as outlined in the project instructions.

* Another improvement that could be made is the timeout mechanism, there should be a better way  to spam the sender with a status message until they respond. If the node is not active, there is no way to stop this.

* There might be concurrency issues so I used simply mutex locks for each shared resources. This might lead to degradation of performance, but provides data integrity. If there is a future implementation, I would structure the program a little bit differently to optimize for this fact.
