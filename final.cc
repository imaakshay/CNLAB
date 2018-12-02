//1st program
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("labpgm1");

int main(int argc,char* argv[]){

CommandLine cmd;
cmd.Parse(argc,argv);

NodeContainer nodes;
nodes.Create(3);

InternetStackHelper stack;
stack.Install(nodes);

PointToPointHelper p2p1;
p2p1.SetDeviceAttribute("DataRate",StringValue("5Mbps"));
p2p1.SetChannelAttribute("Delay",StringValue("1ms"));


Ipv4AddressHelper address;
address.SetBase("10.1.1.0","255.255.255.0");

NetDeviceContainer devices;

devices = p2p1.Install(nodes.Get(0),nodes.Get(1));
Ipv4InterfaceContainer interfaces = address.Assign(devices);

address.SetBase("10.1.2.0","255.255.255.0");
devices = p2p1.Install(nodes.Get(1),nodes.Get(2));
interfaces = address.Assign(devices);

Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
em->SetAttribute("ErrorRate",DoubleValue(0.00002));
devices.Get(1)->SetAttribute("ReceiveErrorModel",PointerValue(em));

Ipv4GlobalRoutingHelper::PopulateRoutingTables();

uint32_t payloadsize=1448;

OnOffHelper onoff("ns3::TcpSocketFactory",Ipv4Address::GetAny());  //2nd Argument : the address of the remote node to send traffic to.

onoff.SetAttribute("OnTime",StringValue("ns3::ConstantRandomVariable[Constant=1]"));
onoff.SetAttribute("OffTime",StringValue("ns3::ConstantRandomVariable[Constant=0]"));
onoff.SetAttribute("DataRate",StringValue("50Mbps"));
onoff.SetAttribute("PacketSize",UintegerValue(payloadsize));

uint16_t port=7;
Address localaddress1(InetSocketAddress(Ipv4Address::GetAny(),port));
PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory",localaddress1); //localaddress1 : address of the sink
ApplicationContainer sinkApp1 = packetSinkHelper1.Install(nodes.Get(2));
sinkApp1.Start(Seconds(0));
sinkApp1.Stop(Seconds(10));

ApplicationContainer apps;
AddressValue remoteaddress(InetSocketAddress(interfaces.GetAddress(1),port));
onoff.SetAttribute("Remote",remoteaddress);            //it sets the address of the destination
apps.Add(onoff.Install(nodes.Get(0)));
apps.Start(Seconds(1));
apps.Stop(Seconds(10));

Simulator::Stop(Seconds(10));

AsciiTraceHelper ascii;
p2p1.EnableAsciiAll(ascii.CreateFileStream("labpgm1.tr"));

Simulator::Run();
Simulator::Destroy();
return 0;
}


//2 program
#include <iostream>
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"

using namespace ns3;

static void PingRtt(std::string context,Time rtt){
  std::cout<<context<<" "<<rtt<<std::endl;
}

int main(int argc,char* argv[]){
CommandLine cmd;
cmd.Parse(argc,argv);

NodeContainer nodes;
nodes.Create(6);

InternetStackHelper stack;
stack.Install(nodes);

CsmaHelper csma;
csma.SetChannelAttribute("DataRate",DataRateValue(DataRate(10000)));
csma.SetChannelAttribute("Delay",TimeValue(MilliSeconds(2)));

Ipv4AddressHelper address;
address.SetBase("10.1.1.0","255.255.255.0");

NetDeviceContainer devices;
devices = csma.Install(nodes);
Ipv4InterfaceContainer interfaces = address.Assign(devices);

uint16_t port = 9;
OnOffHelper onoff("ns3::UdpSocketFactory",Address(InetSocketAddress(interfaces.GetAddress(2),port)));
onoff.SetConstantRate(DataRate("500Mbps"));

ApplicationContainer apps = onoff.Install(nodes.Get(0));
apps.Start(Seconds(6));
apps.Stop(Seconds(10));

PacketSinkHelper packetSinkHelper1("ns3::UdpSocketFactory",Address(InetSocketAddress(Ipv4Address::GetAny(),port)));
ApplicationContainer sinkapp = packetSinkHelper1.Install(nodes.Get(0));
sinkapp.Start(Seconds(0));
sinkapp.Stop(Seconds(10));

V4PingHelper ping = V4PingHelper(interfaces.GetAddress(2));
NodeContainer pingers;
pingers.Add(nodes.Get(0));
pingers.Add(nodes.Get(1));

ApplicationContainer apps1;
apps1  = ping.Install(pingers);
apps1.Start(Seconds(1));
apps1.Stop(Seconds(5));

Config::Connect("/NodeList/*/ApplicationList/*/$ns3::V4Ping/Rtt",MakeCallback(&PingRtt));

AsciiTraceHelper ascii;
csma.EnableAsciiAll(ascii.CreateFileStream("labpgm2.tr"));
Simulator::Stop(Seconds(12));
Simulator::Run();
Simulator::Destroy();
return 0;
}

