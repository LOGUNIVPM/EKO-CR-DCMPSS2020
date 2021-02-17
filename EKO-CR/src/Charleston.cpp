#include "EKO-CR.hpp"

//values obtained from the circuit analysis and corrected to take account of the components tolerances
#define OMEGA_PREWARPING 101020.f
//filters coefficient in the s-domain (Bandpass B0=B2=0, Highpass B1=0)
#define BANDPASS_B1 1e6f
#define BANDPASS_A0 1.f
#define BANDPASS_A1 31982.f
#define BANDPASS_A2 10206e6f
#define HIGHPASS_B0 1.f
#define HIGHPASS_A0 1.f
#define HIGHPASS_A1 13958.f

//waveform attack, decay and release parameters obtained from the comparison with the real sample
#define DECAY_STOP 0.3f
#define DECAY_TIME 0.06f
#define ATTACK_TAU 0.0003f
#define RELEASE_TAU 0.007f

#define NOISE_LEVEL 0.2f //set the noise level


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


struct secondOrderBandPassDF2 { //Second order bandpass implemented with Direct Form 2
	float b0, a0, a1, a2;
	float y, w, wz1, wz2;

	secondOrderBandPassDF2() {
    	reset();
    	setCoeff(OMEGA_PREWARPING);
    }

    void reset() {
    	y = w = wz1 = wz2 = 0.f;
    }

    void setCoeff(float prewarp) { //apply bilinear transformation with prewarping
    	float K = prewarp/tan(prewarp*0.5*APP->engine->getSampleTime());

    	//z-domain coefficients
    	b0 = BANDPASS_B1*K;
    	//b1 = 0
    	//b2 = -b0
    	a0 = BANDPASS_A0*K*K + BANDPASS_A1*K + BANDPASS_A2;
    	a1 = 2*BANDPASS_A2 - 2*BANDPASS_A0*K*K;
    	a2 = BANDPASS_A0*K*K - BANDPASS_A1*K + BANDPASS_A2;
    	//normalize with respect to a0
    	b0 = b0/a0;
    	a1 = a1/a0;
    	a2 = a2/a0;
    }

    float process(float x) {
    	w = x - wz1*a1 - wz2*a2;
    	y = (w - wz2)*b0; //y=w*b0 + wz1*b1 + wz2*b2 (b1=0, b2=-b0)
    	wz2 = wz1;
    	wz1 = w;
    	return y;
    }
};


struct firstOrderHighPassDF2 { //First order highpass implemented with Direct Form 2
	float b0, a0, a1;
	float y, w, wz1;

	firstOrderHighPassDF2() {
    	reset();
    	setCoeff(OMEGA_PREWARPING);
    }

    void reset() {
    	y = w = wz1 = 0.f;
    }

    void setCoeff(float prewarp) { //apply bilinear transformation with prewarping
    	float K = prewarp/tan(prewarp*0.5*APP->engine->getSampleTime());

    	//z-domain coefficients
    	b0 = HIGHPASS_B0*K;
    	//b1 = -b0
    	a0 = HIGHPASS_A0*K + HIGHPASS_A1;
    	a1 = -HIGHPASS_A0*K + HIGHPASS_A1;
    	//normalize with respect to a0
    	b0 = b0/a0;
    	a1 = a1/a0;
    }

    float process(float x) {
    	w = x - wz1*a1;
    	y = (w - wz1)*b0; //y=w*b0 + wz1*b1 (b1=-b0)
    	wz1 = w;
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
	secondOrderBandPassDF2 bp;
	firstOrderHighPassDF2 hp;
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

	outputs[MAIN_OUTPUT].setVoltage(out*4);

	lights[LIGHT_TRIGGER].setSmoothBrightness(led, 5e-6f);
}


void Charleston::onSampleRateChange() {
	bp.setCoeff(OMEGA_PREWARPING);
	hp.setCoeff(OMEGA_PREWARPING);
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
