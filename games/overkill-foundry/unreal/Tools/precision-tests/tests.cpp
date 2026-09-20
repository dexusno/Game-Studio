#include "precision.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace foundry::presentation;
int checks=0;
void require(bool condition,const char* explanation) {
    ++checks;
    if(!condition)throw std::runtime_error(explanation);
}
int main() {
    try {
        PrecisionAttempt ready;
        require(!ready.stop(),"No result before explicit Start");
        require(!ready.tick(5,true),"Waiting on the ready screen does not spend an attempt");
        require(ready.position()==0,"Pre-start time does not move marker");
        require(ready.start(),"First Start succeeds");
        require(!ready.start(),"A second Start cannot reset the timer");
        require(!ready.tick(.9,true),"Centre has not timed out");
        require(ready.stop()==2,"A stop at centre yields Perfect");
        require(!ready.stop()&&!ready.tick(20,true)&&!ready.start(),"Result cannot repeat or restart");
        PrecisionAttempt timeout;
        timeout.start();
        require(timeout.tick(1.8,true)==0,"No input times out as Miss");
        require(!timeout.stop(),"Input after timeout cannot improve the committed category");
        PrecisionAttempt focus;
        focus.start();focus.tick(.4,true);
        require(!focus.tick(30,false)&&focus.paused(),"Focus loss pauses without resolving");
        require(focus.elapsed()==.4,"Background time does not move marker");
        require(!focus.tick(30,true),"First resumed frame cannot include background time");
        focus.tick(.5,true);
        require(focus.stop()==2,"Focused elapsed time survives a pause");
        for(int width:{100,120,130}) {
            PrecisionAttempt p(width);
            require(p.score(.5)==2,"Perfect centre at each legal upgrade width");
            for(double sign:{-1.0,1.0}) {
                const double perfect=.5+sign*p.perfectHalfWidth();
                const double good=.5+sign*p.goodHalfWidth();
                require(p.score(perfect)==2,"Perfect edge is inclusive");
                require(p.score(perfect+sign*.000001)==1,"Outside Perfect but inside Good yields Good");
                require(p.score(good)==1,"Good edge is inclusive");
                require(p.score(good+sign*.000001)==0,"Outside Good yields Miss");
            }
        }
        PrecisionAttempt base,wider(120),widest(130);
        require(base.score(.55)==1&&wider.score(.55)==2,"20 percent wider Perfect has actual input effect");
        require(wider.score(.557)==1&&widest.score(.557)==2,"Additional10 percent width has actual input effect");
        PrecisionAttempt bad;
        bad.start();bad.tick(std::numeric_limits<double>::quiet_NaN(),true);bad.tick(-5,true);
        require(bad.elapsed()==0,"Invalid frame times cannot advance or corrupt the timer");
        require(bad.score(std::numeric_limits<double>::quiet_NaN())==0,"Invalid marker is never rewarded");
        require(bad.score(-.1)==0&&bad.score(1.1)==0,"Off-track input is Miss");
        std::cout<<"PASS "<<checks<<" Precision input/window assertions; actual Slate and human timing untested\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<"FAIL "<<error.what()<<'\n';return 1;}
}