//3 program
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <iostream>
#include "ns3/csma-module.h"

using namespace ns3;

class MyApp:public Application
{
public:
MyApp();
virtual ~MyApp();
void Setup(Ptr<Socket> socket,Address address,uint32_t packetSize,uint32_t nPackets,
DataRate dataRate);
private:
virtual void StartApplication(void);
virtual void StopApplication(void);
void ScheduleTx(void);
void SendPacket(void);

Ptr<Socket> m_socket;
Address m_peer;
uint32_t m_packetSize;
uint32_t m_nPackets;
DataRate m_dataRate;
EventId m_sendEvent;
bool m_running;
uint32_t m_packetsSent;

};



MyApp::MyApp() //constructor
:m_socket(0),
m_peer(),
m_packetSize(0),
m_nPackets(0),
m_dataRate(0),
m_sendEvent(),
m_running(false),
m_packetsSent(0)
{
}

MyApp::~MyApp() //destructor
{
m_socket=0;
}

void MyApp::Setup(Ptr<Socket> socket,Address address,uint32_t packetSize,
uint32_t nPackets,DataRate dataRate)
{
m_socket=socket;
m_peer=address;
m_packetSize=packetSize;
m_nPackets=nPackets;
m_dataRate=dataRate;
}

void MyApp::StartApplication(void)
{
m_running=true;
m_packetsSent=0;
m_socket->Bind();
m_socket->Connect(m_peer);
SendPacket();
}


void MyApp::StopApplication(void)
{
m_running=false;
if(m_sendEvent.IsRunning())
{
Simulator::Cancel(m_sendEvent);
}
if(m_socket)
{
m_socket->Close();
}
}


void MyApp::SendPacket(void)
{
Ptr<Packet> packet=Create<Packet>(m_packetSize);
m_socket->Send(packet);
if(++m_packetsSent<m_nPackets)
{
ScheduleTx();
}
}

void MyApp::ScheduleTx(void)
{
if(m_running)
{
Time tNext(Seconds(m_packetSize*8/static_cast<double>(m_dataRate.GetBitRate())));
m_sendEvent=Simulator::Schedule(tNext,&MyApp::SendPacket,this);
}
}

static void CwndChange(uint32_t oldCwnd,uint32_t newCwnd)
{
NS_LOG_UNCOND(Simulator::Now().GetSeconds()<<"\t"<<newCwnd);
}

static void RxDrop(Ptr<const Packet> p)
{
NS_LOG_UNCOND("RxDropat"<<Simulator::Now().GetSeconds());
}


int main(int argc,char* argv[])
{
CommandLine cmd;
cmd.Parse(argc,argv);
//NS_LOG_INFO("Createnodes.");
NodeContainer nodes;
nodes.Create(4);//4csmanodesarecreated
CsmaHelper csma;
csma.SetChannelAttribute("DataRate",StringValue("5Mbps"));
csma.SetChannelAttribute("Delay",TimeValue(MilliSeconds(0.0001)));
NetDeviceContainer devices;
devices=csma.Install(nodes);

Ptr<RateErrorModel> em=CreateObject<RateErrorModel>();
em->SetAttribute("ErrorRate",DoubleValue(0.00001));
devices.Get(1)->SetAttribute("ReceiveErrorModel",PointerValue(em));

InternetStackHelper stack;
stack.Install(nodes);
Ipv4AddressHelper address;
address.SetBase("10.1.1.0","255.255.255.0");
Ipv4InterfaceContainer interfaces=address.Assign(devices);
uint16_t sinkPort=8080;

Address sinkAddress (InetSocketAddress (interfaces.GetAddress (1), sinkPort));
PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress
(Ipv4Address::GetAny(),sinkPort));
ApplicationContainer sinkApps=packetSinkHelper.Install(nodes.Get(1));
sinkApps.Start(Seconds(0.));
sinkApps.Stop(Seconds(20.));


