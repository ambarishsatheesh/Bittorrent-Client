# Bittorrent-Client
* Bittorrent Client implementing [BTP/1.0](https://wiki.theory.org/BitTorrentSpecification) plus select extensions ([BEPs](https://www.bittorrent.org/beps/bep_0000.html) 0015, 0020, 0023, 0027)
* Features QBittorrent-inspired GUI written using Qt5, using 
* [Supports UDP Tracker Protocol](https://www.bittorrent.org/beps/bep_0015.html), [Peer ID Conventions](https://www.bittorrent.org/beps/bep_0020.html), [Compact Peer Lists](https://www.bittorrent.org/beps/bep_0023.html), and [Private Torrents](https://www.bittorrent.org/beps/bep_0027.html)
* Features a Bencoder (uses [iterator-based parsing](https://gist.github.com/ambarishsatheesh/14b5122f0767944e19b4636800db4d75))
* Supports Asynchronous I/O (TCP and UDP networking) using [Boost Asio](https://www.boost.org/doc/libs/1_72_0/doc/html/boost_asio/overview.html), with event handling aided by [Boost Signals2](https://www.boost.org/doc/libs/1_72_0/doc/html/signals2.html)
* HTTP tracker support using [Boost Beast](https://github.com/boostorg/beast)
* Multithreading implemented using std::thread
* Features user-configurable network settings (TCP/UDP ports, max/min peers, max download/upload speed)
* Supports Windows.

![](/screenshots/screenshot1.png)
![](/screenshots/creator.png)
![](/screenshots/info.png)
![](/screenshots/settings.jpg)
![](/screenshots/console.png)
