
/*
 * Copyright (c) 2014-2019 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <vector>

#include "SpanRenderer.h"
#include <opflexagent/logging.h>
#include <opflexagent/SpanManager.h>
#include <boost/optional.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/format.hpp>


namespace opflexagent {
    using boost::optional;
    using namespace boost::adaptors;

    SpanRenderer::SpanRenderer(Agent& agent_) : agent(agent_),
                                                taskQueue(agent.getAgentIOService()) {

    }

    void SpanRenderer::start() {
        LOG(DEBUG) << "starting span renderer";
        agent.getSpanManager().registerListener(this);
    }

    void SpanRenderer::stop() {
        agent.getSpanManager().unregisterListener(this);
    }

    void SpanRenderer::spanUpdated(const opflex::modb::URI& spanURI) {
        taskQueue.dispatch(spanURI.toString(),
                           [=]() { handleSpanUpdate(spanURI); });
    }


    void SpanRenderer::handleSpanUpdate(const opflex::modb::URI& spanURI) {
        LOG(DEBUG) << "Span handle update";
        SpanManager& spMgr = agent.getSpanManager();
        optional<shared_ptr<SpanManager::SessionState>> seSt =
                                             spMgr.getSessionState(spanURI);
        // Is the session state pointer set
        if (!seSt) {
            return;
        }
        // There should be at least one source and one destination.
        // need to accomodate for a change from previous configuration.
        if (seSt.get()->getSrcEndPointMap().empty() ||
            seSt.get()->getDstEndPointMap().empty()) {
            LOG(DEBUG) << "Nothing to do";
            return;
        }
        //get the source ports.
        vector<string> srcPort;
        for (auto src : seSt.get()->getSrcEndPointMap()) {
            srcPort.push_back(src.second.get()->getPort());
        }
        // get the destination IPs
        vector<address> dstIp;
        for (auto dst : seSt.get()->getDstEndPointMap()) {
            dstIp.push_back(dst.second.get()->getAddress());
        }

        LOG(DEBUG) << "creating mirror";
        createMirror(seSt.get()->getName(), srcPort, dstIp);
    }

    bool SpanRenderer::createMirror(std::string sess, std::vector<string> srcPort, std::vector<address> dstIp) {
        using namespace std;
        // delete any existing mirror with this name
        boost::format fmtr("ovs-vsctl -- --id=@rec get mirror %1% -- remove bridge br-int mirrors @rec");
        string cmd = (fmtr % sess).str();
        int result = std::system(cmd.c_str());

        // setup ERSPAN port
        fmtr.parse("ovs-vsctl add-port br-int %1%  -- set int %1% type=erspan options:key=1 options:remote_ip=%2% options:erspan_ver=1 options:erspan_idx=1\n");
        std::vector<string> erspan;
        string outPort = "output-port=";
        for (auto dst : dstIp | indexed(1)) {
            string erspName = sess + "-" + "ersp" + to_string(dst.index());
            erspan.push_back(erspName);
            // delete existing ERSPAN port
            cmd = "ovs-vsctl del-port br-int " + erspName;
            result = std::system(cmd.c_str());
            // create erspan port
            cmd = (fmtr % erspName % dst.value()).str();
            result = std::system(cmd.c_str());
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
        result = std::system(cmdStr.c_str());
        return true;
    }

}
