# Bittorrent Client
Bittorrent Client implementing [BTP/1.0](https://wiki.theory.org/BitTorrentSpecification) plus select extensions ([BEPs](https://www.bittorrent.org/beps/bep_0000.html) 0015, 0020, 0023, 0027)

### Features
* qBittorrent-inspired GUI written using Qt5
* [Supports UDP Tracker Protocol](https://www.bittorrent.org/beps/bep_0015.html), [Peer ID Conventions](https://www.bittorrent.org/beps/bep_0020.html), [Compact Peer Lists](https://www.bittorrent.org/beps/bep_0023.html), and [Private Torrents](https://www.bittorrent.org/beps/bep_0027.html)
* Bencode parser (implemented using [iterator-based parsing](https://gist.github.com/ambarishsatheesh/14b5122f0767944e19b4636800db4d75))
* Asynchronous I/O (TCP and UDP networking) support using [Boost Asio](https://www.boost.org/doc/libs/1_72_0/doc/html/boost_asio/overview.html), with event handling aided by [Boost Signals2](https://www.boost.org/doc/libs/1_72_0/doc/html/signals2.html)
* HTTP tracker support using [Boost Beast](https://github.com/boostorg/beast)
* Multithreading support allows handling of multiple torrents, connections to peers, downloads/uploads, and GUI interaction to maximise network and file I/O performance
* Features user-configurable network settings (TCP/UDP ports, max/min peers, max download/upload speed)
* Simple network scheduling using a [token bucket](https://en.wikipedia.org/wiki/Token_bucket) to control bandwidth 
* Real-time data updates including torrent, tracker, download progress, and network transfer information
* Real-time network transfer speed graph implemented using [QCustomPlot plotting library](https://www.qcustomplot.com/)
* Torrent filtering based on trackers and sorting using any of the columns
* Logging to console implemented using Emil Ernerfeldt's [Loguru library](https://github.com/emilk/loguru)
* Torrent piece and info_hash SHA1 hashing implemented using [Stephan Brumme's Hashing library](https://github.com/stbrumme/hash-library)
* Supports Windows.

![](/screenshots/screenshot1.png)
![](/screenshots/creator.png)
![](/screenshots/info.png)
![](/screenshots/trackers.png)
![](/screenshots/settings.jpg)
![](/screenshots/console.png)
