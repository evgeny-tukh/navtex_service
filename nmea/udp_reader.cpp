#include "udp_reader.h"

bool UdpReader::connect () {
    UdpReaderCfg *config = (UdpReaderCfg *) cfg;
    
    receiver = socket (AF_INET, SOCK_DGRAM, IPPROTO_IP);

    if (receiver != INVALID_SOCKET) {
        uint32_t on = true;
        uint32_t ttl = 255;
        SOCKADDR_IN nic;

        nic.sin_addr.S_un.S_addr = config->bindTo.empty () ? INADDR_ANY : inet_addr (config->bindTo.c_str ());
        nic.sin_family = AF_INET;
        nic.sin_port = htons (config->inPort);

        setsockopt (receiver, SOL_SOCKET, SO_REUSEADDR, (char *) & on, sizeof (on));
        setsockopt (receiver, IPPROTO_IP, IP_TTL, (const char *) & ttl, sizeof (ttl));
        setsockopt (receiver, IPPROTO_IP, IP_MULTICAST_TTL, (const char *) & ttl, sizeof (ttl));
        connected = bind (receiver, (const SOCKADDR *) & nic, sizeof (nic)) == S_OK;

        if (connected) {
            setsockopt (receiver, SOL_SOCKET, SO_BROADCAST, (char *) & on, sizeof (on));
            return true;
        }

        closesocket (receiver);
    }

    receiver = INVALID_SOCKET;
    connected = false;
    return false;
}

bool UdpReader::disconnect () {
    if (receiver != INVALID_SOCKET) {
        closesocket (receiver);
        receiver = INVALID_SOCKET;
    }
    connected = false;
    return true;
}

size_t UdpReader::dataAvailable () {
    unsigned long bytesAvailable;
    if (connected && ioctlsocket (receiver, FIONREAD, & bytesAvailable) == S_OK) return bytesAvailable;
    return 0;
}

bool UdpReader::getAvailableData (Buffer& result) {
    if (receiver == INVALID_SOCKET) return false;
    bool ok = false;
    auto bytesAvailable = dataAvailable ();
    if (bytesAvailable > 0) {
        result.resize (bytesAvailable);
        SOCKADDR_IN origin;
        int error = 0;
        int size = sizeof (origin);
        int received = recvfrom (receiver, (char *) result.data (), bytesAvailable, 0, (SOCKADDR *) & origin, & size);
        if (received < 0) error = WSAGetLastError ();
        return received > 0;
    } else {
        result.clear ();
    }
    return true;
}

bool UdpReader::write (char *data) {
    if (!connected) connect ();
    if (receiver == INVALID_SOCKET) return false;
    UdpReaderCfg *config = (UdpReaderCfg *) cfg;
    SOCKADDR_IN dest;
    dest.sin_addr.S_un.S_addr = INADDR_BROADCAST;
    dest.sin_family = AF_INET;
    dest.sin_port = htons (config->outPort);
    return sendto (receiver, data, strlen (data), 0, (const SOCKADDR *) & dest, sizeof (dest)) > 0;
}
