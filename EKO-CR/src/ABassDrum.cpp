#include "EKO-CR.hpp"
#include "SVF.hpp"

// definisco i parametri necessari: 1 ingresso, 1 uscita, 1 cutoff, 1 damping

struct ABassDrum : Module {
	enum ParamIds {
		PARAM_CUTOFF,
		PARAM_DAMP,
		NUM_PARAMS,
	};
	enum InputIds {
		INPUT_A,
		NUM_INPUTS,
	};
	enum OutputIds {
		OUTPUT_1,
		NUM_OUTPUTS,
	};

	enum LightsIds {
		NUM_LIGHTS,
	};

// La frequenza di risonanza estrapolata dall'analisi in frequenza del segnale è a 80Hz
// La curva esponenziale che approssima l'inviluppo del segnale è stata ricavata da una analisi in Matlab

	float fc = 78;
    float dp = 0.1491;


// bt rileva quando in ingresso viene ricevuto un impulso

	dsp::BooleanTrigger bt;

// SVF è il passa basso che utilizzo per filtrare la delta

	SVF<float> * filt = new SVF<float>(fc,dp);
	float hpf, bpf, lpf;

// configuro i parametri definiti in precedenza
	ABassDrum() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PARAM_CUTOFF, 1.f, 150.f, fc, "Cutoff","Hz");
		configParam(PARAM_DAMP, 0.01, 0.5, dp, "Damping", "");

		hpf = bpf = lpf = 0;
	}

	void process(const ProcessArgs &args) override;

};

void ABassDrum::process(const ProcessArgs &args) {

	// READ INPUT
	    float status = inputs[INPUT_A].getVoltage();
	    float triggered = bt.process(status);

	    float fcs = params[PARAM_CUTOFF].getValue();

    // SVF FILTER
	    filt->setCoeffs(fcs, params[PARAM_DAMP].getValue());
	    filt->process(triggered, &hpf, &bpf, &lpf);

	// WRITE OUTPUT
	    float out = 500.f * lpf;
	    outputs[OUTPUT_1].setVoltage(out);

		}

// Grafica

struct ABassDrumWidget : ModuleWidget {

	ABassDrumWidget(ABassDrum* module) {

		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ATemplate.svg")));
		box.size = Vec(6*RACK_GRID_WIDTH, RACK_GRID_HEIGHT);

		{
					ATitle * title = new ATitle(box.size.x);
					title->setText("ABassDrum");
					addChild(title);
				}

				{
					ATextLabel * title = new ATextLabel(Vec(12, 245));
					title->setText("IN");
					addChild(title);
				}
				{
					ATextLabel * title = new ATextLabel(Vec(23, 65));
					title->setText("CUTOFF");
					addChild(title);
				}
				{
					ATextLabel * title = new ATextLabel(Vec(30, 145));
					title->setText("DAMP");
					addChild(title);
				}
				{
					ATextLabel * title = new ATextLabel(Vec(55, 245));
					title->setText("OUT");
					addChild(title);
				}

		addInput(createInput<PJ301MPort>(Vec(10, 280), module, ABassDrum::INPUT_A));

		addOutput(createOutput<PJ301MPort>(Vec(55, 280), module, ABassDrum::OUTPUT_1));

		addParam(createParam<RoundBlackKnob>(Vec(30, 100), module, ABassDrum::PARAM_CUTOFF));
		addParam(createParam<RoundBlackKnob>(Vec(30, 180), module, ABassDrum::PARAM_DAMP));


	}

};



Model * modelABassDrum = createModel<ABassDrum, ABassDrumWidget>("ABassDrum");
