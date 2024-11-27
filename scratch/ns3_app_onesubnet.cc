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


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Tap-UDP-Dumbbell-Example-Modified");


int
main (int argc, char *argv[])
{
  std::string mode = "UseBridge";
  std::string tapName = "tap-test1";

  CommandLine cmd;
  cmd.AddValue ("mode", "Mode setting of TapBridge", mode);
  cmd.AddValue ("tapName", "Name of the OS tap device", tapName);
  cmd.Parse (argc, argv);

  GlobalValue::Bind ("SimulatorImplementationType",
		     StringValue ("ns3::RealtimeSimulatorImpl"));
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  //
  // The topology has a csma network of four nodes  
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
  ipv4Left.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfacesLeft = ipv4Left.Assign (devicesLeft);

  TapBridgeHelper tapBridge (interfacesLeft.GetAddress (0));
  tapBridge.SetAttribute ("Mode", StringValue (mode));
  tapBridge.SetAttribute ("DeviceName", StringValue (tapName));
  tapBridge.Install (nodesLeft.Get (0), devicesLeft.Get (0));


// Send some data to the NC in the Podman Container on port 6060
  std::string remote1 ("10.1.1.6");
  Ipv4Address remoteIp1 (remote1.c_str ());
  uint16_t port1 = 6060;

  UdpEchoClientHelper client2 (remoteIp1, port1);
  std::ostringstream msg;
  msg << "Hello External Device! How are you there\n" << '\0';
  client2.SetAttribute ("PacketSize", UintegerValue (msg.str ().length ()));

  ApplicationContainer apps2 = client2.Install (nodesLeft.Get (1));
  apps2.Start (Seconds (0.5));
  apps2.Stop (Seconds (10.0));
  client2.SetFill (apps2.Get (0), (uint8_t *) msg.str ().c_str (),
		   msg.str ().length (), msg.str ().length ());

  csmaLeft.EnablePcapAll ("node-network");

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AnimationInterface anim ("one-subnet-ns3-app.xml");
  // Trace routing tables 
  Ipv4GlobalRoutingHelper g;
  Ptr < OutputStreamWrapper > routingStream =
    Create < OutputStreamWrapper > ("dynamic-global-routing.routes",
				    std::ios::out);
  g.PrintRoutingTableAllAt (Seconds (12), routingStream);

  Simulator::Stop (Seconds (600.));
  Simulator::Run ();
  Simulator::Destroy ();
}