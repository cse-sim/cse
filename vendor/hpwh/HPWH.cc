/*
Copyright (c) 2014-2016 Ecotope Inc.
All rights reserved.



Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:



* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.



* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.



* Neither the name of the copyright holders nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission from the copyright holders.



THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include "HPWH.hh"
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>

using std::endl;
using std::cout;
using std::string;

const float HPWH::DENSITYWATER_kgperL = 0.998f;
const float HPWH::CPWATER_kJperkgC = 4.181f;
const float HPWH::HEATDIST_MINVALUE = 0.0001f;
const float HPWH::UNINITIALIZED_LOCATIONTEMP = -500.f;

//ugh, this should be in the header
const std::string HPWH::version_maint = "0";

#define SETPOINT_FIX	// #define to include fixes for
						// setpoint-below-water-temp issues
						//   1-22-2017

//the HPWH functions
//the publics
HPWH::HPWH() :
simHasFailed(true), isHeating(false), setpointFixed(false), hpwhVerbosity(VRB_silent),
messageCallback(NULL), messageCallbackContextPtr(NULL), numHeatSources(0),
setOfSources(NULL), tankTemps_C(NULL), doTempDepression(false), locationTemperature_C(UNINITIALIZED_LOCATIONTEMP)
{  }

HPWH::HPWH(const HPWH &hpwh){
	simHasFailed = hpwh.simHasFailed;

	hpwhVerbosity = hpwh.hpwhVerbosity;

	//these should actually be the same pointers
	messageCallback = hpwh.messageCallback;
	messageCallbackContextPtr = hpwh.messageCallbackContextPtr;

	isHeating = hpwh.isHeating;

	numHeatSources = hpwh.numHeatSources;
	setOfSources = new HeatSource[numHeatSources];
	for (int i = 0; i < numHeatSources; i++) {
		setOfSources[i] = hpwh.setOfSources[i];
		setOfSources[i].hpwh = this;
	}



	tankVolume_L = hpwh.tankVolume_L;
	tankUA_kJperHrC = hpwh.tankUA_kJperHrC;

	setpoint_C = hpwh.setpoint_C;
	numNodes = hpwh.numNodes;
	nodeDensity = hpwh.nodeDensity;
	tankTemps_C = new double[numNodes];
	for (int i = 0; i < numNodes; i++) {
		tankTemps_C[i] = hpwh.tankTemps_C[i];
	}


	outletTemp_C = hpwh.outletTemp_C;
	energyRemovedFromEnvironment_kWh = hpwh.energyRemovedFromEnvironment_kWh;
	standbyLosses_kWh = hpwh.standbyLosses_kWh;

	tankMixesOnDraw = hpwh.tankMixesOnDraw;
	doTempDepression = hpwh.doTempDepression;

	locationTemperature_C = hpwh.locationTemperature_C;

}

HPWH & HPWH::operator=(const HPWH &hpwh){
	if (this == &hpwh) {
		return *this;
	}


	simHasFailed = hpwh.simHasFailed;

	hpwhVerbosity = hpwh.hpwhVerbosity;

	//these should actually be the same pointers
	messageCallback = hpwh.messageCallback;
	messageCallbackContextPtr = hpwh.messageCallbackContextPtr;

	isHeating = hpwh.isHeating;

	numHeatSources = hpwh.numHeatSources;

	delete[] setOfSources;
	setOfSources = new HeatSource[numHeatSources];
	for (int i = 0; i < numHeatSources; i++) {
		setOfSources[i] = hpwh.setOfSources[i];
		setOfSources[i].hpwh = this;
		//HeatSource assignment will fail (causing the simulation to fail) if a
		//HeatSource has backups/companions.
		//This could be dealt with in this function (tricky), but the HeatSource
		//assignment can't know where its backup/companion pointer goes so it either
		//fails or silently does something that's not at all useful.
		//I prefer it to fail.  -NDK  1/2016
	}


	tankVolume_L = hpwh.tankVolume_L;
	tankUA_kJperHrC = hpwh.tankUA_kJperHrC;

	setpoint_C = hpwh.setpoint_C;
	numNodes = hpwh.numNodes;
	nodeDensity = hpwh.nodeDensity;

	delete[] tankTemps_C;
	tankTemps_C = new double[numNodes];
	for (int i = 0; i < numNodes; i++) {
		tankTemps_C[i] = hpwh.tankTemps_C[i];
	}


	outletTemp_C = hpwh.outletTemp_C;
	energyRemovedFromEnvironment_kWh = hpwh.energyRemovedFromEnvironment_kWh;
	standbyLosses_kWh = hpwh.standbyLosses_kWh;

	tankMixesOnDraw = hpwh.tankMixesOnDraw;
	doTempDepression = hpwh.doTempDepression;

	locationTemperature_C = hpwh.locationTemperature_C;

	return *this;
}

HPWH::~HPWH() {
	delete[] tankTemps_C;
	delete[] setOfSources;
}

string HPWH::getVersion(){
	std::stringstream version;

	version << version_major << '.' << version_minor << '.' << version_maint;

	return version.str();
}


int HPWH::runOneStep(double inletT_C, double drawVolume_L,
	double tankAmbientT_C, double heatSourceAmbientT_C,
	DRMODES DRstatus, double minutesPerStep) {
	//returns 0 on successful completion, HPWH_ABORT on failure

	//check for errors
	if (doTempDepression == true && minutesPerStep != 1) {
		msg("minutesPerStep must equal one for temperature depression to work.  \n");
		simHasFailed = true;
		return HPWH_ABORT;
	}


	if (hpwhVerbosity >= VRB_typical) {
		msg("Beginning runOneStep.  \nTank Temps: ");
		printTankTemps();
		msg("Step Inputs: InletT_C:  %.2lf, drawVolume_L:  %.2lf, tankAmbientT_C:  %.2lf, heatSourceAmbientT_C:  %.2lf, DRstatus:  %d, minutesPerStep:  %.2lf \n",
			inletT_C, drawVolume_L, tankAmbientT_C, heatSourceAmbientT_C, DRstatus, minutesPerStep);
	}
	//is the failure flag is set, don't run
	if (simHasFailed) {
		if (hpwhVerbosity >= VRB_reluctant) msg("simHasFailed is set, aborting.  \n");
		return HPWH_ABORT;
	}



	//reset the output variables
	outletTemp_C = 0;
	energyRemovedFromEnvironment_kWh = 0;
	standbyLosses_kWh = 0;

	for (int i = 0; i < numHeatSources; i++) {
		setOfSources[i].runtime_min = 0;
		setOfSources[i].energyInput_kWh = 0;
		setOfSources[i].energyOutput_kWh = 0;
	}

	// if you are doing temp. depression, set tank and heatSource ambient temps
	// to the tracked locationTemperature
	double temperatureGoal = tankAmbientT_C;
	if (doTempDepression) {
		if (locationTemperature_C == UNINITIALIZED_LOCATIONTEMP) locationTemperature_C = tankAmbientT_C;
		tankAmbientT_C = locationTemperature_C;
		heatSourceAmbientT_C = locationTemperature_C;
	}


	//process draws and standby losses
	updateTankTemps(drawVolume_L, inletT_C, tankAmbientT_C, minutesPerStep);


	//do HeatSource choice
	for (int i = 0; i < numHeatSources; i++) {
		if (hpwhVerbosity >= VRB_emetic)  msg("Heat source choice:\theatsource %d can choose from %lu turn on logics and %lu shut off logics\n", i, setOfSources[i].turnOnLogicSet.size(), setOfSources[i].shutOffLogicSet.size());

		if (isHeating == true) {
			//check if anything that is on needs to turn off (generally for lowT cutoffs)
			//things that just turn on later this step are checked for this in shouldHeat
			if (setOfSources[i].isEngaged() && setOfSources[i].shutsOff()) {
				setOfSources[i].disengageHeatSource();
				//check if the backup heat source would have to shut off too
				if (setOfSources[i].backupHeatSource != NULL && setOfSources[i].backupHeatSource->shutsOff() != true) {
					//and if not, go ahead and turn it on
					setOfSources[i].backupHeatSource->engageHeatSource();
				}
			}

			//if there's a priority HeatSource (e.g. upper resistor) and it needs to
			//come on, then turn everything off and start it up
			if (setOfSources[i].isVIP) {
				if (hpwhVerbosity >= VRB_emetic) msg("\tVIP check");
				if (setOfSources[i].shouldHeat()) {
					turnAllHeatSourcesOff();
					setOfSources[i].engageHeatSource();
					//stop looking if the VIP needs to run
					break;
				}
			}
		}
		//if nothing is currently on, then check if something should come on
		else /* (isHeating == false) */ {
			if (setOfSources[i].shouldHeat()) {
				setOfSources[i].engageHeatSource();
				//engaging a heat source sets isHeating to true, so this will only trigger once
			}
		}

	}  //end loop over heat sources

	if (hpwhVerbosity >= VRB_emetic){
		msg("after heat source choosing:  ");
		for (int i = 0; i < numHeatSources; i++) {
			msg("heat source %d: %d \t", i, setOfSources[i].isEngaged());
		}
		msg("\n");
	}




	//change the things according to DR schedule
	if (DRstatus == DR_BLOCK) {
		//force off
		turnAllHeatSourcesOff();
		isHeating = false;
	}
	else if (DRstatus == DR_ALLOW) {
		//do nothing
	}
	else if (DRstatus == DR_ENGAGE) {
		//if nothing else is on, force the first heat source on
		//this may or may not be desired behavior, pending more research (and funding)
		if (areAllHeatSourcesOff() == true) {
			if (compressorIndex > -1) {
				setOfSources[compressorIndex].engageHeatSource();
			} else if (lowestElementIndex > -1) {
				setOfSources[lowestElementIndex].engageHeatSource();
			}
		}
	}

	//do heating logic
	double minutesToRun = minutesPerStep;

	for (int i = 0; i < numHeatSources; i++) {
		// check/apply lock-outs
		if (hpwhVerbosity >= VRB_emetic)  msg("Checking lock-out logic for heat source %d:\n", i);
		if (setOfSources[i].shouldLockOut(heatSourceAmbientT_C)) {
			setOfSources[i].lockOutHeatSource();
		}
		if (setOfSources[i].shouldUnlock(heatSourceAmbientT_C)) {
			setOfSources[i].unlockHeatSource();
		}

		//going through in order, check if the heat source is on
		if (setOfSources[i].isEngaged()) {

			HeatSource* heatSourcePtr;
			if (setOfSources[i].isLockedOut() && setOfSources[i].backupHeatSource != NULL) {
				heatSourcePtr = setOfSources[i].backupHeatSource;
			} else {
				heatSourcePtr = &setOfSources[i];
			}

			//and add heat if it is
			heatSourcePtr->addHeat(heatSourceAmbientT_C, minutesToRun);
			//if it finished early
			if (heatSourcePtr->runtime_min < minutesToRun) {
				//debugging message handling
				if (hpwhVerbosity >= VRB_emetic){
					msg("done heating! runtime_min minutesToRun %.2lf %.2lf\n", heatSourcePtr->runtime_min, minutesToRun);
				}

				//subtract time it ran and turn it off
				minutesToRun -= heatSourcePtr->runtime_min;
				setOfSources[i].disengageHeatSource();
				//and if there's a heat source that follows this heat source (regardless of lockout) that's able to come on,
				if (setOfSources[i].followedByHeatSource != NULL && setOfSources[i].followedByHeatSource->shutsOff() == false) {
					//turn it on
					setOfSources[i].followedByHeatSource->engageHeatSource();
				}
			}
		}
	}

	if (areAllHeatSourcesOff() == true) {
		isHeating = false;
	}


	//track the depressed local temperature
	if (doTempDepression) {
		bool compressorRan = false;
		for (int i = 0; i < numHeatSources; i++) {
			if (setOfSources[i].isEngaged() && !setOfSources[i].isLockedOut() && setOfSources[i].depressesTemperature) {
				compressorRan = true;
			}
		}

		if (compressorRan){
			temperatureGoal -= maxDepression_C;		//hardcoded 4.5 degree total drop - from experimental data. Changed to an input
		}
		else{
			//otherwise, do nothing, we're going back to ambient
		}

		// shrink the gap by the same percentage every minute - that gives us
		// exponential behavior the percentage was determined by a fit to
		// experimental data - 9.4 minute half life and 4.5 degree total drop
		//minus-equals is important, and fits with the order of locationTemperature
		//and temperatureGoal, so as to not use fabs() and conditional tests
		locationTemperature_C -= (locationTemperature_C - temperatureGoal)*(1 - 0.9289);
	}



	//settle outputs

	//outletTemp_C and standbyLosses_kWh are taken care of in updateTankTemps

	//sum energyRemovedFromEnvironment_kWh for each heat source;
	for (int i = 0; i < numHeatSources; i++) {
		energyRemovedFromEnvironment_kWh += (setOfSources[i].energyOutput_kWh - setOfSources[i].energyInput_kWh);
	}


	//cursory check for inverted temperature profile
	if (tankTemps_C[numNodes - 1] < tankTemps_C[0]) {
		if (hpwhVerbosity >= VRB_reluctant) msg("The top of the tank is cooler than the bottom.  \n");
	}


	if (simHasFailed) {
		if (hpwhVerbosity >= VRB_reluctant) msg("The simulation has encountered an error.  \n");
		return HPWH_ABORT;
	}


	if (hpwhVerbosity >= VRB_typical) msg("Ending runOneStep.  \n\n\n\n");
	return 0;  //successful completion of the step returns 0
} //end runOneStep



