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

//
// Network topology
//
//       n0    n1   n2   n3
//       |     |    |    |
//     =====================
//
// - Two packet socket flows:  from n0 to n1 and from n3 to n0
// - Default 512 byte packets generated by traffic generator
// - Output from the PacketSink trace source will be sent to the 
//      csma-packet-socket-sink.tr file
//   ASCII trace output will be sent to the csma-packet-socket.tr file

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("CsmaPacketSocketExample");

std::ofstream g_os;

static void
SinkRx (std::string path, Ptr<const Packet> p, const Address &address)
{
  g_os << p->GetSize () << std::endl;
}

int
main (int argc, char *argv[])
{
#if 0 
  LogComponentEnable ("CsmaPacketSocketExample", LOG_LEVEL_INFO);
#endif

  CommandLine cmd;
  cmd.Parse (argc, argv);

  g_os.open ("csma-packet-socket-sink.tr",std::ios_base::binary | std::ios_base::out);

  // Here, we will explicitly create four nodes.
  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  nodes.Create (4);

  PacketSocketHelper packetSocket;
  packetSocket.Install (nodes);

  // create the shared medium used by all csma devices.
  NS_LOG_INFO ("Create channels.");
  Ptr<CsmaChannel> channel = CreateObjectWithAttributes<CsmaChannel> (
      "DataRate", DataRateValue (DataRate (5000000)),
      "Delay", TimeValue (MilliSeconds (2)));

  // use a helper function to connect our nodes to the shared channel.
  NS_LOG_INFO ("Build Topology.");
  CsmaHelper csma;
  csma.SetDeviceAttribute ("EncapsulationMode", StringValue ("Llc"));
  NetDeviceContainer devs = csma.Install (nodes, channel);

  NS_LOG_INFO ("Create Applications.");
  // Create the OnOff application to send raw datagrams
  PacketSocketAddress socket;
  socket.SetSingleDevice (devs.Get (0)->GetIfIndex ());
  socket.SetPhysicalAddress (devs.Get (1)->GetAddress ());
  socket.SetProtocol (2);
  OnOffHelper onoff ("ns3::PacketSocketFactory", Address (socket));
  onoff.SetConstantRate (DataRate ("500kb/s"));
  ApplicationContainer apps = onoff.Install (nodes.Get (0));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  socket.SetSingleDevice (devs.Get (3)->GetIfIndex ());
  socket.SetPhysicalAddress (devs.Get (0)->GetAddress ());
  socket.SetProtocol (3);
  onoff.SetAttribute ("Remote", AddressValue (socket));
  apps = onoff.Install (nodes.Get (3));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));

  // Install packet sink on node 0 to receive packets from node 1
  PacketSinkHelper sink = PacketSinkHelper ("ns3::PacketSocketFactory",
                                            socket);
  apps = sink.Install (nodes.Get (0));
  apps.Start (Seconds (0.0));
  apps.Stop (Seconds (20.0));

  // While the below trace sink is hooked to all nodes (the wildcard "*")
  // only one packet sink (on node 0) is actually added above, so 
  // only the receive events on node 0 will be traced
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                   MakeCallback (&SinkRx));
 
  // Configure tracing of all enqueue, dequeue, and NetDevice receive events
  // Trace output will be sent to the csma-packet-socket.tr file
  NS_LOG_INFO ("Configure Tracing.");

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("csma-packet-socket.tr"));

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");

  g_os.close ();

  return 0;
}
