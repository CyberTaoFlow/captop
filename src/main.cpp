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

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>

#include <options.hpp>

namespace
{
    std::string name    = "captop";
    std::string version = name + " v1.3";
}


void usage()
{
    std::cerr << version << std::endl << std::endl;
    std::cerr << "usage: " + name + " [OPTIONS] [BPF expression]\n\n"
                 "Pcap options:\n"
                 "  -B --buffer SIZE             Set the operating system capture buffer size.\n"
                 "  -c count                     Exit after receiving count packets.\n"
                 "  -s snaplen                   Specify the capture length of packets in bytes.\n"
                 "  -t --timeout NUM             Specify the timeout in msec.\n"
                 "  -O --no-optimize             Do not run the packet-matching code optimizer.\n"
                 "\nInterface:\n"
                 "  -i --interface IFNAME        Listen on interface.\n"
                 "  -o --output IFNAME           Inject packets to interface.\n"
                 "\nFile:\n"
                 "  -r --read  FILE              Read packets from file.\n"
                 "  -w --write FILE              Write packets to file.\n"
                 "\nMiscellaneous:\n"
                 "     --version                 Print the version strings and exit.\n"
                 "  -? --help                    Print this help.\n";

    _Exit(0);
}


extern int pcap_top_gen(struct options const &, std::string const &bpf);
extern int pcap_top_live(struct options const &, std::string const &bpf);
extern int pcap_top_file(struct options const &, std::string const &bpf);


bool any_strcmp(const char *arg, const char *opt)
{
    return strcmp(arg,opt) == 0;
}
template <typename ...Ts>
bool any_strcmp(const char *arg, const char *opt, Ts&&...args)
{
    return (strcmp(arg,opt) == 0 ? true : any_strcmp(arg, std::forward<Ts>(args)...));
}


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
            i++;
            if (i == argc)
            {
                throw std::runtime_error("buffer size missing");
            }

            opt.buffer_size = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-c", "--count") ) {
            i++;
            if (i == argc)
            {
                throw std::runtime_error("count missing");
            }

            opt.count = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-s", "--snaplen") ) {
            i++;
            if (i == argc)
            {
                throw std::runtime_error("snaplen missing");
            }

            opt.snaplen = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-t", "--timeout") ) {
            i++;
            if (i == argc)
            {
                throw std::runtime_error("timeout missing");
            }

            opt.timeout = static_cast<size_t>(std::atoi(argv[i]));
            continue;
        }

        if ( any_strcmp(argv[i], "-i", "--interface") ) {
            i++;
            if (i == argc)
            {
                throw std::runtime_error("interface missing");
            }

            opt.in.ifname = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "-o", "--output") ) {
            i++;
            if (i == argc)
            {
                throw std::runtime_error("output interface missing");
            }

            opt.out.ifname = argv[i];
            continue;
        }

        if ( any_strcmp("-r", "--read") ) {
            i++;
            if (i == argc)
            {
                throw std::runtime_error("filename missing");
            }

            opt.in.filename = argv[i];
            continue;
        }

        if ( any_strcmp(argv[i], "-w", "--write") ) {
            i++;
            if (i == argc)
            {
                throw std::runtime_error("filename missing");
            }

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


        if ( any_strcmp(argv[i], "-h", "-?", "--help") )
            usage();

        if (argv[i][0] != '-')
            break;

        throw std::runtime_error(std::string(argv[i]) + " unknown option!");
    }

    if (!opt.in.filename.empty())
        return pcap_top_file(opt, i == argc ? "" : argv[i]);

    if (!opt.in.ifname.empty())
        return pcap_top_live(opt, i == argc ? "" : argv[i]);

    if (!opt.out.ifname.empty() || !opt.out.filename.empty())
        return pcap_top_gen(opt, i == argc ? "" : argv[i]);

    throw std::runtime_error("interface/filename missing");
}
catch(std::exception &e)
{
    std::cerr << name << ": " << e.what() << std::endl;
}
