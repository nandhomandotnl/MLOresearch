#include <iostream>
#include "ns3/command-line.h"
#include "ns3/node-container.h"
#include "ns3/net-device-container.h"
#include "ns3/wifi-helper.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/application-container.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

using namespace ns3;

int main(int argc, char* argv[]){
  uint32_t nAP = 1; // number of Access point
  uint32_t nWifi = 2; // number of stations
  double distance = 10; // meters
  uint32_t payloadSize = 1000; // 1000 Bytes = 1 Packet
  double simtime = 30.0; // seconds
  int gi = 3200; // Guard Interval [nanoseconds]
  
  CommandLine cmd(__FILE__);
  cmd.AddValue("nWifi", "Number of Wifi STAs", nWifi);
  cmd.AddValue("distance", "Distance between AP and STA", distance);
  cmd.AddValue("payloadSize", "Payload Size", payloadSize);
  cmd.AddValue("simtime", "Simulation Time", simtime);
  cmd.Parse(argc, argv);

  NodeContainer ap;
  ap.Create(nAP);
  NodeContainer sta;
  sta.Create(nWifi);
  
  NetDeviceContainer apdevices;
  NetDeviceContainer stadevices;

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211be); // Wifi-7
  wifi.ConfigHeOptions("GuardInterval", TimeValue(NanoSeconds(gi)),
                      "MpduBufferSize", UintegerValue(256));
  
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
  
  SpectrumWifiPhyHelper phy(3);  // 3 links per device (MLD)
  phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
  
  WifiMacHelper wifiMac;
  // No OFDMA
  wifiMac.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
              "EnableUlOfdma", BooleanValue(false),
              "EnableBsrp", BooleanValue(false),
              "AccessReqInterval", TimeValue(Seconds(0)));
  
  Ssid ssid = Ssid("ns3-ssid");

  // setup STAs
  // setup STAs
  for (uint32_t i=0; i<nWifi; ++i){
      wifiMac.SetType("ns3::StaWifiMac",
                  "Ssid", SsidValue(ssid),
                  "QosSupported", BooleanValue(true),
                  "ActiveProbing", BooleanValue(false));
  
      // Link 1: 2.4 GHz
      phy.SetChannel(spectrumChannel);
      phy.Set(0, "ChannelSettings", StringValue("{0, 20, BAND_2_4GHZ, 0}"));
      wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                      "DataMode", StringValue("EhtMcs11"),
                      "ControlMode", StringValue("ErpOfdmRate24Mbps"));
      
      // Link 2: 5 GHz
      phy.Set(1, "ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));
      wifi.SetRemoteStationManager(1, "ns3::ConstantRateWifiManager",
                      "DataMode", StringValue("EhtMcs9"),
                      "ControlMode", StringValue("OfdmRate24Mbps"));
      
      // Link 3: 6 GHz
      phy.Set(2, "ChannelSettings", StringValue("{0, 80, BAND_6GHZ, 0}"));
      wifi.SetRemoteStationManager(2, "ns3::ConstantRateWifiManager",
                      "DataMode", StringValue("EhtMcs7"),
                      "ControlMode", StringValue("HeMcs4"));
      
      stadevices.Add(wifi.Install(phy, wifiMac, sta.Get(i)));
  }
  // setup AP
  for (uint32_t i=0; i<nAP; ++i){
      wifiMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssid),
                  "QosSupported", BooleanValue(true));
      apdevices.Add(wifi.Install(phy, wifiMac, ap.Get(i)));
  }
  
  // Assigning Spatial Streams
  RngSeedManager::SetSeed(1);  // setting random seed value
  RngSeedManager::SetRun(1);
  int64_t streamNumber = 150;
  streamNumber += wifi.AssignStreams(apdevices, streamNumber);
  streamNumber += wifi.AssignStreams(stadevices, streamNumber); 
  
  // Configure Mobility
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  positionAlloc->Add(Vector(distance, 0.0, 0.0));  // AP and STA are 'distance' meters apart
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(sta);
  mobility.Install(ap); 
  
  // Internet Stack Layer
  InternetStackHelper stack;
  stack.Install(ap);
  stack.Install(sta);
  Ipv4AddressHelper address;
  address.SetBase("10.1.0.0", "255.255.255.0");
  Ipv4InterfaceContainer wifiApInterface = address.Assign(apdevices);
  Ipv4InterfaceContainer wifiStaInterfaces = address.Assign(stadevices);

  // Setting Applications
  std::cout << "Setting UDP Applications" << std::endl;
  ApplicationContainer serverApp;
  auto serverNodes = std::ref(ap); // uplink transmissions
  Ipv4InterfaceContainer serverInterfaces;
  NodeContainer clientNodes;
  for (std::size_t i=0; i<nWifi; ++i){
      serverInterfaces.Add(wifiApInterface.Get(0));
      clientNodes.Add(sta.Get(i));
  }
  
  UdpServerHelper Server(9);  // Port 9 is for UDP
  serverApp = Server.Install(serverNodes.get());
  serverApp.Start(Seconds(0.0));
  serverApp.Stop(Seconds(simtime + 1.0));
  
  // Setting periodic traffic
  for (std::size_t i=0; i<nWifi; ++i){
      UdpClientHelper Client(serverInterfaces.GetAddress(0), 9);
      Client.SetAttribute("MaxPackets", UintegerValue(500000000));
      Client.SetAttribute("Interval", TimeValue(Seconds(0.002))); // 2 ms
      Client.SetAttribute("PacketSize", UintegerValue(payloadSize));
  
      ApplicationContainer clientApps;
      InetSocketAddress rmt(serverInterfaces.GetAddress(0), 9);
      AddressValue remoteAddress(rmt);
      Client.SetAttribute("RemoteAddress", remoteAddress);
      clientApps.Add(Client.Install(clientNodes.Get(i)));
      clientApps.Start(Seconds(1.0) + MicroSeconds(i*100)); // Line x
      // clientApps.Start(Seconds(1.0));  // simultaneous-tx
      clientApps.Stop(Seconds(simtime + 1.0));
  }
  
  // Flow Monitor
  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();
  
  // Routing
  Simulator::Schedule(Seconds(0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

  
  // Setting Simulator
  std::cout<< "Setting up Simulator\n";
  Simulator::Stop(Seconds(simtime + 1.0));
  Simulator::Run();
  std::stringstream fname;
  fname << "MLO/flowmon-data-sta-mloinf" << nWifi << ".xml";
  std::cout << fname.str();
  flowmon->SerializeToXmlFile(fname.str(), true, true);
  Simulator::Destroy();

  return 0;
}
