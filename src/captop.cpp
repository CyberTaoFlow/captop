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

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <chrono>

#include <thread>
#include <atomic>

#include <options.hpp>
#include <vt100.hpp>

#include <signal.h>
#include <time.h>
#include <pcap/pcap.h>


namespace global
{
    char errbuf[PCAP_ERRBUF_SIZE];
    char errbuf2[PCAP_ERRBUF_SIZE];

    std::atomic_long inject;

    std::atomic_long count;
    std::atomic_long bandw;

    pcap_t *in, *out;

    std::chrono::time_point<std::chrono::system_clock> now_;

    const unsigned char ping[1500] =
    {
            0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xbf, /* L`..UF.. */
            0x97, 0xe2, 0xff, 0xae, 0x08, 0x00, 0x45, 0x00, /* ......E. */
            0x00, 0x54, 0xb3, 0xf9, 0x40, 0x00, 0x40, 0x01, /* .T..@.@. */
            0xf5, 0x32, 0xc0, 0xa8, 0x00, 0x02, 0xad, 0xc2, /* .2...... */
            0x23, 0x10, 0x08, 0x00, 0xf2, 0xea, 0x42, 0x04, /* #.....B. */
            0x00, 0x01, 0xfe, 0xeb, 0xfc, 0x52, 0x00, 0x00, /* .....R.. */
            0x00, 0x00, 0x06, 0xfe, 0x02, 0x00, 0x00, 0x00, /* ........ */
            0x00, 0x00, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, /* ........ */
            0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, /* ........ */
            0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, /* .. !"#$% */
            0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, /* &'()*+,- */
            0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, /* ./012345 */
            0x36, 0x37                                      /* 67 */
    };
}


void print_pcap_stats(pcap_t *p, uint64_t count, uint64_t inject)
{
    struct pcap_stat stat;

    std::cout << count          << " packets captured" << std::endl;
    if (global::out)
        std::cout << inject << " packets injected" << std::endl;

    if (pcap_stats(p, &stat) != -1)
    {
        std::cout << stat.ps_recv   << " packets received by filter" << std::endl;
        std::cout << stat.ps_drop   << " packets dropped by kernel" << std::endl;
        std::cout << stat.ps_ifdrop << " packets dropped by interface" << std::endl;
    }
}


void set_stop(int)
{
    pcap_breakloop(global::in);
    print_pcap_stats(global::in, global::count, global::inject);
    _Exit(0);
}


template <typename T>
std::string to_string_(std::ostringstream &out, T &&arg)
{
    out << std::move(arg);
    return out.str();
}
template <typename T, typename ...Ts>
std::string to_string_(std::ostringstream &out, T &&arg, Ts&&... args)
{
    out << std::move(arg);
    return to_string_(out, std::forward<Ts>(args)...);
}
template <typename ...Ts>
inline std::string
to_string(Ts&& ... args)
{
    std::ostringstream out;
    return to_string_(out, std::forward<Ts>(args)...);
}

template <typename T>
std::string highlight (T const &value)
{
    return to_string(vt100::BOLD, value, vt100::RESET);
}


std::string
pretty(double value)
{
    if (value < 1000000000) {
    if (value < 1000000) {
    if (value < 1000) {
         return to_string(value);
    }
    else return to_string(value/1000, "_K");
    }
    else return to_string(value/1000000, "_M");
    }
    else return to_string(value/1000000000, "_G");
}


template <typename T, typename Duration>
double persecond(T value, Duration dur)
{
    return static_cast<double>(value) * 1000000 /
        std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
}


void thread_stats(pcap_t *p)
{
    struct pcap_stat stat_ = {0, 0, 0}, stat = {0, 0, 0};

    auto now_   = std::chrono::system_clock::now();
    auto count_ = global::count.load(std::memory_order_relaxed);
    auto bandw_ = global::bandw.load(std::memory_order_relaxed);

    if (pcap_stats(p, &stat_) < 0)
        return;

     for(;; std::this_thread::sleep_for(std::chrono::seconds(1)))
     {
        pcap_stats(p, &stat);

        auto now = std::chrono::system_clock::now();

        auto count = global::count.load(std::memory_order_relaxed);
        auto bandw = global::bandw.load(std::memory_order_relaxed);

        auto delta  = now - now_;

        auto pps    = persecond(count - count_, delta);
        auto band   = persecond((bandw - bandw_) * 8, delta);
        auto drop   = persecond(stat.ps_drop - stat_.ps_drop, delta);
        auto ifdrop = persecond(stat.ps_ifdrop - stat_.ps_ifdrop, delta);

        std::cout << "packets: "   << highlight(count) << " (" << highlight(pps) << " pps) ";
        std::cout << "drop: "      << highlight(drop) << " pps, ifdrop: " << highlight(ifdrop) << " pps, ";
        std::cout << "bandwidth: " << highlight(pretty(band)) << "bit/sec " << std::endl;

        count_ = count;
        bandw_ = bandw;
        now_   = now;
        stat_  = stat;
    }
}


