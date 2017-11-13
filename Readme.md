CS5450 Lab 3 Peer to Peer Network
Yu Hsiang Sean Hsu
yh742@cornell.edu

The following serves a progress and finding log for this project:

SETUP

- Finding a linux machine actually took a long time. Due to the lack of lab, I had to try using VirtualBox.
Unfornately, the performance on my 2014 MacBook Air was extremely choppy. I then tried installing ubuntu on
a removable flash drive because I didn't want to mess up my PC's ESP/EFI partition. After several attempts, I was finally
able to boot off a USB drive and use it as a filesystem

PART 1 QT

- I downloaded Qt4.8 SDK which was archived somewhere on the qt website. I then installed qt creator to write code on in it.
- The first part of this project required us to play around with the Qt4.8 framework. Since the usage of the connect() call
has changed (SLOTS and SIGNALS) in qt5 I was hung up for a bit trying to get it compile. Otherwise, I have used QT a bit before
so it was pretty straightforward.

PART 2 Event-Driven Network Programming

- For the second part, I implemented a echo server to read back my own packets. I simply followed the implementation requested
on the pdf document to create this function. The following class was used to serialize the data:

    - QByteArray - a buffer to store bytes underneath
    - QDataStream - a stream that can be used to convert maps to bytes
    - QVariantMap - a map that stores a QString as a key and QVariant (which can be any Qt Type)

PART 3 Gossip Protocol

- To begin with, I read the RC-850 article here: http://www.ietf.org/rfc/rfc1036.txt
- Then I started creating the serialization of both rumor and status messages
- There are some issues with casting data types and serialization that I found that needs to be addressed

    - When a QMap<QString, quint32> needs to be casted into QVariant, we need to use:
        - Q_DECLARE_METATYPE as well as typedef to make the conversion
        - Unfortunately the code compiled but I couldn't get it to work (QVariant Save and Load errors)
        - Thus, I explicitly declared all status messages as QMap<QString, QMap<QString, quint32> >

    - When dealing with status messages, it seem like if we insert an empty QMap as the value of "Want" (this happens when
    the chat box is empty), the deserialization process seems to think it is empty. So instead, I placed a value of
    <"uninit", 0> to signify that the status message is empty.

- For the unique id of my machine I used QHostInfo to get the local host name with a randomly generated number appended to it
- For neighboring nodes, I assigned the first bindable address. Each node is allowed to talk to the port above and below it
(except for max and min port ranged nodes)
    - I later found an scenario in which this setup might not work. If a middle host suddenly drops off the network there is no
    nodes to facilitate communication. There needs to be some rebinding or ports in order for this to work more robustly in
    future implementations.
- There are 4 objects used in this program that have SIGNALS:
    - mTimeoutTimer - timeout is issued and the last rumor message is resent when a Rumor does not receive a corresponding status
    message back.
    - mEntropyTimer - timeout is issued every 10 second and a status message is propagated to a random neighbor
    - textline
    - mSocket
