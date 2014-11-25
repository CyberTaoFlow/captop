captop 
======

Simple top, for pcap network interfaces.


Usage
-----

```
captop v1.4

usage: captop [OPTIONS] [BPF expression]

Pcap options:
  -B --buffer SIZE             Set the operating system capture buffer size.
  -c count                     Exit after receiving count packets.
  -s snaplen                   Specify the capture length of packets in bytes.
  -t --timeout NUM             Specify the timeout in msec.
  -O --no-optimize             Do not run the packet-matching code optimizer.
  -g --genlen                  Length of injected packets (when no input is specified).

Interface:
  -i --interface IFNAME        Listen on interface.
  -o --output IFNAME           Inject packets to interface.

File:
  -r --read  FILE              Read packets from file.
  -w --write FILE              Write packets to file.

Miscellaneous:
     --version                 Print the version strings and exit.
  -? --help                    Print this help.
```