int HPWH::runNSteps(int N, double *inletT_C, double *drawVolume_L,
	double *tankAmbientT_C, double *heatSourceAmbientT_C,
	DRMODES *DRstatus, double minutesPerStep) {
	//returns 0 on successful completion, HPWH_ABORT on failure

	//these are all the accumulating variables we'll need
	double energyRemovedFromEnvironment_kWh_SUM = 0;
	double standbyLosses_kWh_SUM = 0;
	double outletTemp_C_AVG = 0;
	double totalDrawVolume_L = 0;
	std::vector<double> heatSources_runTimes_SUM(numHeatSources);
	std::vector<double> heatSources_energyInputs_SUM(numHeatSources);
	std::vector<double> heatSources_energyOutputs_SUM(numHeatSources);

	if (hpwhVerbosity >= VRB_typical) msg("Begin runNSteps.  \n");
	//run the sim one step at a time, accumulating the outputs as you go
	for (int i = 0; i < N; i++) {
		runOneStep(inletT_C[i], drawVolume_L[i], tankAmbientT_C[i], heatSourceAmbientT_C[i],
			DRstatus[i], minutesPerStep);

		if (simHasFailed) {
			if (hpwhVerbosity >= VRB_reluctant) {
				msg("RunNSteps has encountered an error on step %d of N and has ceased running.  \n", i + 1);
			}
			return HPWH_ABORT;
		}

		energyRemovedFromEnvironment_kWh_SUM += energyRemovedFromEnvironment_kWh;
		standbyLosses_kWh_SUM += standbyLosses_kWh;

		outletTemp_C_AVG += outletTemp_C * drawVolume_L[i];
		totalDrawVolume_L += drawVolume_L[i];

		for (int j = 0; j < numHeatSources; j++) {
			heatSources_runTimes_SUM[j] += getNthHeatSourceRunTime(j);
			heatSources_energyInputs_SUM[j] += getNthHeatSourceEnergyInput(j);
			heatSources_energyOutputs_SUM[j] += getNthHeatSourceEnergyOutput(j);
		}

		//print minutely output
		if (hpwhVerbosity == VRB_minuteOut) {
			msg("%f,%f,%f,", tankAmbientT_C[i], drawVolume_L[i], inletT_C[i]);
			for (int j = 0; j < numHeatSources; j++) {
				msg("%f,%f,", getNthHeatSourceEnergyInput(j), getNthHeatSourceEnergyOutput(j));
			}
			msg("%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
				tankTemps_C[0 * numNodes / 12], tankTemps_C[1 * numNodes / 12],
				tankTemps_C[2 * numNodes / 12], tankTemps_C[3 * numNodes / 12],
				tankTemps_C[4 * numNodes / 12], tankTemps_C[5 * numNodes / 12],
				tankTemps_C[6 * numNodes / 12], tankTemps_C[7 * numNodes / 12],
				tankTemps_C[8 * numNodes / 12], tankTemps_C[9 * numNodes / 12],
				tankTemps_C[10 * numNodes / 12], tankTemps_C[11 * numNodes / 12],
				getNthSimTcouple(1), getNthSimTcouple(2), getNthSimTcouple(3),
				getNthSimTcouple(4), getNthSimTcouple(5), getNthSimTcouple(6));
		}

	}
	//finish weighted avg. of outlet temp by dividing by the total drawn volume
	outletTemp_C_AVG /= totalDrawVolume_L;

	//now, reassign all of the accumulated values to their original spots
	energyRemovedFromEnvironment_kWh = energyRemovedFromEnvironment_kWh_SUM;
	standbyLosses_kWh = standbyLosses_kWh_SUM;
	outletTemp_C = outletTemp_C_AVG;

	for (int i = 0; i < numHeatSources; i++) {
		setOfSources[i].runtime_min = heatSources_runTimes_SUM[i];
		setOfSources[i].energyInput_kWh = heatSources_energyInputs_SUM[i];
		setOfSources[i].energyOutput_kWh = heatSources_energyOutputs_SUM[i];
	}

	if (hpwhVerbosity >= VRB_typical) msg("Ending runNSteps.  \n\n\n\n");
	return 0;
}



void HPWH::setVerbosity(VERBOSITY hpwhVrb){
	hpwhVerbosity = hpwhVrb;
}
void HPWH::setMessageCallback(void(*callbackFunc)(const string message, void* contextPtr), void* contextPtr){
	messageCallback = callbackFunc;
	messageCallbackContextPtr = contextPtr;
}
void HPWH::sayMessage(const string message) const{
	if (messageCallback != NULL) {
		(*messageCallback)(message, messageCallbackContextPtr);
	}
	else {
		std::cout << message;
	}
}
void HPWH::msg(const char* fmt, ...) const {
	va_list ap; va_start(ap, fmt);
	msgV(fmt, ap);
}
void HPWH::msgV(const char* fmt, va_list ap /*=NULL*/) const {
	char outputString[MAXOUTSTRING];

	const char* p;
	if (ap) {
#if defined( _MSC_VER)
		vsprintf_s< MAXOUTSTRING>(outputString, fmt, ap);
#else
		vsnprintf(outputString, MAXOUTSTRING, fmt, ap);
#endif
		p = outputString;
	}
	else {
		p = fmt;
	}
	sayMessage(p);
}		// HPWH::msgV


void HPWH::printHeatSourceInfo(){
	std::stringstream ss;
	double runtime = 0, outputVar = 0;

	ss << std::left;
	ss << std::fixed;
	ss << std::setprecision(2);
	for (int i = 0; i < getNumHeatSources(); i++) {
		ss << "heat source " << i << ": " << isNthHeatSourceRunning(i) << "\t\t";
	}
	ss << endl;

	for (int i = 0; i < getNumHeatSources(); i++) {
		ss << "input energy kwh: " << std::setw(7) << getNthHeatSourceEnergyInput(i) << "\t";
	}
	ss << endl;

	for (int i = 0; i < getNumHeatSources(); i++) {
		runtime = getNthHeatSourceRunTime(i);
		if (runtime != 0) { outputVar = getNthHeatSourceEnergyInput(i) / (runtime / 60.0); }
		else { outputVar = 0; }
		ss << "input power kw: " << std::setw(7) << outputVar << "\t\t";
	}
	ss << endl;

	for (int i = 0; i < getNumHeatSources(); i++) {
		ss << "output energy kwh: " << std::setw(7) << getNthHeatSourceEnergyOutput(i) << "\t";
	}
	ss << endl;

	for (int i = 0; i < getNumHeatSources(); i++) {
		runtime = getNthHeatSourceRunTime(i);
		if (runtime != 0) { outputVar = getNthHeatSourceEnergyOutput(i) / (runtime / 60.0); }
		else { outputVar = 0; }
		ss << "output power kw: " << std::setw(7) << outputVar << "\t";
	}
	ss << endl;

	for (int i = 0; i < getNumHeatSources(); i++) {
		ss << "run time min: " << std::setw(7) << getNthHeatSourceRunTime(i) << "\t\t";
	}
	ss << endl << endl << endl;


	msg(ss.str().c_str());
}


void HPWH::printTankTemps() {
	std::stringstream ss;

	ss << std::left;

	for (int i = 0; i < getNumNodes(); i++) {
		ss << std::setw(9) << getTankNodeTemp(i) << " ";
	}
	ss << endl;

	msg(ss.str().c_str());
}


// public members to write to CSV file
int HPWH::WriteCSVHeading(FILE* outFILE, const char* preamble) const {
	fprintf(outFILE, "%s", preamble);
	fprintf(outFILE, "h_src%dIn (Wh),h_src%dOut (Wh)", 1, 1);
	for (int iHS = 1; iHS < getNumHeatSources(); iHS++) {
		fprintf(outFILE, ",h_src%dIn (Wh),h_src%dOut (Wh)", iHS + 1, iHS + 1);
	}
	for (int iTC = 0; iTC < 6; iTC++)  fprintf(outFILE, ",tcouple%d (C)", iTC + 1);
	fprintf(outFILE, "\n");
	return 0;
}
int HPWH::WriteCSVRow(FILE* outFILE, const char* preamble) const {
	fprintf(outFILE, "%s", preamble);
	fprintf(outFILE, "%0.2f,%0.2f", getNthHeatSourceEnergyInput(0)*1000.,
		getNthHeatSourceEnergyOutput(0)*1000.);
	for (int iHS = 1; iHS < getNumHeatSources(); iHS++) {
		fprintf(outFILE, ",%0.2f,%0.2f", getNthHeatSourceEnergyInput(iHS)*1000.,
			getNthHeatSourceEnergyOutput(iHS)*1000.);
	}
	for (int iTC = 0; iTC < 6; iTC++)  fprintf(outFILE, ",%0.2f", getNthSimTcouple(iTC + 1));
	fprintf(outFILE, "\n");
	return 0;
}


bool HPWH::isSetpointFixed(){
	return setpointFixed;
}

int HPWH::setSetpoint(double newSetpoint){
	if (setpointFixed == true) {
		if (hpwhVerbosity >= VRB_reluctant) msg("Unwilling to set setpoint for your currently selected model.  \n");
		return HPWH_ABORT;
	}
	else{
		setpoint_C = newSetpoint;
	}
	return 0;
}
int HPWH::setSetpoint(double newSetpoint, UNITS units) {
	if (units == UNITS_C) {
		return setSetpoint(newSetpoint);
	}
	else if (units == UNITS_F) {
		return setSetpoint(F_TO_C(newSetpoint));
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getNthSimTcouple.  \n");
		return HPWH_ABORT;
	}
}
double HPWH::getSetpoint(){
	return setpoint_C;
}


int HPWH::resetTankToSetpoint(){
	for (int i = 0; i < numNodes; i++) {
		tankTemps_C[i] = setpoint_C;
	}
	return 0;
}


int HPWH::setAirFlowFreedom(double fanFraction) {
	if (fanFraction < 0 || fanFraction > 1) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to set the fan fraction outside of bounds.  \n");
		simHasFailed = true;
		return HPWH_ABORT;
	}
	else {
		for (int i = 0; i < numHeatSources; i++) {
			if (setOfSources[i].typeOfHeatSource == TYPE_compressor) setOfSources[i].airflowFreedom = fanFraction;
		}
	}
	return 0;
}

int HPWH::setDoTempDepression(bool doTempDepress) {
	this->doTempDepression = doTempDepress;
	return 0;
}

int HPWH::setTankSize(double HPWH_size_L) {
	if (HPWH_size_L <= 0) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to set the tank volume outside of bounds.  \n");
		simHasFailed = true;
		return HPWH_ABORT;
	}
	else {
		this->tankVolume_L = HPWH_size_L;
	}
	return 0;
}
int HPWH::setTankSize(double HPWH_size, UNITS units) {
	if (units == UNITS_L) {
		return setTankSize(HPWH_size);
	}
	else if (units == UNITS_GAL) {
		return setTankSize(GAL_TO_L(HPWH_size));
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for setTankSize.  \n");
		return HPWH_ABORT;
	}
}

int HPWH::setUA(double UA_kJperHrC) {
	tankUA_kJperHrC = UA_kJperHrC;
	return 0;
}
int HPWH::setUA(double UA, UNITS units) {
	if (units == UNITS_kJperHrC) {
		return setUA(UA);
	}
	else if (units == UNITS_BTUperHrF) {
		return setUA(UAf_TO_UAc(UA));
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for setUA.  \n");
		return HPWH_ABORT;
	}
}

int HPWH::getUA(double& UA_kJperHrC) const {
	UA_kJperHrC = tankUA_kJperHrC;
	return 0;
}
int HPWH::getUA(double& UA, UNITS units) const
{
	int ret = getUA(UA);
	if (units == UNITS_kJperHrC) {
		// UA is already in correct units
	}
	else if (units == UNITS_BTUperHrF) {
		UA = UA / UAf_TO_UAc( 1.);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) {
			msg("Incorrect unit specification for getUA.  \n");
		}
		UA = -1.;
		ret = HPWH_ABORT;
	}
	return ret;
}

int HPWH::setMaxTempDepression(double maxDepression) {
  this->maxDepression_C = maxDepression;
  return 0;
}
int HPWH::setMaxTempDepression(double maxDepression, UNITS units) {
  if(units == UNITS_C) {
    return setMaxTempDepression(maxDepression);
  }
  else if(units == UNITS_F) {
    return setMaxTempDepression(F_TO_C(maxDepression));
  }
  else {
    if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for max Temp Depression.  \n");
    return HPWH_ABORT;
  }
}

