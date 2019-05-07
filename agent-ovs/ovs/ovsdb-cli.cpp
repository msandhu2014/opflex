/**
 * Build cmd:
 * g++ ovsdb-cli.cpp  -I/usr/include/boost -lboost_system -o cli
 */
#include <vector>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <boost/asio.hpp>
#include <boost/range/adaptors.hpp>

using namespace boost::asio::ip;
using namespace std;
using namespace boost::adaptors;

class CLI {

    public:
    bool createMirror(std::string sess, std::vector<string> srcPort, std::vector<address> dstIp) {
        using namespace std;
        // delete any existing mirror with this name
        boost::format fmtr("ovs-vsctl -- --id=@rec get mirror %1% -- remove bridge br-int mirrors @rec");
        string cmd = (fmtr % sess).str();
        std::system(cmd.c_str());

        // setup ERSPAN port
        fmtr.parse("ovs-vsctl add-port br-int %1%  -- set int %1% type=erspan options:key=1 options:remote_ip=%2% options:erspan_ver=1 options:erspan_idx=1\n");
        std::vector<string> erspan; 
        string outPort = "output-port=";
        for (auto dst : dstIp | indexed(1)) {
            string erspName = sess + "-" + "ersp" + to_string(dst.index());
            erspan.push_back(erspName);
            // delete existing ERSPAN port
            cmd = "ovs-vsctl del-port br-int " + erspName;
            std::system(cmd.c_str());
            // create erspan port
            cmd = (fmtr % erspName % dst.value()).str();
            std::system(cmd.c_str());
        }
        std::string cmdStr;
        cmdStr = fmtr.parse("ovs-vsctl -- set bridge br-int mirrors=@m ").str();
        string srcPortStr = "select-src_port=";
        string dstPortStr = "select-dst-port=";
        for (auto src : srcPort | indexed(1)) {
            cmdStr += (fmtr.parse("-- --id=@s%1% get port %2% ") % src.index() % src.value()).str();
            if (srcPortStr.find("@") != std::string::npos) {
                srcPortStr += ",";
            }
            if (dstPortStr.find("@") != std::string::npos) {
                dstPortStr += ",";
            }
            srcPortStr += "@s" + to_string(src.index());
            dstPortStr += "@s" + to_string(src.index());
            //cout << "srcPortStr " << srcPortStr << ", dstPortStr " << dstPortStr << '\n';
        }
        for (auto dst : erspan | indexed(1)) {
            cmdStr += (fmtr.parse("-- --id=@d%1% get port %2% ") % dst.index() % dst.value()).str();
            if (outPort.find("@") != std::string::npos) {
                outPort += ",";
            }
            outPort += "@d" + to_string(dst.index());
        }
        cmdStr += "-- --id=@m create mirror name=" + sess + " " + srcPortStr + " " + dstPortStr + " " + outPort;
        cout << cmdStr << '\n';
        std::system(cmdStr.c_str());
        return true;
        /*
        //create one erspan port for every dstIp
        string cmd = "ovs-vsctl -- set bridge br-int mirrors=@m";
        for (auto ip : dstIp) {
        */
    }
};

int main() {

    CLI cli;
    vector<string> src = {"p1-tap", "p2-tap"};
    vector<address> dstIp = {address::from_string("10.30.120.240")};
    cli.createMirror("sess1", src, dstIp);
    /*
    string cmd = "sudo ovs-vsctl -- set bridge br-int mirrors=@m";
    cout << cmd << '\n';
    cmd += "--id=@p1 get port p1-tap";
    cout << cmd << '\n';
	//std::system("ovs-vsctl show > out.txt");
	//std::cout << std::ifstream("out.txt").rdbuf();
    */
}
