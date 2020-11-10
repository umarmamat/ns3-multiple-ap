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

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/netanim-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1   n2   n3   n4
//                   point-to-point  |    |    |    |
//                                   ================
//                                     LAN 10.1.2.0

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ThirdScriptExample");

int 
main (int argc, char *argv[])
{
  bool verbose = true;
  uint32_t nCsma = 0;
  uint32_t nWifi = 8;
  bool tracing = false;

  CommandLine cmd;
  cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
  cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);

  cmd.Parse (argc,argv);

  // The underlying restriction of 18 is due to the grid position
  // allocator's configuration; the grid layout will exceed the
  // bounding box if more than 18 nodes are provided.
  if (nWifi > 18)
    {
      std::cout << "nWifi should be 18 or less; otherwise grid layout exceeds the bounding box" << std::endl;
      return 1;
    }

  if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }
 // wifi ap1
  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));
  csmaNodes.Create (nCsma);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces;
  csmaInterfaces = address.Assign (csmaDevices);

  address.SetBase ("10.1.3.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
//wifi 2  ******
  NodeContainer p2pNodes1;
  p2pNodes1.Create (2);

  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices1;
  p2pDevices1 = pointToPoint1.Install (p2pNodes1);

  NodeContainer csmaNodes1;
  csmaNodes1.Add (p2pNodes1.Get (1));
  csmaNodes1.Create (nCsma);

  CsmaHelper csma1;
  csma1.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma1.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices1;
  csmaDevices1 = csma1.Install (csmaNodes1);

  NodeContainer wifiStaNodes1;
  wifiStaNodes1.Create (nWifi);
  NodeContainer wifiApNode1 = p2pNodes1.Get (0);

  YansWifiChannelHelper channel1 = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy1 = YansWifiPhyHelper::Default ();
  phy1.SetChannel (channel1.Create ());

  WifiHelper wifi1;
  wifi1.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac1;
  Ssid ssid1 = Ssid ("ns-3-ssid");
  mac1.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid1),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices1;
  staDevices1 = wifi1.Install (phy1, mac1, wifiStaNodes1);

  mac1.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid1));

  NetDeviceContainer apDevices1;
  apDevices1 = wifi1.Install (phy1, mac1, wifiApNode1);

  MobilityHelper mobility1;

  mobility1.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility1.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility1.Install (wifiStaNodes1);

  mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility1.Install (wifiApNode1);

  InternetStackHelper stack1;
  stack1.Install (csmaNodes1);
  stack1.Install (wifiApNode1);
  stack1.Install (wifiStaNodes1);

  Ipv4AddressHelper address1;

  address1.SetBase ("10.1.1.1", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces1;
  p2pInterfaces1 = address1.Assign (p2pDevices1);

  address1.SetBase ("10.1.2.2", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces1;
  csmaInterfaces1 = address1.Assign (csmaDevices1);

  address1.SetBase ("10.1.3.3", "255.255.255.0");
  address1.Assign (staDevices1);
  address1.Assign (apDevices1);

  UdpEchoServerHelper echoServer1 (9);

  ApplicationContainer serverApps1 = echoServer1.Install (csmaNodes1.Get (nCsma));
  serverApps1.Start (Seconds (1.0));
  serverApps1.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient1 (csmaInterfaces1.GetAddress (nCsma), 9);
  echoClient1.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient1.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient1.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps1 = 
    echoClient1.Install (wifiStaNodes1.Get (nWifi - 1));
  clientApps1.Start (Seconds (2.0));
  clientApps1.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //three
   NodeContainer p2pNodes2;
  p2pNodes2.Create (2);

  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices2;
  p2pDevices2 = pointToPoint2.Install (p2pNodes1);

  NodeContainer csmaNodes2;
  csmaNodes2.Add (p2pNodes2.Get (1));
  csmaNodes2.Create (nCsma);

  CsmaHelper csma2;
  csma2.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma2.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  NetDeviceContainer csmaDevices2;
  csmaDevices2 = csma2.Install (csmaNodes2);

  NodeContainer wifiStaNodes2;
  wifiStaNodes2.Create (nWifi);
  NodeContainer wifiApNode2 = p2pNodes2.Get (0);

  YansWifiChannelHelper channel2 = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy2 = YansWifiPhyHelper::Default ();
  phy2.SetChannel (channel2.Create ());

  WifiHelper wifi2;
  wifi2.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac2;
  Ssid ssid2 = Ssid ("ns-3-ssid2");
  mac2.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid2),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices2;
  staDevices2 = wifi2.Install (phy2, mac2, wifiStaNodes2);

  mac2.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid2));

  NetDeviceContainer apDevices2;
  apDevices2 = wifi2.Install (phy2, mac2, wifiApNode2);

  MobilityHelper mobility2;

  mobility2.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (5.0),
                                 "MinY", DoubleValue (5.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility2.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (50, 100, 50, 100)));
  mobility2.Install (wifiStaNodes2);

  mobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility2.Install (wifiApNode2);

  InternetStackHelper stack2;
  stack2.Install (csmaNodes2);
  stack2.Install (wifiApNode2);
  stack2.Install (wifiStaNodes2);

  Ipv4AddressHelper address2;

  address2.SetBase ("10.1.7.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces2;
  p2pInterfaces2 = address2.Assign (p2pDevices2);

  address2.SetBase ("10.1.8.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces2;
  csmaInterfaces2 = address2.Assign (csmaDevices2);

  address2.SetBase ("10.1.9.0", "255.255.255.0");
  address2.Assign (staDevices2);
  address2.Assign (apDevices2);

  UdpEchoServerHelper echoServer2 (9);

  ApplicationContainer serverApps2 = echoServer2.Install (csmaNodes2.Get (nCsma));
  serverApps2.Start (Seconds (1.0));
  serverApps2.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient2 (csmaInterfaces2.GetAddress (nCsma), 9);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps2 = 
    echoClient2.Install (wifiStaNodes2.Get (nWifi - 1));
  clientApps2.Start (Seconds (2.0));
  clientApps2.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  ///// end
  Simulator::Stop (Seconds (10.0));

  if (tracing == true)
    {
      //1
      pointToPoint.EnablePcapAll ("third");
      phy.EnablePcap ("third", apDevices.Get (0));
      csma.EnablePcap ("third", csmaDevices.Get (0), true);
      //2
      pointToPoint1.EnablePcapAll ("third1");
      phy1.EnablePcap ("third1", apDevices1.Get (0));
      csma1.EnablePcap ("third1", csmaDevices1.Get (0), true);
    
      pointToPoint2.EnablePcapAll ("third1");
      phy2.EnablePcap ("third1", apDevices2.Get (0));
      csma2.EnablePcap ("third1", csmaDevices2.Get (0), true);
    }
  AnimationInterface anim ("wifi two.xml");
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