Ptr<Socket> ns3TcpSocket=Socket::CreateSocket(nodes.Get(0),
TcpSocketFactory::GetTypeId());
ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow",MakeCallback
(&CwndChange));

Ptr<MyApp> app=CreateObject<MyApp>();

app->Setup(ns3TcpSocket,sinkAddress,1040,1000,DataRate("50Mbps"));
nodes.Get(0)->AddApplication(app);app->SetStartTime(Seconds(1.));
app->SetStopTime(Seconds(20.));
devices.Get(1)->TraceConnectWithoutContext("PhyRxDrop",MakeCallback(&RxDrop));


Simulator::Stop(Seconds(20));
Simulator::Run();
Simulator::Destroy();
return 0;
}


//4 program
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

int main(int argc,char* argv[]){
bool verbose=true;
uint32_t nWifi = 3;

CommandLine cmd;
cmd.AddValue("nWifi","No of wifi station nodes",nWifi);
cmd.AddValue("verbose","tell stations to log if true",verbose);
cmd.Parse(argc,argv);

if(verbose){
LogComponentEnable("UdpEchoClientApplication",LOG_LEVEL_INFO);
LogComponentEnable("UdpEchoServerApplication",LOG_LEVEL_INFO);
}

NodeContainer p2pNodes;
p2pNodes.Create(2);       //creating n0 and n1 that will be connected via point to point connection

PointToPointHelper p2p1;
p2p1.SetDeviceAttribute("DataRate",StringValue("5Mbps"));    //Setting channel attributes for p2p connection
p2p1.SetChannelAttribute("Delay",StringValue("2ms"));

NetDeviceContainer p2pDevices;
p2pDevices = p2p1.Install(p2pNodes);          //installing devices


NodeContainer wifiStaNodes;
wifiStaNodes.Create(nWifi);                     //creating 3 wifi station nodes : n2,n3,n4
NodeContainer wifiApNode = p2pNodes.Get(0);     //creating wifi Access Point Node which is -> n0

//Now we have to set up the physical layer and channel attributes for wifi
//we will be using default configuration for them

YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
YansWifiPhyHelper phy = YansWifiPhyHelper::Default();

//now we associate the physical layer with channel created
//so that all the physical layer devices communicate on same channel

phy.SetChannel(channel.Create());

//now we have to set up the mac layer for wifi nodes
WifiHelper wifi;
wifi.SetRemoteStationManager("ns3::AarfWifiManager"); //this method tells-->which control rate algorithm to use (we are using AARF algorithm)

WifiMacHelper mac;
Ssid ssid("ns3-ssid");  //ssid = service set identifier : basically its the unique name of the wifi network
mac.SetType("ns3::StaWifiMac",
"Ssid",SsidValue(ssid),                     //setting mac attributes for station nodes
"ActiveProbing",BooleanValue(false));

NetDeviceContainer staDevices;
staDevices = wifi.Install(phy,mac,wifiStaNodes);


mac.SetType("ns3::ApWifiMac",
"Ssid",SsidValue(ssid));

NetDeviceContainer apDevices;
apDevices = wifi.Install(phy,mac,wifiApNode);

//Now we are going to set Mobility Model
//Now we want our STA nodes(n2,n3,n4)  to be mobile .i.e, we want them to move around inside a bounding box
//And also we want AP node(n0) to be stationary ....So we will use MobilityHelper class

MobilityHelper mobility;

mobility.SetPositionAllocator(
"ns3::GridPositionAllocator",
"MinX",DoubleValue(10),
"MinY",DoubleValue(-10),
"DeltaX",DoubleValue(7),                //This code tells the mobility helper to use a two-dimensional grid to initially place the STA nodes.
"DeltaY",DoubleValue(12),
"GridWidth",UintegerValue(3),
"LayoutType",StringValue("RowFirst")
);

//We have arranged our nodes on an initial grid, but now we need to tell them how to move.
//We choose the RandomWalk2dMobilityModel which has the nodes move in a random direction
// at a random speed around inside a bounding box.

mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
"Bounds",RectangleValue(Rectangle(-50,50,-50,50)));
mobility.Install(wifiStaNodes);

