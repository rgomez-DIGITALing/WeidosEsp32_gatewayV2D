#include "weidosTasks.h"
#include "telemetryGlobalVariables.h"

#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoModbus.h>
#include <time.h>

#include <SDLoggerAzure.h>
SDLoggerClass modbusLogger("sysLog/modules/modbus","modbus.txt");

#define ETHERNET_TIMEOUT            60000
#define ETHERNET_RESPONSE_TIMEOUT   4000

#define MODBUS_BEGIN_TRIES      3
#define MODBUS_REQUEST_TRIES    3

#define MODBUS_ADDRESS      1
#define MODBUS_TIMEOUT      5000
#define REG_ADDRESS         19000
#define NUM_REGISTERS       122
#define NUM_DATA            NUM_REGISTERS/2

#define REG_ADDRESS2         828
#define NUM_REGISTERS2       20
#define NUM_DATA2            NUM_REGISTERS2/2

#define REG_ADDRESS3         10085
#define NUM_REGISTERS3       2
#define NUM_DATA3            NUM_REGISTERS3/2


EthernetClient ethClient;
ModbusTCPClient modbusTCPClient(ethClient);

///////Ethernet data
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x01 };   //General
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x02 };   //Transelevador 1
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x03 };   //Transelevador 2
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x04 };   //Transelevador 3
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x05 };   //Robot
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x06 };   //Linea empaquetado
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x07 };   //Modula 4
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x08 };   //Modula 11
//byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x09 };   //AC oficinas (General por conducto)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x0A };   //Aire comprimido

//IPAddress serverIP(10, 88, 47, 202);        //General
//IPAddress serverIP(10, 88, 47, 242);        //Transelevador 1
//IPAddress serverIP(10, 88, 47, 243);        //Transelevador 2
//IPAddress serverIP(10, 88, 47, 244);        //Transelevador 3
//IPAddress serverIP(10, 88, 47, 220);        //Robot
//IPAddress serverIP(10, 88, 47, 221);        //Linea empaquetado
//IPAddress serverIP(10, 88, 47, 222);        //Modula 4
//IPAddress serverIP(10, 88, 47, 223);        //Modula 11
//IPAddress serverIP(10, 88, 47, 241);        //AC oficinas (General por conducto)
IPAddress serverIP(10, 88, 47, 203);          //Aire comprimido

void weidosSetup(){
    Serial.begin(115200);
    //while(!Serial){}
    delay(5000);
    modbusLogger.logInfo("Initializing modbusTask");

    Serial.println("Welcome");
    Ethernet.init(ETHERNET_CS);
    while(!Ethernet.begin(mac, ETHERNET_TIMEOUT, ETHERNET_RESPONSE_TIMEOUT)){
    Serial.print("e");
    }
    modbusLogger.logInfo("Ethernet connected successfully");

    Serial.println();
    Serial.print("Local IP: ");
    Serial.println(Ethernet.localIP());

    modbusTCPClient.setTimeout(MODBUS_TIMEOUT);  
    modbusTCPClient.begin(serverIP);

    for(int i=0; i<MODBUS_BEGIN_TRIES; i++)
    {
        if(!modbusTCPClient.connected())
        {
            modbusTCPClient.begin(serverIP);
            Serial.print("m");
            delay(1000);
        }else break;
    }

    modbusLogger.logInfo("Modbus Client connected successfully");
    modbusLogger.logInfo("End of modbusTask setup");
    
    Serial.println("Modbus Client connected");
    Serial.println("End of set up");
};


void getData(){
    modbusTCPClient.begin(serverIP);
    
    for(int i=0; i<MODBUS_BEGIN_TRIES; i++)
    {
        if(!modbusTCPClient.connected())
        {
            modbusTCPClient.begin(serverIP);
        }else break;
    }

    for(int i=0; i<MODBUS_REQUEST_TRIES; i++)
    {
        int response = modbusTCPClient.requestFrom(MODBUS_ADDRESS, INPUT_REGISTERS, REG_ADDRESS, NUM_REGISTERS);    
        timestamp = time(NULL);
        if(!response)
        {
            modbusLogger.logError("No response for modbus batch 1. Last error: ");
            modbusLogger.logError(modbusTCPClient.lastError());
            
            Serial.println("No response");
            Serial.print("Last error: ");
            Serial.println(modbusTCPClient.lastError());
            modbusTCPClient.begin(serverIP);
            comStatus = 0;
            continue;
        }
        assignDataToGlobalVariables();
        comStatus = 1;
        //timestamp = time(NULL);
        modbusLogger.logInfo("modbus batch 1 successfull read!");
        break;
    }

    for(int i=0; i<MODBUS_REQUEST_TRIES; i++)
    {
        int response = modbusTCPClient.requestFrom(MODBUS_ADDRESS, INPUT_REGISTERS, REG_ADDRESS2, NUM_REGISTERS2);    
        if(!response)
        {
            modbusLogger.logError("No response for modbus batch 2. Last error: ");
            modbusLogger.logError(modbusTCPClient.lastError());

            Serial.println("No response for batch 2");
            Serial.print("Last error: ");
            Serial.println(modbusTCPClient.lastError());
            modbusTCPClient.begin(serverIP);
            comStatus = 0;
            continue;
        }
        assignDataToGlobalVariables2();
        comStatus = 1;
        modbusLogger.logInfo("modbus batch 2 successfull read!");
        break;
    }

    for(int i=0; i<MODBUS_REQUEST_TRIES; i++)
    {
        int response = modbusTCPClient.requestFrom(MODBUS_ADDRESS, INPUT_REGISTERS, REG_ADDRESS3, NUM_REGISTERS3);    
        if(!response)
        {
            modbusLogger.logError("No response for modbus batch 3. Last error: ");
            modbusLogger.logError(modbusTCPClient.lastError());

            Serial.println("No response for batch 3");
            Serial.print("Last error: ");
            Serial.println(modbusTCPClient.lastError());
            modbusTCPClient.begin(serverIP);
            comStatus = 0;
            continue;
        }
        assignDataToGlobalVariables3();
        comStatus = 1;
        modbusLogger.logInfo("modbus batch 3 successfull read!");
        break;
    }

    time_t now = time(NULL);

    return;
}





