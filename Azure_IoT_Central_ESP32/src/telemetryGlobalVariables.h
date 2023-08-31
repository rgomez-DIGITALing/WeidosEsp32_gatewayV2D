#ifndef TELEMETRY_GLOBAL_VARIABLES
#define TELEMETRY_GLOBAL_VARIABLES

#include <time.h>

void clearData();

extern float voltageL1N;
extern float voltageL2N;
extern float voltageL3N;
extern float avgVoltageLN;

extern float voltageL1L2;
extern float voltageL2L3;
extern float voltageL1L3;
extern float avgVoltageLL;

extern float currentL1;
extern float currentL2;
extern float currentL3;
extern float currentNeutral;
extern float avgCurrentL;
extern float currentTotal;

extern float realPowerL1N;
extern float realPowerL2N;
extern float realPowerL3N;
extern float realPowerTotal;

extern float apparentPowerL1N;
extern float apparentPowerL2N;
extern float apparentPowerL3N;
extern float apparentPowerTotal;

extern float reactivePowerL1N;
extern float reactivePowerL2N;
extern float reactivePowerL3N;
extern float reactivePowerTotal;

extern float cosPhiL1;
extern float cosPhiL2;
extern float cosPhiL3;
extern float avgCosPhi;
extern float frequency;
extern float rotField;

extern float realEnergyL1N;
extern float realEnergyL2N;
extern float realEnergyL3N;
extern float realEnergyTotal;

extern float apparentEnergyL1;
extern float apparentEnergyL2;
extern float apparentEnergyL3;
extern float apparentEnergyTotal;

extern float reactiveEnergyL1;
extern float reactiveEnergyL2;
extern float reactiveEnergyL3;
extern float reactiveEnergyTotal;

extern float THDVoltsL1N;
extern float THDVoltsL2N;
extern float THDVoltsL3N;
extern float avgTHDVoltsLN;

extern float THDCurrentL1N;
extern float THDCurrentL2N;
extern float THDCurrentL3N;
extern float avgTHDCurrentLN;

extern float THDVoltsL1L2;
extern float THDVoltsL2L3;
extern float THDVoltsL1L3;
extern float avgTHDVoltsLL;

extern float powerFactorL1N;
extern float powerFactorL2N;
extern float powerFactorL3N;
extern float powerFactorTotal;

extern int comStatus;       //new
extern time_t timestamp;    //new


#endif