HPWH::HeatingLogic HPWH::topThird(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {9,10,11,12}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("top third", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::topThird_absolute(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {9,10,11,12}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("top third absolute", nodeWeights, d, true);
}


HPWH::HeatingLogic HPWH::bottomThird(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {1,2,3,4}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("bottom third", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::bottomSixth(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {1,2}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("bottom sixth", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::secondSixth(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {3,4}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("second sixth", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::thirdSixth(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {5,6}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("third sixth", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::fourthSixth(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {7,8}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("fourth sixth", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::fifthSixth(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {9,10}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("fifth sixth", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::topSixth(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {11,12}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("top sixth", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::bottomHalf(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {1,2,3,4,5,6}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("bottom half", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::bottomTwelth(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {7,8,9,10,11,12}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("bottom twelth", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::standby(double d) const {
	std::vector<NodeWeight> nodeWeights;
	nodeWeights.emplace_back(13); // uses very top computation node
	return HPWH::HeatingLogic("standby", nodeWeights, d);
}

HPWH::HeatingLogic HPWH::topNodeMaxTemp(double d) const {
	std::vector<NodeWeight> nodeWeights;
	nodeWeights.emplace_back(13); // uses very top computation node
	return HPWH::HeatingLogic("top node", nodeWeights, d, true, std::greater<double>());
}

HPWH::HeatingLogic HPWH::bottomNodeMaxTemp(double d) const {
	std::vector<NodeWeight> nodeWeights;
	nodeWeights.emplace_back(0); // uses very bottom computation node
	return HPWH::HeatingLogic("bottom node", nodeWeights, d, true, std::greater<double>());
}

HPWH::HeatingLogic HPWH::bottomTwelthMaxTemp(double d) const {
	std::vector<NodeWeight> nodeWeights;
	nodeWeights.emplace_back(1);
	return HPWH::HeatingLogic("bottom twelth", nodeWeights, d, true, std::greater<double>());
}

HPWH::HeatingLogic HPWH::largeDraw(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {1,2,3,4}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("large draw", nodeWeights, d, true);
}

HPWH::HeatingLogic HPWH::largerDraw(double d) const {
	std::vector<NodeWeight> nodeWeights;
	for (auto i : {1,2,3,4,5,6}) {
			nodeWeights.emplace_back(i);
	}
	return HPWH::HeatingLogic("larger draw", nodeWeights, d, true);
}

int HPWH::getNumNodes() const {
	return numNodes;
}

double HPWH::getTankNodeTemp(int nodeNum) const {
	if (nodeNum > numNodes || nodeNum < 0) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to access the temperature of a tank node that does not exist.  \n");
		return double(HPWH_ABORT);
	}
	return tankTemps_C[nodeNum];
}

double HPWH::getTankNodeTemp(int nodeNum, UNITS units) const {
	double result = getTankNodeTemp(nodeNum);
	if (result == double(HPWH_ABORT)) {
		return result;
	}

	if (units == UNITS_C) {
		return result;
	}
	else if (units == UNITS_F) {
		return C_TO_F(result);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getTankNodeTemp.  \n");
		return double(HPWH_ABORT);
	}
}


double HPWH::getNthSimTcouple(int N) const {
	if (N > 6 || N < 1) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to access a simulated thermocouple that does not exist.  \n");
		return double(HPWH_ABORT);
	}

	double averageTemp_C = 0;
	//specify N from 1-6, so use N-1 for node number
	for (int i = (N - 1)*(numNodes / 6); i < N*(numNodes / 6); i++) {
		averageTemp_C += getTankNodeTemp(i);
	}
	averageTemp_C /= (numNodes / 6);
	return averageTemp_C;
}

double HPWH::getNthSimTcouple(int N, UNITS units) const {
	double result = getNthSimTcouple(N);
	if (result == double(HPWH_ABORT)) {
		return result;
	}

	if (units == UNITS_C) {
		return result;
	}
	else if (units == UNITS_F) {
		return C_TO_F(result);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getNthSimTcouple.  \n");
		return double(HPWH_ABORT);
	}
}


int HPWH::getNumHeatSources() const {
	return numHeatSources;
}


double HPWH::getNthHeatSourceEnergyInput(int N) const {
	//energy used by the heat source is positive - this should always be positive
	if (N > numHeatSources || N < 0) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to access the energy input of a heat source that does not exist.  \n");
		return double(HPWH_ABORT);
	}
	return setOfSources[N].energyInput_kWh;
}

double HPWH::getNthHeatSourceEnergyInput(int N, UNITS units) const {
	//energy used by the heat source is positive - this should always be positive
	double returnVal = getNthHeatSourceEnergyInput(N);
	if (returnVal == double(HPWH_ABORT)) {
		return returnVal;
	}

	if (units == UNITS_KWH) {
		return returnVal;
	}
	else if (units == UNITS_BTU) {
		return KWH_TO_BTU(returnVal);
	}
	else if (units == UNITS_KJ) {
		return KWH_TO_KJ(returnVal);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getNthHeatSourceEnergyInput.  \n");
		return double(HPWH_ABORT);
	}
}


double HPWH::getNthHeatSourceEnergyOutput(int N) const {
	//returns energy from the heat source into the water - this should always be positive
	if (N > numHeatSources || N < 0) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to access the energy output of a heat source that does not exist.  \n");
		return double(HPWH_ABORT);
	}
	return setOfSources[N].energyOutput_kWh;
}

double HPWH::getNthHeatSourceEnergyOutput(int N, UNITS units) const {
	//returns energy from the heat source into the water - this should always be positive
	double returnVal = getNthHeatSourceEnergyOutput(N);
	if (returnVal == double(HPWH_ABORT)) {
		return returnVal;
	}

	if (units == UNITS_KWH) {
		return returnVal;
	}
	else if (units == UNITS_BTU) {
		return KWH_TO_BTU(returnVal);
	}
	else if (units == UNITS_KJ) {
		return KWH_TO_KJ(returnVal);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getNthHeatSourceEnergyInput.  \n");
		return double(HPWH_ABORT);
	}
}


double HPWH::getNthHeatSourceRunTime(int N) const {
	if (N > numHeatSources || N < 0) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to access the run time of a heat source that does not exist.  \n");
		return double(HPWH_ABORT);
	}
	return setOfSources[N].runtime_min;
}


int HPWH::isNthHeatSourceRunning(int N) const{
	if (N > numHeatSources || N < 0) {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have attempted to access the status of a heat source that does not exist.  \n");
		return HPWH_ABORT;
	}
	if (setOfSources[N].isEngaged()){
		return 1;
	}
	else{
		return 0;
	}
}


HPWH::HEATSOURCE_TYPE HPWH::getNthHeatSourceType(int N) const{
	return setOfSources[N].typeOfHeatSource;
}



double HPWH::getOutletTemp() const {
	return outletTemp_C;
}

double HPWH::getOutletTemp(UNITS units) const {
	double returnVal = getOutletTemp();
	if (returnVal == double(HPWH_ABORT)) {
		return returnVal;
	}

	if (units == UNITS_C) {
		return returnVal;
	}
	else if (units == UNITS_F) {
		return C_TO_F(returnVal);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getOutletTemp.  \n");
		return double(HPWH_ABORT);
	}
}


double HPWH::getEnergyRemovedFromEnvironment() const {
	//moving heat from the space to the water is the positive direction
	return energyRemovedFromEnvironment_kWh;
}

double HPWH::getEnergyRemovedFromEnvironment(UNITS units) const {
	//moving heat from the space to the water is the positive direction
	double returnVal = getEnergyRemovedFromEnvironment();

	if (units == UNITS_KWH) {
		return returnVal;
	}
	else if (units == UNITS_BTU) {
		return KWH_TO_BTU(returnVal);
	}
	else if (units == UNITS_KJ){
		return KWH_TO_KJ(returnVal);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getEnergyRemovedFromEnvironment.  \n");
		return double(HPWH_ABORT);
	}
}


double HPWH::getStandbyLosses() const {
	//moving heat from the water to the space is the positive direction
	return standbyLosses_kWh;
}

double HPWH::getStandbyLosses(UNITS units) const {
	//moving heat from the water to the space is the positive direction
	double returnVal = getStandbyLosses();

	if (units == UNITS_KWH) {
		return returnVal;
	}
	else if (units == UNITS_BTU) {
		return KWH_TO_BTU(returnVal);
	}
	else if (units == UNITS_KJ){
		return KWH_TO_KJ(returnVal);
	}
	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("Incorrect unit specification for getStandbyLosses.  \n");
		return double(HPWH_ABORT);
	}
}

double HPWH::getTankHeatContent_kJ() const {
	// returns tank heat content relative to 0 C using kJ

	//get average tank temperature
	double avgTemp = 0;
	for (int i = 0; i < numNodes; i++) avgTemp += tankTemps_C[i];
	avgTemp /= numNodes;

	double totalHeat = avgTemp * DENSITYWATER_kgperL * CPWATER_kJperkgC * tankVolume_L;
	return totalHeat;
}

double HPWH::getLocationTemp_C() const {
  return locationTemperature_C;
}




//the privates
void HPWH::updateTankTemps(double drawVolume_L, double inletT_C, double tankAmbientT_C, double minutesPerStep) {
	//set up some useful variables for calculations
	double volPerNode_LperNode = tankVolume_L / numNodes;
	double drawFraction;
	int wholeNodesToDraw;
	this->outletTemp_C = 0;

	if (drawVolume_L > 0){
		//calculate how many nodes to draw (wholeNodesToDraw), and the remainder (drawFraction)
		drawFraction = drawVolume_L / volPerNode_LperNode;
		if (drawFraction > numNodes) {
			if (hpwhVerbosity >= VRB_reluctant) msg("Drawing more than the tank volume in one step is undefined behavior.  Terminating simulation.  \n");
			simHasFailed = true;
			return;
		}


		wholeNodesToDraw = (int)std::floor(drawFraction);
		drawFraction -= wholeNodesToDraw;

		//move whole nodes
		if (wholeNodesToDraw > 0) {
			for (int i = 0; i < wholeNodesToDraw; i++) {
				//add temperature of drawn nodes for outletT average
				outletTemp_C += tankTemps_C[numNodes - 1 - i];
			}

			for (int i = numNodes - 1; i >= 0; i--) {
				if (i > wholeNodesToDraw - 1) {
					//move nodes up
					tankTemps_C[i] = tankTemps_C[i - wholeNodesToDraw];
				}
				else {
					//fill in bottom nodes with inlet water
					tankTemps_C[i] = inletT_C;
				}
			}
		}
		//move fractional node
		if (drawFraction > 0) {
			//add temperature for outletT average
			outletTemp_C += drawFraction*tankTemps_C[numNodes - 1];
			//move partial nodes up
			for (int i = numNodes - 1; i > 0; i--) {
				tankTemps_C[i] = tankTemps_C[i] * (1.0 - drawFraction) + tankTemps_C[i - 1] * drawFraction;
			}
			//fill in bottom partial node with inletT
			tankTemps_C[0] = tankTemps_C[0] * (1.0 - drawFraction) + inletT_C*drawFraction;
		}

		//fill in average outlet T - it is a weighted averaged, with weights == nodes drawn
		this->outletTemp_C /= (wholeNodesToDraw + drawFraction);


		//Account for mixing at the bottom of the tank
		if (tankMixesOnDraw == true && drawVolume_L > 0) {
			int mixedBelowNode = numNodes / 3;
			double ave = 0;

			for (int i = 0; i < mixedBelowNode; i++) {
				ave += tankTemps_C[i];
			}
			ave /= mixedBelowNode;

			for (int i = 0; i < mixedBelowNode; i++) {
				tankTemps_C[i] += ((ave - tankTemps_C[i]) / 3.0);
			}
		}

	} //end if(draw_volume_L > 0)


	//calculate standby losses
	//get average tank temperature
	double avgTemp = 0;
	for (int i = 0; i < numNodes; i++) avgTemp += tankTemps_C[i];
	avgTemp /= numNodes;

	//kJ's lost as standby in the current time step
	double standbyLosses_kJ = (tankUA_kJperHrC * (avgTemp - tankAmbientT_C) * (minutesPerStep / 60.0));
	standbyLosses_kWh = KJ_TO_KWH(standbyLosses_kJ);

	//The effect of standby loss on temperature in each segment
	double lossPerNode_C = (standbyLosses_kJ / numNodes) / ((volPerNode_LperNode * DENSITYWATER_kgperL) * CPWATER_kJperkgC);
	for (int i = 0; i < numNodes; i++) tankTemps_C[i] -= lossPerNode_C;
}  //end updateTankTemps


void HPWH::turnAllHeatSourcesOff() {
	for (int i = 0; i < numHeatSources; i++) {
		setOfSources[i].disengageHeatSource();
	}
	isHeating = false;
}


bool HPWH::areAllHeatSourcesOff() const {
	bool allOff = true;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isEngaged() == true) {
			allOff = false;
		}
	}
	return allOff;
}

double HPWH::tankAvg_C(const std::vector<HPWH::NodeWeight> nodeWeights) const {
	double sum = 0;
	double totWeight = 0;

	for (auto nodeWeight : nodeWeights) {
		// bottom calc node only
		if (nodeWeight.nodeNum == 0) {
			sum += tankTemps_C[0] * nodeWeight.weight;
			totWeight += nodeWeight.weight;
		}
		// top calc node only
		else if (nodeWeight.nodeNum == 13) {
			sum += tankTemps_C[numNodes - 1] * nodeWeight.weight;
			totWeight += nodeWeight.weight;
		}
		else {
			for (int n = 0; n < nodeDensity; ++n) {
				int calcNode = (nodeWeight.nodeNum - 1) * nodeDensity + n;
				sum += tankTemps_C[calcNode] * nodeWeight.weight;
				totWeight += nodeWeight.weight;
			}
		}
	}
	return sum / totWeight;
}

//these are the HeatSource functions
//the public functions
HPWH::HeatSource::HeatSource(HPWH *parentInput)
	:hpwh(parentInput), isOn(false), lockedOut(false), backupHeatSource(NULL), companionHeatSource(NULL),
	followedByHeatSource(NULL), minT(-273.15), maxT(100), hysteresis_dC(0), airflowFreedom(1.0),
	typeOfHeatSource(TYPE_none) {}

HPWH::HeatSource::HeatSource(const HeatSource &hSource){
	hpwh = hSource.hpwh;
	isOn = hSource.isOn;
	lockedOut = hSource.lockedOut;

	runtime_min = hSource.runtime_min;
	energyInput_kWh = hSource.energyInput_kWh;
	energyOutput_kWh = hSource.energyOutput_kWh;

	isVIP = hSource.isVIP;

	if (hSource.backupHeatSource != NULL || hSource.companionHeatSource != NULL || hSource.followedByHeatSource != NULL) {
		hpwh->simHasFailed = true;
		if (hpwh->hpwhVerbosity >= VRB_reluctant)  hpwh->msg("HeatSources cannot be copied if they contain pointers to other HeatSources\n");
	}

	for (int i = 0; i < CONDENSITY_SIZE; i++) {
		condensity[i] = hSource.condensity[i];
	}
	shrinkage = hSource.shrinkage;

	perfMap = hSource.perfMap;

	//i think vector assignment works correctly here
	turnOnLogicSet = hSource.turnOnLogicSet;
	shutOffLogicSet = hSource.shutOffLogicSet;

	minT = hSource.minT;
	maxT = hSource.maxT;
	hysteresis_dC = hSource.hysteresis_dC;

	depressesTemperature = hSource.depressesTemperature;
	airflowFreedom = hSource.airflowFreedom;

	configuration = hSource.configuration;
	typeOfHeatSource = hSource.typeOfHeatSource;

	lowestNode = hSource.lowestNode;


}

HPWH::HeatSource& HPWH::HeatSource::operator=(const HeatSource &hSource){
	if (this == &hSource) {
		return *this;
	}

	hpwh = hSource.hpwh;
	isOn = hSource.isOn;
	lockedOut = hSource.lockedOut;

	runtime_min = hSource.runtime_min;
	energyInput_kWh = hSource.energyInput_kWh;
	energyOutput_kWh = hSource.energyOutput_kWh;

	isVIP = hSource.isVIP;

	if (hSource.backupHeatSource != NULL || hSource.companionHeatSource != NULL || hSource.followedByHeatSource != NULL) {
		hpwh->simHasFailed = true;
		if (hpwh->hpwhVerbosity >= VRB_reluctant)  hpwh->msg("HeatSources cannot be copied if they contain pointers to other HeatSources\n");
	}
	else {
		companionHeatSource = NULL;
		backupHeatSource = NULL;
		followedByHeatSource = NULL;
	}

	for (int i = 0; i < CONDENSITY_SIZE; i++) {
		condensity[i] = hSource.condensity[i];
	}
	shrinkage = hSource.shrinkage;

	perfMap = hSource.perfMap;

	//i think vector assignment works correctly here
	turnOnLogicSet = hSource.turnOnLogicSet;
	shutOffLogicSet = hSource.shutOffLogicSet;

	minT = hSource.minT;
	maxT = hSource.maxT;
	hysteresis_dC = hSource.hysteresis_dC;

	depressesTemperature = hSource.depressesTemperature;
	airflowFreedom = hSource.airflowFreedom;

	configuration = hSource.configuration;
	typeOfHeatSource = hSource.typeOfHeatSource;

	lowestNode = hSource.lowestNode;

	return *this;
}



void HPWH::HeatSource::setCondensity(double cnd1, double cnd2, double cnd3, double cnd4,
	double cnd5, double cnd6, double cnd7, double cnd8,
	double cnd9, double cnd10, double cnd11, double cnd12) {
	condensity[0] = cnd1;
	condensity[1] = cnd2;
	condensity[2] = cnd3;
	condensity[3] = cnd4;
	condensity[4] = cnd5;
	condensity[5] = cnd6;
	condensity[6] = cnd7;
	condensity[7] = cnd8;
	condensity[8] = cnd9;
	condensity[9] = cnd10;
	condensity[10] = cnd11;
	condensity[11] = cnd12;
}

int HPWH::HeatSource::findParent() const {
	for (int i = 0; i < hpwh->numHeatSources; ++i) {
		if (this == hpwh->setOfSources[i].backupHeatSource) {
			return i;
		}
	}
	return -1;
}

bool HPWH::HeatSource::isEngaged() const {
	return isOn;
}

bool HPWH::HeatSource::isLockedOut() const {
	return lockedOut;
}

void HPWH::HeatSource::lockOutHeatSource() {
	lockedOut = true;
}

void HPWH::HeatSource::unlockHeatSource() {
	lockedOut = false;
}

bool HPWH::HeatSource::shouldLockOut(double heatSourceAmbientT_C) const {

	// if it's already locked out, keep it locked out
	if (isLockedOut() == true) {
		return true;
	}
	else {
		//when the "external" temperature is too cold - typically used for compressor low temp. cutoffs
		//when running, use hysteresis
		bool lock = false;
		if (isEngaged() == true && heatSourceAmbientT_C < minT - hysteresis_dC) {
			lock = true;
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic) hpwh->msg("\tlock-out: running below minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
		}
		//when not running, don't use hysteresis
		else if (isEngaged() == false && heatSourceAmbientT_C < minT) {
			lock = true;
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic) hpwh->msg("\tlock-out: already below minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
		}

		//when the "external" temperature is too warm - typically used for resistance lockout
		//when running, use hysteresis
		if (isEngaged() == true && heatSourceAmbientT_C > maxT + hysteresis_dC) {
			lock = true;
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic) hpwh->msg("\tlock-out: running above maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
		}
		//when not running, don't use hysteresis
		else if (isEngaged() == false && heatSourceAmbientT_C > maxT) {
			lock = true;
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic) hpwh->msg("\tlock-out: already above maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
		}
		if (lock == true && backupHeatSource == NULL) {
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic) hpwh->msg("\nWARNING: lock-out triggered, but no backupHeatSource defined. Simulation will continue without lock-out");
			lock = false;
		}
		if (hpwh->hpwhVerbosity >= VRB_typical) hpwh->msg("\n");
		return lock;
	}
}

bool HPWH::HeatSource::shouldUnlock(double heatSourceAmbientT_C) const {

	// if it's already unlocked, keep it unlocked
	if (isLockedOut() == false) {
		return true;
	}
	else {
		//when the "external" temperature is no longer too cold or too warm
		//when running, use hysteresis
		bool unlock = false;
		if (isEngaged() == true && heatSourceAmbientT_C > minT + hysteresis_dC && heatSourceAmbientT_C < maxT - hysteresis_dC) {
			unlock = true;
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C > minT + hysteresis_dC) hpwh->msg("\tunlock: running above minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C < maxT - hysteresis_dC) hpwh->msg("\tunlock: running below maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
		}
		//when not running, don't use hysteresis
		else if (isEngaged() == false && heatSourceAmbientT_C > minT && heatSourceAmbientT_C < maxT) {
			unlock = true;
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C > minT) hpwh->msg("\tunlock: already above minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
			if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C < maxT) hpwh->msg("\tunlock: already below maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
		}
		if (hpwh->hpwhVerbosity >= VRB_typical) hpwh->msg("\n");
		return unlock;
	}
}

void HPWH::HeatSource::engageHeatSource() {
	isOn = true;
	hpwh->isHeating = true;
	if (companionHeatSource != NULL &&
		companionHeatSource->shutsOff() != true &&
		companionHeatSource->isEngaged() == false) {
		companionHeatSource->engageHeatSource();
	}
}


void HPWH::HeatSource::disengageHeatSource() {
	isOn = false;
}

bool HPWH::HeatSource::shouldHeat() const {
	//return true if the heat source logic tells it to come on, false if it doesn't,
	//or if an unsepcified selector was used
	bool shouldEngage = false;

	for (int i = 0; i < (int)turnOnLogicSet.size(); i++) {
		if (hpwh->hpwhVerbosity >= VRB_emetic)  hpwh->msg("\tshouldHeat logic: %s ", turnOnLogicSet[i].description.c_str());

		double average = hpwh->tankAvg_C(turnOnLogicSet[i].nodeWeights);
		double comparison;

		if (turnOnLogicSet[i].isAbsolute) {
			comparison = turnOnLogicSet[i].decisionPoint;
		} else {
			comparison = hpwh->setpoint_C - turnOnLogicSet[i].decisionPoint;
		}

		if (turnOnLogicSet[i].compare(average, comparison)) {
			shouldEngage = true;

			//debugging message handling
			if (hpwh->hpwhVerbosity >= VRB_typical) hpwh->msg("engages!\n");
			if (hpwh->hpwhVerbosity >= VRB_emetic){
				hpwh->msg("average: %.2lf \t setpoint: %.2lf \t decisionPoint: %.2lf \n", average, hpwh->setpoint_C, turnOnLogicSet[i].decisionPoint);
			}
		}

		//quit searching the logics if one of them turns it on
		if (shouldEngage) break;

		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("returns: %d \t", shouldEngage);
		}
	}  //end loop over set of logic conditions

	//if everything else wants it to come on, but if it would shut off anyways don't turn it on
	if (shouldEngage == true && shutsOff() == true) {
		shouldEngage = false;
		if (hpwh->hpwhVerbosity >= VRB_typical) hpwh->msg("but is denied by shutsOff");
	}

	if (hpwh->hpwhVerbosity >= VRB_typical) hpwh->msg("\n");
	return shouldEngage;
}


bool HPWH::HeatSource::shutsOff() const {
	bool shutOff = false;

	if (hpwh->tankTemps_C[0] >= hpwh->setpoint_C) {
		shutOff = true;
		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("shutsOff  bottom node hot: %.2d C  \n returns true", hpwh->tankTemps_C[0]);
		}
		return shutOff;
	}

	for (int i = 0; i < (int)shutOffLogicSet.size(); i++) {
		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("\tshutsOff logic: %s ", shutOffLogicSet[i].description.c_str());
		}

		double average = hpwh->tankAvg_C(shutOffLogicSet[i].nodeWeights);
		double comparison;

		if (shutOffLogicSet[i].isAbsolute) {
			comparison = shutOffLogicSet[i].decisionPoint;
		} else {
			comparison = hpwh->setpoint_C - shutOffLogicSet[i].decisionPoint;
		}

		if (shutOffLogicSet[i].compare(average, comparison)) {
			shutOff = true;

			//debugging message handling
			if (hpwh->hpwhVerbosity >= VRB_typical) hpwh->msg("shuts down %s\n", shutOffLogicSet[i].description.c_str());
		}
	}

	if (hpwh->hpwhVerbosity >= VRB_emetic){
		hpwh->msg("returns: %d \n", shutOff);
	}
	return shutOff;
}


void HPWH::HeatSource::addHeat(double externalT_C, double minutesToRun) {
	double input_BTUperHr, cap_BTUperHr, cop, captmp_kJ, leftoverCap_kJ = 0.0;

	// set the leftover capacity of the Heat Source to 0, so the first round of
	// passing it on works
	leftoverCap_kJ = 0.0;

	switch (configuration){
	case CONFIG_SUBMERGED:
	case CONFIG_WRAPPED:
	{
		static std::vector<double> heatDistribution(hpwh->numNodes);
		//clear the heatDistribution vector, since it's static it is still holding the
		//distribution from the last go around
		heatDistribution.clear();
		//calcHeatDist takes care of the swooping for wrapped configurations
		calcHeatDist(heatDistribution);

		// calculate capacity btu/hr, input btu/hr, and cop
		getCapacity(externalT_C, getCondenserTemp(), input_BTUperHr, cap_BTUperHr, cop);

		//some outputs for debugging
		if (hpwh->hpwhVerbosity >= VRB_typical){
			hpwh->msg("capacity_kWh %.2lf \t\t cap_BTUperHr %.2lf \n", BTU_TO_KWH(cap_BTUperHr)*(minutesToRun) / 60.0, cap_BTUperHr);
		}
		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("heatDistribution: %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf \n", heatDistribution[0], heatDistribution[1], heatDistribution[2], heatDistribution[3], heatDistribution[4], heatDistribution[5], heatDistribution[6], heatDistribution[7], heatDistribution[8], heatDistribution[9], heatDistribution[10], heatDistribution[11]);
		}
		//the loop over nodes here is intentional - essentially each node that has
		//some amount of heatDistribution acts as a separate resistive element
		//maybe start from the top and go down?  test this with graphs
		for (int i = hpwh->numNodes - 1; i >= 0; i--){
			//for(int i = 0; i < hpwh->numNodes; i++){
			captmp_kJ = BTU_TO_KJ(cap_BTUperHr * minutesToRun / 60.0 * heatDistribution[i]);
			if (captmp_kJ != 0){
				//add leftoverCap to the next run, and keep passing it on
				leftoverCap_kJ = addHeatAboveNode(captmp_kJ + leftoverCap_kJ, i, minutesToRun);
			}
		}

		//after you've done everything, any leftover capacity is time that didn't run
		this->runtime_min = (1.0 - (leftoverCap_kJ / BTU_TO_KJ(cap_BTUperHr * minutesToRun / 60.0))) * minutesToRun;
#if 1	// error check, 1-22-2017
		if (runtime_min < 0.)
			if (hpwh->hpwhVerbosity >= VRB_reluctant)
				hpwh->msg("Internal error: Negative runtime = %0.3f min\n", runtime_min);
#endif
	}
	break;

	case CONFIG_EXTERNAL:
		//Else the heat source is external. Sanden system is only current example
		//capacity is calculated internal to this function, and cap/input_BTUperHr, cop are outputs
		this->runtime_min = addHeatExternal(externalT_C, minutesToRun, cap_BTUperHr, input_BTUperHr, cop);
		break;
	}

	// Write the input & output energy
	energyInput_kWh = BTU_TO_KWH(input_BTUperHr * runtime_min / 60.0);
	energyOutput_kWh = BTU_TO_KWH(cap_BTUperHr * runtime_min / 60.0);
}



//the private functions
void HPWH::HeatSource::sortPerformanceMap() {
	std::sort(perfMap.begin(), perfMap.end(),
	[](const HPWH::HeatSource::perfPoint & a, const HPWH::HeatSource::perfPoint & b) -> bool {
		return a.T_F < b.T_F;
	});
}


double HPWH::HeatSource::expitFunc(double x, double offset) {
	double val;
	val = 1 / (1 + exp(x - offset));
	return val;
}


void HPWH::HeatSource::normalize(std::vector<double> &distribution) {
	double sum_tmp = 0.0;

	for (unsigned int i = 0; i < distribution.size(); i++) {
		sum_tmp += distribution[i];
	}
	for (unsigned int i = 0; i < distribution.size(); i++) {
		if (sum_tmp > 0.0) {
			distribution[i] /= sum_tmp;
		}
		else {
			distribution[i] = 0.0;
		}
		//this gives a very slight speed improvement (milliseconds per simulated year)
		if (distribution[i] < HEATDIST_MINVALUE) distribution[i] = 0;
	}
}


double HPWH::HeatSource::getCondenserTemp() {
	double condenserTemp_C = 0.0;
	int tempNodesPerCondensityNode = hpwh->numNodes / CONDENSITY_SIZE;
	int j = 0;

	for (int i = 0; i < hpwh->numNodes; i++) {
		j = i / tempNodesPerCondensityNode;
		if (condensity[j] != 0) {
			condenserTemp_C += (condensity[j] / tempNodesPerCondensityNode) * hpwh->tankTemps_C[i];
			//the weights don't need to be added to divide out later because they should always sum to 1

			if (hpwh->hpwhVerbosity >= VRB_emetic){
				hpwh->msg("condenserTemp_C:\t %.2lf \ti:\t %d \tj\t %d \tcondensity[j]:\t %.2lf \ttankTemps_C[i]:\t %.2lf\n", condenserTemp_C, i, j, condensity[j], hpwh->tankTemps_C[i]);
			}
		}
	}
	if (hpwh->hpwhVerbosity >= VRB_typical){
		hpwh->msg("condenser temp %.2lf \n", condenserTemp_C);
	}
	return condenserTemp_C;
}


void HPWH::HeatSource::getCapacity(double externalT_C, double condenserTemp_C, double &input_BTUperHr, double &cap_BTUperHr, double &cop) {
	double COP_T1, COP_T2;    			   //cop at ambient temperatures T1 and T2
	double inputPower_T1_Watts, inputPower_T2_Watts; //input power at ambient temperatures T1 and T2
	double externalT_F, condenserTemp_F;

	// Convert Celsius to Fahrenheit for the curve fits
	condenserTemp_F = C_TO_F(condenserTemp_C);
	externalT_F = C_TO_F(externalT_C);

	// Get bounding performance map points for interpolation/extrapolation
	bool extrapolate = false;
	size_t i_prev = 0;
	size_t i_next = 1;
	for (size_t i = 0; i < perfMap.size(); ++i) {
		if (externalT_F < perfMap[i].T_F){
			if (i == 0) {
				extrapolate = true;
				i_prev = 0;
				i_next = 1;
			} else {
				i_prev = i - 1;
				i_next = i;
			}
			break;
		} else {
			if (i == perfMap.size() - 1) {
				extrapolate = true;
				i_prev = i - 1;
				i_next = i;
				break;
			}
		}
	}


	// Calculate COP and Input Power at each of the two reference temepratures
	COP_T1 = perfMap[i_prev].COP_coeffs[0];
	COP_T1 += perfMap[i_prev].COP_coeffs[1] * condenserTemp_F;
	COP_T1 += perfMap[i_prev].COP_coeffs[2] * condenserTemp_F * condenserTemp_F;

	COP_T2 = perfMap[i_next].COP_coeffs[0];
	COP_T2 += perfMap[i_next].COP_coeffs[1] * condenserTemp_F;
	COP_T2 += perfMap[i_next].COP_coeffs[2] * condenserTemp_F * condenserTemp_F;

	inputPower_T1_Watts = perfMap[i_prev].inputPower_coeffs[0];
	inputPower_T1_Watts += perfMap[i_prev].inputPower_coeffs[1] * condenserTemp_F;
	inputPower_T1_Watts += perfMap[i_prev].inputPower_coeffs[2] * condenserTemp_F * condenserTemp_F;

	inputPower_T2_Watts = perfMap[i_next].inputPower_coeffs[0];
	inputPower_T2_Watts += perfMap[i_next].inputPower_coeffs[1] * condenserTemp_F;
	inputPower_T2_Watts += perfMap[i_next].inputPower_coeffs[2] * condenserTemp_F * condenserTemp_F;

	if (hpwh->hpwhVerbosity >= VRB_emetic){
		hpwh->msg("inputPower_T1_constant_W   linear_WperF   quadratic_WperF2  \t%.2lf  %.2lf  %.2lf \n", perfMap[0].inputPower_coeffs[0], perfMap[0].inputPower_coeffs[1], perfMap[0].inputPower_coeffs[2]);
		hpwh->msg("inputPower_T2_constant_W   linear_WperF   quadratic_WperF2  \t%.2lf  %.2lf  %.2lf \n", perfMap[1].inputPower_coeffs[0], perfMap[1].inputPower_coeffs[1], perfMap[1].inputPower_coeffs[2]);
		hpwh->msg("inputPower_T1_Watts:  %.2lf \tinputPower_T2_Watts:  %.2lf \n", inputPower_T1_Watts, inputPower_T2_Watts);

		if (extrapolate) {
			hpwh->msg("Warning performance extrapolation\n\tExternal Temperature: %.2lf\tNearest temperatures:  %.2lf, %.2lf \n\n", externalT_F, perfMap[i_prev].T_F, perfMap[i_next].T_F);
		}

	}

	// Interpolate to get COP and input power at the current ambient temperature
	cop = COP_T1 + (externalT_F - perfMap[i_prev].T_F) * ((COP_T2 - COP_T1) / (perfMap[i_next].T_F - perfMap[i_prev].T_F));
	input_BTUperHr = KWH_TO_BTU((inputPower_T1_Watts + (externalT_F - perfMap[i_prev].T_F) *
		((inputPower_T2_Watts - inputPower_T1_Watts)
		/ (perfMap[i_next].T_F - perfMap[i_prev].T_F))
		) / 1000.0);  //1000 converts w to kw
	cap_BTUperHr = cop * input_BTUperHr;

	//here is where the scaling for flow restriction happens
	//the input power doesn't change, we just scale the cop by a small percentage
	//that is based on the flow rate.  The equation is a fit to three points,
	//measured experimentally - 12 percent reduction at 150 cfm, 10 percent at
	//200, and 0 at 375. Flow is expressed as fraction of full flow.
	if (airflowFreedom != 1){
		double airflow = 375 * airflowFreedom;
		cop *= 0.00056*airflow + 0.79;
	}
	if (hpwh->hpwhVerbosity >= VRB_typical){
		hpwh->msg("cop: %.2lf \tinput_BTUperHr: %.2lf \tcap_BTUperHr: %.2lf \n", cop, input_BTUperHr, cap_BTUperHr);
	}
}


void HPWH::HeatSource::calcHeatDist(std::vector<double> &heatDistribution) {

	// Populate the vector of heat distribution
	for (int i = 0; i < hpwh->numNodes; i++) {
		if (i < lowestNode) {
			heatDistribution.push_back(0);
		}
		else {
			int k;
			if (configuration == CONFIG_SUBMERGED) { // Inside the tank, no swoopiness required
				//intentional integer division
				k = i / int(hpwh->numNodes / CONDENSITY_SIZE);
				heatDistribution.push_back(condensity[k]);
			}
			else if (configuration == CONFIG_WRAPPED) { // Wrapped around the tank, send through the logistic function
				double temp = 0;  //temp for temporary not temperature
				double offset = 5.0 / 1.8;
				temp = expitFunc((hpwh->tankTemps_C[i] - hpwh->tankTemps_C[lowestNode]) / this->shrinkage, offset);
				temp *= (hpwh->setpoint_C - hpwh->tankTemps_C[i]);
#if defined( SETPOINT_FIX)
				if (temp < 0.)
					temp = 0.;
#endif
				heatDistribution.push_back(temp);
			}
		}
	}
	normalize(heatDistribution);

}


double HPWH::HeatSource::addHeatAboveNode(double cap_kJ, int node, double minutesToRun) {
	double Q_kJ, deltaT_C, targetTemp_C;
	int setPointNodeNum;

	double volumePerNode_L = hpwh->tankVolume_L / hpwh->numNodes;

	if (hpwh->hpwhVerbosity >= VRB_emetic){
		hpwh->msg("node %2d   cap_kwh %.4lf \n", node, KJ_TO_KWH(cap_kJ));
	}

	// find the first node (from the bottom) that does not have the same temperature as the one above it
	// if they all have the same temp., use the top node, hpwh->numNodes-1
	setPointNodeNum = node;
	for (int i = node; i < hpwh->numNodes - 1; i++){
		if (hpwh->tankTemps_C[i] != hpwh->tankTemps_C[i + 1]) {
			break;
		}
		else{
			setPointNodeNum = i + 1;
		}
	}

	// maximum heat deliverable in this timestep
	while (cap_kJ > 0 && setPointNodeNum < hpwh->numNodes) {
		// if the whole tank is at the same temp, the target temp is the setpoint
		if (setPointNodeNum == (hpwh->numNodes - 1)) {
			targetTemp_C = hpwh->setpoint_C;
		}
		//otherwise the target temp is the first non-equal-temp node
		else {
			targetTemp_C = hpwh->tankTemps_C[setPointNodeNum + 1];
		}

		deltaT_C = targetTemp_C - hpwh->tankTemps_C[setPointNodeNum];

		//heat needed to bring all equal temp. nodes up to the temp of the next node. kJ
		Q_kJ = CPWATER_kJperkgC * volumePerNode_L * DENSITYWATER_kgperL * (setPointNodeNum + 1 - node) * deltaT_C;

		//Running the rest of the time won't recover
		if (Q_kJ > cap_kJ){
			for (int j = node; j <= setPointNodeNum; j++) {
				hpwh->tankTemps_C[j] += cap_kJ / CPWATER_kJperkgC / volumePerNode_L / DENSITYWATER_kgperL / (setPointNodeNum + 1 - node);
			}
			cap_kJ = 0;
		}
#if defined( SETPOINT_FIX)
		else if (Q_kJ > 0.)
		{	// temp will recover by/before end of timestep
			for (int j = node; j <= setPointNodeNum; j++)
				hpwh->tankTemps_C[j] = targetTemp_C;
			cap_kJ -= Q_kJ;
		}
		setPointNodeNum++;
#else
		//temp will recover by/before end of timestep
		else{
			for (int j = node; j <= setPointNodeNum; j++){
				hpwh->tankTemps_C[j] = targetTemp_C;
			}
			setPointNodeNum++;
			cap_kJ -= Q_kJ;
		}
#endif
	}

	//return the unused capacity
	return cap_kJ;
}


double HPWH::HeatSource::addHeatExternal(double externalT_C, double minutesToRun, double &cap_BTUperHr, double &input_BTUperHr, double &cop) {
	double heatingCapacity_kJ, deltaT_C, timeUsed_min, nodeHeat_kJperNode, nodeFrac;
	double inputTemp_BTUperHr = 0, capTemp_BTUperHr = 0, copTemp = 0;
	double volumePerNode_LperNode = hpwh->tankVolume_L / hpwh->numNodes;
	double timeRemaining_min = minutesToRun;

	input_BTUperHr = 0;
	cap_BTUperHr = 0;
	cop = 0;

	do{
		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("bottom tank temp: %.2lf \n", hpwh->tankTemps_C[0]);
		}

		//how much heat is available this timestep
		getCapacity(externalT_C, hpwh->tankTemps_C[0], inputTemp_BTUperHr, capTemp_BTUperHr, copTemp);
		heatingCapacity_kJ = BTU_TO_KJ(capTemp_BTUperHr * (minutesToRun / 60.0));
		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("\theatingCapacity_kJ stepwise: %.2lf \n", heatingCapacity_kJ);
		}

		//adjust capacity for how much time is left in this step
		heatingCapacity_kJ = heatingCapacity_kJ * (timeRemaining_min / minutesToRun);
		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("\theatingCapacity_kJ remaining this node: %.2lf \n", heatingCapacity_kJ);
		}

		//calculate what percentage of the bottom node can be heated to setpoint
		//with amount of heat available this timestep
		deltaT_C = hpwh->setpoint_C - hpwh->tankTemps_C[0];
		nodeHeat_kJperNode = volumePerNode_LperNode * DENSITYWATER_kgperL * CPWATER_kJperkgC * deltaT_C;
		//protect against dividing by zero - if bottom node is at (or above) setpoint,
		//add no heat
		if (nodeHeat_kJperNode <= 0) { nodeFrac = 0; }
		else { nodeFrac = heatingCapacity_kJ / nodeHeat_kJperNode; }

		if (hpwh->hpwhVerbosity >= VRB_emetic){
			hpwh->msg("nodeHeat_kJperNode: %.2lf nodeFrac: %.2lf \n\n", nodeHeat_kJperNode, nodeFrac);
		}
		//if more than one, round down to 1 and subtract the amount of time it would
		//take to heat that node from the timeRemaining
		if (nodeFrac > 1){
			nodeFrac = 1;
			timeUsed_min = (nodeHeat_kJperNode / heatingCapacity_kJ)*timeRemaining_min;
			timeRemaining_min -= timeUsed_min;
		}
		//otherwise just the fraction available
		//this should make heatingCapacity == 0  if nodeFrac < 1
		else{
			timeUsed_min = timeRemaining_min;
			timeRemaining_min = 0;
		}

		//move all nodes down, mixing if less than a full node
		for (int n = 0; n < hpwh->numNodes - 1; n++) {
			hpwh->tankTemps_C[n] = hpwh->tankTemps_C[n] * (1 - nodeFrac) + hpwh->tankTemps_C[n + 1] * nodeFrac;
		}
		//add water to top node, heated to setpoint
		hpwh->tankTemps_C[hpwh->numNodes - 1] = hpwh->tankTemps_C[hpwh->numNodes - 1] * (1 - nodeFrac) + hpwh->setpoint_C * nodeFrac;


		//track outputs - weight by the time ran
		input_BTUperHr += inputTemp_BTUperHr*timeUsed_min;
		cap_BTUperHr += capTemp_BTUperHr*timeUsed_min;
		cop += copTemp*timeUsed_min;


		//if there's still time remaining and you haven't heated to the cutoff
		//specified in shutsOff logic, keep heating
	} while (timeRemaining_min > 0 && shutsOff() != true);

	//divide outputs by sum of weight - the total time ran
	input_BTUperHr /= (minutesToRun - timeRemaining_min);
	cap_BTUperHr /= (minutesToRun - timeRemaining_min);
	cop /= (minutesToRun - timeRemaining_min);

	if (hpwh->hpwhVerbosity >= VRB_emetic){
		hpwh->msg("final remaining time: %.2lf \n", timeRemaining_min);
	}
	//return the time left
	return minutesToRun - timeRemaining_min;
}






void HPWH::HeatSource::setupAsResistiveElement(int node, double Watts) {
	int i;

	isOn = false;
	isVIP = false;
	for (i = 0; i < CONDENSITY_SIZE; i++) {
		condensity[i] = 0;
	}
	condensity[node] = 1;

	perfMap.reserve(2);

	perfMap.push_back({
		50, // Temperature (T_F)
		{Watts, 0.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
		{1.0, 0.0, 0.0} // COP Coefficients (COP_coeffs)
	});

	perfMap.push_back({
		67, // Temperature (T_F)
		{Watts, 0.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
		{1.0, 0.0, 0.0} // COP Coefficients (COP_coeffs)
	});

	configuration = CONFIG_SUBMERGED; //immersed in tank

	typeOfHeatSource = TYPE_resistance;
}


void HPWH::HeatSource::addTurnOnLogic(HeatingLogic logic){
	this->turnOnLogicSet.push_back(logic);
}
void HPWH::HeatSource::addShutOffLogic(HeatingLogic logic) {
	this->shutOffLogicSet.push_back(logic);
}

void HPWH::calcDerivedValues(){
	static char outputString[MAXOUTSTRING];  //this is used for debugging outputs

	// tank node density (number of calculation nodes per regular node)
	nodeDensity = numNodes / 12;

	//condentropy/shrinkage
	double condentropy = 0;
	double alpha = 1, beta = 2;  // Mapping from condentropy to shrinkage
	for (int i = 0; i < numHeatSources; i++) {
		if (hpwhVerbosity >= VRB_emetic)  msg(outputString, "Heat Source %d \n", i);

		// Calculate condentropy and ==> shrinkage
		condentropy = 0;
		for (int j = 0; j < CONDENSITY_SIZE; j++) {
			if (setOfSources[i].condensity[j] > 0) {
				condentropy -= setOfSources[i].condensity[j] * log(setOfSources[i].condensity[j]);
				if (hpwhVerbosity >= VRB_emetic)  msg(outputString, "condentropy %.2lf \n", condentropy);
			}
		}
		setOfSources[i].shrinkage = alpha + condentropy * beta;
		if (hpwhVerbosity >= VRB_emetic)  msg(outputString, "shrinkage %.2lf \n\n", setOfSources[i].shrinkage);
	}


	//lowest node
	int lowest = 0;
	for (int i = 0; i < numHeatSources; i++) {
		lowest = 0;
		if (hpwhVerbosity >= VRB_emetic)  msg(outputString, "Heat Source %d \n", i);

		for (int j = 0; j < numNodes; j++) {
			if (hpwhVerbosity >= VRB_emetic)  msg(outputString, "j: %d  j/ (numNodes/CONDENSITY_SIZE) %d \n", j, j / (numNodes / CONDENSITY_SIZE));

			if (setOfSources[i].condensity[(j / (numNodes / CONDENSITY_SIZE))] > 0) {
				lowest = j;
				break;
			}
		}
		if (hpwhVerbosity >= VRB_emetic)  msg(outputString, " lowest : %d \n", lowest);

		setOfSources[i].lowestNode = lowest;
	}

	// define condenser index and lowest resistance element index
	compressorIndex = -1; // Default = No compressor
	lowestElementIndex = -1; // Default = No resistance elements
	int lowestElementPos = CONDENSITY_SIZE;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].typeOfHeatSource == HPWH::TYPE_compressor) {
			compressorIndex = i;  // NOTE: Maybe won't work with multiple compressors (last compressor will be used)
		} else {
			for (int j = 0; j < CONDENSITY_SIZE; j++) {
				if (setOfSources[i].condensity[j] > 0.0 && j < lowestElementPos) {
					lowestElementIndex = i;
					lowestElementPos = j;
					break;
				}
			}
		}
	}

	if (hpwhVerbosity >= VRB_emetic)  msg(outputString, " compressorIndex : %d \n", compressorIndex);
	if (hpwhVerbosity >= VRB_emetic)  msg(outputString, " lowestElementIndex : %d \n", lowestElementIndex);

	//heat source ability to depress temp
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].typeOfHeatSource == TYPE_compressor) {
			setOfSources[i].depressesTemperature = true;
		}
		else if (setOfSources[i].typeOfHeatSource == TYPE_resistance) {
			setOfSources[i].depressesTemperature = false;
		}
	}



}

