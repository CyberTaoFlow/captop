/*
 *  Copyright (c) 2014 Nicola Bonelli <nicola@pfq.io>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <pcap/pcap.h>

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>

#include <global.hpp>
#include <options.hpp>
#include <util.hpp>

namespace
{
    std::string name    = "captop";
    std::string version = name + " v1.15";
}


void usage()
{
    std::cerr << version << " (" << pcap_lib_version() << ")" << std::endl << std::endl;

    std::cerr << "usage: " + name + " [OPTIONS] [BPF expression]\n\n"
                 "Pcap options:\n"
                 "  -B --buffer SIZE             Set the operating system capture buffer size.\n"
                 "  -c count                     Exit after receiving count packets.\n"
                 "  -s --snaplen VALUE           Specify the capture length of packets in bytes.\n"
                 "  -t --timeout NUM             Specify the timeout in msec.\n"
                 "  -O --no-optimize             Do not run the packet-matching code optimizer.\n"
                 "     --next                    Use pcap_next instead of pcap_loop.\n"
                 "\nRange Filters:\n"
                 "  -F --filter [RANGES]         Range filters: e.g. -F 1-100,1024,8000-8010\n"
                 "\nGenerator:\n"
                 "  -R --rand-ip                 Randomize IPs addresses.\n"
                 "  -g --genlen  VALUE           Specify the length of injected packets.\n"
                 "\nInterface:\n"
                 "  -i --interface IFNAME        Listen on interface.\n"
                 "  -o --output IFNAME           Inject packets into interface.\n"
                 "\nHandler:\n"
                 "  -H --handler source.c        Dynamically load the pcap handler.\n"
                 "     --compiler PATH           Specify the compiler to use.\n"
                 "     --arg STRING              Specify additional arguments for compiler.\n"
                 "\nThread:\n"
                 "     --thread INT              Launch multiple capture threads (one per core).\n"
                 "     --first-core INT          Specify the index of the first core.\n"
#ifdef PCAP_VERSION_FANOUT
                 "     --fanout GROUP STRING     Enable fanout!\n"
#endif
                 "\nFile:\n"
                 "  -r --read  FILE              Read packets from file.\n"
                 "  -w --write FILE              Write packets to file.\n"
                 "\nMiscellaneous:\n"
                 "     --version                 Print the version strings and exit.\n"
                 "  -? --help                    Print this help.\n";

    _Exit(0);
}


extern int pcap_top(struct options const &, std::string const &);


int
main(int argc, char *argv[])
try
{
    auto opt = default_options;
    int i = 1;

    if (argc < 2)
        usage();

    for(; i < argc; ++i)
    {
        if ( any_strcmp(argv[i], "-B", "--buffer") ) {

            if (++i == argc)
                throw std::runtime_error("buffer size missing");

            opt.buffer_size = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-c") ) {

            if (++i == argc)
                throw std::runtime_error("count missing");

            opt.count = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-g", "--genlen") ) {

            if (++i == argc)
                throw std::runtime_error("genlen missing");

            opt.genlen = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-s", "--snaplen") ) {

            if (++i == argc)
                throw std::runtime_error("snaplen missing");

            opt.snaplen = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-t", "--timeout") ) {

            if (++i == argc)
                throw std::runtime_error("timeout missing");

            opt.timeout = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-i", "--interface") ) {

            if (++i == argc)
                throw std::runtime_error("interface missing");

            opt.in.ifname = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "-o", "--output") ) {

            if (++i == argc)
                throw std::runtime_error("output interface missing");

            opt.out.ifname = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "-r", "--read") ) {

            if (++i == argc)
                throw std::runtime_error("filename missing");

            opt.in.filename = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "-w", "--write") ) {

            if (++i == argc)
                throw std::runtime_error("filename missing");

            opt.out.filename = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "--version") ) {
            std::cout << version << std::endl;
            _Exit(0);
        }

        if ( any_strcmp(argv[i], "-O", "--no-optimize") ) {
            opt.oflag = false;
            continue;
        }

        if ( any_strcmp(argv[i], "--next") ) {
            opt.next = true;
            continue;
        }

        if ( any_strcmp(argv[i], "-R", "--rand-ip") ) {
            opt.rand_ip = true;
            continue;
        }

        if ( any_strcmp(argv[i], "-H", "--handler") ) {

            if (++i == argc)
                throw std::runtime_error("source code missing");

            opt.handler = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "--compiler") ) {

            if (++i == argc)
                throw std::runtime_error("compiler missing");

            opt.compiler = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "--arg") ) {

            if (++i == argc)
                throw std::runtime_error("compiler argument missing");

            opt.arguments.push_back(argv[i]);
            continue;
        }

        if ( any_strcmp(argv[i], "-F", "--filter") ) {

            if (++i == argc)
                throw std::runtime_error("filter missing");

            opt.next = true;
            opt.rfilt = range_filter{argv[i]};
            continue;
        }

        if ( any_strcmp(argv[i], "--thread") ) {

            if (++i == argc)
                throw std::runtime_error("number of thread missing");

            opt.numthread = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "--first-core") ) {

            if (++i == argc)
                throw std::runtime_error("index of first core missing");

            opt.firstcore = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

#ifdef PCAP_VERSION_FANOUT
        if ( any_strcmp(argv[i], "--fanout") ) {

            if (++i == argc)
                throw std::runtime_error("number of group missing");
            opt.group = static_cast<size_t>(std::atoi(argv[i]));

            if (++i == argc)
                throw std::runtime_error("fanout algorithm missing");

            opt.fanout = argv[i];
            continue;
        }
#endif
        if ( any_strcmp(argv[i], "-h", "-?", "--help") )
            usage();

        if (argv[i][0] != '-')
            break;

        throw std::runtime_error(std::string(argv[i]) + " unknown option!");
    }

    return pcap_top(opt, i == argc ? "" : argv[i]);
}
catch(std::exception &e)
{
    std::cerr << name << ": " << e.what() << std::endl;
}