//We want the access point to remain in a fixed position during the simulation.
//We accomplish this by setting the mobility model for this node to be the ns3::ConstantPositionMobilityModel


mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
mobility.Install(wifiApNode);

//Now installing protocols
InternetStackHelper stack;
stack.Install(p2pNodes.Get(1));
stack.Install(wifiApNode);
stack.Install(wifiStaNodes);

Ipv4AddressHelper address;
address.SetBase("10.1.1.0","255.255.255.0");
Ipv4InterfaceContainer p2pInterfaces = address.Assign(p2pDevices);

address.SetBase("10.1.2.0","255.255.255.0");
address.Assign(staDevices);
address.Assign(apDevices);

UdpEchoServerHelper echoServer(9);
ApplicationContainer serverapp = echoServer.Install(p2pNodes.Get(1));
serverapp.Start(Seconds(1));
serverapp.Stop(Seconds(10));

UdpEchoClientHelper echoClient(p2pInterfaces.GetAddress(1),9);    //address of the server
echoClient.SetAttribute("MaxPackets",UintegerValue(1));
echoClient.SetAttribute("Interval",TimeValue(Seconds(1)));
echoClient.SetAttribute("PacketSize",UintegerValue(1024));

ApplicationContainer clientapp = echoClient.Install(wifiStaNodes.Get(2));
clientapp.Start(Seconds(2));
clientapp.Stop(Seconds(10));

Ipv4GlobalRoutingHelper::PopulateRoutingTables();

Simulator::Stop(Seconds(10));

AsciiTraceHelper ascii;
p2p1.EnableAsciiAll(ascii.CreateFileStream("TraceFileWifides.tr"));
phy.EnableAsciiAll(ascii.CreateFileStream("TraceFileWifisrc.tr"));

Simulator::Run();
Simulator::Destroy();
return 0;
}


//5 program
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mobile-application-helper.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("EpcFirstExample");
int main(int argc, char *argv[])
{
    uint16_t numberOfNodes = 2;
    double simTime = 1.1;
    double distance = 60.0;
    double interPacketInterval = 100;

    CommandLine cmd;
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    cmd.Parse(argc, argv);
    Ptr<Node> pgw = epcHelper->GetPgwNode();

    // Create a single RemoteHost
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);

    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // Create the Internet
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);


    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

    // interface 0 is localhost, 1 is the p2p device
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);


    NodeContainer ueNodes;
    NodeContainer enbNodes;
    enbNodes.Create(numberOfNodes);
    ueNodes.Create(numberOfNodes);


    // Install Mobility Model
    MobileApplicationHelper mobileApplicatonHelper(enbNodes, ueNodes, numberOfNodes);
    mobileApplicatonHelper.SetupMobilityModule(distance);

    // Install LTE Devices to the nodes
    mobileApplicatonHelper.SetupDevices(lteHelper, epcHelper, ipv4RoutingHelper);

    // Install and start applications on UEs and remote host
    uint16_t dlPort = 1234;
    uint16_t ulPort = 2000;
    uint16_t otherPort = 3000;
    ApplicationContainer clientApps;
    ApplicationContainer serverApps;

    mobileApplicatonHelper.SetupApplications(serverApps, clientApps, remoteHost, remoteHostAddr, ulPort, dlPort, otherPort, interPacketInterval);

    serverApps.Start(Seconds(0.01));
    clientApps.Start(Seconds(0.01));
     clientApps.Stop(Seconds(8));
    lteHelper->EnableTraces();
    // Uncomment to enable PCAP tracing
    p2ph.EnablePcapAll("lena-epc-first");

    AsciiTraceHelper ascii;
    p2ph.EnableAsciiAll(ascii.CreateFileStream("cdma.tr"));

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();
    /*GtkConfigStore config;
 config.ConfigureAttributes();*/
    Simulator::Destroy();
    return 0;
}