int HPWH::checkInputs(){
	int returnVal = 0;
	//use a returnVal so that all checks are processed and error messages written

	if (numHeatSources <= 0) {
		msg("You must have at least one HeatSource");
		returnVal = HPWH_ABORT;
	}
	if ((numNodes % 12) != 0) {
		msg("The number of nodes must be a multiple of 12");
		returnVal = HPWH_ABORT;
	}



	double condensitySum;
	//loop through all heat sources to check each for malconfigurations
	for (int i = 0; i < numHeatSources; i++) {
		//check the heat source type to make sure it has been set
		if (setOfSources[i].typeOfHeatSource == TYPE_none) {
			if (hpwhVerbosity >= VRB_reluctant) msg("Heat source %d does not have a specified type.  Initialization failed.\n", i);
			returnVal = HPWH_ABORT;
		}
		//check to make sure there is at least one onlogic or parent with onlogic
		int parent = setOfSources[i].findParent();
		if (setOfSources[i].turnOnLogicSet.size() == 0 && (parent == -1 || setOfSources[parent].turnOnLogicSet.size() == 0)) {
			msg("You must specify at least one logic to turn on the element or the element must be set as a backup for another heat source with at least one logic.");
			returnVal = HPWH_ABORT;
		}
		//check is condensity sums to 1
		condensitySum = 0;
		for (int j = 0; j < CONDENSITY_SIZE; j++)  condensitySum += setOfSources[i].condensity[j];
		if (fabs(condensitySum - 1.0) > 1e-6) {
			msg("The condensity for hearsource %d does not sum to 1.  \n", i);
			msg("It sums to %f \n", condensitySum);
			returnVal = HPWH_ABORT;
		}
		//check that air flows are all set properly
		if (setOfSources[i].airflowFreedom > 1.0 || setOfSources[i].airflowFreedom <= 0.0) {
			msg("The airflowFreedom must be between 0 and 1 for heatsource %d.  \n", i);
			returnVal = HPWH_ABORT;
		}


	}



	//if there's no failures, return 0
	return returnVal;
}