void assignDataToGlobalVariables(){
    voltageL1N = getNextData();
    voltageL2N = getNextData();
    voltageL3N = getNextData();
    voltageL1L2 = getNextData();
    voltageL2L3 = getNextData();
    voltageL1L3 = getNextData(); 
    currentL1 = getNextData();
    currentL2 = getNextData();
    currentL3 = getNextData();
    currentTotal = getNextData();
    realPowerL1N = getNextData();
    realPowerL2N = getNextData();
    realPowerL3N = getNextData();
    realPowerTotal = getNextData();
    apparentPowerL1N = getNextData();
    apparentPowerL2N = getNextData();
    apparentPowerL3N = getNextData();
    apparentPowerTotal = getNextData();
    reactivePowerL1N = getNextData();
    reactivePowerL2N = getNextData();
    reactivePowerL3N = getNextData();
    reactivePowerTotal = getNextData();
    cosPhiL1 = getNextData();
    cosPhiL2 = getNextData();
    cosPhiL3 = getNextData();
    frequency = getNextData();
    rotField = getNextData();
    realEnergyL1N = getNextData()/1000.0f;
    realEnergyL2N = getNextData()/1000.0f;
    realEnergyL3N = getNextData()/1000.0f;
    realEnergyTotal = getNextData()/1000.0f;
    getNextData();       //realEnergyConsL1 deleted variable
    getNextData();       //realEnergyConsL2 deleted variable
    getNextData();       //realEnergyConsL3 deleted variable
    getNextData();      //realEnergyConsTotal deleted variable
    getNextData();      //realEnergyDelivL1 deleted variable     
    getNextData();      //realEnergyDelivL2 deleted variable
    getNextData();      //realEnergyDelivL3 deleted variable
    getNextData();      //realEnergyDelivTotal deleted variable
    apparentEnergyL1 = getNextData()/1000.0f;
    apparentEnergyL2 = getNextData()/1000.0f;
    apparentEnergyL3 = getNextData()/1000.0f;
    apparentEnergyTotal = getNextData()/1000.0f;
    reactiveEnergyL1 = getNextData()/1000.0f;
    reactiveEnergyL2 = getNextData()/1000.0f;
    reactiveEnergyL3 = getNextData()/1000.0f;
    reactiveEnergyTotal = getNextData()/1000.0f;
    getNextData();      //reactiveEnergyIndL1 deleted variable
    getNextData();      //reactiveEnergyIndL2 deleted variable   
    getNextData();      //reactiveEnergyIndL3 deleted variable    
    getNextData();      //reactiveEnergyIndTotal deleted variable
    getNextData();      //reactiveEnergyCapL1 deleted variable
    getNextData();      //reactiveEnergyCapL2 deleted variable
    getNextData();      //reactiveEnergyCapL3 deleted variable
    getNextData();      //reactiveEnergyCapTotal deleted variable
    THDVoltsL1N = getNextData();
    THDVoltsL2N = getNextData();
    THDVoltsL3N = getNextData();
    THDCurrentL1N = getNextData();
    THDCurrentL2N = getNextData();
    THDCurrentL3N = getNextData();
}

void assignDataToGlobalVariables2(){
    powerFactorL1N = getNextData();
    powerFactorL2N = getNextData();
    powerFactorL3N = getNextData();
    powerFactorTotal = getNextData();
    THDVoltsL1L2 = getNextData();
    THDVoltsL2L3 = getNextData();
    THDVoltsL1L3 = getNextData();
}

void assignDataToGlobalVariables3(){
    currentNeutral = getNextData();
}

void computeData(){
    avgVoltageLN = (voltageL1N + voltageL2N + voltageL3N)/3.0f;
    avgVoltageLL = (voltageL1L2 + voltageL2L3 + voltageL1L3)/3.0f;
    avgCurrentL = (currentL1 + currentL2 + currentL3)/3.0f;
    if(apparentPowerTotal != 0) avgCosPhi = realPowerTotal/apparentPowerTotal;
    else avgCosPhi = -1;
    if(isnan(avgCosPhi)) avgCosPhi = -1;  //Check if, after all, it is still NaN
    avgTHDVoltsLN = (THDVoltsL1N + THDVoltsL2N + THDVoltsL3N)/3.0f;
    avgTHDCurrentLN = (THDCurrentL1N + THDCurrentL2N + THDCurrentL3N)/3.0f;
    avgTHDVoltsLL = (THDVoltsL1L2 + THDVoltsL2L3 + THDVoltsL1L3)/3.0f;
}



float getNextData(){
    long msb = modbusTCPClient.read();
    long lsb =  modbusTCPClient.read();
    uint32_t rawData = (msb << 16) + lsb; // Bit Shift operation to join both registers
    float data = *(float *)&rawData; 
    return data;
};
