#include "EKO-CR.hpp"
#include "SVF.hpp"
#include "RCFilter.hpp"
#include "DPW.hpp"


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
	
	dsp::SchmittTrigger gateDetect; // Trigger del Gate
	
	RCFilter<float> *rc_env1; // Filtro RC per l'ADSR 1
	RCFilter<float> *rc_env2; // Filtro RC per l'ADSR 2
	// Inviluppo Primario
	bool isAtk1, isRunning1;
	float Atau1, Dtau1, Rtau1, sus1;
	float env1;
	// Inviluppo Secondario
	bool isAtk2, isRunning2;
	float Atau2, Dtau2, Rtau2, sus2;
	float env2;

	DPW<double> *Osc; // Oscillatore DPW di tipo sawtooth 
	unsigned int dpwOrder = 4; // Ordine dell'oscillatore

	RCFilter<float> *filterRC; // Filtro RC per sagomare il sawtooth

	SVF<float> * filterSV; // Filtro SV per seconda armonica

	//
	double saw; // Sawtooth in uscita dal DPW
	float saw_rc; // Segnale filtrato dal filtro RC
	float saw_sv; // Segnale inviluppato in ingresso al filtro SV
	float hpf, bpf, lpf; // uscite dal filtro SV

    Triangle() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		Osc = new DPW<double>();
		Osc->setPitch(3703.0); // Setto la frequenza dell'oscillatore a f0 = 3703
		Osc->onDPWOrderChange(dpwOrder); // Cambio l'ordine del DPW

		rc_env1 = new RCFilter<float>(0.999);
		rc_env2 = new RCFilter<float>(0.999);
		isAtk1 = isAtk2 = false;
		isRunning1 = isRunning2 = false;
		env1 = env2 = 0.f;
		
		// Valori dei parametri dei 2 inviluppi, ottenuti empiricamente
		// 
		Atau1 = 1e-9f;
		Dtau1 = 0.21446f;
		Rtau1 = 0.00727f;
		sus1 = 1e-9f;
		//
		Atau2 = 0.00651f;
		Dtau2 = 0.04916f;
		Rtau2 = 0.010241f;
		sus2 = 0.033253f;	

		filterRC = new RCFilter<float>;
		float fc_rc = std::pow(2.4,10.0);
		filterRC->setCutoff(fc_rc);

		filterSV = new SVF<float>(7400, 0.010688);
		hpf = bpf = lpf = 0.f;
		saw = saw_rc = saw_sv = 0.f;
	}

	void process(const ProcessArgs &args) override;

};

void Triangle::process(const ProcessArgs &args) {

	// Sawtooth
	saw = Osc->process();

	// RC Filter
	saw_rc = filterRC->process(saw);
	
	// Gate Input
	bool gate = inputs[IN_GATE].getVoltage() >= 1.0;
	if (gateDetect.process(gate)) {
		isAtk1 = isAtk2 = true;
		isRunning1 = isRunning2 = true;
	}

    // Envelopes 
	if (isRunning1) {
			if (gate) {
				if (isAtk1) {
					// ATK
					rc_env1->setTau(Atau1);
					env1 = rc_env1->process(1.0);
					if (env1 >= 1.0 - 0.001) {
						isAtk1 = false;
					}
				}
				else {
					// DEC
					rc_env1->setTau(Dtau1);
					if (env1 <= sus1 + 0.001)
						env1 = sus1;
					else
						env1 = rc_env1->process(sus1);
				}
			} else {
				// REL
				rc_env1->setTau(Rtau1);
				env1 = rc_env1->process(0.0);
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
					rc_env2->setTau(Atau2);
					env2 = rc_env2->process(1.0);
					if (env2 >= 1.0 - 0.001) {
						isAtk2 = false;
					}
				}
				else {
					// DEC
					rc_env2->setTau(Dtau2);
					if (env2 <= sus2 + 0.001)
						env2 = sus2;
					else
						env2 = rc_env2->process(sus2);
				}
			} else {
				// REL
				rc_env2->setTau(Rtau2);
				env2 = rc_env2->process(0.0);
				if (env2 <= 0.001)
					isRunning2 = false;
			}
		} else {
			env2 = 0.0;
		}

	env1 = clamp(env1, 0.f, 1.f);
	env2 = clamp(env2, 0.f, 1.f);

	// SV Filter
	filterSV->setCoeffs(0.1678*args.sampleRate,0.010688);
	saw_sv = env2 * saw_rc;
	filterSV->process(saw_sv, &hpf, &bpf, &lpf);
	
	// Output 
	if (outputs[OUT_TRI].isConnected()){
    outputs[OUT_TRI].setVoltage(10.f*(env1*saw_rc+hpf));
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
