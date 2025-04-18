/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
// This program is derived from tap-wifi-dumbell.cc (https://www.nsnam.org/doxygen/tap-wifi-dumbbell_8cc.html) and all copyrights are according to original author.
// Modified by Ing. Dorinda Bassey

#include <iostream>
#include <fstream>
#include <csignal>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/tap-bridge-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/netanim-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ContainerNs3App");
class UdpPortFilter : public ns3::PacketFilter
{
public:
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("UdpPortFilter")
      .SetParent<PacketFilter> ()
      .SetGroupName("TrafficControl")
      .AddConstructor<UdpPortFilter> ();
    return tid;
  }

  UdpPortFilter () {}
  virtual ~UdpPortFilter () {}

  // Required: NS-3 uses this to see if it should even try classifying the packet
  virtual bool CheckProtocol (Ptr<QueueDiscItem> item) const override
  {
    std::cout << " check Average Goodput: " << std::endl;
    Ptr<Packet> packet = item->GetPacket ()->Copy ();
    UdpHeader ipHeader;
    //packet->PeekHeader (ipHeader);
    if (packet->PeekHeader (ipHeader))
    {
      std::cout << " peeked udp: " << std::endl;
      Ptr<Ipv4QueueDiscItem> ipv4Item = DynamicCast<Ipv4QueueDiscItem> (item);
      return ipv4Item->GetHeader().GetProtocol() == UdpL4Protocol::PROT_NUMBER; 
    }
    return false;
 }

  // Required method to classify

  // Decide what to do with the UDP packet
  virtual int32_t DoClassify (Ptr<QueueDiscItem> item) const override
  {
    Ptr<Packet> packet = item->GetPacket ()->Copy ();

    Ipv4Header ipHeader;
    packet->RemoveHeader (ipHeader);  // Must remove before reading transport layer

    UdpHeader udpHeader;
    packet->PeekHeader (udpHeader);

    if (udpHeader.GetDestinationPort () == 8080)
    {
      std::cout << " udpport is 8080: " << std::endl;
      NS_LOG_INFO ("UdpPortFilter: allowing UDP packet to port 8080");
      return 0; // allow it
    }

    return -1; // everything else drop
  }
};

Ptr<FlowMonitor> flowMonitor;
FlowMonitorHelper flowHelper;
void SignalHandler(int signum) {
    std::cout << "Interrupt signal (" << signum << ") received.\n";
    if (flowMonitor != nullptr) {
        flowMonitor->CheckForLostPackets();
        flowMonitor->SerializeToXmlFile("interrupted-flowmon.xml", true, true);
        // Access FlowMonitor stats after simulation
        std::map<FlowId, FlowMonitor::FlowStats> flowStats = flowMonitor->GetFlowStats();
        for (auto& flow : flowStats) {
            FlowId flowId = flow.first;
            FlowMonitor::FlowStats stats = flow.second;

            // Output statistics for the flow
            std::cout << "Flow " << flowId << std::endl;
            std::cout << "  Packets Sent: " << stats.txPackets << std::endl;
            std::cout << "  Packets Received: " << stats.rxPackets << std::endl;
            std::cout << "  Dropped Packets: " << stats.lostPackets << std::endl;
            std::cout << "  Packet Loss Ratio: " << (stats.lostPackets / (float)stats.txPackets) * 100 << "%" << std::endl;
        }
    } else {
        std::cout << "FlowMonitor was not initialized.\n";
    }
    exit(signum);
}

