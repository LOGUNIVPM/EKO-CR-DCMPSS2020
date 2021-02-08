#include "EKO-CR.hpp"

//values obtained from the circuit analysis and corrected to take account of the components tolerances
#define OMEGA_PREWARPING 101373.f
#define BANDPASS_OMEGA_CUTOFF 101373.f
#define BANDPASS_DAMPING 0.15f
#define HIGHPASS_OMEGA_CUTOFF 13956.f

//waveform attack, decay and release parameters obtained from the comparison with the real sample
#define DECAY_STOP 0.3f
#define DECAY_TIME 0.06f
#define ATTACK_TAU 0.0003f
#define RELEASE_TAU 0.007f

#define NOISE_LEVEL 0.2f


struct lowPass {
    float yn, yn1, a;

    lowPass() {
    	a = 0.9f;
    	reset();
    }

    void reset() {
    	yn = yn1 = 0.f;
    }

    void init(float setVal) {
        yn = 0.f;
        yn1 = setVal;
    }

    void setTau(float tau) {
    	a = tau / (tau + APP->engine->getSampleTime());
    }

    float process(float xn) {
    	yn = a*yn1 + (1 - a)*xn;
    	yn1 = yn;
    	return yn;
    }
};


struct secondOrderBandPassTPT { //Zavalishin SVF with TPT (only bandpass output)
	float g, d;
	float BP, BP2, s1, s2, v22;

	secondOrderBandPassTPT() {
    	reset();
    	setCoeff(BANDPASS_OMEGA_CUTOFF, BANDPASS_DAMPING, OMEGA_PREWARPING);
    }

    void reset() {
    	BP = BP2 = s1 = s2 = v22 = 0.f;
    }

    void setCoeff(float cutoff, float damp, float prewarp) {
    	g = cutoff/prewarp * tan(prewarp*0.5*APP->engine->getSampleTime()); //prewarped gain
    	d = 1/(1 + 2*damp*g + g*g);
    }

    float process(float x) {
    	BP = (g*(x-s2)+s1)*d;
    	BP2 = BP + BP;
    	s1 = BP2 - s1;
    	v22 = g*BP2;
    	s2 = s2 + v22;
    	return BP;
    }
};


struct firstOrderHighPassTPT { //Zavalishin first order highpass with TPT
	float g2, Ghp;
	float y, xs, s;

	firstOrderHighPassTPT() {
    	reset();
    	setCoeff(HIGHPASS_OMEGA_CUTOFF, OMEGA_PREWARPING);
    }

    void reset() {
    	y = xs = s = 0.f;
    }

    void setCoeff(float cutoff, float prewarp) {
    	float g = cutoff/prewarp * tan(prewarp*0.5*APP->engine->getSampleTime()); //prewarped gain
    	g2 = g + g;
    	Ghp = 1/(1 + g);
    }

    float process(float x) {
    	xs = x - s;
    	y = xs*Ghp;
    	s = s + y*g2;
    	return y;
    }
};


struct Charleston : Module {
	enum ParamIds {
		NUM_PARAMS,
	};
	enum InputIds {
		TRIGGER_INPUT,
		NUM_INPUTS,
	};
	enum OutputIds {
		MAIN_OUTPUT,
		NUM_OUTPUTS,
	};

	enum LightsIds {
		LIGHT_TRIGGER,
		NUM_LIGHTS,
	};

	dsp::SchmittTrigger ts;
	lowPass lp;
	secondOrderBandPassTPT bp;
	firstOrderHighPassTPT hp;
	float in;
	float inNoise;
	bool isRunning;
	bool isLin;
	bool isAtk;


	Charleston() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		in = 0.f;
		inNoise = 0.f;
		isRunning = false;
		isLin = false;
		isAtk = false;
	}

	void process(const ProcessArgs &args) override;

	void onSampleRateChange() override;
};


void Charleston::process(const ProcessArgs &args) {

	float out;
	float Dstep = (DECAY_STOP - 1.f) / (args.sampleRate*DECAY_TIME);
	float led = 0.f;

	if (ts.process(inputs[TRIGGER_INPUT].getVoltage())) {
		isRunning = true;
		isLin = true;
		isAtk = true;
		lp.reset();
		led = 1.f;
	}

	//generation of the waveform at the filter input
	if (isRunning) {
		if (isLin) {
			if (isAtk) { //Attack
				lp.setTau(ATTACK_TAU);
				in = lp.process(1.f);
				if (in >= 1 - 0.001) {
					isAtk = false;
				}
			}
			else { //Decay
				in += Dstep;
				if (in <= DECAY_STOP + 0.001) {
					isLin = false;
					lp.init(DECAY_STOP);
				}
			}
		}
		else { //Release
			lp.setTau(RELEASE_TAU);
			in = lp.process(0.f);
			if (in <= 0.001) {
				isRunning = false;
			}
		}

		inNoise = in + (random::uniform() - 0.5f) * NOISE_LEVEL * in; //noise signal with zero mean is scaled and added to the waveform

		//Third order filter
		out = bp.process(inNoise); //second order bandpass
		out = hp.process(out); //first order highpass
	}
	else {
		out = 0;
	}

	outputs[MAIN_OUTPUT].setVoltage(out*40);

	lights[LIGHT_TRIGGER].setSmoothBrightness(led, 5e-6f);
}


void Charleston::onSampleRateChange() {
	bp.setCoeff(BANDPASS_OMEGA_CUTOFF, BANDPASS_DAMPING, OMEGA_PREWARPING);
	hp.setCoeff(HIGHPASS_OMEGA_CUTOFF, OMEGA_PREWARPING);
}


struct CharlestonWidget : ModuleWidget {

	CharlestonWidget(Charleston* module) {

			setModule(module);
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Charleston.svg")));
			box.size = Vec(6*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

			addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.077, 28.048)), module, Charleston::TRIGGER_INPUT));

			addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(20.675, 50.146)), module, Charleston::MAIN_OUTPUT));

			addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(27.781, 50.448)), module, Charleston::LIGHT_TRIGGER));
	}
};


Model * modelCharleston = createModel<Charleston, CharlestonWidget>("Charleston");
