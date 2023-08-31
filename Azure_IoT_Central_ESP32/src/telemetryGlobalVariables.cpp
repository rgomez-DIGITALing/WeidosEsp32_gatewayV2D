#include "telemetryGlobalVariables.h"

float voltageL1N = -1;
float voltageL2N = -1;
float voltageL3N = -1;
float avgVoltageLN = -1;    //Compute

float voltageL1L2 = -1;
float voltageL2L3 = -1;
float voltageL1L3 = -1;
float avgVoltageLL = -1; //Compute

float currentL1 = -1;
float currentL2 = -1;
float currentL3 = -1;
float currentNeutral = -1; //new
float avgCurrentL = -1; //Compute
float currentTotal = -1;

float realPowerL1N = -1;
float realPowerL2N = -1;
float realPowerL3N = -1;
float realPowerTotal = -1;

float apparentPowerL1N = -1;
float apparentPowerL2N = -1;
float apparentPowerL3N = -1;
float apparentPowerTotal = -1;

float reactivePowerL1N = -1;
float reactivePowerL2N = -1;
float reactivePowerL3N = -1;
float reactivePowerTotal = -1;

float cosPhiL1 = -1;
float cosPhiL2 = -1;
float cosPhiL3 = -1;
float avgCosPhi = -1; //Compute
float frequency = -1;
float rotField = -1;

float realEnergyL1N = -1;
float realEnergyL2N = -1;
float realEnergyL3N = -1;
float realEnergyTotal = -1;

float apparentEnergyL1 = -1;
float apparentEnergyL2 = -1;
float apparentEnergyL3 = -1;
float apparentEnergyTotal = -1;

float reactiveEnergyL1 = -1;
float reactiveEnergyL2 = -1;
float reactiveEnergyL3 = -1;
float reactiveEnergyTotal = -1;

float THDVoltsL1N = -1;
float THDVoltsL2N = -1;
float THDVoltsL3N = -1;
float avgTHDVoltsLN = -1; //compute

float THDCurrentL1N = -1;
float THDCurrentL2N = -1;
float THDCurrentL3N = -1;
float avgTHDCurrentLN = -1; //compute

float THDVoltsL1L2 = -1; //new
float THDVoltsL2L3 = -1; //new
float THDVoltsL1L3 = -1; //new
float avgTHDVoltsLL = -1; //new

float powerFactorL1N = -1; //new
float powerFactorL2N = -1; //new
float powerFactorL3N = -1; //new
float powerFactorTotal = -1; //new

int comStatus = -1;
time_t timestamp = 0;




void clearData(){
    voltageL1N = -1;
    voltageL2N = -1;
    voltageL3N = -1;
    avgVoltageLN = -1;    //Compute
    voltageL1L2 = -1;
    voltageL2L3 = -1;
    voltageL1L3 = -1;
    avgVoltageLL = -1; //Compute
    currentL1 = -1;
    currentL2 = -1;
    currentL3 = -1;
    currentNeutral = -1; //new
    avgCurrentL = -1; //Compute
    currentTotal = -1;
    realPowerL1N = -1;
    realPowerL2N = -1;
    realPowerL3N = -1;
    realPowerTotal = -1;
    apparentPowerL1N = -1;
    apparentPowerL2N = -1;
    apparentPowerL3N = -1;
    apparentPowerTotal = -1;
    reactivePowerL1N = -1;
    reactivePowerL2N = -1;
    reactivePowerL3N = -1;
    reactivePowerTotal = -1;
    cosPhiL1 = -1;
    cosPhiL2 = -1;
    cosPhiL3 = -1;
    avgCosPhi = -1; //Compute
    frequency = -1;
    rotField = -1;
    realEnergyL1N = -1;
    realEnergyL2N = -1;
    realEnergyL3N = -1;
    realEnergyTotal = -1;
    apparentEnergyL1 = -1;
    apparentEnergyL2 = -1;
    apparentEnergyL3 = -1;
    apparentEnergyTotal = -1;
    reactiveEnergyL1 = -1;
    reactiveEnergyL2 = -1;
    reactiveEnergyL3 = -1;
    reactiveEnergyTotal = -1;
    THDVoltsL1N = -1;
    THDVoltsL2N = -1;
    THDVoltsL3N = -1;
    avgTHDVoltsLN = -1; //compute
    THDCurrentL1N = -1;
    THDCurrentL2N = -1;
    THDCurrentL3N = -1;
    avgTHDCurrentLN = -1; //compute
    THDVoltsL1L2 = -1; //new
    THDVoltsL2L3 = -1; //new
    THDVoltsL1L3 = -1; //new
    avgTHDVoltsLL = -1; //new
    powerFactorL1N = -1; //new
    powerFactorL2N = -1; //new
    powerFactorL3N = -1; //new
    powerFactorTotal = -1; //new
    comStatus = -1;        //new
    timestamp = 0;         //new
    return;
}