#ifndef HPWH_ABRIDGED
int HPWH::HPWHinit_file(string configFile){
	simHasFailed = true;  //this gets cleared on successful completion of init

	//clear out old stuff if you're re-initializing
	if (tankTemps_C != NULL) delete[] tankTemps_C;
	if (setOfSources != NULL) delete[] setOfSources;


	//open file, check and report errors
	std::ifstream inputFILE;
	inputFILE.open(configFile.c_str());
	if (!inputFILE.is_open()) {
		if (hpwhVerbosity >= VRB_reluctant) msg("Input file failed to open.  \n");
		return HPWH_ABORT;
	}

	//some variables that will be handy
	int heatsource, sourceNum, nTemps;
	string tempString, units;
	double tempDouble, dblArray[12];

	//being file processing, line by line
	string line_s;
	std::stringstream line_ss;
	string token;
	while (std::getline(inputFILE, line_s)){
		line_ss.clear();
		line_ss.str(line_s);

		//grab the first word, and start comparing
		line_ss >> token;
		if (token.at(0) == '#' || line_s.empty()) {
			//if you hit a comment, skip to next line
			continue;
		}
		else if (token == "numNodes") {
			line_ss >> numNodes;
		}
		else if (token == "volume") {
			line_ss >> tempDouble >> units;
			if (units == "gal")  tempDouble = GAL_TO_L(tempDouble);
			else if (units == "L"); //do nothing, lol
			else {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s.  \n", token.c_str());
				return HPWH_ABORT;
			}
			tankVolume_L = tempDouble;
		}
		else if (token == "UA") {
			line_ss >> tempDouble >> units;
			if (units != "kJperHrC") {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s.  \n", token.c_str());
				return HPWH_ABORT;
			}
			tankUA_kJperHrC = tempDouble;
		}
		else if (token == "depressTemp") {
			line_ss >> tempString;
			if (tempString == "true") {
				doTempDepression = true;
			}
			else if (tempString == "false") {
				doTempDepression = false;
			}
			else {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Improper value for %s\n", token.c_str());
				return HPWH_ABORT;
			}
		}
		else if (token == "mixOnDraw") {
			line_ss >> tempString;
			if (tempString == "true") {
				tankMixesOnDraw = true;
			}
			else if (tempString == "false") {
				tankMixesOnDraw = false;
			}
			else {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Improper value for %s\n", token.c_str());
				return HPWH_ABORT;
			}
		}
		else if (token == "setpoint") {
			line_ss >> tempDouble >> units;
			if (units == "F")  tempDouble = F_TO_C(tempDouble);
			else if (units == "C"); //do nothing, lol
			else {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s.  \n", token.c_str());
				return HPWH_ABORT;
			}
			setpoint_C = tempDouble;
			//tank will be set to setpoint at end of function
		}
		else if (token == "setpointFixed") {
			line_ss >> tempString;
			if (tempString == "true")  setpointFixed = true;
			else if (tempString == "false")  setpointFixed = false;
			else {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Improper value for %s\n", token.c_str());
				return HPWH_ABORT;
			}
		}
		else if (token == "verbosity") {
			line_ss >> token;
			if (token == "silent") {
				hpwhVerbosity = VRB_silent;
			}
			else if (token == "reluctant") {
				hpwhVerbosity = VRB_reluctant;
			}
			else if (token == "typical") {
				hpwhVerbosity = VRB_typical;
			}
			else if (token == "emetic") {
				hpwhVerbosity = VRB_emetic;
			}
			else {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect verbosity on input.  \n");
				return HPWH_ABORT;
			}
		}
		else if (token == "numHeatSources") {
			line_ss >> numHeatSources;
			setOfSources = new HeatSource[numHeatSources];
			for (int i = 0; i < numHeatSources; i++) {
				setOfSources[i] = HeatSource(this);
			}
		}
		else if (token == "heatsource") {
			if (numHeatSources == 0) {
				msg("You must specify the number of heatsources before setting their properties.  \n");
				return HPWH_ABORT;
			}
			line_ss >> heatsource >> token;
			if (token == "isVIP") {
				line_ss >> tempString;
				if (tempString == "true")  setOfSources[heatsource].isVIP = true;
				else if (tempString == "false")  setOfSources[heatsource].isVIP = false;
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Improper value for %s for heat source %d\n", token.c_str(), heatsource);
					return HPWH_ABORT;
				}
			}
			else if (token == "isOn") {
				line_ss >> tempString;
				if (tempString == "true")  setOfSources[heatsource].isOn = true;
				else if (tempString == "false")  setOfSources[heatsource].isOn = false;
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Improper value for %s for heat source %d\n", token.c_str(), heatsource);
					return HPWH_ABORT;
				}
			}
			else if (token == "minT") {
				line_ss >> tempDouble >> units;
				if (units == "F")  tempDouble = F_TO_C(tempDouble);
				else if (units == "C"); //do nothing, lol
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s.  \n", token.c_str());
					return HPWH_ABORT;
				}
				setOfSources[heatsource].minT = tempDouble;
			}
			else if (token == "maxT") {
				line_ss >> tempDouble >> units;
				if (units == "F")  tempDouble = F_TO_C(tempDouble);
				else if (units == "C"); //do nothing, lol
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s.  \n", token.c_str());
					return HPWH_ABORT;
				}
				setOfSources[heatsource].maxT = tempDouble;
			}
			else if (token == "onlogic" || token == "offlogic") {
				line_ss >> tempString;
				if (tempString == "nodes") {
					std::vector<int> nodeNums;
					std::vector<double> weights;
					std::string nextToken;
					line_ss >> nextToken;
					while (std::regex_match(nextToken, std::regex("\\d+"))) {
						int nodeNum = std::stoi(nextToken);
						if (nodeNum > 13 || nodeNum < 0) {
							if (hpwhVerbosity >= VRB_reluctant)  msg("Node number for heatsource %d %s must be between 0 and 13.  \n", heatsource, token.c_str());
							return HPWH_ABORT;
						}
						nodeNums.push_back(nodeNum);
						line_ss >> nextToken;
					}
					if (nextToken == "weights") {
						line_ss >> nextToken;
						while (std::regex_match(nextToken, std::regex("-?\\d*\\.\\d+(?:e-?\\d+)?"))) {
							weights.push_back(std::stod(nextToken));
							line_ss >> nextToken;
						}
					} else {
						for (auto n : nodeNums) {
							n += 0; // used to get rid of unused variable compiler warning
							weights.push_back(1.0);
						}
					}
					if (nodeNums.size() != weights.size()) {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Number of weights for heatsource %d %s (%d) does not macht number of nodes for %s (%d).  \n", heatsource, token.c_str(), weights.size(), token.c_str(), nodeNums.size());
						return HPWH_ABORT;
					}
					if (nextToken != "absolute" && nextToken != "relative") {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Improper definition, \"%s\", for heat source %d %s. Should be \"relative\" or \"absoute\".\n", nextToken.c_str(), heatsource, token.c_str());
						return HPWH_ABORT;
					}
					bool absolute = (nextToken == "absolute");
					std::string compareStr;
					line_ss >> compareStr >> tempDouble >> units;
					std::function<bool(double,double)> compare;
					if (compareStr == "<") compare = std::less<double>();
					else if (compareStr == ">") compare = std::greater<double>();
					else {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Improper comparison, \"%s\", for heat source %d %s. Should be \"<\" or \">\".\n", compareStr.c_str(), heatsource, token.c_str());
						return HPWH_ABORT;
					}
					if (units == "F") {
						if (absolute) {
							tempDouble = F_TO_C(tempDouble);
						}
						else {
							tempDouble = dF_TO_dC(tempDouble);
						}
					}
					else if (units == "C"); //do nothing, lol
					else {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s from heatsource %d.  \n", token.c_str(), heatsource);
						return HPWH_ABORT;
					}
					std::vector<NodeWeight> nodeWeights;
					for (int i = 0; i < nodeNums.size(); i++ ) {
							nodeWeights.emplace_back(nodeNums[i],weights[i]);
					}
					HPWH::HeatingLogic logic("custom", nodeWeights, tempDouble, absolute, compare);
					if (token == "onlogic") {
						setOfSources[heatsource].addTurnOnLogic(logic);
					} else { // "offlogic"
						setOfSources[heatsource].addShutOffLogic(logic);
					}
				}
				else if (token == "onlogic") {
					line_ss >> tempDouble >> units;
					if (units == "F")  tempDouble = dF_TO_dC(tempDouble);
					else if (units == "C"); //do nothing, lol
					else {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s from heatsource %d.  \n", token.c_str(), heatsource);
						return HPWH_ABORT;
					}
					if (tempString == "topThird") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::topThird(tempDouble));
					}
					else if (tempString == "bottomThird") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::bottomThird(tempDouble));
					}
					else if (tempString == "standby") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::standby(tempDouble));
					}
					else if (tempString == "bottomSixth") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::bottomSixth(tempDouble));
					}
					else if (tempString == "secondSixth") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::secondSixth(tempDouble));
					}
					else if (tempString == "thirdSixth") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::thirdSixth(tempDouble));
					}
					else if (tempString == "fourthSixth") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::fourthSixth(tempDouble));
					}
					else if (tempString == "fifthSixth") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::fifthSixth(tempDouble));
					}
					else if (tempString == "topSixth") {
						setOfSources[heatsource].addTurnOnLogic(HPWH::topSixth(tempDouble));
					}
					else {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
						return HPWH_ABORT;
					}
				}
				else if (token == "offlogic") {
					line_ss >> tempDouble >> units;
					if (units == "F")  tempDouble = F_TO_C(tempDouble);
					else if (units == "C"); //do nothing, lol
					else {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s from heatsource %d.  \n", token.c_str(), heatsource);
						return HPWH_ABORT;
					}
					if (tempString == "topNodeMaxTemp") {
						setOfSources[heatsource].addShutOffLogic(HPWH::topNodeMaxTemp(tempDouble));
					}
					else if (tempString == "bottomNodeMaxTemp") {
						setOfSources[heatsource].addShutOffLogic(HPWH::bottomNodeMaxTemp(tempDouble));
					}
					else if (tempString == "bottomTwelthMaxTemp") {
						setOfSources[heatsource].addShutOffLogic(HPWH::bottomTwelthMaxTemp(tempDouble));
					}
					else if (tempString == "largeDraw") {
						setOfSources[heatsource].addShutOffLogic(HPWH::largeDraw(tempDouble));
					}
					else if (tempString == "largerDraw") {
						setOfSources[heatsource].addShutOffLogic(HPWH::largerDraw(tempDouble));
					}
					else {
						if (hpwhVerbosity >= VRB_reluctant)  msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
						return HPWH_ABORT;
					}
				}
			}
			else if (token == "type") {
				line_ss >> tempString;
				if (tempString == "resistor") {
					setOfSources[heatsource].typeOfHeatSource = TYPE_resistance;
				}
				else if (tempString == "compressor") {
					setOfSources[heatsource].typeOfHeatSource = TYPE_compressor;
				}
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
					return HPWH_ABORT;
				}
			}
			else if (token == "coilConfig") {
				line_ss >> tempString;
				if (tempString == "wrapped") {
					setOfSources[heatsource].configuration = HeatSource::CONFIG_WRAPPED;
				}
				else if (tempString == "submerged") {
					setOfSources[heatsource].configuration = HeatSource::CONFIG_SUBMERGED;
				}
				else if (tempString == "external") {
					setOfSources[heatsource].configuration = HeatSource::CONFIG_EXTERNAL;
				}
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
					return HPWH_ABORT;
				}
			}
			else if (token == "condensity") {
				line_ss >> dblArray[0] >> dblArray[1] >> dblArray[2] >> dblArray[3] >> dblArray[4] >> dblArray[5] >> dblArray[6] >> dblArray[7] >> dblArray[8] >> dblArray[9] >> dblArray[10] >> dblArray[11];
				setOfSources[heatsource].setCondensity(dblArray[0], dblArray[1], dblArray[2], dblArray[3], dblArray[4], dblArray[5], dblArray[6], dblArray[7], dblArray[8], dblArray[9], dblArray[10], dblArray[11]);
			}
			else if (token == "nTemps") {
				line_ss >> nTemps;
				setOfSources[heatsource].perfMap.resize(nTemps);
			}
			else if (std::regex_match(token, std::regex("T\\d+"))){
				std::smatch match;
				std::regex_match(token, match, std::regex("T(\\d+)"));
				nTemps = std::stoi(match[1].str());
				int maxTemps = setOfSources[heatsource].perfMap.size();

				if (maxTemps < nTemps) {
					if (maxTemps == 0) {
						if (true || hpwhVerbosity >= VRB_reluctant){
							msg("%s specified for heatsource %d before definition of nTemps.  \n", token.c_str(), heatsource);
						}
						return HPWH_ABORT;
					}
					else {
						if (true || hpwhVerbosity >= VRB_reluctant){
							msg("Incorrect specification for %s from heatsource %d. nTemps, %d, is less than %d.  \n", token.c_str(), heatsource, maxTemps, nTemps);
						}
						return HPWH_ABORT;
					}
				}
				line_ss >> tempDouble >> units;
				//        if (units == "F")  tempDouble = F_TO_C(tempDouble);
				if (units == "F");
				//        else if (units == "C") ; //do nothing, lol
				else if (units == "C") tempDouble = C_TO_F(tempDouble);
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s from heatsource %d.  \n", token.c_str(), heatsource);
					return HPWH_ABORT;
				}
				setOfSources[heatsource].perfMap[nTemps - 1].T_F = tempDouble;
			}
			else if (std::regex_match(token, std::regex("(?:inPow|cop)T\\d+(?:const|lin|quad)"))){
				std::smatch match;
				std::regex_match(token, match, std::regex("(inPow|cop)T(\\d+)(const|lin|quad)"));
				string var = match[1].str();
				nTemps = std::stoi(match[2].str());
				string coeff = match[3].str();
				int coeff_num;
				if (coeff == "const"){
					coeff_num = 0;
				}
				else if (coeff == "lin"){
					coeff_num = 1;
				}
				else if (coeff == "quad"){
					coeff_num = 2;
				}

				int maxTemps = setOfSources[heatsource].perfMap.size();

				if (maxTemps < nTemps) {
					if (maxTemps == 0) {
						if (hpwhVerbosity >= VRB_reluctant){
							msg("%s specified for heatsource %d before definition of nTemps.  \n", token.c_str(), heatsource);
						}
						return HPWH_ABORT;
					}
					else {
						if (hpwhVerbosity >= VRB_reluctant){
							msg("Incorrect specification for %s from heatsource %d. nTemps, %d, is less than %d.  \n", token.c_str(), heatsource, maxTemps, nTemps);
						}
						return HPWH_ABORT;
					}
				}
				line_ss >> tempDouble;

				if (var == "inPow") {
					setOfSources[heatsource].perfMap[nTemps - 1].inputPower_coeffs[coeff_num] = tempDouble;
				}
				else if (var == "cop") {
					setOfSources[heatsource].perfMap[nTemps - 1].COP_coeffs[coeff_num] = tempDouble;
				}
			}
			else if (token == "hysteresis"){
				line_ss >> tempDouble >> units;
				if (units == "F")  tempDouble = dF_TO_dC(tempDouble);
				else if (units == "C"); //do nothing, lol
				else {
					if (hpwhVerbosity >= VRB_reluctant)  msg("Incorrect units specification for %s from heatsource %d.  \n", token.c_str(), heatsource);
					return HPWH_ABORT;
				}
				setOfSources[heatsource].hysteresis_dC = tempDouble;
			}
			else if (token == "backupSource"){
				line_ss >> sourceNum;
				setOfSources[heatsource].backupHeatSource = &setOfSources[sourceNum];
			}
			else if (token == "companionSource"){
				line_ss >> sourceNum;
				setOfSources[heatsource].companionHeatSource = &setOfSources[sourceNum];
			}
			else if (token == "followedBySource"){
				line_ss >> sourceNum;
				setOfSources[heatsource].followedByHeatSource = &setOfSources[sourceNum];
			}
			else {
				if (hpwhVerbosity >= VRB_reluctant)  msg("Improper specifier (%s) for heat source %d\n", token.c_str(), heatsource);
			}

		} //end heatsource options
		else {
			msg("Improper keyword: %s  \n", token.c_str());
			return HPWH_ABORT;
		}

	} //end while over lines


	//take care of the non-input processing
	hpwhModel = MODELS_CustomFile;

	tankTemps_C = new double[numNodes];
	resetTankToSetpoint();

	isHeating = false;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isOn) {
			isHeating = true;
		}
		setOfSources[i].sortPerformanceMap();
	}

	calcDerivedValues();

	if (checkInputs() == HPWH_ABORT) return HPWH_ABORT;
	simHasFailed = false;
	return 0;
}
#endif

