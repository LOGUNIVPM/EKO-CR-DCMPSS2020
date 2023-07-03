/*--------------------------- Cymbal1 ---------------------------*
 *
 * Author: Alessandro Nicolini <alessandro.nicolini.an@gmail.com.>
 *
 * For a detailed guide of the code and functions see the book:
 * "Developing Virtual Synthesizers with VCV Rack" by L.Gabrielli
 *
 * Copyright 2020, Leonardo Gabrielli
 *
 *-----------------------------------------------------------------*/

#include "EKO-CR.hpp"

static const float  attack = 0.0045; //attack time for ADSR [0.0020 - 0.0065]
static const float  decay = 0.1035;  //decay  time for ADSR [0.0065 - 0.1100]
const float Hcoeffs[3] = { 0.7288, -0.7288, -0.4577 }; // b0,b1,a1 HighPass
const float Bcoeffs[5] = { 0.2537, 0.0, -0.2537, -0.0730, 0.4926 }; //b0,b1,b2,a1,a2 BandPass

struct Filter_FirstOrder_DF2 { // Filtro generico del primo ordine (DirectFormII)
	float b0, b1, a1;
	float y, v, vz1;

	Filter_FirstOrder_DF2(){
	const float vector_default[3] = { 0.7288, -0.7288, -0.4577 }; // b0,b1,a1 HighPass
	init(vector_default);
	}

	Filter_FirstOrder_DF2(const float * coefs){
    init(coefs);
    }

	void init(const float * coefs) {
	    b0 = coefs[0];
	    b1 = coefs[1];
	    a1 = coefs[2];
	    y=v=vz1=0;
	}

    float process(float x) {
    	v = x - vz1*a1;
    	y = v*b0 + vz1*b1;
    	vz1 = v;
    	return y;
    }
};

struct Filter_SecondOrder_DF2 { // Filtro generico del secondo ordine (DirectFormII)
	float b0, b1, b2, a1, a2;
	float y, v, vz1, vz2;

	Filter_SecondOrder_DF2(){
	const float vector_default[5] = { 0.2537, 0.0, -0.2537, -0.0730, 0.4926 }; //b0,b1,b2,a1,a2 BandPass
	init(vector_default);
	}

	Filter_SecondOrder_DF2(const float * coefs){
	init(coefs);
	}

	void init(const float * coefs) {
		 b0 = coefs[0];
		 b1 = coefs[1];
		 b2 = coefs[2];
		 a1 = coefs[3];
		 a2 = coefs[4];
		 y=v=vz1=vz2=0;
		}

	float process(float x) {
	     v = x - vz1*a1 - vz2*a2;
	     y = v*b0 + vz1*b1 + vz2*b2;
	     vz2 = vz1;
	     vz1 = v;
	     return y;
	    }
	};

struct Cymbal1 : Module {
	enum ParamIds {
		AMPLITUDE,
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
	struct Filter_FirstOrder_DF2 hp;
	struct Filter_SecondOrder_DF2 bp;
	float env;
	float inNoise;
	bool isRunning;
	bool gate;
	bool isAtk;

	Cymbal1() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(AMPLITUDE, 0.f, 4.f, 2.f,"Amp");
		env = 0.f;
		inNoise = 0.f;
		isRunning = false;
		gate = false;
		isAtk = false;
	}

	void process(const ProcessArgs &args) override;
};

void Cymbal1::process(const ProcessArgs &args) {

	float noise_level = params[AMPLITUDE].getValue(); //potenziometro ampiezza noise
	float out;
	float Astep = 1.f/(args.sampleRate*attack);
	float Dstep = (-1.f) / (args.sampleRate*decay);
	float led = 0.f;

	if (ts.process(inputs[TRIGGER_INPUT].getVoltage())) {
		isRunning = true;
		isAtk = true;
		led = 1.f;
	}

    if (isRunning){
		    //Attack
		    if(isAtk){
		  	    env += Astep;
			    if (env >= 1.0)
				    isAtk = false;
		    }
		    else {
			    //Decay
			    if (env <= 0.001) {
				    env = 0.0;
				    isRunning = false;
			    }
			    else {
				    env+=Dstep;
			    }
		    }
	    inNoise = (random::uniform() - 0.5f) * noise_level * env; //segnale noise con media nulla

	    //Filtraggio
	    out = hp.process(inNoise); //first order highpass
	    out = bp.process(out); // second order bandpass
    }
    else {
	    out=0.0;
    }
	outputs[MAIN_OUTPUT].setVoltage(out*5);
	lights[LIGHT_TRIGGER].setSmoothBrightness(led, 5e-6f);
}

struct Cymbal1Widget : ModuleWidget {

	   Cymbal1Widget(Cymbal1* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ATemplate.svg")));
		box.size = Vec(6*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		{
			ATitle * title = new ATitle(box.size.x);
			title->setText("Cymbal1");
			addChild(title);
		}
		{
			ATextLabel * title = new ATextLabel(Vec(13, 250));
			title->setText("IN");
			addChild(title);
		}
		{
			ATextLabel * title = new ATextLabel(Vec(10, 65));
			title->setText("AMPLITUDE");
			addChild(title);
		}
		{
			ATextLabel * title = new ATextLabel(Vec(55, 250));
			title->setText("OUT");
			addChild(title);
		}

		addInput(createInput<PJ301MPort>(Vec(10, 200), module, Cymbal1::TRIGGER_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(55, 200), module, Cymbal1::MAIN_OUTPUT));
		addParam(createParam<RoundBlackKnob>(Vec(30, 100), module, Cymbal1::AMPLITUDE));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(68, 190), module, Cymbal1::LIGHT_TRIGGER));
  }
};

Model * modelCymbal1 = createModel<Cymbal1, Cymbal1Widget>("Cymbal1");
