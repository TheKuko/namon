/** 
 *  @file       ringBuffer.tpp
 *  @brief      Ring Buffer template functions
 *  @author     Jozef Zuzelka <xzuzel00@stud.fit.vutbr.cz>
 *  @date
 *   - Created: 22.03.2017 17:04
 *   - Edited:  21.05.2017 15:33
 */


template <class EnhancedPacketBlock>
int RingBuffer<EnhancedPacketBlock>::push(const pcap_pkthdr *header, const u_char *packet)
{
    if (full()) 
    {
        droppedElem++;
        return 1;
    }

    if (last >= buffer.size()) 
        last = 0;

    buffer[last].setOriginalPacketLength(header->len);
	uint64_t usecUnixTime = header->ts.tv_sec * (uint64_t)1000000 + header->ts.tv_usec;
    buffer[last].setTimestamp(usecUnixTime);
    buffer[last].setPacketData(packet, header->caplen);
    ++last;
    ++size;

    cv_condVar.notify_all();
    return 0;
}


template <class T>
int RingBuffer<T>::push(T &elem)
{
    if (full()) 
    {
        droppedElem++;
        return 1;
    }

    //! @todo ked prepisujeme novy prvok tak dealokovat alokovanu pamat v starom prvku (ip v netflow)
    if (last >= buffer.size()) 
        last = 0;
    buffer[last] = move(elem);
    ++last;
    ++size;

    cv_condVar.notify_all();
    return 0;
}


template<class T>
void RingBuffer<T>::pop()
{
    first = (first+1) % buffer.size() ;
    --size;
}


template<class EnhancedPacketBlock>
void RingBuffer<EnhancedPacketBlock>::write(ofstream &file)
{
    log(LogLevel::INFO, "Writing to the output file started.");
    while (!shouldStop)
    {
        std::unique_lock<std::mutex> mlock(m_condVar);
        cv_condVar.wait(mlock, std::bind(&RingBuffer::newItemOrStop, this));
        mlock.unlock();
        while(!empty())
        {
            buffer[first].write(file);
            pop();
        }
        file.flush();
        if (file.bad()) // e.g. out of space
        {
            log(LogLevel::ERR, "Output error.");
            throw "Output file error"; //! @todo catch it
        }
    }
    log(LogLevel::INFO, "Writing to the output file stopped.");
}


template<class Netflow>
void RingBuffer<Netflow>::run(Cache *cache)
{
    while (!shouldStop)
    {
        std::unique_lock<std::mutex> mlock(m_condVar);
        cv_condVar.wait(mlock, std::bind(&RingBuffer::newItemOrStop, this));
        mlock.unlock();
        while(!empty())
        {
            TEntryOrTTree *cacheRecord = cache->find(buffer[first]);
            // if we found some TEntry, check if it still valid
            if (cacheRecord != nullptr && cacheRecord->isEntry())
            {
                TEntry *foundEntry = static_cast<TEntry *>(cacheRecord);
                // If the record exists but is invalid, run determineApp() in update mode
                // to find new application, else update endTime.
                if (!foundEntry->valid())
                    determineApp(&buffer[first], *foundEntry, UPDATE);
                else
                    foundEntry->getNetflowPtr()->setEndTime(buffer[first].getEndTime());
            }
            else 
            { // else it is either TTree or it is not in the whole map (nullptr)
              // (both means it's not in the cache at all)
                TEntry *e = new TEntry;
                // If an error occured (can't open procfs file, etc.)
                if (!determineApp(&buffer[first], *e, FIND))
                {
                    // insert new record into map
                    if (cacheRecord == nullptr)
                        cache->insert(e);
                    else // else insert it into subtree
                        static_cast<TTree *>(cacheRecord)->insert(e);
                }
                else
                    delete e;
            }
            pop();
        }
    }
    log(LogLevel::INFO, "Caching stopped.");
}
