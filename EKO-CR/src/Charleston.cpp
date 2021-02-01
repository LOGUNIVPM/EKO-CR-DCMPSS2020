#include "EKO-CR.hpp"

#define EPSILON 1e-9f

struct lowPass {
    float yn, yn1, a;

    lowPass() {
    	a = 0.9f;
    	reset();
    }

    void reset() {
    	yn = yn1 = 0.f;
    }

    void init() {
        yn = 0.f;
        yn1 = 0.8f;
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


struct secondOrderBandPassTPT {
	float g, d;
	float BP, BP2, s1, s2, v22;

	secondOrderBandPassTPT() {
    	reset();
    	setCoeff();
    }

    void reset() {
    	BP = BP2 = s1 = s2 = v22 = 0.f;
    }

    void setCoeff() {
    	float omegaP = 2*M_PI*17470; //prewarping point
    	float omegaC = 109830; //cutoff
    	float R = 0.0189; //damping
    	g = omegaC/omegaP*tan(omegaP*0.5*APP->engine->getSampleTime()); //prewarped gain
    	d = 1/(1 + 2*R*g + g*g);
    }

    float process(float x) { //Zavalishin
    	BP = (g*(x-s2)+s1)*d;
    	BP2 = BP + BP;
    	s1 = BP2 - s1;
    	v22 = g*BP2;
    	s2 = s2 + v22;
    	return BP;
    }
};


struct firstOrderHighPassTPT {
	float g2, Ghp;
	float y, xs, s;

	firstOrderHighPassTPT() {
    	reset();
    	setCoeff();
    }

    void reset() {
    	y = xs = s = 0.f;
    }

    void setCoeff() {
    	float omegaP = 2*M_PI*17470; //prewarping point
    	float omegaC = 13955; //cutoff
    	float g = omegaC/omegaP*tan(omegaP*0.5*APP->engine->getSampleTime()); //prewarped gain
    	g2 = g + g;
    	Ghp = 1/(1 + g);
    }

    float process(float x) { //Zavalishin
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
	float Dstep = 0.2f / (EPSILON + args.sampleRate * 0.05f);
	float led = 0.f;

	if (ts.process(inputs[TRIGGER_INPUT].getVoltage())) {
		isRunning = true;
		isLin = true;
		isAtk = true;
		lp.reset();
		led = 1.f;
	}

	//generazione della forma d'onda in ingresso al filtro
	if (isRunning) {
		if (isLin) { //BJT in zona lineare
			if (isAtk) { //Attack
				lp.setTau(0.0001f);
				in = lp.process(1.f);
				if (in >= 1 - 0.001) {
					isAtk = false;
				}
				inNoise = in + (random::uniform() - 0.5f) * 0.15f;
			}
			else { //Decay
				in = in - Dstep;
				if (in <= 0.8 + 0.001) {
					isLin = false;
					lp.init();
				}
				inNoise = in + (random::uniform() - 0.5f) * 0.15f * in;
			}
		}
		else { //Release (BJT in cutoff)
			lp.setTau(0.007f);
			in = lp.process(0.f);
			if (in <= 0.001) {
				isRunning = false;
			}
			inNoise = in + (random::uniform() - 0.5f) * 0.15f * in;
		}

		//filtraggio
		out = bp.process(inNoise);
		out = hp.process(out);
	}
	else {
		out = 0;
		inNoise = 0;
	}

	outputs[MAIN_OUTPUT].setVoltage(out*15);

	lights[LIGHT_TRIGGER].setSmoothBrightness(led, 5e-6f);
}


void Charleston::onSampleRateChange() {
	bp.setCoeff();
	hp.setCoeff();
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