int HPWH::HPWHinit_resTank(){
	//a default resistance tank, nominal 50 gallons, 0.95 EF, standard double 4.5 kW elements
	return this->HPWHinit_resTank(GAL_TO_L(47.5), 0.95, 4500, 4500);
}
int HPWH::HPWHinit_resTank(double tankVol_L, double energyFactor, double upperPower_W, double lowerPower_W){
	//low power element will cause divide by zero/negative UA in EF -> UA conversion
	if (lowerPower_W < 550) {
		if (hpwhVerbosity >= VRB_reluctant)  msg("Resistance tank wattage below 550 W.  DOES NOT COMPUTE\n");
		return HPWH_ABORT;
	}
	if (energyFactor <= 0) {
		if (hpwhVerbosity >= VRB_reluctant)  msg("Energy Factor less than zero.  DOES NOT COMPUTE\n");
		return HPWH_ABORT;
	}

	//use tank size setting function since it has bounds checking
	int failure = this->setTankSize(tankVol_L);
	if (failure == HPWH_ABORT) {
		return failure;
	}


	numNodes = 12;
	tankTemps_C = new double[numNodes];
	setpoint_C = F_TO_C(127.0);

	//start tank off at setpoint
	resetTankToSetpoint();


	doTempDepression = false;
	tankMixesOnDraw = true;

	numHeatSources = 2;
	setOfSources = new HeatSource[numHeatSources];

	HeatSource resistiveElementBottom(this);
	HeatSource resistiveElementTop(this);
	resistiveElementBottom.setupAsResistiveElement(0, lowerPower_W);
	resistiveElementTop.setupAsResistiveElement(8, upperPower_W);

	//standard logic conditions
	resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
	resistiveElementBottom.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));

	resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
	resistiveElementTop.isVIP = true;

	setOfSources[0] = resistiveElementTop;
	setOfSources[1] = resistiveElementBottom;

	setOfSources[0].followedByHeatSource = &setOfSources[1];

	// (1/EnFac + 1/RecovEff) / (67.5 * ((24/41094) - 1/(RecovEff * Power_btuperHr)))
	double recoveryEfficiency = 0.98;
	double numerator = (1.0 / energyFactor) - (1.0 / recoveryEfficiency);
	double temp = 1.0 / (recoveryEfficiency * lowerPower_W*3.41443);
	double denominator = 67.5 * ((24.0 / 41094.0) - temp);
	double UA_btuPerHrF = numerator / denominator;
	tankUA_kJperHrC = UA_btuPerHrF * (1.8 / 0.9478);


	hpwhModel = MODELS_CustomResTank;

	//calculate oft-used derived values
	calcDerivedValues();

	if (checkInputs() == HPWH_ABORT) return HPWH_ABORT;

	isHeating = false;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isOn) {
			isHeating = true;
		}
		setOfSources[i].sortPerformanceMap();
	}


	if (hpwhVerbosity >= VRB_emetic){
		for (int i = 0; i < numHeatSources; i++) {
			msg("heat source %d: %p \n", i, &setOfSources[i]);
		}
		msg("\n\n");
	}

	simHasFailed = false;
	return 0;  //successful init returns 0
}