void packet_handler(u_char *user, const struct pcap_pkthdr *h, const u_char *payload)
{
    global::count.fetch_add(1, std::memory_order_relaxed);
    global::bandw.fetch_add(h->len, std::memory_order_relaxed);

    if (global::out && pcap_inject(global::out, payload, h->caplen) != -1)
        global::inject.fetch_add(1, std::memory_order_relaxed);
}


int
pcap_top_inject_live(options const &opt)
{
    // print header...
    //

    std::cout << "injecting to " << opt.ofname << "..." << std::endl;

    // create a pcap handler
    //

    global::out = pcap_open_live(opt.ofname.c_str(), opt.snaplen, 1, opt.timeout, global::errbuf2);
    if (global::out == nullptr)
        throw std::runtime_error("pcap_open_offline:" + std::string(global::errbuf2));

    return 0;
}


int
pcap_top_live(options const &opt, std::string const &filter)
{
    bpf_program fcode;

    // set signal handlers...
    //

    if (signal(SIGINT, set_stop) == SIG_ERR)
        throw std::runtime_error("signal");

    // open output device...
    //

    if (!opt.ofname.empty())
        pcap_top_inject_live(opt);


    // print header...
    //

    std::cout << "listening on " << opt.ifname << ", snaplen " << opt.snaplen;

    // create a pcap handler
    //

    int status;

    global::in = pcap_create(opt.ifname.c_str(), global::errbuf);
    if (global::in == nullptr)
        throw std::runtime_error(std::string(global::errbuf));

    if (opt.buffer_size)
    {
        std::cout << ", buffer size " << opt.buffer_size;
        if ((status = pcap_set_buffer_size(global::in, opt.buffer_size)) != 0)
            throw std::runtime_error(std::string("pcap_set_buffer: ") + pcap_statustostr(status));
    }

    // snaplen...
    //
    if ((status = pcap_set_snaplen(global::in, opt.snaplen)) != 0)
        throw std::runtime_error(std::string("pcap_set_snaplen: ") + pcap_statustostr(status));

    // snaplen...
    //
    if ((status = pcap_set_promisc(global::in, 1)) != 0)
        throw std::runtime_error(std::string("pcap_set_promisc: ") + pcap_statustostr(status));

    // set timeout...
    //

    std::cout << ", timeout " << opt.timeout << "_ms";
    if ((status = pcap_set_timeout(global::in, opt.timeout)) != 0)
    {
        throw std::runtime_error(std::string("pcap_set_timeout: ") + pcap_statustostr(status));
    }

    std::cout<< std::endl;

    // activate...
    //
    if ((status = pcap_activate(global::in)) != 0)
        throw std::runtime_error(pcap_statustostr(status));

    // set BPF...
    //
    if (!filter.empty())
    {
        if (pcap_compile(global::in, &fcode, filter.c_str(), opt.oflag, PCAP_NETMASK_UNKNOWN) < 0)
            throw std::runtime_error(std::string("pcap_compile: ") + pcap_geterr(global::in));
    }

    // run thread of stats
    //

    std::thread (thread_stats, global::in).detach();

    // start capture...
    //
    if (pcap_loop(global::in, opt.count, packet_handler, nullptr) == -1)
        throw std::runtime_error("pcap_loop: " + std::string(global::errbuf));

    print_pcap_stats(global::in, global::count, global::inject);

    pcap_close(global::in);

    return 0;
}


int
pcap_top_file(options const &opt, std::string const &filter)
{
    // set signal handlers...
    //

    if (signal(SIGINT, set_stop) == SIG_ERR)
        throw std::runtime_error("signal");

    // open output device...
    //

    if (!opt.ofname.empty())
        pcap_top_inject_live(opt);

    // print header...
    //

    std::cout << "reading " << opt.filename << "..." << std::endl;

    // create a pcap handler
    //

    global::in = pcap_open_offline(opt.filename.c_str(), global::errbuf);
    if (global::in == nullptr)
        throw std::runtime_error("pcap_open_offline:" + std::string(global::errbuf));

    // run thread of stats
    //

    std::thread (thread_stats, global::in).detach();

    // start capture...
    //
    if (pcap_loop(global::in, opt.count, packet_handler, nullptr) == -1)
        throw std::runtime_error("pcap_loop: " + std::string(global::errbuf));

    std::this_thread::sleep_for(std::chrono::seconds(1));

    print_pcap_stats(global::in, global::count, global::inject);

    pcap_close(global::in);

    return 0;
}


int
pcap_top_gen(options const &opt, std::string const &filter)
{
    struct pcap_pkthdr hdr = { { 0, 0 }, 64, 64};

    pcap_top_inject_live(opt);

    // run thread of stats
    //

    std::thread (thread_stats, global::out).detach();

    for(size_t n = 0; n < opt.count; n++)
        packet_handler(nullptr, &hdr, global::ping);

    return 0;
}