int
main (int argc, char *argv[])
{
  LogComponentEnableAll(LOG_NONE);
  std::string mode = "UseBridge";
  std::string tapName = "tap-x";
  std::string queueType = "DropTail"; // or "FqCoDel"

  CommandLine cmd;
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.AddValue ("queueType", "Queue discipline to use (DropTail or FqCoDel)", queueType);
  cmd.Parse (argc, argv);

  GlobalValue::Bind ("SimulatorImplementationType",
		     StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // The topology has a csma network of four nodes on the left side.
  //
  NodeContainer nodesLeft;
  nodesLeft.Create (4);

  CsmaHelper csmaLeft;
  csmaLeft.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csmaLeft.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  NetDeviceContainer devicesLeft = csmaLeft.Install (nodesLeft);

  MobilityHelper mobilityL;
  mobilityL.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityL.Install (nodesLeft);
  nodesLeft.Get (3)->GetObject < MobilityModel >
    ()->SetPosition (Vector (400.0, 200.0, 0.0));

  nodesLeft.Get (2)->GetObject < MobilityModel >
    ()->SetPosition (Vector (300.0, 200.0, 0.0));

  nodesLeft.Get (1)->GetObject < MobilityModel >
    ()->SetPosition (Vector (200.0, 200.0, 0.0));

  nodesLeft.Get (0)->GetObject < MobilityModel >
    ()->SetPosition (Vector (100.0, 200.0, 0.0));

  InternetStackHelper internetLeft;
  internetLeft.Install (nodesLeft);

  Ipv4AddressHelper ipv4Left;
  ipv4Left.SetBase ("192.168.2.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeft = ipv4Left.Assign (devicesLeft);

  TapBridgeHelper tapBridge (interfacesLeft.GetAddress (0));
  tapBridge.SetAttribute ("Mode", StringValue (mode));
  tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
  tapBridge.Install (nodesLeft.Get (0), devicesLeft.Get (0));

  //
  // The topology has a csma network of four nodes on the right side.
  //
  NodeContainer nodesRight;
  nodesRight.Create (4);

  CsmaHelper csmaRight;
  csmaRight.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csmaRight.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  NetDeviceContainer devicesRight = csmaRight.Install (nodesRight);

  InternetStackHelper internetRight;
  internetRight.Install (nodesRight);

  Ipv4AddressHelper ipv4Right;
  ipv4Right.SetBase ("192.168.6.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesRight = ipv4Right.Assign (devicesRight);

  //
  // Stick in the point-to-point line between the sides.
  //
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("512kbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("10ms"));

  NodeContainer nodes = NodeContainer (nodesLeft.Get (1), nodesRight.Get (0));
  //add traffic control qdisc

  // Install devices first
  NetDeviceContainer devices = p2p.Install(nodes);

  TrafficControlHelper trafficControl;
  QueueDiscContainer qdiscs;
  // Configure FqCoDel with secure settings
  if (queueType == "FqCoDel") {
    trafficControl.SetRootQueueDisc("ns3::FqCoDelQueueDisc", 
        "EnableSetAssociativeHash", BooleanValue(true),     // Set associative hashing
        "UseEcn", BooleanValue(true),                       // Enable ECN marking
        "Perturbation", UintegerValue(256),                 // Randomize flow hashing
        "DropBatchSize", UintegerValue(1),                  // Drop only 1 per batch (precise trimming)
        "Flows", UintegerValue(1024),                       // Max flow queues
        "MaxSize", QueueSizeValue(QueueSize("100p"))        // Total queue size limit (adjust as needed)
    );

    // Apply traffic control to devices
    qdiscs = trafficControl.Install(devices);

    // Attach your custom UDP port filter
    Ptr<QueueDisc> q = qdiscs.Get(0); // or Get(1) depending on direction
    Ptr<QueueDisc> y = qdiscs.Get(1); //this is the direction that is working to stop udp messages going out
    Ptr<UdpPortFilter> filter = CreateObject<UdpPortFilter>();
    q->AddPacketFilter(filter);
    y->AddPacketFilter(filter);
  } else {
    // Default DropTail, no TrafficControl layer needed
    trafficControl.SetRootQueueDisc("ns3::PfifoFastQueueDisc");
    // Apply traffic control to devices
    qdiscs = trafficControl.Install(devices);
  }

  Ipv4StaticRoutingHelper staticRouting;
  Ptr<Ipv4StaticRouting> routing = staticRouting.GetStaticRouting(nodesLeft.Get(0)->GetObject<Ipv4>());
  
  // Add a route to block traffic to a specific node
  routing->AddNetworkRouteTo(Ipv4Address("192.168.6.0"), Ipv4Mask("255.255.255.0"), 1);
  
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("192.168.4.0", "255.255.255.192");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  MobilityHelper mobility1;
  mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility1.Install (nodesRight);
  nodesRight.Get (0)->GetObject < MobilityModel >
    ()->SetPosition (Vector (500.0, 500.0, 0.0));
  nodesRight.Get (1)->GetObject < MobilityModel >
    ()->SetPosition (Vector (500.0, 600.0, 0.0));
  nodesRight.Get (2)->GetObject < MobilityModel >
    ()->SetPosition (Vector (500.0, 700.0, 0.0));

  nodesRight.Get (3)->GetObject < MobilityModel >
    ()->SetPosition (Vector (500.0, 800.0, 0.0));

    
// Send some data Docker Container
  std::string remote1 ("192.168.2.6");
  Ipv4Address remoteIp1 (remote1.c_str ());
  uint16_t port1 = 8080;

  UdpEchoClientHelper client2 (remoteIp1, port1);
  std::ostringstream msg;
  msg << "Hello External Device! from node 4\n" << '\0';
  client2.SetAttribute ("PacketSize", UintegerValue (msg.str ().length ()));
  client2.SetAttribute ("MaxPackets", UintegerValue (10000));
  client2.SetAttribute ("Interval", TimeValue (MicroSeconds (10))); // Send every 100us
  ApplicationContainer apps2 = client2.Install (nodesRight.Get (3));
  apps2.Start (Seconds (0.5));
  apps2.Stop (Seconds (10.0));
  client2.SetFill (apps2.Get (0), (uint8_t *) msg.str ().c_str (),
		   msg.str ().length (), msg.str ().length ());

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AnimationInterface anim ("container-ns3-application.xml");
  // Trace routing tables 
  Ipv4GlobalRoutingHelper g;
  Ptr < OutputStreamWrapper > routingStream =
    Create < OutputStreamWrapper > ("dynamic-global-routing.routes",
				    std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (12), routingStream);

  // === FLOW MONITOR SETUP ===
  //FlowMonitorHelper flowmon;
  flowMonitor = flowHelper.InstallAll();

  signal(SIGINT, SignalHandler);
  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}

