// Freight building handshake: 004e15f0/1690/16d0/18e0 and 004e0830.
// Sequence completion is supplied by the sprite controller, not a fixed delay.
// Reservation assumes the caller already checked owner/race compatibility.
// Used by StepHaul/StepWork; boat approach remains a separate adapter.
#pragma once
namespace freightgate {
struct State {
    int busy=0, phase=0, request=0, reservation=-1;
    bool reserve(int boat) {
        if(reservation!=-1) return false;
        reservation=boat;return true;
    }
    bool release(int boat) {
        if(reservation!=boat) return false;
        reservation=-1;return true;
    }
    bool begin(int boat) {
        if(reservation!=boat || busy!=0) return false;
        busy=1;request=1;phase=1;return true;
    }
    void end(int boat) {
        if(reservation==boat && busy==1 && request!=0) request=0;
    }
    // Return whether GetMineWorkAnim must reconfigure sequence 11.
    bool step(int type,bool sequenceEnded,bool bodyEnded) {
        if(busy!=1) return false;
        bool configure=false;
        if(sequenceEnded) {
            if(phase==1) {phase=request?2:3;configure=true;}
            else if(phase==2) {
                if(!request) {phase=3;configure=true;}
                else configure=type==59 || type==96 || type==82;
            } else if(phase==3) {phase=0;configure=true;}
        }
        if(!request && phase==0 && bodyEnded) busy=0;
        return configure;
    }
};
}