int HPWH::HPWHinit_genericHPWH(double tankVol_L, double energyFactor, double resUse_C){
	//return 0 on success, HPWH_ABORT for failure
	simHasFailed = true;  //this gets cleared on successful completion of init

	//clear out old stuff if you're re-initializing
	if (tankTemps_C != NULL) delete[] tankTemps_C;
	if (setOfSources != NULL) delete[] setOfSources;




	//except where noted, these values are taken from MODELS_GE2014STDMode on 5/17/16
	numNodes = 12;
	tankTemps_C = new double[numNodes];
	setpoint_C = F_TO_C(127.0);

	//start tank off at setpoint
	resetTankToSetpoint();

	//custom settings - these are set later
	//tankVolume_L = GAL_TO_L(45);
	//tankUA_kJperHrC = 6.5;

	doTempDepression = false;
	tankMixesOnDraw = true;

	numHeatSources = 3;
	setOfSources = new HeatSource[numHeatSources];

	HeatSource compressor(this);
	HeatSource resistiveElementBottom(this);
	HeatSource resistiveElementTop(this);

	//compressor values
	compressor.isOn = false;
	compressor.isVIP = false;
	compressor.typeOfHeatSource = TYPE_compressor;

	double split = 1.0 / 4.0;
	compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

	compressor.perfMap.reserve(2);

	compressor.perfMap.push_back({
		50, // Temperature (T_F)
		{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
		{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
	});

	compressor.perfMap.push_back({
		70, // Temperature (T_F)
		{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
		{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
	});

	compressor.minT = F_TO_C(45.);
	compressor.maxT = F_TO_C(120.);
	compressor.hysteresis_dC = dF_TO_dC(2);
	compressor.configuration = HeatSource::CONFIG_WRAPPED;

	//top resistor values
	resistiveElementTop.setupAsResistiveElement(6, 4500);
	resistiveElementTop.isVIP = true;

	//bottom resistor values
	resistiveElementBottom.setupAsResistiveElement(0, 4000);
	resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

	//logic conditions
	//this is set customly, from input
	//resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(19.6605)));
	resistiveElementTop.addTurnOnLogic(HPWH::topThird(resUse_C));

	resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

	compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
	compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));

	//custom adjustment for poorer performance
	//compressor.addShutOffLogic(HPWH::lowT(F_TO_C(37)));


	//end section of parameters from GE model




	//set tank volume from input
	//use tank size setting function since it has bounds checking
	int failure = this->setTankSize(tankVol_L);
	if (failure == HPWH_ABORT) {
		if (hpwhVerbosity >= VRB_reluctant) msg("Failure to set tank size in generic hpwh init.");
		return failure;
	}


	// derive conservative (high) UA from tank volume
	//   curve fit by Jim Lutz, 5-25-2016
	double tankVol_gal = tankVol_L / GAL_TO_L(1.);
	double v1 = 7.5156316175 * pow(tankVol_gal, 0.33) + 5.9995357658;
	tankUA_kJperHrC = 0.0076183819 * v1 * v1;




	//do a linear interpolation to scale COP curve constant, using measured values
	// Chip's attempt 24-May-2014
	double uefSpan = 3.4 - 2.0;

	//force COP to be 70% of GE at UEF 2 and 95% at UEF 3.4
	//use a fudge factor to scale cop and input power in tandem to maintain constant capacity
	double fUEF = (energyFactor - 2.0) / uefSpan;
	double genericFudge = (1. - fUEF)*.7 + fUEF*.95;

	compressor.perfMap[0].COP_coeffs[0] *= genericFudge;
	compressor.perfMap[0].COP_coeffs[1] *= genericFudge;
	compressor.perfMap[0].COP_coeffs[2] *= genericFudge;

	compressor.perfMap[1].COP_coeffs[0] *= genericFudge;
	compressor.perfMap[1].COP_coeffs[1] *= genericFudge;
	compressor.perfMap[1].COP_coeffs[2] *= genericFudge;

	compressor.perfMap[0].inputPower_coeffs[0] /= genericFudge;
	compressor.perfMap[0].inputPower_coeffs[1] /= genericFudge;
	compressor.perfMap[0].inputPower_coeffs[2] /= genericFudge;

	compressor.perfMap[1].inputPower_coeffs[0] /= genericFudge;
	compressor.perfMap[1].inputPower_coeffs[1] /= genericFudge;
	compressor.perfMap[1].inputPower_coeffs[2] /= genericFudge;




	//set everything in its places
	setOfSources[0] = resistiveElementTop;
	setOfSources[1] = resistiveElementBottom;
	setOfSources[2] = compressor;

	//and you have to do this after putting them into setOfSources, otherwise
	//you don't get the right pointers
	setOfSources[2].backupHeatSource = &setOfSources[1];
	setOfSources[1].backupHeatSource = &setOfSources[2];

	setOfSources[0].followedByHeatSource = &setOfSources[1];
	setOfSources[1].followedByHeatSource = &setOfSources[2];



	//standard finishing up init, borrowed from init function

	hpwhModel = MODELS_genericCustomUEF;

	//calculate oft-used derived values
	calcDerivedValues();

	if (checkInputs() == HPWH_ABORT) return HPWH_ABORT;

	isHeating = false;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isOn) {
			isHeating = true;
		}
		setOfSources[i].sortPerformanceMap();
	}

	if (hpwhVerbosity >= VRB_emetic){
		for (int i = 0; i < numHeatSources; i++) {
			msg("heat source %d: %p \n", i, &setOfSources[i]);
		}
		msg("\n\n");
	}

	simHasFailed = false;

	return 0;
}




