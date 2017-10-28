#ifndef CAPTURE_CAPTURE_SAVER_H_
#define CAPTURE_CAPTURE_SAVER_H_

#include <iostream>
#include <string>

#include "capture/capture.h"

class CaptureSaver : public Capture {
public:
    explicit CaptureSaver(Source* source, Analyzer* analyzer, int target);
    virtual ~CaptureSaver() {}

protected:
    int target_player;
    void runLoop();
};

#endif  // CAPTURE_CAPTURE_SAVER_H_
