/** 
 *  @file       fileHandler.cpp
 *  @brief      File handler source file
 *  @author     Jozef Zuzelka (xzuzel00)
 *  Mail:       xzuzel00@stud.fit.vutbr.cz
 *  Created:    06.03.2017 14:51
 *  Edited:     18.03.2017 22:35
 *  Version:    1.0.0
 */

#include <string>                   //  string
#include <sys/utsname.h>            //  uname() TODO -lc pri preklade
#include "debug.hpp"                //  D(), log()
#include "fileHandler.hpp"



void initOFile(std::ofstream &oFile)
{
    utsname u;
    uname(&u);
    std::string os = u.sysname + std::string(" ") + u.release + std::string(",") + u.version;

    SectionHeaderBlock shb(os);
    shb.write(oFile);
    InterfaceDescriptionBlock idb(os);
    idb.write(oFile);

    log(LogLevel::INFO, "The output file has been initialized.");
    ///System/Library/CoreServices/SystemVersion.plist
    //sw_vers
    //uname
    //http://stackoverflow.com/questions/11072804/how-do-i-determine-the-os-version-at-runtime-in-os-x-or-ios-without-using-gesta
}


int RingBuffer::push(const pcap_pkthdr *header, const u_char *packet)
{
    if (full()) 
    {
        log(LogLevel::WARNING, "Packet dropped!");
        return 1;
    }
    else
        ++size;

    if (last >= buffer.size()) 
        last = 0;
    buffer[last].setCapturedPacketLength(header->caplen);
    buffer[last].setOriginalPacketLength(header->len);
    buffer[last].setTimestamp(header->ts.tv_usec); //! @todo    Will usec be precise enough?
    buffer[last].setPacketData(packet);
    ++last;

    std::lock_guard<std::mutex> guard(m_mutex);
    m_rcvdPacket = true;
    m_condVar.notify_one();
    return 0;
}


void RingBuffer::pop()
{
    first = (first+1) % buffer.size() ;
    --size;
    if (empty())
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_rcvdPacket = false;
    }
}


void RingBuffer::write()
{
    log(LogLevel::INFO, "Writing to the output file started.");
    while (!stop())
    {
        std::unique_lock<std::mutex> mlock(m_mutex);
        m_condVar.wait(mlock, std::bind(&RingBuffer::receivedPacket, this));
        mlock.unlock();
        while(!empty())
        {
            buffer[first].write(oFile); //! @todo   mutex needed
            pop();
        }
    }
    log(LogLevel::INFO, "Writing to the output file stopped.");
}