int HPWH::HPWHinit_presets(MODELS presetNum) {
	//return 0 on success, HPWH_ABORT for failure
	simHasFailed = true;  //this gets cleared on successful completion of init

	//clear out old stuff if you're re-initializing
	if (tankTemps_C != NULL) delete[] tankTemps_C;
	if (setOfSources != NULL) delete[] setOfSources;


	//resistive with no UA losses for testing
	if (presetNum == MODELS_restankNoUA) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 0; //0 to turn off


		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(8, 4500);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.isVIP = true;

		//assign heat sources into array in order of priority
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;

		setOfSources[0].followedByHeatSource = &setOfSources[1];
	}

	//resistive tank with massive UA loss for testing
	else if (presetNum == MODELS_restankHugeUA) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = 50;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 120;
		tankUA_kJperHrC = 500; //0 to turn off

		doTempDepression = false;
		tankMixesOnDraw = false;


		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		//set up a resistive element at the bottom, 4500 kW
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(9, 4500);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(20));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(15));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(20));
		resistiveElementTop.isVIP = true;


		//assign heat sources into array in order of priority
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;

		setOfSources[0].followedByHeatSource = &setOfSources[1];

	}

	//realistic resistive tank
	else if (presetNum == MODELS_restankRealistic) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 10; //0 to turn off

		doTempDepression = false;
		//should eventually put tankmixes to true when testing progresses
		tankMixesOnDraw = false;

		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(9, 4500);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(20));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(15));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(20));
		resistiveElementTop.isVIP = true;

		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;

		setOfSources[0].followedByHeatSource = &setOfSources[1];
	}

	//basic compressor tank for testing
	else if (presetNum == MODELS_basicIntegrated) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = 50;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 120;
		tankUA_kJperHrC = 10; //0 to turn off
		//tankUA_kJperHrC = 0; //0 to turn off

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);
		HeatSource compressor(this);

		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(9, 4500);

		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(20));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(15));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(20));
		resistiveElementTop.isVIP = true;


		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double oneSixth = 1.0 / 6.0;
		compressor.setCondensity(oneSixth, oneSixth, oneSixth, oneSixth, oneSixth, oneSixth, 0, 0, 0, 0, 0, 0);

		//GE tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.290 * 1000, 0.00159 * 1000, 0.00000107 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.49, -0.0187, -0.0000133} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.375 * 1000, 0.00121 * 1000, 0.00000216 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{5.60, -0.0252, 0.00000254} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = 0;
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED; //wrapped around tank

		compressor.addTurnOnLogic(HPWH::bottomThird(20));
		compressor.addTurnOnLogic(HPWH::standby(15));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}

	//simple external style for testing
	else if (presetNum == MODELS_externalTest) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = 50;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 120;
		//tankUA_kJperHrC = 10; //0 to turn off
		tankUA_kJperHrC = 0; //0 to turn off

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);

		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		//GE tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.290 * 1000, 0.00159 * 1000, 0.00000107 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.49, -0.0187, -0.0000133} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.375 * 1000, 0.00121 * 1000, 0.00000216 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{5.60, -0.0252, 0.00000254} // COP Coefficients (COP_coeffs)
		});

		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = 0;  //no hysteresis
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;

		compressor.addTurnOnLogic(HPWH::bottomThird(20));
		compressor.addTurnOnLogic(HPWH::standby(15));

		//lowT cutoff
		compressor.addShutOffLogic(HPWH::bottomNodeMaxTemp(20));


		//set everything in its places
		setOfSources[0] = compressor;
	}
	//voltex 60 gallon
	else if (presetNum == MODELS_AOSmithPHPT60) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 215.8;
		tankUA_kJperHrC = 7.31;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.467 * 1000, 0.00281 * 1000, 0.0000072 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.86, -0.0222, -0.00001} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.541 * 1000, 0.00147 * 1000, 0.0000176 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{6.58, -0.0392, 0.0000407} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4250);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 2000);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);

		//logic conditions
		double compStart = dF_TO_dC(43.6);
		double standbyT = dF_TO_dC(23.8);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(compStart));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(25.0)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_AOSmithPHPT80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 283.9;
		tankUA_kJperHrC = 8.8;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.467 * 1000, 0.00281 * 1000, 0.0000072 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.86, -0.0222, -0.00001} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.541 * 1000, 0.00147 * 1000, 0.0000176 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{6.58, -0.0392, 0.0000407} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4250);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 2000);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);


		//logic conditions
		double compStart = dF_TO_dC(43.6);
		double standbyT = dF_TO_dC(23.8);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(compStart));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(25.0)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2012) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 172;
		tankUA_kJperHrC = 6.8;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.3 * 1000, 0.00159 * 1000, 0.00000107 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.7, -0.0210, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.378 * 1000, 0.00121 * 1000, 0.00000216 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.8, -0.0167, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4200);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4200);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);


		//logic conditions
		//    double compStart = dF_TO_dC(24.4);
		double compStart = dF_TO_dC(40.0);
		double standbyT = dF_TO_dC(5.2);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));
		// compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(66)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(compStart));
		//resistiveElementBottom.addShutOffLogic(HPWH::lowTreheat(lowTcutoff));
		//GE element never turns off?

		// resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(31.0)));
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(28.0)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_Sanden80) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = 65;
		setpointFixed = true;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 315;
		//tankUA_kJperHrC = 10; //0 to turn off
		tankUA_kJperHrC = 7;

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);

		compressor.isOn = false;
		compressor.isVIP = true;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(5);

		compressor.perfMap.push_back({
			17, // Temperature (T_F)
			{1650, 5.5, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.2, -0.015, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			35, // Temperature (T_F)
			{1100, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.7, -0.015, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{880, 3.1, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.25, -0.025, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{740, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.2, -0.03, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{790, 2, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.15, -0.04, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.hysteresis_dC = 4;
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;


		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(8);
		compressor.addTurnOnLogic(HPWH::HeatingLogic("eighth node absolute", nodeWeights, F_TO_C(113), true));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(8.2639)));

		//lowT cutoff
		std::vector<NodeWeight> nodeWeights1;
		nodeWeights1.emplace_back(1);
		compressor.addShutOffLogic(HPWH::HeatingLogic("bottom node absolute", nodeWeights1, F_TO_C(135), true, std::greater<double>()));
		compressor.depressesTemperature = false;  //no temp depression

		//set everything in its places
		setOfSources[0] = compressor;
	}
	else if (presetNum == MODELS_Sanden40) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = 65;
		setpointFixed = true;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 160;
		//tankUA_kJperHrC = 10; //0 to turn off
		tankUA_kJperHrC = 5;

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);

		compressor.isOn = false;
		compressor.isVIP = true;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(5);

		compressor.perfMap.push_back({
			17, // Temperature (T_F)
			{1650, 5.5, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.2, -0.015, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			35, // Temperature (T_F)
			{1100, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.7, -0.015, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{880, 3.1, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.25, -0.025, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{740, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.2, -0.03, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{790, 2, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.15, -0.04, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.hysteresis_dC = 4;
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;


		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(4);
		compressor.addTurnOnLogic(HPWH::HeatingLogic("fourth node absolute", nodeWeights, F_TO_C(113), true));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(8.2639)));

		//lowT cutoff
		std::vector<NodeWeight> nodeWeights1;
		nodeWeights1.emplace_back(1);
		compressor.addShutOffLogic(HPWH::HeatingLogic("bottom node absolute", nodeWeights1, F_TO_C(135), true, std::greater<double>()));
		compressor.depressesTemperature = false;  //no temp depression

		//set everything in its places
		setOfSources[0] = compressor;
	}
	else if (presetNum == MODELS_AOSmithHPTU50 || presetNum == MODELS_RheemHBDR2250 || presetNum == MODELS_RheemHBDR4550) {
		numNodes = 24;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 171;
		tankUA_kJperHrC = 6;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

                // performance map
		compressor.perfMap.reserve(3);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{170, 2.02, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.93, -0.027, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{144.5, 2.42, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.67, -0.037, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{94.1, 3.15, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{11.1, -0.056, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		if (presetNum == MODELS_RheemHBDR2250) {
			resistiveElementTop.setupAsResistiveElement(8, 2250);
		} else {
			resistiveElementTop.setupAsResistiveElement(8, 4500);
		}
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		if (presetNum == MODELS_RheemHBDR2250) {
			resistiveElementBottom.setupAsResistiveElement(0, 2250);
		} else {
			resistiveElementBottom.setupAsResistiveElement(0, 4500);
		}
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(35);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));


		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(11); nodeWeights.emplace_back(12);
		resistiveElementTop.addTurnOnLogic(HPWH::HeatingLogic("top sixth absolute", nodeWeights, F_TO_C(105), true));
//		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(28)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

                setOfSources[0].companionHeatSource = &setOfSources[2];


	}
	else if (presetNum == MODELS_AOSmithHPTU66 || presetNum == MODELS_RheemHBDR2265 || presetNum == MODELS_RheemHBDR4565) {
		numNodes = 24;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		if (presetNum == MODELS_AOSmithHPTU66) {
			tankVolume_L = 244.6;
		} else {
			tankVolume_L = 221.4;
		}
		tankUA_kJperHrC = 8;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

                // performance map
		compressor.perfMap.reserve(3);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{170, 2.02, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.93, -0.027, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{144.5, 2.42, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.67, -0.037, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{94.1, 3.15, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{11.1, -0.056, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		if (presetNum == MODELS_RheemHBDR2265) {
			resistiveElementTop.setupAsResistiveElement(8, 2250);
		} else {
			resistiveElementTop.setupAsResistiveElement(8, 4500);
		}
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		if (presetNum == MODELS_RheemHBDR2265) {
			resistiveElementBottom.setupAsResistiveElement(0, 2250);
		} else {
			resistiveElementBottom.setupAsResistiveElement(0, 4500);
		}
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(35);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(11); nodeWeights.emplace_back(12);
		resistiveElementTop.addTurnOnLogic(HPWH::HeatingLogic("top sixth absolute", nodeWeights, F_TO_C(105), true));
//		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(31)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

                setOfSources[0].companionHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_AOSmithHPTU80 || presetNum == MODELS_RheemHBDR2280 || presetNum == MODELS_RheemHBDR4580) {
		numNodes = 24;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 299.5;
		tankUA_kJperHrC = 9;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;
		compressor.configuration = HPWH::HeatSource::CONFIG_WRAPPED;

		double split = 1.0 / 3.0;
		compressor.setCondensity(split, split, split, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(3);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{170, 2.02, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.93, -0.027, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{144.5, 2.42, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.67, -0.037, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{94.1, 3.15, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{11.1, -0.056, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.0);
		compressor.hysteresis_dC = dF_TO_dC(1);

		//top resistor values
		if (presetNum == MODELS_RheemHBDR2280) {
			resistiveElementTop.setupAsResistiveElement(8, 2250);
		} else {
			resistiveElementTop.setupAsResistiveElement(8, 4500);
		}
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		if (presetNum == MODELS_RheemHBDR2280) {
			resistiveElementBottom.setupAsResistiveElement(0, 2250);
		} else {
			resistiveElementBottom.setupAsResistiveElement(0, 4500);
		}
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(35);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		std::vector<NodeWeight> nodeWeights;
//		nodeWeights.emplace_back(9); nodeWeights.emplace_back(10);
		nodeWeights.emplace_back(11); nodeWeights.emplace_back(12);
		resistiveElementTop.addTurnOnLogic(HPWH::HeatingLogic("top sixth absolute", nodeWeights, F_TO_C(105), true));
//		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(35)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

                setOfSources[0].companionHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_AOSmithHPTU80_DR) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 283.9;
		tankUA_kJperHrC = 9;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{142.6, 2.152, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.989258, -0.038320, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{120.14, 2.513, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{8.188, -0.0432, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(34.1636);
		double standbyT = dF_TO_dC(7.1528);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80.108)));

		// resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(39.9691)));
                resistiveElementTop.addTurnOnLogic(HPWH::topThird_absolute(F_TO_C(87)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014STDMode) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(45);
		tankUA_kJperHrC = 6.5;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(19.6605)));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));
		//    compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014STDMode_80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(75.4);
		tankUA_kJperHrC = 10.;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
		});

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(19.6605)));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));
		compressor.minT = F_TO_C(37);
		//    compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(45);
		tankUA_kJperHrC = 6.5;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.addShutOffLogic(HPWH::topNodeMaxTemp(F_TO_C(116.6358)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(11.0648)));
		// compressor.addShutOffLogic(HPWH::largerDraw(F_TO_C(62.4074)));

		resistiveElementBottom.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(60)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;


		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014_80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(75.4);
		tankUA_kJperHrC = 10.;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.addShutOffLogic(HPWH::topNodeMaxTemp(F_TO_C(116.6358)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(11.0648)));
		// compressor.addShutOffLogic(HPWH::largerDraw(F_TO_C(62.4074)));

		resistiveElementBottom.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(60)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;


		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014_80DR) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(75.4);
		tankUA_kJperHrC = 10.;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		// resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.addTurnOnLogic(HPWH::topThird_absolute(F_TO_C(87)));
		// resistiveElementTop.addShutOffLogic(HPWH::topNodeMaxTemp(F_TO_C(116.6358)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(11.0648)));
		// compressor.addShutOffLogic(HPWH::largerDraw(F_TO_C(62.4074)));

		resistiveElementBottom.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(60)));
		// resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;


		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_RheemHB50) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(45);
		tankUA_kJperHrC = 7;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{280, 4.97342, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.634009, -0.029485, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{280, 5.35992, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.3, -0.03, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.hysteresis_dC =  dF_TO_dC(1);
		compressor.minT = F_TO_C(40.0);
		compressor.maxT = F_TO_C(120.0);

		compressor.configuration = HPWH::HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4200);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 2250);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(38);
		double standbyT = dF_TO_dC(13.2639);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(76.7747)));

		resistiveElementTop.addTurnOnLogic(HPWH::topSixth(dF_TO_dC(20.4167)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_Stiebel220E) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(56);
		//tankUA_kJperHrC = 10; //0 to turn off
		tankUA_kJperHrC = 9;

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElement(this);

		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		resistiveElement.setupAsResistiveElement(0, 1500);
		resistiveElement.hysteresis_dC = dF_TO_dC(0);

		compressor.setCondensity(0, 0.12, 0.22, 0.22, 0.22, 0.22, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{295.55337, 2.28518, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.744118, -0.025946, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{282.2126, 2.82001, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{8.012112, -0.039394, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(32.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = 0;  //no hysteresis
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		compressor.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(6.5509)));
		compressor.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		compressor.depressesTemperature = false;  //no temp depression

		//set everything in its places
		setOfSources[0] = compressor;
		setOfSources[1] = resistiveElement;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[0].backupHeatSource = &setOfSources[1];

	}
	else if (presetNum == MODELS_Generic1) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();
		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 9;
		doTempDepression = false;
		tankMixesOnDraw = true;
		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{472.58616, 2.09340, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{2.942642, -0.0125954, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{439.5615, 2.62997, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.95076, -0.01638033, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40.0)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(80)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(110)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(35)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_Generic2) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();
		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 7.5;
		doTempDepression = false;
		tankMixesOnDraw = true;
		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{272.58616, 2.09340, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{4.042642, -0.0205954, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{239.5615, 2.62997, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.25076, -0.02638033, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(40.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(60)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(80)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(40)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_Generic3) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 5;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{172.58616, 2.09340, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.242642, -0.0285954, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{139.5615, 2.62997, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.75076, -0.03638033, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(35.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(55)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(60)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(40)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_UEF2generic) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(45);
		tankUA_kJperHrC = 6.5;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{4.29, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.61, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
		});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(18.6605)));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}

	else {
		if (hpwhVerbosity >= VRB_reluctant) msg("You have tried to select a preset model which does not exist.  \n");
		return HPWH_ABORT;
	}

	hpwhModel = presetNum;

	//calculate oft-used derived values
	calcDerivedValues();

	if (checkInputs() == HPWH_ABORT) return HPWH_ABORT;

	isHeating = false;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isOn) {
			isHeating = true;
		}
		setOfSources[i].sortPerformanceMap();
	}

	if (hpwhVerbosity >= VRB_emetic){
		for (int i = 0; i < numHeatSources; i++) {
			msg("heat source %d: %p \n", i, &setOfSources[i]);
		}
		msg("\n\n");
	}

	simHasFailed = false;
	return 0;  //successful init returns 0
}  //end HPWHinit_presets
