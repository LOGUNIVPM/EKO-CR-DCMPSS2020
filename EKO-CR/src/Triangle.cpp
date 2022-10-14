#include "EKO-CR.hpp"
#include "SVF.hpp"
#include "RCFilter.hpp"
#include "DPW.hpp"


#define DPWOSC_TYPE double
#define POLYCHMAX 16

struct Triangle : Module {
	enum ParamIds {
	    NUM_PARAMS,
	};
	enum InputIds {
		IN_GATE,
		NUM_INPUTS,
	};
	enum OutputIds {
		OUT_TRI,
		NUM_OUTPUTS,
	};

  	enum LightsIds {
		NUM_LIGHTS,
	};
	
	dsp::SchmittTrigger gateDetect; // Trigger di Gate
	
	RCFilter<double> *rc_env; // Filtro RC per l'ADSR
	// Inviluppo Primario
	bool isAtk1, isRunning1;
	double Atau1, Dtau1, Rtau1, sus1;
	double env1;
	// Inviluppo Secondario
	bool isAtk2, isRunning2;
	double Atau2, Dtau2, Rtau2, sus2;
	double env2;

	DPW<double> *Osc; // Oscillatore DPW di tipo sawtooth 
	unsigned int dpwOrder = 4; // Ordine dell'oscillatore

	RCFilter<double> *filterRC; // Filtro RC per sagomare il sawtooth

	SVF<double> * filterSV; // Filtro SV per seconda armonica

	//
	double saw; // Sawtooth in uscita dal DPW
	double saw_rc; // Segnale filtrato dal filtro RC
	double hpf, bpf, lpf; // uscite dal filtro SV

    Triangle() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		Osc = new DPW<double>();
		Osc->setPitch(3703.0); // Setto la frequenza dell'oscillatore a f0 = 3703
		Osc->onDPWOrderChange(dpwOrder); // Cambio l'ordine del DPW

		rc_env = new RCFilter<double>(0.999);
		isAtk1 = isAtk2 = false;
		isRunning1 = isRunning2 = false;
		env1 = env2 = 0.0;
		//
		Atau1 = 0.0;
		Dtau1 = 0.21446;
		Rtau1 = 0.012048;
		sus1 = 0.0;
		//
		Atau2 = 0.024097;
		Dtau2 = 0.38916;
		Rtau2 = 0.060241;
		sus2 = 0.0;	

		filterRC = new RCFilter<double>;
		double fc_rc = pow(2.5,10.0);
		filterRC->setCutoff(fc_rc);

		filterSV = new SVF<double>(7400, 0.021688);
		hpf = bpf = lpf = 0.0;
		saw = saw_rc = 0.0;
	}

	void process(const ProcessArgs &args) override;

};

void Triangle::process(const ProcessArgs &args) {

	// Gate Input
	bool gate = inputs[IN_GATE].getVoltage() >= 1.0;
	if (gateDetect.process(gate)) {
		isAtk1 = isAtk2 = true;
		isRunning1 = isRunning2 = true;
	}

	// Sawtooth
	saw = Osc->process();

	// RCFilter
	saw_rc = filterRC->process(saw);

	// SVFilter
	filterSV->process(saw_rc, &hpf, &bpf, &lpf);

    // 
	if (isRunning1) {
			if (gate) {
				if (isAtk1) {
					// ATK
					rc_env->setTau(Atau1);
					env1 = rc_env->process(1.0);
					if (env1 >= 1.0 - 0.001) {
						isAtk1 = false;
					}
				}
				else {
					// DEC
					rc_env->setTau(Dtau1);
					if (env1 <= sus1 + 0.001)
						env1 = sus1;
					else
						env1 = rc_env->process(sus1);
				}
			} else {
				// REL
				rc_env->setTau(Rtau1);
				env1 = rc_env->process(0.0);
				if (env1 <= 0.001)
					isRunning1 = false;
			}
		} else {
			env1 = 0.0;
		}

	if (isRunning2) {
			if (gate) {
				if (isAtk2) {
					// ATK
					rc_env->setTau(Atau1);
					env2 = rc_env->process(1.0);
					if (env2 >= 1.0 - 0.001) {
						isAtk2 = false;
					}
				}
				else {
					// DEC
					rc_env->setTau(Dtau1);
					if (env2 <= sus2 + 0.001)
						env2 = sus2;
					else
						env2 = rc_env->process(sus2);
				}
			} else {
				// REL
				rc_env->setTau(Rtau2);
				env2 = rc_env->process(0.0);
				if (env2 <= 0.001)
					isRunning2 = false;
			}
		} else {
			env2 = 0.0;
		}

	if (outputs[OUT_TRI].isConnected()){
    outputs[OUT_TRI].setVoltage(10*(env1*saw_rc+env2*bpf));
	}
}



struct TriangleWidget : ModuleWidget {

	TriangleWidget(Triangle* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ATemplate.svg")));
        box.size = Vec(6*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		{
		ATitle * title = new ATitle(box.size.x);
		title->setText("Triangle");
		addChild(title);
	}
	{
		ATextLabel * title = new ATextLabel(Vec(33, 160));
		title->setText("IN");
		addChild(title);
	}

	{
		ATextLabel * title = new ATextLabel(Vec(33, 240));
		title->setText("OUT");
		addChild(title);
	}

	addInput(createInput<PJ301MPort>(Vec(30, 200), module, Triangle::IN_GATE));
	addOutput(createOutput<PJ3410Port>(Vec(30, 280), module, Triangle::OUT_TRI));

	}

};



Model * modelTriangle = createModel<Triangle, TriangleWidget>("Triangle");
