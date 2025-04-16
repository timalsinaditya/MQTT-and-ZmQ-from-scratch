/*
 * ZmqMessage.h
 *
 *  Created on: Jan 30, 2025
 *      Author: autives
 */

#ifndef MSG_ZMQMESSAGE_H_
#define MSG_ZMQMESSAGE_H_


#include "inet/common/packet/chunk/Chunk_m.h"

struct ZmqFrame {
    bool more;           // MORE flag
    std::string data;    // Frame data
};

class ZmqMessage : public inet::FieldsChunk {
  public:
    std::vector<ZmqFrame> frames;

    ZmqMessage() : inet::FieldsChunk() {}
    virtual ~ZmqMessage() {}

    void addFrame(const ZmqFrame &frame) {
        frames.push_back(frame);
    }

    inet::B calculateChunkLength() const {
        inet::B totalSize = inet::B(0);
        for (const auto& frame : frames) {
            totalSize += inet::B(sizeof(bool)); // 'more' flag
            totalSize += inet::B(frame.data.size()); // Frame data size
        }
        return totalSize;
    }
};

#endif /* MSG_ZMQMESSAGE_H_ */
